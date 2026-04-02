/*
#define _USE_MATH_DEFINES // M_PI

#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <conio.h> // For _getch() on Windows
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdlib> // Environment variables

#include "BusinessManager.hpp"
#include "Table.hpp"
#include "UI.hpp"
#include "ansi.hpp"

// Database connection configuration
const std::string DB_NAME = "MainstreetMetricsDB";

namespace Config {
    // Validation constants
    const size_t MIN_USERNAME_LENGTH = 3;
    const size_t MAX_USERNAME_LENGTH = 20;
    const size_t MIN_PASSWORD_LENGTH = 8;
    const size_t MAX_BUSINESS_NAME_LENGTH = 100;
    const size_t MAX_DESCRIPTION_LENGTH = 500;
    const int MIN_RATING = 1;
    const int MAX_RATING = 5;
    const double MIN_LATITUDE = -90.0;
    const double MAX_LATITUDE = 90.0;
    const double MIN_LONGITUDE = -180.0;
    const double MAX_LONGITUDE = 180.0;

    // UI constants
    const int CURSOR_END_ROW = 30;
    const int DESCRIPTION_DISPLAY_LENGTH = 50;

    // Earth radius in kilometers for distance calculation
    const double EARTH_RADIUS_KM = 6371.0;

    // Valid categories - limited to what can be sorted by
    const std::vector<std::string> VALID_CATEGORIES = {
        "food", "retail", "services", "entertainment", "healthcare"
    };
}

/***
* Purpose: Converts degrees to radians for geographic calculations.
* Parameters: Angle in degrees.
* Result: Angle in radians.
***/
double degreesToRadians(double degrees) {
    return degrees * M_PI / 180.0;
}

/***
* Purpose: Calculates the great-circle distance between two geographic points using the Haversine formula.
* Parameters: Latitude and longitude of two points in degrees.
* Result: Distance in kilometers.
***/
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    // Convert coordinates to radians
    double lat1Rad = degreesToRadians(lat1);
    double lat2Rad = degreesToRadians(lat2);
    double deltaLat = degreesToRadians(lat2 - lat1);
    double deltaLon = degreesToRadians(lon2 - lon1);

    // Haversine formula
    double a = std::sin(deltaLat / 2) * std::sin(deltaLat / 2) +
        std::cos(lat1Rad) * std::cos(lat2Rad) *
        std::sin(deltaLon / 2) * std::sin(deltaLon / 2);

    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

    return Config::EARTH_RADIUS_KM * c;
}

/***
* Purpose: Extends the Business structure to include calculated distance from user.
* Parameters: None (struct definition).
***/
struct BusinessWithDistance {
    Business business;
    double distanceKm;

    BusinessWithDistance(const Business& b, double dist = 0.0)
        : business(b), distanceKm(dist) {
    }
};

/***
* Purpose: Validates username input according to security rules.
* Parameters: The username string to validate.
* Result: True if valid, false otherwise with error message displayed.
***/
bool validateUsername(const std::string& username) {
    if (username.length() < Config::MIN_USERNAME_LENGTH ||
        username.length() > Config::MAX_USERNAME_LENGTH) {
        std::cout << "[Validation Error] Username must be "
            << Config::MIN_USERNAME_LENGTH << "-"
            << Config::MAX_USERNAME_LENGTH << " characters.\n";
        return false;
    }

    // Check for alphanumeric and underscores only
    for (char c : username) {
        if (!std::isalnum(c) && c != '_') {
            std::cout << "[Validation Error] Username can only contain letters, numbers, and underscores.\n";
            return false;
        }
    }

    return true;
}

/***
* Purpose: Validates password strength according to security requirements.
* Parameters: The password string to validate.
* Result: True if valid, false otherwise with error message displayed.
***/
bool validatePassword(const std::string& password) {
    if (password.length() < Config::MIN_PASSWORD_LENGTH) {
        std::cout << "[Validation Error] Password must be at least "
            << Config::MIN_PASSWORD_LENGTH << " characters.\n";
        return false;
    }

    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;

    for (char c : password) {
        if (std::isupper(c)) hasUpper = true;
        if (std::islower(c)) hasLower = true;
        if (std::isdigit(c)) hasDigit = true;
        if (std::ispunct(c)) hasSpecial = true;
    }

    if (!hasUpper || !hasLower || !hasDigit) {
        std::cout << "[Validation Error] Password must contain uppercase, lowercase, and digits.\n";
        return false;
    }

    return true;
}

/***
* Purpose: Validates rating input to ensure it's within acceptable range.
* Parameters: The rating value to validate.
* Result: True if valid (1-5), false otherwise with error message displayed.
***/
bool validateRating(int rating) {
    if (rating < Config::MIN_RATING || rating > Config::MAX_RATING) {
        std::cout << "[Validation Error] Rating must be between "
            << Config::MIN_RATING << " and " << Config::MAX_RATING << ".\n";
        return false;
    }
    return true;
}

/***
* Purpose: Validates business name input with semantic validation.
* Parameters: The business name string to validate.
* Result: True if valid, false otherwise with error message displayed.
***/
bool validateBusinessName(const std::string& name) {
    // Syntactic validation
    if (name.empty() || name.length() > Config::MAX_BUSINESS_NAME_LENGTH) {
        std::cout << "[Validation Error] Business name must be 1-"
            << Config::MAX_BUSINESS_NAME_LENGTH << " characters.\n";
        return false;
    }

    // Semantic validation - check for meaningful content
    bool hasAlphanumeric = false;
    for (char c : name) {
        if (std::isalnum(c)) {
            hasAlphanumeric = true;
            break;
        }
    }

    if (!hasAlphanumeric) {
        std::cout << "[Validation Error] Business name must contain at least one letter or number.\n";
        return false;
    }

    return true;
}

/***
* Purpose: Validates category against the list of valid categories.
* Parameters: The category string to validate.
* Result: True if valid, false otherwise with error message displayed.
***/
bool validateCategory(const std::string& category) {
    auto it = std::find(Config::VALID_CATEGORIES.begin(), Config::VALID_CATEGORIES.end(), category);
    if (it == Config::VALID_CATEGORIES.end()) {
        std::cout << "[Validation Error] Category must be one of: ";
        for (size_t i = 0; i < Config::VALID_CATEGORIES.size(); ++i) {
            std::cout << Config::VALID_CATEGORIES[i];
            if (i < Config::VALID_CATEGORIES.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        return false;
    }
    return true;
}

/***
* Purpose: Validates latitude coordinate with range checking.
* Parameters: The latitude value to validate.
* Result: True if valid (-90 to 90), false otherwise with error message displayed.
***/
bool validateLatitude(double lat) {
    if (lat < Config::MIN_LATITUDE || lat > Config::MAX_LATITUDE) {
        std::cout << "[Validation Error] Latitude must be between "
            << Config::MIN_LATITUDE << " and " << Config::MAX_LATITUDE << ".\n";
        return false;
    }
    return true;
}

/***
* Purpose: Validates longitude coordinate with range checking.
* Parameters: The longitude value to validate.
* Result: True if valid (-180 to 180), false otherwise with error message displayed.
***/
bool validateLongitude(double lon) {
    if (lon < Config::MIN_LONGITUDE || lon > Config::MAX_LONGITUDE) {
        std::cout << "[Validation Error] Longitude must be between "
            << Config::MIN_LONGITUDE << " and " << Config::MAX_LONGITUDE << ".\n";
        return false;
    }
    return true;
}

/***
* Purpose: Validates description length.
* Parameters: The description string to validate.
* Result: True if valid, false otherwise with error message displayed.
***/
bool validateDescription(const std::string& description) {
    if (description.length() > Config::MAX_DESCRIPTION_LENGTH) {
        std::cout << "[Validation Error] Description must be under "
            << Config::MAX_DESCRIPTION_LENGTH << " characters.\n";
        return false;
    }
    return true;
}

/***
* Purpose: Safely reads a line of input with buffer clearing and quit functionality.
* Parameters: None.
* Result: The input string with trailing whitespace removed, or "QUIT_SIGNAL" if user wants to quit.
***/
std::string safeGetline() {
    std::string input;
    std::getline(std::cin, input);

    // Check for quit command (case-insensitive)
    std::string lowerInput = input;
    std::transform(lowerInput.begin(), lowerInput.end(), lowerInput.begin(), ::tolower);
    if (lowerInput == "quit") {
        return "QUIT_SIGNAL";
    }

    return input;
}

/***
* Purpose: Safely reads an integer with comprehensive error handling, validation, and quit functionality.
* Parameters: None.
* Result: The validated integer value, or INT_MIN if user wants to quit.
***/
int safeGetInt() {
    std::string input;
    int value;

    while (true) {
        std::getline(std::cin, input);

        // Check for quit
        std::string lowerInput = input;
        std::transform(lowerInput.begin(), lowerInput.end(), lowerInput.begin(), ::tolower);
        if (lowerInput == "quit") {
            return INT_MIN;
        }

        std::istringstream iss(input);
        if (iss >> value && iss.eof()) {
            return value;
        }

        std::cout << "[Input Error] Please enter a valid integer number (or 'quit' to cancel): ";
    }
}

/***
* Purpose: Safely reads a double with comprehensive error handling, validation, and quit functionality.
* Parameters: None.
* Result: The validated double value, or NaN if user wants to quit.
***/
double safeGetDouble() {
    std::string input;
    double value;

    while (true) {
        std::getline(std::cin, input);

        // Check for quit
        std::string lowerInput = input;
        std::transform(lowerInput.begin(), lowerInput.end(), lowerInput.begin(), ::tolower);
        if (lowerInput == "quit") {
            return std::numeric_limits<double>::quiet_NaN();
        }

        std::istringstream iss(input);
        if (iss >> value && iss.eof()) {
            return value;
        }

        std::cout << "[Input Error] Please enter a valid decimal number (or 'quit' to cancel): ";
    }
}

/***
* Purpose: Truncates a string to a specified length and adds ellipsis if needed.
* Parameters: The input string and maximum length.
* Result: The truncated string with "..." appended if truncation occurred.
***/
std::string truncateString(const std::string& str, size_t maxLength) {
    if (str.length() <= maxLength) {
        return str;
    }
    return str.substr(0, maxLength - 3) + "...";
}

/***
* Purpose: Escapes special characters in strings for CSV export.
* Parameters: The input string to escape.
* Result: CSV-safe string with quotes and commas properly escaped.
***/
std::string escapeCSV(const std::string& str) {
    if (str.find(',') != std::string::npos ||
        str.find('"') != std::string::npos ||
        str.find('\n') != std::string::npos) {
        std::string escaped = "\"";
        for (char c : str) {
            if (c == '"') escaped += "\"\"";
            else escaped += c;
        }
        escaped += "\"";
        return escaped;
    }
    return str;
}

/***
* Purpose: Exports a single business with its reviews to a CSV file.
* Parameters: Business object, vector of reviews, filename, and user location coordinates.
* Result: None, creates CSV file and displays success/error message to user.
***/
void exportBusinessToCSV(const Business& business,
    const std::vector<Review>& reviews,
    const std::string& filename,
    double userLat = 0.0,
    double userLon = 0.0) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << "[Error] Could not create file: " << filename << "\n";
        return;
    }

    double distance = calculateDistance(userLat, userLon, business.latitude, business.longitude);

    // Write CSV header
    file << "Business ID,Name,Category,Description,Average Rating,Special Deal,";
    file << "Latitude,Longitude,Distance (km),Review User,Review Rating,Review Comment\n";

    // If there are reviews, write one row per review
    if (!reviews.empty()) {
        for (const auto& review : reviews) {
            file << escapeCSV(business.id) << ","
                << escapeCSV(business.name) << ","
                << escapeCSV(business.category) << ","
                << escapeCSV(business.description) << ","
                << std::fixed << std::setprecision(2) << business.avg_rating << ","
                << escapeCSV(business.special_deal.empty() ? "None" : business.special_deal) << ","
                << std::setprecision(6) << business.latitude << ","
                << business.longitude << ","
                << std::setprecision(2) << distance << ","
                << escapeCSV(review.user_id) << ","
                << review.rating << ","
                << escapeCSV(review.comment) << "\n";
        }
    }
    else {
        // No reviews - write business data with empty review fields
        file << escapeCSV(business.id) << ","
            << escapeCSV(business.name) << ","
            << escapeCSV(business.category) << ","
            << escapeCSV(business.description) << ","
            << std::fixed << std::setprecision(2) << business.avg_rating << ","
            << escapeCSV(business.special_deal.empty() ? "None" : business.special_deal) << ","
            << std::setprecision(6) << business.latitude << ","
            << business.longitude << ","
            << std::setprecision(2) << distance << ","
            << "," << "," << "\n";
    }

    file.close();

    std::cout << fg(Ansi::Color::Green) << "[Success] " << Ansi::Reset
        << "Data exported to: " << filename << "\n";
    std::cout << "Business data with " << reviews.size() << " review(s) exported.\n";
}

/***
* Purpose: Displays a list of businesses in a formatted table with comprehensive information.
* Parameters: Vector of BusinessWithDistance objects, table title, flag for showing distance.
* Result: None, prints formatted table to console with ID, name, category, rating, description, and optional distance.
***/
void displayBusinessTable(const std::vector<BusinessWithDistance>& businesses,
    const std::string& title = "Businesses",
    bool showDistance = false) {
    if (businesses.empty()) {
        std::cout << "\n" << fg(Ansi::Color::Yellow) << "[Info] " << Ansi::Reset
            << "No businesses to display.\n";
        return;
    }

    // Table headers
    std::vector<std::string> headers = { "ID", "Name", "Category", "Rating", "Description" };
    std::vector<size_t> widths = { 26, 20, 12, 7, 40 };

    if (showDistance) {
        headers.push_back("Distance");
        widths.push_back(12);
    }

    Table table(headers, widths, '=', '|');

    // Add rows
    for (const auto& bwd : businesses) {
        const Business& b = bwd.business;

        std::ostringstream ratingStream;
        ratingStream << std::fixed << std::setprecision(1) << b.avg_rating << " #";

        std::string shortDesc = truncateString(b.description, Config::DESCRIPTION_DISPLAY_LENGTH);

        std::vector<std::string> row = {
            truncateString(b.id, 24),
            truncateString(b.name, 18),
            truncateString(b.category, 10),
            ratingStream.str(),
            shortDesc
        };

        if (showDistance) {
            std::ostringstream distStream;
            distStream << std::fixed << std::setprecision(2) << bwd.distanceKm << " km";
            row.push_back(distStream.str());
        }

        table.addRow(row);
    }

    std::cout << "\n" << Ansi::Bold << fg(Ansi::Color::Cyan) << "=== " << title << " ==="
        << Ansi::Reset << "\n";
    table.print();
    std::cout << fg(Ansi::Color::BrightBlack) << "Tip: Note the Business ID to view details or bookmark."
        << Ansi::Reset << "\n\n";
}

/***
* Purpose: Displays detailed information about a specific business including full description and reviews.
* Parameters: BusinessManager reference, business ID string.
* Result: None, prints detailed business information and reviews to console.
***/
void displayBusinessDetails(BusinessManager& manager, const std::string& businessId) {
    // Get all businesses and find the one with matching ID
    std::vector<Business> allBusinesses = manager.getAllBusinesses();
    Business* selectedBusiness = nullptr;

    for (auto& b : allBusinesses) {
        if (b.id == businessId) {
            selectedBusiness = &b;
            break;
        }
    }

    if (!selectedBusiness) {
        std::cout << fg(Ansi::Color::Red) << "[Error] " << Ansi::Reset
            << "Business not found with ID: " << businessId << "\n";
        return;
    }

    // Display detailed information
    std::cout << "\n" << Ansi::Bold << bg(Ansi::BgColor::Blue) << fg(Ansi::Color::White)
        << "==================================================================="
        << Ansi::Reset << "\n";
    std::cout << Ansi::Bold << fg(Ansi::Color::Cyan) << "  BUSINESS DETAILS" << Ansi::Reset << "\n";
    std::cout << Ansi::Bold << bg(Ansi::BgColor::Blue) << fg(Ansi::Color::White)
        << "==================================================================="
        << Ansi::Reset << "\n\n";

    std::cout << Ansi::Bold << "Business ID:  " << Ansi::Reset << selectedBusiness->id << "\n";
    std::cout << Ansi::Bold << "Name:         " << Ansi::Reset << selectedBusiness->name << "\n";
    std::cout << Ansi::Bold << "Category:     " << Ansi::Reset << selectedBusiness->category << "\n";
    std::cout << Ansi::Bold << "Rating:       " << Ansi::Reset << fg(Ansi::Color::Yellow);

    // Display star rating visually
    std::cout << std::fixed << std::setprecision(1) << selectedBusiness->avg_rating << " ";
    int fullStars = static_cast<int>(selectedBusiness->avg_rating);
    for (int i = 0; i < fullStars; ++i) {
        std::cout << "#";
    }
    for (int i = fullStars; i < 5; ++i) {
        std::cout << "-";
    }
    std::cout << Ansi::Reset << "\n";

    std::cout << Ansi::Bold << "Location:     " << Ansi::Reset
        << "(" << selectedBusiness->latitude << ", "
        << selectedBusiness->longitude << ")\n";

    if (!selectedBusiness->special_deal.empty()) {
        std::cout << Ansi::Bold << fg(Ansi::Color::Green) << "Special Deal: " << Ansi::Reset
            << bg(Ansi::BgColor::Green) << fg(Ansi::Color::Black) << " " << selectedBusiness->special_deal
            << " " << Ansi::Reset << "\n";
    }

    std::cout << "\n" << Ansi::Bold << "Description:" << Ansi::Reset << "\n";
    std::cout << selectedBusiness->description << "\n";

    // Get and display reviews (up to 3)
    std::cout << "\n" << Ansi::Bold << fg(Ansi::Color::Cyan) << "--- Recent Reviews ---" << Ansi::Reset << "\n";

    std::vector<Review> reviews = manager.getReviewsForBusiness(businessId);

    if (reviews.empty()) {
        std::cout << fg(Ansi::Color::BrightBlack) << "No reviews yet. Be the first to review!\n" << Ansi::Reset;
    }
    else {
        int displayCount = std::min(3, static_cast<int>(reviews.size()));
        for (int i = 0; i < displayCount; ++i) {
            std::cout << "\n" << fg(Ansi::Color::Yellow);
            for (int j = 0; j < reviews[i].rating; ++j) std::cout << "#";
            for (int j = reviews[i].rating; j < 5; ++j) std::cout << "-";
            std::cout << Ansi::Reset << " (" << reviews[i].rating << "/5)\n";
            std::cout << "User: " << reviews[i].user_id << "\n";
            std::cout << "\"" << reviews[i].comment << "\"\n";
        }

        if (reviews.size() > 3) {
            std::cout << "\n" << fg(Ansi::Color::BrightBlack)
                << "... and " << (reviews.size() - 3) << " more review(s)\n"
                << Ansi::Reset;
        }
    }

    std::cout << "\n" << Ansi::Bold << bg(Ansi::BgColor::Blue) << fg(Ansi::Color::White)
        << "==================================================================="
        << Ansi::Reset << "\n\n";
}

/***
* Purpose: Moves cursor to end of screen to prevent UI element overlap.
* Parameters: The row number to move cursor to (default: configured end row).
* Result: None, repositions terminal cursor.
***/
void moveCursorToEnd(int row = Config::CURSOR_END_ROW) {
    std::cout << Ansi::moveTo(row, 1) << std::flush;
}

/***
* Purpose: Displays a help screen with navigation instructions and feature overview.
* Parameters: None.
* Result: None, prints help information to console.
***/
void displayHelpScreen() {
    std::cout << Ansi::clearScreen << Ansi::moveTo(1, 1);
    std::cout << Ansi::Bold << bg(Ansi::BgColor::Cyan) << fg(Ansi::Color::Black)
        << "==================================================================="
        << Ansi::Reset << "\n";
    std::cout << Ansi::Bold << fg(Ansi::Color::Cyan) << "            MAINSTREET METRICS - HELP & INSTRUCTIONS"
        << Ansi::Reset << "\n";
    std::cout << Ansi::Bold << bg(Ansi::BgColor::Cyan) << fg(Ansi::Color::Black)
        << "==================================================================="
        << Ansi::Reset << "\n\n";

    std::cout << Ansi::Bold << "NAVIGATION:" << Ansi::Reset << "\n";
    std::cout << "  * Use " << fg(Ansi::Color::Green) << "ARROW KEYS" << Ansi::Reset
        << " or " << fg(Ansi::Color::Green) << "WASD" << Ansi::Reset << " to move between menu options\n";
    std::cout << "  * Press " << fg(Ansi::Color::Yellow) << "ENTER" << Ansi::Reset
        << " or " << fg(Ansi::Color::Yellow) << "SPACE" << Ansi::Reset << " to select\n";
    std::cout << "  * Press " << fg(Ansi::Color::Red) << "ESC" << Ansi::Reset << " to go back or cancel\n";
    std::cout << "  * Type " << fg(Ansi::Color::Red) << "QUIT" << Ansi::Reset
        << " at any input prompt to cancel the current operation\n\n";

    std::cout << Ansi::Bold << "FEATURES:" << Ansi::Reset << "\n";
    std::cout << "  > " << fg(Ansi::Color::Cyan) << "Register/Login" << Ansi::Reset
        << " - Create account with bot verification\n";
    std::cout << "  > " << fg(Ansi::Color::Cyan) << "Browse Businesses" << Ansi::Reset
        << " - View by category, rating, or location\n";
    std::cout << "  > " << fg(Ansi::Color::Cyan) << "Add Business" << Ansi::Reset
        << " - Verified users can add new businesses\n";
    std::cout << "  > " << fg(Ansi::Color::Cyan) << "Add Review" << Ansi::Reset
        << " - Rate and review businesses (1-5 stars)\n";
    std::cout << "  > " << fg(Ansi::Color::Cyan) << "Bookmarks" << Ansi::Reset
        << " - Save your favorite businesses\n";
    std::cout << "  > " << fg(Ansi::Color::Cyan) << "Export CSV" << Ansi::Reset
        << " - Generate individual business reports with reviews\n\n";

    std::cout << Ansi::Bold << "VALID CATEGORIES:" << Ansi::Reset << "\n";
    std::cout << "  * ";
    for (size_t i = 0; i < Config::VALID_CATEGORIES.size(); ++i) {
        std::cout << fg(Ansi::Color::Cyan) << Config::VALID_CATEGORIES[i] << Ansi::Reset;
        if (i < Config::VALID_CATEGORIES.size() - 1) std::cout << ", ";
    }
    std::cout << "\n\n";

    std::cout << Ansi::Bold << "TIPS:" << Ansi::Reset << "\n";
    std::cout << "  * Business IDs are displayed in tables - use them to view details\n";
    std::cout << "  * Location sorting shows distance from your coordinates\n";
    std::cout << "  * CSV exports include business data with all reviews\n";
    std::cout << "  * Only verified users can add businesses\n";
    std::cout << "  * Type 'quit' at any input prompt to cancel\n\n";

    std::cout << Ansi::Bold << "ACCESSIBILITY:" << Ansi::Reset << "\n";
    std::cout << "  * Color-coded UI for easy navigation\n";
    std::cout << "  * Clear error messages and validation feedback\n";
    std::cout << "  * Keyboard-only navigation (no mouse required)\n";
    std::cout << "  * Table format for structured data viewing\n\n";

    std::cout << fg(Ansi::Color::BrightBlack) << "Press any key to return to main menu..." << Ansi::Reset;
    _getch();
}

/***
* Purpose: Handles keyboard input for UI navigation using arrow keys or WASD with visual feedback.
* Parameters: Reference to MenuManager object.
* Result: Returns selected menu option name as string, or empty string if cancelled.
***/
std::string navigateMenu(MenuManager& menu) {
    std::cout << Ansi::hideCursor;

    while (true) {
        std::cout << Ansi::clearScreen << Ansi::moveTo(1, 1);
        menu.draw();

        // Display navigation hint at bottom
        std::cout << Ansi::moveTo(22, 1) << fg(Ansi::Color::BrightBlack)
            << "Use Arrow Keys/WASD to navigate | ENTER/SPACE to select | ESC to go back | H for Help"
            << Ansi::Reset;

        int key = _getch();

        // Handle special keys (arrows)
        if (key == 224 || key == 0) {
            key = _getch();
            switch (key) {
            case 72: // Up arrow
                menu.navigate(Direction::Up);
                break;
            case 80: // Down arrow
                menu.navigate(Direction::Down);
                break;
            case 75: // Left arrow
                menu.navigate(Direction::Left);
                break;
            case 77: // Right arrow
                menu.navigate(Direction::Right);
                break;
            }
        }
        // Handle WASD
        else if (key == 'w' || key == 'W') {
            menu.navigate(Direction::Up);
        }
        else if (key == 's' || key == 'S') {
            menu.navigate(Direction::Down);
        }
        else if (key == 'a' || key == 'A') {
            menu.navigate(Direction::Left);
        }
        else if (key == 'd' || key == 'D') {
            menu.navigate(Direction::Right);
        }
        // Handle help
        else if (key == 'h' || key == 'H') {
            std::cout << Ansi::showCursor;
            displayHelpScreen();
            std::cout << Ansi::hideCursor;
        }
        // Handle selection
        else if (key == 13 || key == ' ') { // Enter or Space
            std::cout << Ansi::showCursor;
            std::cout << Ansi::clearScreen << Ansi::moveTo(1, 1);
            return menu.getSelectedName();
        }
        // Handle escape
        else if (key == 27) { // ESC
            std::cout << Ansi::showCursor;
            std::cout << Ansi::clearScreen << Ansi::moveTo(1, 1);
            return "";
        }
    }
}

/***
* Purpose: Handles user login process with comprehensive input validation and security checks.
* Parameters: Reference to BusinessManager object.
* Result: None, updates login state in BusinessManager and displays result to user.
***/
void loginUser(BusinessManager& manager) {
    std::cout << "\n" << Ansi::Bold << fg(Ansi::Color::Cyan) << "=== User Login ==="
        << Ansi::Reset << "\n";
    std::cout << fg(Ansi::Color::BrightBlack) << "(Type 'quit' to cancel)" << Ansi::Reset << "\n";

    std::cout << "Username: ";
    std::string username = safeGetline();
    if (username == "QUIT_SIGNAL") {
        std::cout << fg(Ansi::Color::Yellow) << "Login cancelled.\n" << Ansi::Reset;
        std::cout << "\nPress any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    std::cout << "Password: ";
    std::string password = safeGetline();
    if (password == "QUIT_SIGNAL") {
        std::cout << fg(Ansi::Color::Yellow) << "Login cancelled.\n" << Ansi::Reset;
        std::cout << "\nPress any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    manager.login(username, password);

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Handles new user registration with strict multi-level validation and bot protection.
* Parameters: Reference to BusinessManager object.
* Result: None, creates new user account if all validation passes and displays result to user.
***/
void registerUser(BusinessManager& manager) {
    std::cout << "\n" << Ansi::Bold << fg(Ansi::Color::Cyan) << "=== User Registration ==="
        << Ansi::Reset << "\n";
    std::cout << fg(Ansi::Color::BrightBlack) << "(Type 'quit' to cancel at any prompt)" << Ansi::Reset << "\n";

    std::string username, password, email;

    // Username validation loop with syntactic and semantic checks
    while (true) {
        std::cout << "Username (" << Config::MIN_USERNAME_LENGTH << "-"
            << Config::MAX_USERNAME_LENGTH << " chars, alphanumeric + underscore): ";
        username = safeGetline();
        if (username == "QUIT_SIGNAL") {
            std::cout << fg(Ansi::Color::Yellow) << "Registration cancelled.\n" << Ansi::Reset;
            std::cout << "\nPress any key to continue...";
            _getch();
            moveCursorToEnd();
            return;
        }
        if (validateUsername(username)) break;
    }

    // Password validation loop with strength requirements
    while (true) {
        std::cout << "Password (" << Config::MIN_PASSWORD_LENGTH
            << "+ chars, must have upper, lower, digit): ";
        password = safeGetline();
        if (password == "QUIT_SIGNAL") {
            std::cout << fg(Ansi::Color::Yellow) << "Registration cancelled.\n" << Ansi::Reset;
            std::cout << "\nPress any key to continue...";
            _getch();
            moveCursorToEnd();
            return;
        }
        if (validatePassword(password)) break;
    }

    // Email input
    std::cout << "Email: ";
    email = safeGetline();
    if (email == "QUIT_SIGNAL") {
        std::cout << fg(Ansi::Color::Yellow) << "Registration cancelled.\n" << Ansi::Reset;
        std::cout << "\nPress any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    manager.registerUser(username, password, email);

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Handles adding a new business with comprehensive validation on all inputs.
* Parameters: Reference to BusinessManager object.
* Result: None, adds business to database if all validations pass and user is verified.
***/
void addBusiness(BusinessManager& manager) {
    if (!manager.isLoggedIn()) {
        std::cout << "\n" << fg(Ansi::Color::Red) << "[Error] " << Ansi::Reset
            << "You must be logged in to add businesses.\n";
        std::cout << "Press any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    std::cout << "\n" << Ansi::Bold << fg(Ansi::Color::Cyan) << "=== Add New Business ==="
        << Ansi::Reset << "\n";
    std::cout << fg(Ansi::Color::BrightBlack) << "(Type 'quit' to cancel at any prompt)" << Ansi::Reset << "\n";

    std::string name, category, description, deal;
    double longitude, latitude;

    // Business name validation with syntactic and semantic checks
    while (true) {
        std::cout << "Business Name (1-" << Config::MAX_BUSINESS_NAME_LENGTH << " chars): ";
        name = safeGetline();
        if (name == "QUIT_SIGNAL") {
            std::cout << fg(Ansi::Color::Yellow) << "Add business cancelled.\n" << Ansi::Reset;
            std::cout << "\nPress any key to continue...";
            _getch();
            moveCursorToEnd();
            return;
        }
        if (validateBusinessName(name)) break;
    }

    // Category validation with limited options
    while (true) {
        std::cout << "Category (";
        for (size_t i = 0; i < Config::VALID_CATEGORIES.size(); ++i) {
            std::cout << Config::VALID_CATEGORIES[i];
            if (i < Config::VALID_CATEGORIES.size() - 1) std::cout << ", ";
        }
        std::cout << "): ";
        category = safeGetline();
        if (category == "QUIT_SIGNAL") {
            std::cout << fg(Ansi::Color::Yellow) << "Add business cancelled.\n" << Ansi::Reset;
            std::cout << "\nPress any key to continue...";
            _getch();
            moveCursorToEnd();
            return;
        }
        if (validateCategory(category)) break;
    }

    // Description validation
    while (true) {
        std::cout << "Description (max " << Config::MAX_DESCRIPTION_LENGTH << " chars): ";
        description = safeGetline();
        if (description == "QUIT_SIGNAL") {
            std::cout << fg(Ansi::Color::Yellow) << "Add business cancelled.\n" << Ansi::Reset;
            std::cout << "\nPress any key to continue...";
            _getch();
            moveCursorToEnd();
            return;
        }
        if (validateDescription(description)) break;
    }

    // Longitude validation with range checking
    while (true) {
        std::cout << "Longitude (" << Config::MIN_LONGITUDE << " to "
            << Config::MAX_LONGITUDE << "): ";
        longitude = safeGetDouble();
        if (std::isnan(longitude)) {
            std::cout << fg(Ansi::Color::Yellow) << "Add business cancelled.\n" << Ansi::Reset;
            std::cout << "\nPress any key to continue...";
            _getch();
            moveCursorToEnd();
            return;
        }
        if (validateLongitude(longitude)) break;
    }

    // Latitude validation with range checking
    while (true) {
        std::cout << "Latitude (" << Config::MIN_LATITUDE << " to "
            << Config::MAX_LATITUDE << "): ";
        latitude = safeGetDouble();
        if (std::isnan(latitude)) {
            std::cout << fg(Ansi::Color::Yellow) << "Add business cancelled.\n" << Ansi::Reset;
            std::cout << "\nPress any key to continue...";
            _getch();
            moveCursorToEnd();
            return;
        }
        if (validateLatitude(latitude)) break;
    }

    std::cout << "Special Deal/Coupon (optional, press Enter to skip): ";
    deal = safeGetline();
    if (deal == "QUIT_SIGNAL") {
        std::cout << fg(Ansi::Color::Yellow) << "Add business cancelled.\n" << Ansi::Reset;
        std::cout << "\nPress any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    manager.addBusiness(name, category, description, longitude, latitude, deal);

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Handles adding or editing a review with comprehensive input validation.
* Parameters: Reference to BusinessManager object.
* Result: None, adds or updates review in database if user is logged in and inputs are valid.
***/
void addReview(BusinessManager& manager) {
    if (!manager.isLoggedIn()) {
        std::cout << "\n" << fg(Ansi::Color::Red) << "[Error] " << Ansi::Reset
            << "You must be logged in to add reviews.\n";
        std::cout << "Press any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    std::cout << "\n" << Ansi::Bold << fg(Ansi::Color::Cyan) << "=== Add/Edit Review ==="
        << Ansi::Reset << "\n";
    std::cout << fg(Ansi::Color::BrightBlack) << "Tip: Get Business ID from the Browse menu"
        << Ansi::Reset << "\n";
    std::cout << fg(Ansi::Color::BrightBlack) << "(Type 'quit' to cancel at any prompt)" << Ansi::Reset << "\n\n";

    std::cout << "Business ID: ";
    std::string businessId = safeGetline();
    if (businessId == "QUIT_SIGNAL") {
        std::cout << fg(Ansi::Color::Yellow) << "Add review cancelled.\n" << Ansi::Reset;
        std::cout << "\nPress any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    int rating;
    while (true) {
        std::cout << "Rating (" << Config::MIN_RATING << "-" << Config::MAX_RATING << " stars): ";
        rating = safeGetInt();
        if (rating == INT_MIN) {
            std::cout << fg(Ansi::Color::Yellow) << "Add review cancelled.\n" << Ansi::Reset;
            std::cout << "\nPress any key to continue...";
            _getch();
            moveCursorToEnd();
            return;
        }
        if (validateRating(rating)) break;
    }

    std::cout << "Comment: ";
    std::string comment = safeGetline();
    if (comment == "QUIT_SIGNAL") {
        std::cout << fg(Ansi::Color::Yellow) << "Add review cancelled.\n" << Ansi::Reset;
        std::cout << "\nPress any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    manager.addOrEditReview(businessId, rating, comment);

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Allows user to view detailed information about a specific business by ID.
* Parameters: Reference to BusinessManager object.
* Result: None, displays detailed business information and provides options to bookmark or export.
***/
void viewBusinessDetails(BusinessManager& manager) {
    std::cout << "\n" << Ansi::Bold << fg(Ansi::Color::Cyan) << "=== View Business Details ==="
        << Ansi::Reset << "\n";
    std::cout << fg(Ansi::Color::BrightBlack) << "(Type 'quit' to cancel)" << Ansi::Reset << "\n";

    std::cout << "Business ID: ";
    std::string businessId = safeGetline();
    if (businessId == "QUIT_SIGNAL") {
        std::cout << fg(Ansi::Color::Yellow) << "View cancelled.\n" << Ansi::Reset;
        std::cout << "\nPress any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    displayBusinessDetails(manager, businessId);

    // Export option
    std::cout << fg(Ansi::Color::Yellow) << "Export this business to CSV? (y/n): " << Ansi::Reset;
    std::string exportResponse = safeGetline();

    if (exportResponse == "y" || exportResponse == "Y") {
        // Get all businesses to find the specific one
        std::vector<Business> allBusinesses = manager.getAllBusinesses();
        Business* selectedBusiness = nullptr;

        for (auto& b : allBusinesses) {
            if (b.id == businessId) {
                selectedBusiness = &b;
                break;
            }
        }

        if (selectedBusiness) {
            std::vector<Review> reviews = manager.getReviewsForBusiness(businessId);
            std::string filename = selectedBusiness->name + "_export.csv";
            // Replace spaces with underscores in filename
            std::replace(filename.begin(), filename.end(), ' ', '_');
            exportBusinessToCSV(*selectedBusiness, reviews, filename);
        }
    }

    // Bookmark option
    if (manager.isLoggedIn()) {
        std::cout << fg(Ansi::Color::Yellow) << "Would you like to bookmark this business? (y/n): "
            << Ansi::Reset;
        std::string response = safeGetline();

        if (response == "y" || response == "Y") {
            manager.toggleBookmark(businessId);
        }
    }

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Displays businesses sorted by category using UI menu navigation and Table display.
* Parameters: Reference to BusinessManager object.
* Result: None, displays filtered businesses in table format with option to export or view details.
***/
void browseByCategory(BusinessManager& manager) {
    MenuManager categoryMenu(bg(Ansi::BgColor::Cyan) + fg(Ansi::Color::Black), fg(Ansi::Color::Green), '#');

    categoryMenu.addElement("food", 5, 3);
    categoryMenu.addElement("retail", 25, 3);
    categoryMenu.addElement("services", 45, 3);
    categoryMenu.addElement("entertainment", 5, 8);
    categoryMenu.addElement("healthcare", 25, 8);
    categoryMenu.addElement("back", 45, 8);

    std::string selection = navigateMenu(categoryMenu);

    if (selection.empty() || selection == "back") {
        moveCursorToEnd();
        return;
    }

    std::vector<Business> businesses = manager.getBusinessesByCategory(selection);

    // Convert to BusinessWithDistance for consistent interface
    std::vector<BusinessWithDistance> businessesWD;
    for (const auto& b : businesses) {
        businessesWD.emplace_back(b, 0.0);
    }

    displayBusinessTable(businessesWD, "Businesses in Category: " + selection, false);

    // View details option
    std::cout << fg(Ansi::Color::Yellow) << "View business details? Enter ID or press Enter to skip: "
        << Ansi::Reset;
    std::string id = safeGetline();

    if (!id.empty() && id != "QUIT_SIGNAL") {
        displayBusinessDetails(manager, id);

        // Export option for individual business
        std::cout << fg(Ansi::Color::Yellow) << "Export this business to CSV? (y/n): " << Ansi::Reset;
        std::string exportResponse = safeGetline();

        if (exportResponse == "y" || exportResponse == "Y") {
            // Find the business
            Business* selectedBusiness = nullptr;
            for (auto& bwd : businessesWD) {
                if (bwd.business.id == id) {
                    selectedBusiness = &bwd.business;
                    break;
                }
            }

            if (selectedBusiness) {
                std::vector<Review> reviews = manager.getReviewsForBusiness(id);
                std::string filename = selectedBusiness->name + "_export.csv";
                std::replace(filename.begin(), filename.end(), ' ', '_');
                exportBusinessToCSV(*selectedBusiness, reviews, filename);
            }
        }
    }

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Displays businesses sorted by rating (highest first) using Table display.
* Parameters: Reference to BusinessManager object.
* Result: None, displays sorted businesses in table format with option to export or view details.
***/
void browseByRating(BusinessManager& manager) {
    std::vector<Business> businesses = manager.getBusinessesByRating();

    // Convert to BusinessWithDistance for consistent interface
    std::vector<BusinessWithDistance> businessesWD;
    for (const auto& b : businesses) {
        businessesWD.emplace_back(b, 0.0);
    }

    displayBusinessTable(businessesWD, "Businesses Sorted by Rating (Highest First)", false);

    // View details option
    std::cout << fg(Ansi::Color::Yellow) << "View business details? Enter ID or press Enter to skip: "
        << Ansi::Reset;
    std::string id = safeGetline();

    if (!id.empty() && id != "QUIT_SIGNAL") {
        displayBusinessDetails(manager, id);

        // Export option for individual business
        std::cout << fg(Ansi::Color::Yellow) << "Export this business to CSV? (y/n): " << Ansi::Reset;
        std::string exportResponse = safeGetline();

        if (exportResponse == "y" || exportResponse == "Y") {
            // Find the business
            Business* selectedBusiness = nullptr;
            for (auto& bwd : businessesWD) {
                if (bwd.business.id == id) {
                    selectedBusiness = &bwd.business;
                    break;
                }
            }

            if (selectedBusiness) {
                std::vector<Review> reviews = manager.getReviewsForBusiness(id);
                std::string filename = selectedBusiness->name + "_export.csv";
                std::replace(filename.begin(), filename.end(), ' ', '_');
                exportBusinessToCSV(*selectedBusiness, reviews, filename);
            }
        }
    }

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Displays businesses sorted by location proximity with distance calculations using Table display.
* Parameters: Reference to BusinessManager object.
* Result: None, displays sorted businesses with distances in table format with export and detail view options.
***/
void browseByLocation(BusinessManager& manager) {
    std::cout << "\n" << Ansi::Bold << fg(Ansi::Color::Cyan) << "=== Browse by Location ==="
        << Ansi::Reset << "\n";
    std::cout << fg(Ansi::Color::BrightBlack) << "(Type 'quit' to cancel at any prompt)" << Ansi::Reset << "\n";

    double userLongitude, userLatitude;

    // Get user location with validation
    while (true) {
        std::cout << "Your Longitude (" << Config::MIN_LONGITUDE << " to "
            << Config::MAX_LONGITUDE << "): ";
        userLongitude = safeGetDouble();
        if (std::isnan(userLongitude)) {
            std::cout << fg(Ansi::Color::Yellow) << "Browse by location cancelled.\n" << Ansi::Reset;
            std::cout << "\nPress any key to continue...";
            _getch();
            moveCursorToEnd();
            return;
        }
        if (validateLongitude(userLongitude)) break;
    }

    while (true) {
        std::cout << "Your Latitude (" << Config::MIN_LATITUDE << " to "
            << Config::MAX_LATITUDE << "): ";
        userLatitude = safeGetDouble();
        if (std::isnan(userLatitude)) {
            std::cout << fg(Ansi::Color::Yellow) << "Browse by location cancelled.\n" << Ansi::Reset;
            std::cout << "\nPress any key to continue...";
            _getch();
            moveCursorToEnd();
            return;
        }
        if (validateLatitude(userLatitude)) break;
    }

    // Get businesses sorted by location
    std::vector<Business> businesses = manager.getBusinessesByLocation(userLongitude, userLatitude);

    // Calculate distances and create BusinessWithDistance objects
    std::vector<BusinessWithDistance> businessesWD;
    for (const auto& b : businesses) {
        double distance = calculateDistance(userLatitude, userLongitude, b.latitude, b.longitude);
        businessesWD.emplace_back(b, distance);
    }

    // Display with distance column
    displayBusinessTable(businessesWD, "Businesses Near You (Sorted by Distance)", true);

    // View details option
    std::cout << fg(Ansi::Color::Yellow) << "View business details? Enter ID or press Enter to skip: "
        << Ansi::Reset;
    std::string id = safeGetline();

    if (!id.empty() && id != "QUIT_SIGNAL") {
        displayBusinessDetails(manager, id);

        // Export option for individual business
        std::cout << fg(Ansi::Color::Yellow) << "Export this business to CSV? (y/n): " << Ansi::Reset;
        std::string exportResponse = safeGetline();

        if (exportResponse == "y" || exportResponse == "Y") {
            // Find the business
            Business* selectedBusiness = nullptr;
            for (auto& bwd : businessesWD) {
                if (bwd.business.id == id) {
                    selectedBusiness = &bwd.business;
                    break;
                }
            }

            if (selectedBusiness) {
                std::vector<Review> reviews = manager.getReviewsForBusiness(id);
                std::string filename = selectedBusiness->name + "_export.csv";
                std::replace(filename.begin(), filename.end(), ' ', '_');
                exportBusinessToCSV(*selectedBusiness, reviews, filename);
            }
        }
    }

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Handles bookmark management with UI navigation for viewing, adding, or removing bookmarks.
* Parameters: Reference to BusinessManager object.
* Result: None, manages user bookmarks and displays results.
***/
void manageBookmarks(BusinessManager& manager) {
    if (!manager.isLoggedIn()) {
        std::cout << "\n" << fg(Ansi::Color::Red) << "[Error] " << Ansi::Reset
            << "You must be logged in to manage bookmarks.\n";
        std::cout << "Press any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    MenuManager bookmarkMenu(bg(Ansi::BgColor::Magenta) + fg(Ansi::Color::White), fg(Ansi::Color::Cyan), '#');

    bookmarkMenu.addElement("View Bookmarks", 5, 3);
    bookmarkMenu.addElement("Toggle Bookmark", 30, 3);
    bookmarkMenu.addElement("back", 5, 8);

    std::string selection = navigateMenu(bookmarkMenu);

    if (selection.empty() || selection == "back") {
        moveCursorToEnd();
        return;
    }

    if (selection == "View Bookmarks") {
        manager.displayBookmarks();
    }
    else if (selection == "Toggle Bookmark") {
        std::cout << fg(Ansi::Color::BrightBlack) << "(Type 'quit' to cancel)" << Ansi::Reset << "\n";
        std::cout << "Business ID: ";
        std::string businessId = safeGetline();
        if (businessId != "QUIT_SIGNAL") {
            manager.toggleBookmark(businessId);
        }
    }

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Displays the main browsing menu using UI navigation with multiple sorting options.
* Parameters: Reference to BusinessManager object.
* Result: None, handles user navigation through various browse options and features.
***/
void browseBusiness(BusinessManager& manager) {
    MenuManager browseMenu(bg(Ansi::BgColor::Green) + fg(Ansi::Color::Black), fg(Ansi::Color::Yellow), '#');

    browseMenu.addElement("All Businesses", 5, 3);
    browseMenu.addElement("By Category", 25, 3);
    browseMenu.addElement("By Rating", 50, 3);
    browseMenu.addElement("By Location", 5, 8);
    browseMenu.addElement("View Details", 25, 8);
    browseMenu.addElement("back", 50, 8);

    std::string selection = navigateMenu(browseMenu);

    if (selection.empty() || selection == "back") {
        moveCursorToEnd();
        return;
    }

    if (selection == "All Businesses") {
        std::vector<Business> businesses = manager.getAllBusinesses();

        std::vector<BusinessWithDistance> businessesWD;
        for (const auto& b : businesses) {
            businessesWD.emplace_back(b, 0.0);
        }

        displayBusinessTable(businessesWD, "All Businesses", false);

        // View details option
        std::cout << fg(Ansi::Color::Yellow) << "View business details? Enter ID or press Enter to skip: "
            << Ansi::Reset;
        std::string id = safeGetline();

        if (!id.empty() && id != "QUIT_SIGNAL") {
            displayBusinessDetails(manager, id);

            // Export option for individual business
            std::cout << fg(Ansi::Color::Yellow) << "Export this business to CSV? (y/n): " << Ansi::Reset;
            std::string exportResponse = safeGetline();

            if (exportResponse == "y" || exportResponse == "Y") {
                // Find the business
                Business* selectedBusiness = nullptr;
                for (auto& bwd : businessesWD) {
                    if (bwd.business.id == id) {
                        selectedBusiness = &bwd.business;
                        break;
                    }
                }

                if (selectedBusiness) {
                    std::vector<Review> reviews = manager.getReviewsForBusiness(id);
                    std::string filename = selectedBusiness->name + "_export.csv";
                    std::replace(filename.begin(), filename.end(), ' ', '_');
                    exportBusinessToCSV(*selectedBusiness, reviews, filename);
                }
            }
        }

        std::cout << "\nPress any key to continue...";
        _getch();
    }
    else if (selection == "By Category") {
        browseByCategory(manager);
    }
    else if (selection == "By Rating") {
        browseByRating(manager);
    }
    else if (selection == "By Location") {
        browseByLocation(manager);
    }
    else if (selection == "View Details") {
        viewBusinessDetails(manager);
    }

    moveCursorToEnd();
}

/***
* Purpose: Main program entry point with menu loop and application initialization.
* Parameters: Command line arguments (unused).
* Result: Program exit code (0 for success).
***/
int sain() {
    // Get URI from env variables:
    char* uri = nullptr;
    size_t len = 0;

    errno_t err = _dupenv_s(&uri, &len, "MAINSTREET_DB_URI");

    if (err || uri == nullptr) {
        std::cerr << "[Fatal Error] Environment variable MAINSTREET_DB_URI not set.\n";
        return 1;
    }

    // Safe uri
    std::string DB_URI(uri);

    // Free allocated memory
    free(uri);

    // Initialize business manager with database connection
    BusinessManager manager(DB_URI, DB_NAME);

    // Display welcome screen
    std::cout << Ansi::clearScreen << Ansi::moveTo(1, 1);
    std::cout << Ansi::Bold << bg(Ansi::BgColor::Blue) << fg(Ansi::Color::White)
        << "==================================================================="
        << Ansi::Reset << "\n";
    std::cout << Ansi::Bold << fg(Ansi::Color::Cyan)
        << "          LOCAL BUSINESS DIRECTORY"
        << Ansi::Reset << "\n";
    std::cout << Ansi::Bold << bg(Ansi::BgColor::Blue) << fg(Ansi::Color::White)
        << "==================================================================="
        << Ansi::Reset << "\n";
    std::cout << fg(Ansi::Color::BrightBlack) << "               Discover and Support Your Local Community"
        << Ansi::Reset << "\n\n";
    std::cout << "Press H at any time for help and navigation instructions.\n";
    std::cout << fg(Ansi::Color::Yellow) << "Type 'quit' at any input prompt to cancel.\n" << Ansi::Reset << "\n";
    std::cout << "Press any key to continue...";
    _getch();

    // Main application loop
    while (true) {
        MenuManager mainMenu(bg(Ansi::BgColor::Blue) + fg(Ansi::Color::White), fg(Ansi::Color::BrightYellow), '#');

        mainMenu.addElement("Login", 5, 3);
        mainMenu.addElement("Register", 25, 3);
        mainMenu.addElement("Browse Businesses", 50, 3);
        mainMenu.addElement("Add Business", 5, 8);
        mainMenu.addElement("Add Review", 25, 8);
        mainMenu.addElement("Bookmarks", 50, 8);
        mainMenu.addElement("Help", 5, 13);
        mainMenu.addElement("Logout", 25, 13);
        mainMenu.addElement("Exit", 50, 13);

        std::string choice = navigateMenu(mainMenu);

        if (choice.empty() || choice == "Exit") {
            std::cout << "\n" << fg(Ansi::Color::Green)
                << "Thank you for using Mainstreet Metrics!"
                << Ansi::Reset << "\n";
            break;
        }

        if (choice == "Login") {
            loginUser(manager);
        }
        else if (choice == "Register") {
            registerUser(manager);
        }
        else if (choice == "Browse Businesses") {
            browseBusiness(manager);
        }
        else if (choice == "Add Business") {
            addBusiness(manager);
        }
        else if (choice == "Add Review") {
            addReview(manager);
        }
        else if (choice == "Bookmarks") {
            manageBookmarks(manager);
        }
        else if (choice == "Help") {
            displayHelpScreen();
        }
        else if (choice == "Logout") {
            manager.logout();
            std::cout << "\nPress any key to continue...";
            _getch();
        }

        moveCursorToEnd();
    }

    return 0;
}*/