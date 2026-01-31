#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <conio.h> // For _getch() on Windows
#include <iomanip>

#include "BusinessManager.hpp"
#include "Table.hpp"
#include "UI.hpp"

const std::string DB_URI = "mongodb+srv://ivanchang30901:ulxXvrCaD0MtY4AZ@cluster0.nihxpln.mongodb.net/?appName=Cluster0";
const std::string DB_NAME = "MainstreetMetricsDB";

// ==========================================
// Security & Validation Functions
// ==========================================

/***
* Purpose: Validates username input according to security rules.
* Parameters: The username string to validate.
* Result: True if valid, false otherwise.
***/
bool validateUsername(const std::string& username) {
    if (username.length() < 3 || username.length() > 20) {
        std::cout << "[Validation Error] Username must be 3-20 characters.\n";
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
* Result: True if valid, false otherwise.
***/
bool validatePassword(const std::string& password) {
    if (password.length() < 8) {
        std::cout << "[Validation Error] Password must be at least 8 characters.\n";
        return false;
    }

    bool hasUpper = false, hasLower = false, hasDigit = false;

    for (char c : password) {
        if (std::isupper(c)) hasUpper = true;
        if (std::islower(c)) hasLower = true;
        if (std::isdigit(c)) hasDigit = true;
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
* Result: True if valid (1-5), false otherwise.
***/
bool validateRating(int rating) {
    if (rating < 1 || rating > 5) {
        std::cout << "[Validation Error] Rating must be between 1 and 5.\n";
        return false;
    }
    return true;
}

/***
* Purpose: Validates business name input.
* Parameters: The business name string to validate.
* Result: True if valid, false otherwise.
***/
bool validateBusinessName(const std::string& name) {
    if (name.empty() || name.length() > 100) {
        std::cout << "[Validation Error] Business name must be 1-100 characters.\n";
        return false;
    }
    return true;
}

/***
* Purpose: Validates latitude coordinate.
* Parameters: The latitude value to validate.
* Result: True if valid (-90 to 90), false otherwise.
***/
bool validateLatitude(double lat) {
    if (lat < -90.0 || lat > 90.0) {
        std::cout << "[Validation Error] Latitude must be between -90 and 90.\n";
        return false;
    }
    return true;
}

/***
* Purpose: Validates longitude coordinate.
* Parameters: The longitude value to validate.
* Result: True if valid (-180 to 180), false otherwise.
***/
bool validateLongitude(double lon) {
    if (lon < -180.0 || lon > 180.0) {
        std::cout << "[Validation Error] Longitude must be between -180 and 180.\n";
        return false;
    }
    return true;
}

/***
* Purpose: Safely reads a line of input with buffer clearing.
* Parameters: None.
* Result: The input string.
***/
std::string safeGetline() {
    std::string input;
    std::getline(std::cin, input);
    return input;
}

/***
* Purpose: Safely reads an integer with input validation.
* Parameters: None.
* Result: The validated integer.
***/
int safeGetInt() {
    int value;
    while (!(std::cin >> value)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "[Input Error] Please enter a valid number: ";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

/***
* Purpose: Safely reads a double with input validation.
* Parameters: None.
* Result: The validated double.
***/
double safeGetDouble() {
    double value;
    while (!(std::cin >> value)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "[Input Error] Please enter a valid number: ";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

// ==========================================
// Display Functions
// ==========================================

/***
* Purpose: Displays a list of businesses in a formatted table.
* Parameters: Vector of Business objects to display, optional title for the table.
* Result: None, prints formatted table to console.
***/
void displayBusinessTable(const std::vector<Business>& businesses, const std::string& title = "Businesses") {
    if (businesses.empty()) {
        std::cout << "\nNo businesses to display.\n";
        return;
    }

    std::vector<std::string> headers = { "Name", "Category", "Rating", "Deal" };
    std::vector<size_t> widths = { 25, 15, 8, 30 };

    Table table(headers, widths, '=', '|');

    for (const auto& b : businesses) {
        std::ostringstream ratingStream;
        ratingStream << std::fixed << std::setprecision(1) << b.avg_rating;

        std::vector<std::string> row = {
            b.name,
            b.category,
            ratingStream.str(),
            b.special_deal.empty() ? "No deal" : b.special_deal
        };

        table.addRow(row);
    }

    std::cout << "\n=== " << title << " ===\n";
    table.print();
    std::cout << "\n";
}

/***
* Purpose: Moves cursor to end of screen to prevent UI overlap.
* Parameters: The row number to move to.
* Result: None, moves cursor position.
***/
void moveCursorToEnd(int row = 25) {
    std::cout << Ansi::moveTo(row, 1) << std::flush;
}

// ==========================================
// Menu Navigation Functions
// ==========================================

/***
* Purpose: Handles keyboard input for UI navigation using arrow keys or WASD.
* Parameters: Reference to MenuManager object.
* Result: Returns selected menu option name, or empty string if cancelled.
***/
std::string navigateMenu(MenuManager& menu) {
    std::cout << Ansi::hideCursor;

    while (true) {
        std::cout << Ansi::clearScreen << Ansi::moveTo(1, 1);
        menu.draw();

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

// ==========================================
// Feature Implementation Functions
// ==========================================

/***
* Purpose: Handles user login process with input validation.
* Parameters: Reference to BusinessManager object.
* Result: None, updates login state in BusinessManager.
***/
void loginUser(BusinessManager& manager) {
    std::cout << "\n=== User Login ===\n";
    std::cout << "Username: ";
    std::string username = safeGetline();

    std::cout << "Password: ";
    std::string password = safeGetline();

    manager.login(username, password);

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Handles new user registration with strict validation.
* Parameters: Reference to BusinessManager object.
* Result: None, creates new user if validation passes.
***/
void registerUser(BusinessManager& manager) {
    std::cout << "\n=== User Registration ===\n";

    std::string username, password, email;

    // Username validation loop
    while (true) {
        std::cout << "Username (3-20 chars, alphanumeric + underscore): ";
        username = safeGetline();
        if (validateUsername(username)) break;
    }

    // Password validation loop
    while (true) {
        std::cout << "Password (8+ chars, must have upper, lower, digit): ";
        password = safeGetline();
        if (validatePassword(password)) break;
    }

    // Email input
    std::cout << "Email: ";
    email = safeGetline();

    manager.registerUser(username, password, email);

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Handles adding a new business with comprehensive validation.
* Parameters: Reference to BusinessManager object.
* Result: None, adds business to database if validation passes.
***/
void addBusiness(BusinessManager& manager) {
    if (!manager.isLoggedIn()) {
        std::cout << "\n[Error] You must be logged in to add businesses.\n";
        std::cout << "Press any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    std::cout << "\n=== Add New Business ===\n";

    std::string name, category, description, deal;
    double longitude, latitude;

    // Business name validation
    while (true) {
        std::cout << "Business Name: ";
        name = safeGetline();
        if (validateBusinessName(name)) break;
    }

    std::cout << "Category (e.g., food, retail, services): ";
    category = safeGetline();

    std::cout << "Description: ";
    description = safeGetline();

    // Longitude validation
    while (true) {
        std::cout << "Longitude (-180 to 180): ";
        longitude = safeGetDouble();
        if (validateLongitude(longitude)) break;
    }

    // Latitude validation
    while (true) {
        std::cout << "Latitude (-90 to 90): ";
        latitude = safeGetDouble();
        if (validateLatitude(latitude)) break;
    }

    std::cout << "Special Deal/Coupon (optional): ";
    deal = safeGetline();

    manager.addBusiness(name, category, description, longitude, latitude, deal);

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Handles adding or editing a review with validation.
* Parameters: Reference to BusinessManager object.
* Result: None, adds/updates review in database.
***/
void addReview(BusinessManager& manager) {
    if (!manager.isLoggedIn()) {
        std::cout << "\n[Error] You must be logged in to add reviews.\n";
        std::cout << "Press any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    std::cout << "\n=== Add/Edit Review ===\n";
    std::cout << "Business ID: ";
    std::string businessId = safeGetline();

    int rating;
    while (true) {
        std::cout << "Rating (1-5): ";
        rating = safeGetInt();
        if (validateRating(rating)) break;
    }

    std::cout << "Comment: ";
    std::string comment = safeGetline();

    manager.addOrEditReview(businessId, rating, comment);

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Displays businesses sorted by category using UI menu and Table.
* Parameters: Reference to BusinessManager object.
* Result: None, displays filtered businesses in table format.
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
    displayBusinessTable(businesses, "Businesses in Category: " + selection);

    std::cout << "Press any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Displays businesses sorted by rating using Table.
* Parameters: Reference to BusinessManager object.
* Result: None, displays sorted businesses in table format.
***/
void browseByRating(BusinessManager& manager) {
    std::vector<Business> businesses = manager.getBusinessesByRating();
    displayBusinessTable(businesses, "Businesses Sorted by Rating");

    std::cout << "Press any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Displays businesses sorted by location proximity using Table.
* Parameters: Reference to BusinessManager object.
* Result: None, displays sorted businesses in table format.
***/
void browseByLocation(BusinessManager& manager) {
    std::cout << "\n=== Browse by Location ===\n";

    double longitude, latitude;

    while (true) {
        std::cout << "Your Longitude (-180 to 180): ";
        longitude = safeGetDouble();
        if (validateLongitude(longitude)) break;
    }

    while (true) {
        std::cout << "Your Latitude (-90 to 90): ";
        latitude = safeGetDouble();
        if (validateLatitude(latitude)) break;
    }

    std::vector<Business> businesses = manager.getBusinessesByLocation(longitude, latitude);
    displayBusinessTable(businesses, "Businesses Near You");

    std::cout << "Press any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Handles bookmark management with UI navigation.
* Parameters: Reference to BusinessManager object.
* Result: None, adds or removes bookmarks or displays saved bookmarks.
***/
void manageBookmarks(BusinessManager& manager) {
    if (!manager.isLoggedIn()) {
        std::cout << "\n[Error] You must be logged in to manage bookmarks.\n";
        std::cout << "Press any key to continue...";
        _getch();
        moveCursorToEnd();
        return;
    }

    MenuManager bookmarkMenu(bg(Ansi::BgColor::Magenta) + fg(Ansi::Color::White), fg(Ansi::Color::Cyan), '#');

    bookmarkMenu.addElement("View Bookmarks", 5, 3);
    bookmarkMenu.addElement("Add Bookmark", 25, 3);
    bookmarkMenu.addElement("Remove Bookmark", 50, 3);
    bookmarkMenu.addElement("back", 5, 8);

    std::string selection = navigateMenu(bookmarkMenu);

    if (selection.empty() || selection == "back") {
        moveCursorToEnd();
        return;
    }

    if (selection == "View Bookmarks") {
        manager.displayBookmarks();
    }
    else if (selection == "Add Bookmark" || selection == "Remove Bookmark") {
        std::cout << "Business ID: ";
        std::string businessId = safeGetline();
        manager.toggleBookmark(businessId);
    }

    std::cout << "\nPress any key to continue...";
    _getch();
    moveCursorToEnd();
}

/***
* Purpose: Displays the main browsing menu using UI navigation.
* Parameters: Reference to BusinessManager object.
* Result: None, handles user navigation through browse options.
***/
void browseBusiness(BusinessManager& manager) {
    MenuManager browseMenu(bg(Ansi::BgColor::Green) + fg(Ansi::Color::Black), fg(Ansi::Color::Yellow), '#');

    browseMenu.addElement("All Businesses", 5, 3);
    browseMenu.addElement("By Category", 25, 3);
    browseMenu.addElement("By Rating", 50, 3);
    browseMenu.addElement("By Location", 5, 8);
    browseMenu.addElement("back", 25, 8);

    std::string selection = navigateMenu(browseMenu);

    if (selection.empty() || selection == "back") {
        moveCursorToEnd();
        return;
    }

    if (selection == "All Businesses") {
        std::vector<Business> businesses = manager.getAllBusinesses();
        displayBusinessTable(businesses, "All Businesses");
        std::cout << "Press any key to continue...";
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

    moveCursorToEnd();
}

/***
* Purpose: Main program entry point with menu loop.
* Parameters: Command line arguments.
* Result: Program exit code.
***/
int main() {

    // DB Connection
    BusinessManager manager(DB_URI, DB_NAME);

    std::cout << Ansi::clearScreen << Ansi::moveTo(1, 1);
    std::cout << "===========================================\n";
    std::cout << "  Byte-Sized Business Boost - Main Menu\n";
    std::cout << "===========================================\n\n";

    while (true) {
        MenuManager mainMenu(Ansi::bg(Ansi::BgColor::Blue) + Ansi::fg(Ansi::Color::White), fg(Ansi::Color::BrightYellow), '#');

        mainMenu.addElement("Login", 5, 3);
        mainMenu.addElement("Register", 25, 3);
        mainMenu.addElement("Browse Businesses", 50, 3);
        mainMenu.addElement("Add Business", 5, 8);
        mainMenu.addElement("Add Review", 25, 8);
        mainMenu.addElement("Bookmarks", 50, 8);
        mainMenu.addElement("Logout", 5, 13);
        mainMenu.addElement("Exit", 25, 13);

        std::string choice = navigateMenu(mainMenu);

        if (choice.empty() || choice == "Exit") {
            std::cout << "\nThank you for using Byte-Sized Business Boost!\n";
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
        else if (choice == "Logout") {
            manager.logout();
            std::cout << "\nPress any key to continue...";
            _getch();
        }

        moveCursorToEnd();
    }

    return 0;
}