#include <vector>
#include <string>
#include <optional>
#include <mutex>

// MongoDB Driver Includes
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>

// Namespaces for cleaner code
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::finalize;

/***
* Purpose: Represent a user account with authentication credentials, verification status, and saved bookmarks.
* Parameters: None.
* Result: A data structure containing all user-related information including their unique identifier,
*          login credentials, email address, verification status, and a collection of bookmarked business IDs.
***/
struct User {
    std::string id;
    std::string username;
    std::string email;
    bool is_verified;
    std::vector<std::string> bookmarks;
};

/***
* Purpose: Represent a business entity with location data, ratings, and promotional information.
* Parameters: None.
* Result: A data structure containing business details including unique identifier, name, category,
*          description text, average rating score, geographic coordinates in longitude and latitude,
*          and any special deals or coupons associated with the business.
***/
struct Business {
    std::string id;
    std::string name;
    std::string category;
    std::string description;
    double avg_rating;
    double longitude;
    double latitude;
    std::string special_deal;
};

/***
* Purpose: Represent a user's review of a business including their rating and comments.
* Parameters: None.
* Result: A data structure linking a specific user to a specific business with their numerical
*          rating (1-5 scale) and textual comment describing their experience.
***/
struct Review {
    std::string business_id;
    std::string user_id;
    int rating;
    std::string comment;
};

/***
* Purpose: Manage business listings, user authentication, reviews, and bookmarks through a MongoDB database,
*          providing comprehensive search, sorting, and user interaction capabilities.
* Parameters: None.
* Result: A complete business directory system that handles user registration and login with security features,
*          allows verified users to add businesses, enables all users to search and filter businesses by various
*          criteria including category, rating, and geographic proximity, supports user reviews with automatic
*          rating aggregation, and provides bookmark functionality for saving favorite businesses.
***/
class BusinessManager {
private:
    mongocxx::instance instance{};
    mongocxx::client client;
    mongocxx::database db;

    mongocxx::collection businesses_coll;
    mongocxx::collection users_coll;
    mongocxx::collection reviews_coll;

    std::optional<User> current_user;
    bool logged_in;
    std::mutex db_mutex;

    /***
    * Purpose: Generate a hashed representation of a password for secure storage in the database.
    * Parameters: The plain-text password string to be hashed.
    * Result: A string containing the hashed password value. This is a simple demonstration hash
    *         using standard library functions with a simulated salt. In production environments,
    *         this should be replaced with industry-standard cryptographic hashing algorithms
    *         such as BCrypt or Argon2 implemented through libraries like OpenSSL.
    ***/
    std::string hashPassword(const std::string& password);

    /***
    * Purpose: Recalculate and update the average rating for a specific business based on all reviews.
    * Parameters: The unique business identifier string for which to update the rating.
    * Result: Aggregates all review ratings for the specified business using MongoDB's aggregation
    *         pipeline, computes the arithmetic mean, and updates the business document's avg_rating
    *         field with the new calculated value. Handles errors gracefully by logging to stderr.
    ***/
    void updateBusinessAverageRating(const std::string& business_id);

    /***
    * Purpose: Validate whether an email address conforms to standard email format patterns.
    * Parameters: The email address string to validate.
    * Result: Returns true if the email matches the expected pattern (text@domain.extension) using
    *         regular expression matching, false otherwise. This checks for basic structural validity
    *         including alphanumeric characters, optional dots or underscores in the local part,
    *         the @ symbol, domain name, and at least one domain extension.
    ***/
    bool isValidEmail(const std::string& email);

    /***
    * Purpose: Query the database and convert MongoDB documents into Business struct objects.
    * Parameters: A BSON document value representing the MongoDB query filter to apply, and optional
    *            find options for sorting or limiting results.
    * Result: Returns a vector of Business objects populated from matching database documents. Safely
    *         handles optional fields and different numeric types (int32 vs double) for ratings. Parses
    *         GeoJSON location data to extract longitude and latitude coordinates. Logs errors to stderr
    *         if query execution fails.
    ***/
    std::vector<Business> fetchBusinesses(bsoncxx::document::value filter,
        mongocxx::options::find opts = mongocxx::options::find{});

    /***
    * Purpose: Display basic information about a business from its database document.
    * Parameters: A BSON document view containing the business data to print.
    * Result: Outputs the business's unique identifier and name to standard output in a formatted string.
    ***/
    void printBusinessDoc(bsoncxx::document::view view);

public:

    bool restoreSession(const std::string& username);

    /***
    * Purpose: Initialize the BusinessManager with a connection to a MongoDB database.
    * Parameters: The MongoDB connection URI string specifying the database server address and credentials,
    *            and the database name string to use for storing collections.
    * Result: Establishes a connection to MongoDB using the provided URI, initializes references to the
    *         businesses, users, and reviews collections, calls ensureIndexes to set up required database
    *         indexes, and sets the initial login state to false. If connection fails, outputs an error
    *         message and terminates the program.
    ***/
    BusinessManager(const std::string& uri_string, const std::string& db_name);

    /***
    * Purpose: Create necessary database indexes to enable geospatial queries on business locations.
    * Parameters: None.
    * Result: Attempts to create a 2dsphere index on the location field of the businesses collection,
    *         which is required for MongoDB's geospatial query operators like $near. Silently handles
    *         the case where the index already exists by catching and ignoring exceptions.
    ***/
    void ensureIndexes();

    /***
    * Purpose: Verify that the user is human and not an automated bot through a simple arithmetic challenge.
    * Parameters: None.
    * Result: Generates two random numbers between 1 and 10, prompts the user to add them together,
    *         accepts their answer via standard input, and returns true if the answer is correct or
    *         false if incorrect. Clears the input buffer after reading to prevent issues with subsequent
    *         input operations. Displays appropriate security messages to the user.
    ***/
    bool performBotCheck();

    /***
    * Purpose: Create a new user account with username, password, and email after validation checks.
    * Parameters: The desired username string, password string for authentication, and email address string.
    * Result: Performs bot verification via CAPTCHA challenge, validates email format using regex, checks
    *         database for existing users with the same username or email, and if all checks pass, inserts
    *         a new user document with hashed password, unverified status, and empty bookmarks array.
    *         Returns true if registration succeeds, false if any validation fails or database operation
    *         encounters an error. Provides detailed feedback messages for each failure scenario.
    ***/
    bool registerUser(const std::string& username, const std::string& password, const std::string& email);

    /***
    * Purpose: Authenticate a user and establish a logged-in session.
    * Parameters: The username string and password string to verify against stored credentials.
    * Result: Hashes the provided password and queries the database for a matching username and password
    *         combination. If found, sets logged_in to true, populates the current_user with all user data
    *         including their ID, username, email, verification status, and bookmarks, then welcomes the
    *         user by name. Returns true on successful login, false if credentials don't match any user.
    ***/
    bool login(const std::string& username, const std::string& password);

    /***
    * Purpose: End the current user session and clear all session data.
    * Parameters: None.
    * Result: Sets logged_in to false, resets the current_user optional to empty state, and displays
    *         a logout confirmation message.
    ***/
    void logout();

    /***
    * Purpose: Check whether a user is currently authenticated.
    * Parameters: None.
    * Result: Returns the current value of the logged_in boolean flag.
    ***/
    bool isLoggedIn() const;

    /***
    * Purpose: Insert a new business listing into the database with complete details and location.
    * Parameters: Business name string, category string, description text, longitude coordinate as double,
    *            latitude coordinate as double, and special deal or coupon text string.
    * Result: Verifies that the current user is both logged in and verified before proceeding. Creates a
    *         BSON document with all business fields including a GeoJSON Point structure for the location
    *         coordinates, initializes avg_rating to 0.0, inserts the document into the businesses collection,
    *         and confirms the addition with a success message. Denies the operation with an error message
    *         if the user is not logged in or not verified.
    ***/
    void addBusiness(const std::string& name, const std::string& category,
        const std::string& desc, double lon, double lat, const std::string& deal);

    /***
    * Purpose: Retrieve all business listings from the database without any filtering.
    * Parameters: None.
    * Result: Returns a vector containing Business objects for every document in the businesses collection.
    ***/
    std::vector<Business> getAllBusinesses();

    /***
    * Purpose: Retrieve business listings that belong to a specific category.
    * Parameters: The category name string to filter by.
    * Result: Returns a vector of Business objects where the category field matches the provided value.
    ***/
    std::vector<Business> getBusinessesByCategory(const std::string& category);

    /***
    * Purpose: Retrieve all business listings sorted by their average rating in descending order.
    * Parameters: None.
    * Result: Returns a vector of Business objects ordered from highest to lowest avg_rating, allowing
    *         users to see the best-rated businesses first.
    ***/
    std::vector<Business> getBusinessesByRating();

    /***
    * Purpose: Retrieve business listings sorted by proximity to a specific geographic location.
    * Parameters: The longitude coordinate as double and latitude coordinate as double representing the
    *            reference point from which to measure distances.
    * Result: Uses MongoDB's $near geospatial query operator with the 2dsphere index to find and sort
    *         businesses by their distance from the provided coordinates. Returns a vector of Business
    *         objects ordered from nearest to farthest, enabling location-based search functionality.
    ***/
    std::vector<Business> getBusinessesByLocation(double lon, double lat);

    /***
    * Purpose: Submit a new review for a business or update an existing review by the current user.
    * Parameters: The business identifier string to review, an integer rating value between 1 and 5,
    *            and a comment string containing the review text.
    * Result: Verifies the user is logged in and the rating is within valid range (1-5). Uses MongoDB's
    *         upsert operation to either insert a new review document if the user hasn't reviewed this
    *         business before, or update their existing review with the new rating and comment. After
    *         the review is saved, automatically triggers recalculation of the business's average rating.
    *         Provides error messages if the user is not logged in, rating is invalid, or database
    *         operation fails.
    ***/
    void addOrEditReview(const std::string& business_id, int rating, const std::string& comment);

    /***
    * Purpose: Add or remove a business from the current user's bookmarks collection.
    * Parameters: The business identifier string to bookmark or unbookmark.
    * Result: Checks if the business ID already exists in the current user's bookmarks vector. If found,
    *         removes it from both the in-memory vector and the database using MongoDB's $pull operator,
    *         then confirms removal. If not found, adds it to both the in-memory vector and database
    *         using $addToSet operator to prevent duplicates, then confirms addition. Only operates if
    *         a user is logged in.
    ***/
    void toggleBookmark(const std::string& business_id);

    /***
    * Purpose: Display all businesses that the current user has bookmarked.
    * Parameters: None.
    * Result: Checks if user is logged in and has any bookmarks saved. For each bookmarked business ID,
    *         retrieves the corresponding business document from the database and displays its information
    *         using printBusinessDoc. Shows a message if no bookmarks exist or user is not logged in.
    ***/
    void displayBookmarks();

    /***
    * Purpose: Retrieve all reviews for a specific business from the database.
    * Parameters: The business identifier string for which to fetch reviews.
    * Result: Queries the reviews collection for all documents matching the business_id, converts each
    *         review document into a Review struct containing the business_id, user_id, rating, and comment,
    *         and returns a vector of all reviews. Returns an empty vector if no reviews exist or if an
    *         error occurs during the query.
    ***/
    std::vector<Review> getReviewsForBusiness(const std::string& business_id);

    bool isVerified() const {
        return logged_in && current_user && current_user.has_value() && current_user->is_verified;
    }

    bool verifyAccount(const std::string& email);
};