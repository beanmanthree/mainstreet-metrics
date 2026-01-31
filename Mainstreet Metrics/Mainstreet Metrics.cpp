#include <iostream>
#include <conio.h>

#include "Table.hpp"
#include "BusinessManager.hpp"
#include "UI.hpp"

const std::string DB_URI = "mongodb+srv://ivanchang30901:ulxXvrCaD0MtY4AZ@cluster0.nihxpln.mongodb.net/?appName=Cluster0";
const std::string DB_NAME = "MainstreetMetricsDB";

void clearScreen() {
    system("cls");
}

int main() {
    MenuManager menu(static_cast<std::string>(Ansi::Inverse) + static_cast<std::string>(Ansi::Bold) + Ansi::fg(Ansi::Color::BrightYellow), Ansi::fg(Ansi::Color::BrightYellow), '#');
    menu.addElement("Play", 0, 0);
    menu.addElement("Options", 20, 0);
    menu.addElement("Exit", 40, 0);

    // Input loop:
    while (true) {
        int ch = _getch();
        std::cout << Ansi::clearScreen;
        // Special keys
        if (ch == 0 || ch == 224) {
            // Get actual keycode
            ch = _getch();

            switch (ch) {
                case 72:
                    menu.navigate(Direction::Up);
                    break;
                case 80:
                    menu.navigate(Direction::Down);

                    break;
                case 75:
                    menu.navigate(Direction::Left);
                    break;
                case 77:
                    menu.navigate(Direction::Right);
                    break;
            }
        }
        // Normal keys
        else {
            if (ch == 13) {
                std::cout << "Enter pressed\n";
            }
            else if (ch == 27) {
                std::cout << "Escape pressed, exiting\n";
                break;
            }
            else {
                std::cout << "Other key: " << ch << "\n";
            }
        }
        
        menu.draw();
        std::cout << "Arrow keys to navigate, buttons are highlighted in yellow." << '\n';
        std::cout << "Enter or space to select a button." << '\n';
    }
    
    /*Table businessInfo({"Category", "Name", "Short Description", "Rating", "Distance"}, {10, 16, 32, 6, 8}, '-', '|', std::vector<std::string>(5, static_cast<std::string>(Ansi::Inverse)));
    businessInfo.addRow({ "Food", "Chipotle", "Tasty mexican food.", "9.3", "1.2km" });
    businessInfo.print();*/

}

/*const std::string URI = "mongodb+srv://ivanchang30901:ulxXvrCaD0MtY4AZ@cluster0.nihxpln.mongodb.net/?appName=Cluster0";
    const std::string DB_NAME = "MainstreetMetricsDB";

    std::cout << "Initializing Byte-Sized Business Boost...\n";

    // Instantiate Manager
    // This will throw an error and exit if the DB connection fails.
    BusinessManager app(URI, DB_NAME);

    int choice = 0;
    bool running = true;

    while (running) {
        std::cout << "\n====================================\n";
        std::cout << "      Byte-Sized Business Boost     \n";
        std::cout << "====================================\n";

        if (app.isLoggedIn()) {
            // === LOGGED IN MENU ===
            std::cout << "1. View All Businesses\n";
            std::cout << "2. Search by Category\n";
            std::cout << "3. Find Businesses Near Me (Geo)\n";
            std::cout << "4. Add New Business\n";
            std::cout << "5. View My Bookmarks\n";
            std::cout << "6. Logout\n";
            std::cout << "0. Exit\n";
            std::cout << "Select: ";
            std::cin >> choice;

            switch (choice) {
            case 1: {
                auto list = app.getAllBusinesses();
                std::cout << "\n--- All Businesses ---\n";
                for (const auto& b : list) {
                    std::cout << "* " << b.name << " [" << b.category << "] Rating: " << b.avg_rating << "/5\n";
                    std::cout << "  Deal: " << b.special_deal << "\n";
                }
                break;
            }
            case 2: {
                std::string cat;
                std::cout << "Enter Category (Food, Retail, Services): ";
                std::cin >> cat; // Simple cin for single word categories
                auto list = app.getBusinessesByCategory(cat);
                for (const auto& b : list) std::cout << "* " << b.name << "\n";
                break;
            }
            case 3: {
                double lat, lon;
                std::cout << "Enter Longitude: "; std::cin >> lon;
                std::cout << "Enter Latitude: "; std::cin >> lat;
                auto list = app.getBusinessesByLocation(lon, lat);
                if (list.empty()) std::cout << "No businesses found near location.\n";
                for (const auto& b : list) std::cout << "* " << b.name << " (Found near you)\n";
                break;
            }
            case 4: {
                // Logic to add business
                std::string name, cat, desc, deal;
                double lat, lon;
                clearInput();

                std::cout << "Business Name: "; std::getline(std::cin, name);
                std::cout << "Category: "; std::getline(std::cin, cat);
                std::cout << "Description: "; std::getline(std::cin, desc);
                std::cout << "Special Deal: "; std::getline(std::cin, deal);
                std::cout << "Longitude: "; std::cin >> lon;
                std::cout << "Latitude: "; std::cin >> lat;

                app.addBusiness(name, cat, desc, lon, lat, deal);
                break;
            }
            case 5:
                app.displayBookmarks();
                break;
            case 6:
                app.logout();
                break;
            case 0:
                running = false;
                break;
            default:
                std::cout << "Invalid option.\n";
            }

        }
        else {
            // === GUEST / LOGIN MENU ===
            std::cout << "1. Login\n";
            std::cout << "2. Register New User\n";
            std::cout << "3. Guest Mode (View Only)\n";
            std::cout << "0. Exit\n";
            std::cout << "Select: ";
            std::cin >> choice;

            switch (choice) {
            case 1: {
                std::string u, p;
                std::cout << "Username: "; std::cin >> u;
                std::cout << "Password: "; std::cin >> p;
                app.login(u, p);
                break;
            }
            case 2: {
                std::string u, p, e;
                std::cout << "Enter desired Username: "; std::cin >> u;
                std::cout << "Enter Password: "; std::cin >> p;
                std::cout << "Enter Email: "; std::cin >> e;

                // Calls our updated registerUser function
                app.registerUser(u, p, e);
                break;
            }
            case 3:
                std::cout << "\n[Guest Mode Active] Functionality limited to viewing.\n";
                {
                    auto list = app.getAllBusinesses();
                    for (const auto& b : list) std::cout << "* " << b.name << "\n";
                }
                break;
            case 0:
                running = false;
                break;
            default:
                clearInput(); // catch bad input types
                std::cout << "Invalid option.\n";
            }
        }
    }

    std::cout << "Goodbye!\n";
    return 0;*/