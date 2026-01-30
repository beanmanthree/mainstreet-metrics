#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <algorithm>
#include <cmath>
#include <chrono>

// MongoDB Driver Includes
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/array.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::finalize;

// ==========================================
// Data Structures
// ==========================================

struct Location {
    double longitude;
    double latitude;
};

struct Business {
    std::string id; // MongoDB ObjectId string
    std::string name;
    std::string category;
    std::string shortDescription;
    std::string currentDeal; // Special coupons/deals
    double averageRating;
    Location location;
};

struct User {
    std::string username;
    std::string email;
    bool isVerified;
};

// ==========================================
// Database Manager Class
// ==========================================

class ByteSizedDB {
private:
    mongocxx::client client;
    mongocxx::database db;
    mongocxx::collection businessesCol;
    mongocxx::collection usersCol;
    mongocxx::collection ratingsCol;

    // Session State
    std::string currentUserId;
    bool isLoggedIn;
    std::mutex dbMutex; // Ensure thread safety for shared resources

    // ---------------------------------------------------------
    // Helper: Password Hashing
    // NOTE: In a real production app, use bcrypt or Argon2.
    // std::hash is NOT cryptographically secure, used here for 
    // demonstration of the logic flow only.
    // ---------------------------------------------------------
    std::string simpleHash(const std::string& password) {
        std::hash<std::string> hasher;
        return std::to_string(hasher(password));
    }

    // ---------------------------------------------------------
    // Helper: Recalculate Average Rating
    // Triggered whenever a review is added or edited.
    // ---------------------------------------------------------
    void updateBusinessAverage(const std::string& businessId) {
        try {
            mongocxx::pipeline pipe;

            // 1. Match all reviews for this business
            pipe.match(document{} << "business_id" << businessId << finalize);

            // 2. Group and calculate average
            pipe.group(document{}
                << "_id" << "$business_id"
                << "avgRating" << open_document
                << "$avg" << "$rating"
                << close_document
                << finalize);

            auto cursor = ratingsCol.aggregate(pipe);

            for (auto&& doc : cursor) {
                auto element = doc["avgRating"];
                if (element && element.type() == bsoncxx::type::k_double) {
                    double newAvg = element.get_double().value;

                    // Update the business document
                    businessesCol.update_one(
                        document{} << "_id" << bsoncxx::oid(businessId) << finalize,
                        document{} << "$set" << open_document
                        << "average_rating" << newAvg
                        << close_document << finalize
                    );
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error recalculating rating: " << e.what() << std::endl;
        }
    }

public:
    // Constructor
    ByteSizedDB(const std::string& uriString, const std::string& dbName)
        : isLoggedIn(false) {
        try {
            mongocxx::uri uri(uriString);
            client = mongocxx::client(uri);
            db = client[dbName];

            businessesCol = db["businesses"];
            usersCol = db["users"];
            ratingsCol = db["ratings"];

            // Create Geospatial Index for Location Sorting (2dsphere)
            // This allows $near queries to work.
            auto index_spec = document{} << "location" << "2dsphere" << finalize;
            businessesCol.create_index(index_spec.view());

        }
        catch (const std::exception& e) {
            std::cerr << "DB Connection Error: " << e.what() << std::endl;
            throw;
        }
    }

    // ==========================================
    // User Management
    // ==========================================

    bool registerUser(const std::string& username, const std::string& password, const std::string& email) {
        std::lock_guard<std::mutex> lock(dbMutex);
        try {
            // Check if user exists
            auto result = usersCol.find_one(document{} << "username" << username << finalize);
            if (result) {
                std::cerr << "Username already exists." << std::endl;
                return false;
            }

            // Create User Document
            auto builder = document{};
            bsoncxx::document::value doc = builder
                << "username" << username
                << "password" << simpleHash(password) // Hashed
                << "email" << email
                << "is_verified" << false // Default false
                << "verification_code" << "123456" // Hardcoded for demo logic (would be random in prod)
                << finalize;

            usersCol.insert_one(doc.view());
            std::cout << "User registered. Please verify your email." << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Registration failed: " << e.what() << std::endl;
            return false;
        }
    }

    // Verification Step (Bot Prevention)
    bool verifyUser(const std::string& username, const std::string& code) {
        std::lock_guard<std::mutex> lock(dbMutex);
        auto result = usersCol.find_one(document{}
            << "username" << username
            << "verification_code" << code
            << finalize);

        if (result) {
            usersCol.update_one(
                document{} << "username" << username << finalize,
                document{} << "$set" << open_document << "is_verified" << true << close_document << finalize
            );
            return true;
        }
        return false;
    }

    bool login(const std::string& username, const std::string& password) {
        std::lock_guard<std::mutex> lock(dbMutex);
        auto result = usersCol.find_one(document{}
            << "username" << username
            << "password" << simpleHash(password)
            << finalize);

        if (result) {
            bsoncxx::document::view view = result->view();
            currentUserId = view["_id"].get_oid().value.to_string();
            isLoggedIn = true;
            std::cout << "Logged in successfully." << std::endl;
            return true;
        }
        std::cout << "Invalid credentials." << std::endl;
        return false;
    }

    void logout() {
        std::lock_guard<std::mutex> lock(dbMutex);
        isLoggedIn = false;
        currentUserId = "";
        std::cout << "Logged out." << std::endl;
    }

    // ==========================================
    // Business Management
    // ==========================================

    bool addBusiness(const std::string& name, const std::string& category,
        const std::string& desc, double lat, double lon, const std::string& deal) {
        if (!isLoggedIn) {
            std::cerr << "Must be logged in to add business." << std::endl;
            return false;
        }

        try {
            // MongoDB GeoJSON Format: { type: "Point", coordinates: [lon, lat] }
            auto builder = document{};
            bsoncxx::document::value doc = builder
                << "name" << name
                << "category" << category
                << "description" << desc
                << "deal" << deal
                << "average_rating" << 0.0
                << "location" << open_document
                << "type" << "Point"
                << "coordinates" << open_array << lon << lat << close_array // Note: Lon, Lat order
                << close_document
                << finalize;

            businessesCol.insert_one(doc.view());
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error adding business: " << e.what() << std::endl;
            return false;
        }
    }

    // ==========================================
    // Ratings & Reviews (Logic: One per user per business)
    // ==========================================

    bool upsertReview(const std::string& businessIdStr, int stars, const std::string& comment) {
        if (!isLoggedIn) {
            std::cerr << "Access Denied: Please log in." << std::endl;
            return false;
        }
        if (stars < 1 || stars > 5) {
            std::cerr << "Rating must be 1-5." << std::endl;
            return false;
        }

        // Check verification (Bot Check)
        auto userDoc = usersCol.find_one(document{} << "_id" << bsoncxx::oid(currentUserId) << finalize);
        if (userDoc && userDoc->view()["is_verified"].get_bool().value == false) {
            std::cerr << "Access Denied: Please verify your account to post reviews." << std::endl;
            return false;
        }

        try {
            // We use update_one with upsert=true options
            mongocxx::options::update options;
            options.upsert(true);

            bsoncxx::oid businessId(businessIdStr);

            // Filter: Find review by this user for this business
            auto filter = document{}
                << "user_id" << currentUserId
                << "business_id" << businessIdStr // Storing as string to match pipeline easier, or OID
                << finalize;

            // Update: Set new data
            auto update = document{}
                << "$set" << open_document
                << "rating" << stars
                << "comment" << comment
                << "date" << bsoncxx::types::b_date(std::chrono::system_clock::now())
                << close_document
                << finalize;

            ratingsCol.update_one(filter.view(), update.view(), options);

            // Recalculate Average for the business
            updateBusinessAverage(businessIdStr);

            std::cout << "Review posted/updated successfully." << std::endl;
            return true;

        }
        catch (const std::exception& e) {
            std::cerr << "Review failed: " << e.what() << std::endl;
            return false;
        }
    }

    // ==========================================
    // Query & Sorting Functions
    // ==========================================

    // Helper to convert BSON doc to Business Struct
    Business docToBusiness(bsoncxx::document::view view) {
        Business b;
        b.id = view["_id"].get_oid().value.to_string();
        b.name = view["name"].get_string().value.data();
        b.category = view["category"].get_string().value.data();
        b.shortDescription = view["description"].get_string().value.data();
        if (view["deal"]) b.currentDeal = view["deal"].get_string().value.data();

        // Handle double/int conversions safely
        auto rating = view["average_rating"];
        if (rating.type() == bsoncxx::type::k_double) b.averageRating = rating.get_double().value;
        else if (rating.type() == bsoncxx::type::k_int32) b.averageRating = (double)rating.get_int32().value;
        else b.averageRating = 0.0;

        // Parse GeoJSON
        auto coords = view["location"]["coordinates"].get_array().value;
        b.location.longitude = coords[0].get_double().value;
        b.location.latitude = coords[1].get_double().value;

        return b;
    }

    std::vector<Business> getBusinessesByCategory(const std::string& category) {
        std::vector<Business> results;
        auto cursor = businessesCol.find(document{} << "category" << category << finalize);
        for (auto&& doc : cursor) {
            results.push_back(docToBusiness(doc));
        }
        return results;
    }

    std::vector<Business> getBusinessesByRating() {
        std::vector<Business> results;
        // Sort by average_rating descending (-1)
        auto opts = mongocxx::options::find{};
        opts.sort(document{} << "average_rating" << -1 << finalize);

        auto cursor = businessesCol.find({}, opts);
        for (auto&& doc : cursor) {
            results.push_back(docToBusiness(doc));
        }
        return results;
    }

    // Sort by Distance using MongoDB $near
    std::vector<Business> getBusinessesByLocation(double myLat, double myLon, double maxDistanceMeters = 50000) {
        std::vector<Business> results;

        // Construct $near query
        // Requires 2dsphere index on "location" field
        auto query = document{}
            << "location" << open_document
            << "$near" << open_document
            << "$geometry" << open_document
            << "type" << "Point"
            << "coordinates" << open_array << myLon << myLat << close_array
            << close_document
            << "$maxDistance" << maxDistanceMeters
            << close_document
            << close_document
            << finalize;

        try {
            auto cursor = businessesCol.find(query.view());
            for (auto&& doc : cursor) {
                results.push_back(docToBusiness(doc));
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Location query error (Ensure 2dsphere index exists): " << e.what() << std::endl;
        }
        return results;
    }
};