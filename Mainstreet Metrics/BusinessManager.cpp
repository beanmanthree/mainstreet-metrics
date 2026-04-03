#include "BusinessManager.hpp"

#include <iostream>
#include <algorithm>
#include <random>
#include <regex>
#include <limits>

#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/pipeline.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::finalize;

bool BusinessManager::restoreSession(const std::string& username) {
    std::lock_guard<std::mutex> lock(db_mutex);

    auto result = users_coll.find_one(document{} << "username" << username << finalize);

    if (result) {
        logged_in = true;
        bsoncxx::document::view view = result->view();

        User u;
        u.id = view["_id"].get_oid().value.to_string();
        u.username = view["username"].get_string().value.data();
        u.email = view["email"].get_string().value.data();
        u.is_verified = view["is_verified"].get_bool().value;

        if (view["bookmarks"] && view["bookmarks"].type() == bsoncxx::type::k_array) {
            for (auto ele : view["bookmarks"].get_array().value) {
                u.bookmarks.push_back(ele.get_oid().value.to_string());
            }
        }

        current_user = u;
        return true;
    }
    return false;
}

std::string BusinessManager::hashPassword(const std::string& password) {
    std::hash<std::string> hasher;
    size_t hash = hasher(password + "SimulatedSalt123");
    return std::to_string(hash);
}

void BusinessManager::updateBusinessAverageRating(const std::string& business_id) {
    try {
        using bsoncxx::builder::basic::make_document;
        using bsoncxx::builder::basic::kvp;

        // Pipeline to calculate average
        mongocxx::pipeline pipe;
        pipe.match(make_document(kvp("business_id", bsoncxx::oid(business_id))));
        pipe.group(make_document(
            kvp("_id", "$business_id"),
            kvp("avg", make_document(kvp("$avg", "$rating")))
        ));

        auto cursor = reviews_coll.aggregate(pipe);

        for (auto&& doc : cursor) {
            double new_avg = doc["avg"].get_double();

            // Update business document
            businesses_coll.update_one(
                make_document(kvp("_id", bsoncxx::oid(business_id))),
                make_document(kvp("$set", make_document(kvp("avg_rating", new_avg))))
            );
        }
    }
    catch (const std::exception& e) {
        //std::cerr << "Error updating average: " << e.what() << std::endl;
    }
}

bool BusinessManager::isValidEmail(const std::string& email) {
    // Standard email regex pattern
    const std::regex pattern(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    return std::regex_match(email, pattern);
}

std::vector<Business> BusinessManager::fetchBusinesses(bsoncxx::document::value filter, mongocxx::options::find opts) {
    std::vector<Business> results;
    try {
        auto cursor = businesses_coll.find(filter.view(), opts);
        for (auto&& doc : cursor) {
            Business b;
            b.id = doc["_id"].get_oid().value.to_string();
            b.name = doc["name"].get_string().value.data();
            b.category = doc["category"].get_string().value.data();
            b.description = doc["description"].get_string().value.data();

            // Handle optional/variable fields safely
            if (doc["avg_rating"]) {
                // Mongo might store 5.0 as int or double depending on driver version/insertion
                auto ele = doc["avg_rating"];
                if (ele.type() == bsoncxx::type::k_double) b.avg_rating = ele.get_double();
                else if (ele.type() == bsoncxx::type::k_int32) b.avg_rating = (double)ele.get_int32();
                else b.avg_rating = 0.0;
            }

            if (doc["special_deal"]) b.special_deal = doc["special_deal"].get_string().value.data();

            // GeoJSON parsing
            if (doc["location"] && doc["location"]["coordinates"]) {
                auto coords = doc["location"]["coordinates"].get_array().value;
                b.longitude = coords[0].get_double();
                b.latitude = coords[1].get_double();
            }

            results.push_back(b);
        }
    }
    catch (const std::exception& e) {
        // std::cerr << "Query Error: " << e.what() << std::endl;
    }
    return results;
}

void BusinessManager::printBusinessDoc(bsoncxx::document::view view) {
    std::string name = view["name"].get_string().value.data();
    std::string id = view["_id"].get_oid().value.to_string();
    std::cout << "<p><a href=\"/cgi-bin/Main.cgi?action=view_details&id=" << id << "\">" << name << "</a></p>\n";
}

BusinessManager::BusinessManager(const std::string& uri_string, const std::string& db_name)
    : logged_in(false) {
    try {
        mongocxx::uri uri(uri_string);
        client = mongocxx::client(uri);
        db = client[db_name];

        businesses_coll = db["businesses"];
        users_coll = db["users"];
        reviews_coll = db["reviews"];

        ensureIndexes();
        // std::cout << "[System] Connected to MongoDB Atlas." << std::endl;
    }
    catch (const std::exception& e) {
        // std::cerr << "[Fatal Error] DB Connection failed: " << e.what() << std::endl;
        exit(1);
    }
}

void BusinessManager::ensureIndexes() {
    try {
        // Create 2dsphere index on "location" field for businesses
        auto index_spec = document{} << "location" << "2dsphere" << finalize;
        businesses_coll.create_index(index_spec.view());
    }
    catch (...) {
        // Index might already exist
    }
}

bool BusinessManager::performBotCheck() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 10);

    int a = distrib(gen);
    int b = distrib(gen);

    // std::cout << "[Security] Verify you are human: What is " << a << " + " << b << "? ";
    int answer;
    std::cin >> answer;

    // Clear buffer
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (answer == (a + b)) return true;
    // std::cout << "[Security] Verification failed.\n";
    return false;
}

bool BusinessManager::registerUser(const std::string& username, const std::string& password, const std::string& email) {
    // std::cout << "\n--- New User Registration ---\n";

    // Bot Verification (CAPTCHA)
    // Verification already performed on the website through window alert.
    /*if (!performBotCheck()) {
        // std::cout << "[Error] Bot verification failed. Registration aborted.\n";
        return false;
    }*/

    // Email Syntax Validation
    if (!isValidEmail(email)) {
        // std::cout << "[Error] Invalid email format (e.g., user@example.com).\n";
        return false;
    }

    std::lock_guard<std::mutex> lock(db_mutex);

    // Database Uniqueness Check (Username)
    auto user_exists = users_coll.find_one(document{} << "username" << username << finalize);
    if (user_exists) {
        // std::cout << "[Error] Username '" << username << "' is already taken.\n";
        return false;
    }

    // Database Uniqueness Check (Email)
    auto email_exists = users_coll.find_one(document{} << "email" << email << finalize);
    if (email_exists) {
        // std::cout << "[Error] The email '" << email << "' is already registered.\n";
        return false;
    }

    // Insert New User
    try {
        auto builder = document{};
        bsoncxx::document::value doc = builder
            << "username" << username
            << "password" << hashPassword(password) // Basic hashing
            << "email" << email
            << "is_verified" << false // Set to false until an admin/email flow validates it
            << "bookmarks" << open_array << close_array
            << finalize;

        users_coll.insert_one(doc.view());
        // std::cout << "[Success] User registered successfully! You may now log in.\n";
        return true;
    }
    catch (const std::exception& e) {
        // std::cerr << "[System Error] Registration failed: " << e.what() << "\n";
        return false;
    }
}

bool BusinessManager::login(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(db_mutex);

    auto result = users_coll.find_one(document{}
        << "username" << username
        << "password" << hashPassword(password)
        << finalize);

    if (result) {
        logged_in = true;
        bsoncxx::document::view view = result->view();

        User u;
        u.id = view["_id"].get_oid().value.to_string();
        u.username = view["username"].get_string().value.data();
        u.email = view["email"].get_string().value.data();
        u.is_verified = view["is_verified"].get_bool().value;

        // Load bookmarks
        if (view["bookmarks"] && view["bookmarks"].type() == bsoncxx::type::k_array) {
            for (auto ele : view["bookmarks"].get_array().value) {
                u.bookmarks.push_back(ele.get_oid().value.to_string());
            }
        }

        current_user = u;
        // std::cout << "Welcome back, " << u.username << "!\n";
        return true;
    }
    else {
        // std::cout << "Invalid credentials.\n";
        return false;
    }
}

void BusinessManager::logout() {
    logged_in = false;
    current_user.reset();
    // std::cout << "Logged out.\n";
}

bool BusinessManager::isLoggedIn() const { return logged_in; }

void BusinessManager::addBusiness(const std::string& name, const std::string& category,
    const std::string& desc, double lon, double lat, const std::string& deal) {
    if (!logged_in || !current_user->is_verified) {
        // std::cout << "Error: You must be logged in and verified to add businesses.\n";
        return;
    }

    auto builder = document{};
    bsoncxx::document::value doc = builder
        << "name" << name
        << "category" << category
        << "description" << desc
        << "avg_rating" << 0.0
        << "special_deal" << deal
        << "location" << open_document
        << "type" << "Point"
        << "coordinates" << open_array << lon << lat << close_array
        << close_document
        << finalize;

    businesses_coll.insert_one(doc.view());
    // std::cout << "Business '" << name << "' added.\n";
}

std::vector<Business> BusinessManager::getAllBusinesses() {
    return fetchBusinesses(document{} << finalize);
}

std::vector<Business> BusinessManager::getBusinessesByCategory(const std::string& category) {
    return fetchBusinesses(document{} << "category" << category << finalize);
}

std::vector<Business> BusinessManager::getBusinessesByRating() {
    mongocxx::options::find opts;
    opts.sort(document{} << "avg_rating" << -1 << finalize);
    return fetchBusinesses(document{} << finalize, opts);
}

std::vector<Business> BusinessManager::getBusinessesByLocation(double lon, double lat) {
    // $near query requires 2dsphere index
    auto query = document{}
        << "location" << open_document
        << "$near" << open_document
        << "$geometry" << open_document
        << "type" << "Point"
        << "coordinates" << open_array << lon << lat << close_array
        << close_document
        << close_document
        << close_document
        << finalize;

    return fetchBusinesses(query);
}

void BusinessManager::addOrEditReview(const std::string& business_id, int rating, const std::string& comment) {
    if (!logged_in) {
        // std::cout << "Please login to leave a review.\n";
        return;
    }

    if (rating < 1 || rating > 5) {
        // std::cout << "Rating must be 1-5.\n";
        return;
    }

    try {
        // Upsert: If user already reviewed this business, update it. If not, insert.
        mongocxx::options::update opts;
        opts.upsert(true);

        auto filter = document{}
            << "user_id" << bsoncxx::oid(current_user->id)
            << "business_id" << bsoncxx::oid(business_id)
            << finalize;

        auto update = document{}
            << "$set" << open_document
            << "rating" << rating
            << "comment" << comment
            << close_document
            << finalize;

        reviews_coll.update_one(filter.view(), update.view(), opts);

        // Trigger recalculation of the business's average
        updateBusinessAverageRating(business_id);
        // std::cout << "Review posted/updated successfully.\n";

    }
    catch (const std::exception& e) {
        // std::cout << "Error posting review: " << e.what() << "\n";
    }
}

void BusinessManager::toggleBookmark(const std::string& business_id) {
    if (!logged_in) return;

    try {
        auto& bookmarks = current_user->bookmarks;
        auto it = std::find(bookmarks.begin(), bookmarks.end(), business_id);

        bsoncxx::oid bid(business_id);
        bsoncxx::oid uid(current_user->id);

        if (it != bookmarks.end()) {
            // Remove
            bookmarks.erase(it);
            users_coll.update_one(
                document{} << "_id" << uid << finalize,
                document{} << "$pull" << open_document << "bookmarks" << bid << close_document << finalize
            );
            // std::cout << "Bookmark removed.\n";
        }
        else {
            // Add
            bookmarks.push_back(business_id);
            users_coll.update_one(
                document{} << "_id" << uid << finalize,
                document{} << "$addToSet" << open_document << "bookmarks" << bid << close_document << finalize
            );
            // std::cout << "Bookmark added.\n";
        }
    }
    catch (const std::exception& e) {
        // std::cout << "Error toggling bookmark: " << e.what() << "\n";
        // std::cout << "Please ensure the Business ID is valid.\n";
    }
}

void BusinessManager::displayBookmarks() {
    if (!logged_in) return;
    if (current_user->bookmarks.empty()) {
        // std::cout << "No bookmarks saved.\n";
        return;
    }

    // std::cout << "--- Your Saved Businesses ---\n";
    for (const auto& bid : current_user->bookmarks) {
        auto doc = businesses_coll.find_one(document{} << "_id" << bsoncxx::oid(bid) << finalize);
        if (doc) {
            printBusinessDoc(doc->view());
        }
    }
}

std::vector<Review> BusinessManager::getReviewsForBusiness(const std::string& business_id) {
    std::vector<Review> reviews;

    try {
        // Query for all reviews matching this business_id
        auto cursor = reviews_coll.find(
            document{} << "business_id" << bsoncxx::oid(business_id) << finalize
        );

        // Convert each document to a Review struct
        for (auto&& doc : cursor) {
            Review r;
            r.business_id = business_id;
            r.user_id = doc["user_id"].get_oid().value.to_string();
            r.rating = doc["rating"].get_int32().value;
            r.comment = doc["comment"].get_string().value.data();
            reviews.push_back(r);
        }
    }
    catch (const std::exception& e) {
        // std::cerr << "Error fetching reviews: " << e.what() << std::endl;
    }

    return reviews;
}

bool BusinessManager::verifyAccount(const std::string& email) {
    if (!logged_in || !current_user) return false;

    std::lock_guard<std::mutex> lock(db_mutex);

    // Look up the user by ID and check if the email matches
    auto result = users_coll.find_one(
        document{} << "_id" << bsoncxx::oid(current_user->id)
        << "email" << email
        << finalize
    );

    if (!result) return false;

    try {
        using bsoncxx::builder::basic::make_document;
        using bsoncxx::builder::basic::kvp;

        users_coll.update_one(
            make_document(kvp("_id", bsoncxx::oid(current_user->id))),
            make_document(kvp("$set", make_document(kvp("is_verified", true))))
        );

        current_user->is_verified = true;
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}