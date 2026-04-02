#define _USE_MATH_DEFINES

#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <map>
#include <algorithm>
#include <stdexcept>

#include "BusinessManager.hpp"

// ============================================================
// Database configuration
// ============================================================
const std::string DB_NAME = "MainstreetMetricsDB";

namespace Config {
    const size_t MIN_USERNAME_LENGTH = 3;
    const size_t MAX_USERNAME_LENGTH = 20;
    const size_t MIN_PASSWORD_LENGTH = 8;
    const size_t MAX_BUSINESS_NAME_LENGTH = 100;
    const size_t MAX_DESCRIPTION_LENGTH = 500;
    const int MIN_RATING = 1;
    const int MAX_RATING = 5;
    const double MIN_LATITUDE  = -90.0;
    const double MAX_LATITUDE  =  90.0;
    const double MIN_LONGITUDE = -180.0;
    const double MAX_LONGITUDE =  180.0;
    const double EARTH_RADIUS_KM = 6371.0;
    const std::vector<std::string> VALID_CATEGORIES = {
        "food", "retail", "services", "entertainment", "healthcare"
    };
}

// ============================================================
// Geographic helpers
// ============================================================
double degreesToRadians(double d) { return d * M_PI / 180.0; }

double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    double r1 = degreesToRadians(lat1), r2 = degreesToRadians(lat2);
    double dl = degreesToRadians(lat2 - lat1), dln = degreesToRadians(lon2 - lon1);
    double a = std::sin(dl/2)*std::sin(dl/2) +
               std::cos(r1)*std::cos(r2)*std::sin(dln/2)*std::sin(dln/2);
    return Config::EARTH_RADIUS_KM * 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
}

struct BusinessWithDistance {
    Business business;
    double distanceKm;
    BusinessWithDistance(const Business& b, double d = 0.0) : business(b), distanceKm(d) {}
};

// ============================================================
// URL decode
// ============================================================
static int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

std::string urlDecode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') { out += ' '; }
        else if (s[i] == '%' && i+2 < s.size()) {
            out += (char)((hexVal(s[i+1]) << 4) | hexVal(s[i+2]));
            i += 2;
        } else { out += s[i]; }
    }
    return out;
}

// ============================================================
// Parse query string or POST body into a map
// ============================================================
std::map<std::string, std::string> parseParams(const std::string& raw) {
    std::map<std::string, std::string> m;
    std::istringstream ss(raw);
    std::string token;
    while (std::getline(ss, token, '&')) {
        auto eq = token.find('=');
        if (eq == std::string::npos) { m[urlDecode(token)] = ""; continue; }
        m[urlDecode(token.substr(0, eq))] = urlDecode(token.substr(eq+1));
    }
    return m;
}

// Get all CGI parameters (supports GET and POST)
std::map<std::string, std::string> getCgiParams() {
    std::string raw;
    const char* method = std::getenv("REQUEST_METHOD");
    if (method && std::string(method) == "POST") {
        const char* lenStr = std::getenv("CONTENT_LENGTH");
        if (lenStr) {
            int len = std::atoi(lenStr);
            if (len > 0 && len < 65536) {
                raw.resize(len);
                std::cin.read(&raw[0], len);
            }
        }
    } else {
        const char* qs = std::getenv("QUERY_STRING");
        if (qs) raw = qs;
    }
    return parseParams(raw);
}

// ============================================================
// HTML helpers
// ============================================================
std::string htmlEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;
        }
    }
    return out;
}

void printHeader() {
    //std::cout << "Content-Type: text/html\r\n\r\n";
    std::cout << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
              << "<meta charset=\"UTF-8\">\n"
              << "<title>Mainstreet Metrics - Local Business Directory</title>\n"
              << "</head>\n<body>\n";
}

void printFooter() {
    std::cout << "<hr>\n"
              << "<p><a href=\"/cgi-bin/Main.cgi\">Home</a> | "
              << "<a href=\"/cgi-bin/Main.cgi?action=help\">Help</a></p>\n"
              << "<script src=\"/session.js\"></script>"
              << "</body>\n</html>\n";
}

void printNav(const std::string& currentUser) {
    std::cout << "<h1>Mainstreet Metrics &mdash; Local Business Directory</h1>\n";
    std::cout << "<nav>\n";
    if (currentUser.empty()) {
        std::cout << "<a href=\"/cgi-bin/Main.cgi?action=login_form\">Login</a> | "
                  << "<a href=\"/cgi-bin/Main.cgi?action=register_form\">Register</a> | ";
    } else {
        std::cout << "<strong>Logged in as: " << htmlEscape(currentUser) << "</strong> | "
                  << "<a href=\"/cgi-bin/Main.cgi?action=logout\">Logout</a> | "
                  << "<a href=\"/cgi-bin/Main.cgi?action=bookmarks\">Bookmarks</a> | "
                  << "<a href=\"/cgi-bin/Main.cgi?action=add_business_form\">Add Business</a> | "
                  << "<a href=\"/cgi-bin/Main.cgi?action=add_review_form\">Add Review</a> | ";
    }
    std::cout << "<a href=\"/cgi-bin/Main.cgi?action=browse\">Browse Businesses</a>\n"
              << "</nav>\n<hr>\n";
}

void printError(const std::string& msg) {
    std::cout << "<p><strong>Error:</strong> " << htmlEscape(msg) << "</p>\n";
}

// ============================================================
// Validation helpers (same logic as console version)
// ============================================================
bool validateUsername(const std::string& u, std::string& err) {
    if (u.length() < Config::MIN_USERNAME_LENGTH || u.length() > Config::MAX_USERNAME_LENGTH) {
        err = "Username must be 3-20 characters."; return false;
    }
    for (char c : u) if (!std::isalnum(c) && c != '_') {
        err = "Username can only contain letters, numbers, and underscores."; return false;
    }
    return true;
}

bool validatePassword(const std::string& p, std::string& err) {
    if (p.length() < Config::MIN_PASSWORD_LENGTH) {
        err = "Password must be at least 8 characters."; return false;
    }
    bool hu=false, hl=false, hd=false;
    for (char c : p) { if(std::isupper(c)) hu=true; if(std::islower(c)) hl=true; if(std::isdigit(c)) hd=true; }
    if (!hu || !hl || !hd) { err = "Password must contain uppercase, lowercase, and digits."; return false; }
    return true;
}

bool validateRating(int r, std::string& err) {
    if (r < 1 || r > 5) { err = "Rating must be between 1 and 5."; return false; }
    return true;
}

bool validateBusinessName(const std::string& n, std::string& err) {
    if (n.empty() || n.length() > Config::MAX_BUSINESS_NAME_LENGTH) {
        err = "Business name must be 1-100 characters."; return false;
    }
    bool ok = false;
    for (char c : n) if (std::isalnum(c)) { ok = true; break; }
    if (!ok) { err = "Business name must contain at least one letter or number."; return false; }
    return true;
}

bool validateCategory(const std::string& c, std::string& err) {
    auto it = std::find(Config::VALID_CATEGORIES.begin(), Config::VALID_CATEGORIES.end(), c);
    if (it == Config::VALID_CATEGORIES.end()) {
        err = "Category must be one of: food, retail, services, entertainment, healthcare."; return false;
    }
    return true;
}

bool validateLatitude(double v, std::string& err) {
    if (v < -90.0 || v > 90.0) { err = "Latitude must be between -90 and 90."; return false; }
    return true;
}

bool validateLongitude(double v, std::string& err) {
    if (v < -180.0 || v > 180.0) { err = "Longitude must be between -180 and 180."; return false; }
    return true;
}

bool validateDescription(const std::string& d, std::string& err) {
    if (d.length() > Config::MAX_DESCRIPTION_LENGTH) {
        err = "Description must be under 500 characters."; return false;
    }
    return true;
}

// ============================================================
// CSV export helper (writes to a file on disk then shows link)
// ============================================================
std::string escapeCSV(const std::string& s) {
    if (s.find(',') != std::string::npos || s.find('"') != std::string::npos || s.find('\n') != std::string::npos) {
        std::string e = "\"";
        for (char c : s) { if (c == '"') e += "\"\""; else e += c; }
        e += "\""; return e;
    }
    return s;
}

// Writes CSV to wwwroot/exports/ and returns the web-accessible path
std::string exportBusinessToCSV(const Business& b, const std::vector<Review>& reviews,
                                 double userLat = 0.0, double userLon = 0.0) {
    std::string safeName = b.name;
    std::replace(safeName.begin(), safeName.end(), ' ', '_');
    // Remove characters that are unsafe in filenames
    safeName.erase(std::remove_if(safeName.begin(), safeName.end(),
        [](char c){ return !std::isalnum(c) && c != '_' && c != '-'; }), safeName.end());

    std::string filename = "C:\\inetpub\\wwwroot\\exports\\" + safeName + "_export.csv";
    std::string webPath  = "/exports/" + safeName + "_export.csv";

    std::ofstream f(filename);
    if (!f.is_open()) return "";

    double dist = calculateDistance(userLat, userLon, b.latitude, b.longitude);

    f << "Business ID,Name,Category,Description,Average Rating,Special Deal,Latitude,Longitude,Distance (km),Review User,Review Rating,Review Comment\n";

    if (!reviews.empty()) {
        for (const auto& r : reviews) {
            f << escapeCSV(b.id) << "," << escapeCSV(b.name) << "," << escapeCSV(b.category) << ","
              << escapeCSV(b.description) << "," << std::fixed << std::setprecision(2) << b.avg_rating << ","
              << escapeCSV(b.special_deal.empty() ? "None" : b.special_deal) << ","
              << std::setprecision(6) << b.latitude << "," << b.longitude << ","
              << std::setprecision(2) << dist << ","
              << escapeCSV(r.user_id) << "," << r.rating << "," << escapeCSV(r.comment) << "\n";
        }
    } else {
        f << escapeCSV(b.id) << "," << escapeCSV(b.name) << "," << escapeCSV(b.category) << ","
          << escapeCSV(b.description) << "," << std::fixed << std::setprecision(2) << b.avg_rating << ","
          << escapeCSV(b.special_deal.empty() ? "None" : b.special_deal) << ","
          << std::setprecision(6) << b.latitude << "," << b.longitude << ","
          << std::setprecision(2) << dist << ",,," << "\n";
    }
    f.close();
    return webPath;
}

// ============================================================
// HTML renderers for each view
// ============================================================

void renderHomePage() {
    std::cout << "<h2>Welcome to Mainstreet Metrics</h2>\n"
              << "<p>Discover and support your local community.</p>\n"
              << "<ul>\n"
              << "<li><a href=\"/cgi-bin/Main.cgi?action=browse\">Browse Businesses</a></li>\n"
              << "<li><a href=\"/cgi-bin/Main.cgi?action=login_form\">Login</a></li>\n"
              << "<li><a href=\"/cgi-bin/Main.cgi?action=register_form\">Register</a></li>\n"
              << "<li><a href=\"/cgi-bin/Main.cgi?action=help\">Help</a></li>\n"
              << "</ul>\n";
}

void renderHelpPage() {
    std::cout << "<h2>Help &amp; Instructions</h2>\n"
              << "<h3>Navigation</h3>\n"
              << "<p>Use the links at the top of each page to navigate between features.</p>\n"
              << "<h3>Features</h3>\n"
              << "<ul>\n"
              << "<li><strong>Register/Login</strong> &mdash; Create an account and sign in.</li>\n"
              << "<li><strong>Browse Businesses</strong> &mdash; View all businesses, filter by category, sort by rating, or find businesses near you.</li>\n"
              << "<li><strong>Add Business</strong> &mdash; Logged-in users can add new businesses.</li>\n"
              << "<li><strong>Add Review</strong> &mdash; Logged-in users can rate and review businesses (1&ndash;5 stars).</li>\n"
              << "<li><strong>Bookmarks</strong> &mdash; Save your favourite businesses for quick access.</li>\n"
              << "<li><strong>Export CSV</strong> &mdash; Download a CSV report of any business including all its reviews.</li>\n"
              << "</ul>\n"
              << "<h3>Valid Categories</h3>\n"
              << "<p>food, retail, services, entertainment, healthcare</p>\n"
              << "<h3>Tips</h3>\n"
              << "<ul>\n"
              << "<li>Business IDs are shown in all tables &mdash; click the View link to see full details.</li>\n"
              << "<li>Location sorting shows distance from the coordinates you enter.</li>\n"
              << "<li>CSV files are saved to /exports/ on the server and a download link is shown.</li>\n"
              << "<li>Only logged-in users can add businesses, add reviews, or manage bookmarks.</li>\n"
              << "</ul>\n";
}

// Renders a table of businesses
void renderBusinessTable(const std::vector<BusinessWithDistance>& bwds, bool showDistance) {
    if (bwds.empty()) {
        std::cout << "<p>No businesses found.</p>\n";
        return;
    }
    std::cout << "<table border=\"1\" cellpadding=\"4\" cellspacing=\"0\">\n<thead>\n<tr>"
              << "<th>Name</th><th>Category</th><th>Avg Rating</th><th>Description</th>";
    if (showDistance) std::cout << "<th>Distance (km)</th>";
    std::cout << "<th>Actions</th></tr>\n</thead>\n<tbody>\n";

    for (const auto& bwd : bwds) {
        const Business& b = bwd.business;
        std::string shortDesc = b.description.length() > 50
            ? b.description.substr(0, 47) + "..." : b.description;

        std::cout << "<tr>"
                  << "<td>" << htmlEscape(b.name) << "</td>"
                  << "<td>" << htmlEscape(b.category) << "</td>"
                  << "<td>" << std::fixed << std::setprecision(1) << b.avg_rating << "</td>"
                  << "<td>" << htmlEscape(shortDesc) << "</td>";

        if (showDistance) {
            std::cout << "<td>" << std::fixed << std::setprecision(2) << bwd.distanceKm << "</td>";
        }

        std::cout << "<td>"
                  << "<a href=\"/cgi-bin/Main.cgi?action=view_details&id=" << htmlEscape(b.id) << "\">View</a>"
                  << "</td>"
                  << "</tr>\n";
    }
    std::cout << "</tbody>\n</table>\n";
}

// Full business detail page
void renderBusinessDetails(BusinessManager& manager, const std::string& id) {
    std::vector<Business> all = manager.getAllBusinesses();
    Business* found = nullptr;
    for (auto& b : all) if (b.id == id) { found = &b; break; }

    if (!found) { printError("Business not found with ID: " + id); return; }

    std::cout << "<h2>" << htmlEscape(found->name) << "</h2>\n"
              << "<table border=\"1\" cellpadding=\"4\">\n"
              << "<tr><th>ID</th><td>" << htmlEscape(found->id) << "</td></tr>\n"
              << "<tr><th>Category</th><td>" << htmlEscape(found->category) << "</td></tr>\n"
              << "<tr><th>Average Rating</th><td>" << std::fixed << std::setprecision(1) << found->avg_rating << " / 5</td></tr>\n"
              << "<tr><th>Location</th><td>Lat: " << found->latitude << ", Lon: " << found->longitude << "</td></tr>\n";

    if (!found->special_deal.empty())
        std::cout << "<tr><th>Special Deal</th><td>" << htmlEscape(found->special_deal) << "</td></tr>\n";

    std::cout << "<tr><th>Description</th><td>" << htmlEscape(found->description) << "</td></tr>\n"
              << "</table>\n";

    // Reviews
    std::vector<Review> reviews = manager.getReviewsForBusiness(id);
    std::cout << "<h3>Reviews (" << reviews.size() << ")</h3>\n";

    if (reviews.empty()) {
        std::cout << "<p>No reviews yet. Be the first to review!</p>\n";
    } else {
        std::cout << "<table border=\"1\" cellpadding=\"4\" cellspacing=\"0\">\n"
                  << "<thead><tr><th>User</th><th>Rating</th><th>Comment</th></tr></thead>\n<tbody>\n";
        for (const auto& r : reviews) {
            std::cout << "<tr><td>" << htmlEscape(r.user_id) << "</td>"
                      << "<td>" << r.rating << " / 5</td>"
                      << "<td>" << htmlEscape(r.comment) << "</td></tr>\n";
        }
        std::cout << "</tbody></table>\n";
    }

    // Export CSV form
    std::cout << "<h3>Export</h3>\n"
              << "<form method=\"post\" action=\"/cgi-bin/Main.cgi\">\n"
              << "<input type=\"hidden\" name=\"action\" value=\"export_csv\">\n"
              << "<input type=\"hidden\" name=\"id\" value=\"" << htmlEscape(id) << "\">\n"
              << "<button type=\"submit\">Export this business to CSV</button>\n"
              << "</form>\n";

    // Bookmark form
    std::cout << "<h3>Bookmark</h3>\n"
              << "<form method=\"post\" action=\"/cgi-bin/Main.cgi\">\n"
              << "<input type=\"hidden\" name=\"action\" value=\"toggle_bookmark\">\n"
              << "<input type=\"hidden\" name=\"id\" value=\"" << htmlEscape(id) << "\">\n"
              << "<button type=\"submit\">Toggle Bookmark</button>\n"
              << "</form>\n";

    // Add review link
    std::cout << "<p><a href=\"/cgi-bin/Main.cgi?action=add_review_form&id=" << htmlEscape(id)
              << "\">Add / Edit Review for this business</a></p>\n";
}

// ============================================================
// Form renderers
// ============================================================

void renderLoginForm(const std::string& errMsg = "") {
    std::cout << "<h2>Login</h2>\n";
    if (!errMsg.empty()) printError(errMsg);
    std::cout << "<form method=\"post\" action=\"/cgi-bin/Main.cgi\">\n"
              << "<input type=\"hidden\" name=\"action\" value=\"login\">\n"
              << "<label>Username: <input type=\"text\" name=\"username\" required maxlength=\"20\"></label><br><br>\n"
              << "<label>Password: <input type=\"password\" name=\"password\" required></label><br><br>\n"
              << "<button type=\"submit\">Login</button>\n"
              << "</form>\n"
              << "<p>No account? <a href=\"/cgi-bin/Main.cgi?action=register_form\">Register here</a></p>\n";
}

void renderRegisterForm(const std::string& errMsg = "") {
    std::cout << "<h2>Register</h2>\n";
    if (!errMsg.empty()) printError(errMsg);
    std::cout << "<form method=\"post\" action=\"/cgi-bin/Main.cgi\">\n"
              << "<input type=\"hidden\" name=\"action\" value=\"register\">\n"
              << "<label>Username (3&ndash;20 chars, letters/numbers/underscore):<br>"
              << "<input type=\"text\" name=\"username\" required minlength=\"3\" maxlength=\"20\"></label><br><br>\n"
              << "<label>Password (8+ chars, must include upper, lower, digit):<br>"
              << "<input type=\"password\" name=\"password\" required minlength=\"8\"></label><br><br>\n"
              << "<label>Email:<br><input type=\"email\" name=\"email\" required></label><br><br>\n"
              << "<button type=\"submit\">Register</button>\n"
              << "</form>\n"
              << "<script src=\"/verification.js\"></script>\n";
}

void renderAddBusinessForm(const std::string& errMsg = "") {
    std::cout << "<h2>Add New Business</h2>\n";
    if (!errMsg.empty()) printError(errMsg);
    std::cout << "<form method=\"post\" action=\"/cgi-bin/Main.cgi\">\n"
              << "<input type=\"hidden\" name=\"action\" value=\"add_business\">\n"
              << "<label>Business Name (1&ndash;100 chars):<br>"
              << "<input type=\"text\" name=\"name\" required maxlength=\"100\"></label><br><br>\n"
              << "<label>Category:<br>\n"
              << "<select name=\"category\" required>\n"
              << "<option value=\"\">-- Select --</option>\n";
    for (const auto& cat : Config::VALID_CATEGORIES)
        std::cout << "<option value=\"" << cat << "\">" << cat << "</option>\n";
    std::cout << "</select></label><br><br>\n"
              << "<label>Description (max 500 chars):<br>"
              << "<textarea name=\"description\" rows=\"4\" cols=\"60\" maxlength=\"500\"></textarea></label><br><br>\n"
              << "<label>Longitude (&minus;180 to 180):<br>"
              << "<input type=\"number\" name=\"longitude\" step=\"any\" min=\"-180\" max=\"180\" required></label><br><br>\n"
              << "<label>Latitude (&minus;90 to 90):<br>"
              << "<input type=\"number\" name=\"latitude\" step=\"any\" min=\"-90\" max=\"90\" required></label><br><br>\n"
              << "<label>Special Deal / Coupon (optional):<br>"
              << "<input type=\"text\" name=\"deal\" maxlength=\"200\"></label><br><br>\n"
              << "<button type=\"submit\">Add Business</button>\n"
              << "</form>\n";
}

void renderAddReviewForm(const std::string& prefillId = "", const std::string& errMsg = "") {
    std::cout << "<h2>Add / Edit Review</h2>\n";
    if (!errMsg.empty()) printError(errMsg);
    std::cout << "<form method=\"post\" action=\"/cgi-bin/Main.cgi\">\n"
              << "<input type=\"hidden\" name=\"action\" value=\"add_review\">\n"
              << "<label>Business ID:<br>"
              << "<input type=\"text\" name=\"business_id\" required value=\"" << htmlEscape(prefillId) << "\"></label><br><br>\n"
              << "<label>Rating (1&ndash;5):<br>\n"
              << "<select name=\"rating\" required>\n"
              << "<option value=\"\">-- Select --</option>\n";
    for (int i = 1; i <= 5; ++i)
        std::cout << "<option value=\"" << i << "\">" << i << " star" << (i>1?"s":"") << "</option>\n";
    std::cout << "</select></label><br><br>\n"
              << "<label>Comment:<br>"
              << "<textarea name=\"comment\" rows=\"4\" cols=\"60\"></textarea></label><br><br>\n"
              << "<button type=\"submit\">Submit Review</button>\n"
              << "</form>\n";
}

void renderBrowseMenu() {
    std::cout << "<h2>Browse Businesses</h2>\n"
              << "<ul>\n"
              << "<li><a href=\"/cgi-bin/Main.cgi?action=browse_all\">All Businesses</a></li>\n"
              << "<li><a href=\"/cgi-bin/Main.cgi?action=browse_category_form\">By Category</a></li>\n"
              << "<li><a href=\"/cgi-bin/Main.cgi?action=browse_rating\">By Rating (Highest First)</a></li>\n"
              << "<li><a href=\"/cgi-bin/Main.cgi?action=browse_location_form\">By Location (Nearest First)</a></li>\n"
              << "</ul>\n";
}

void renderBrowseCategoryForm() {
    std::cout << "<h2>Browse by Category</h2>\n"
              << "<form method=\"get\" action=\"/cgi-bin/Main.cgi\">\n"
              << "<input type=\"hidden\" name=\"action\" value=\"browse_category\">\n"
              << "<label>Category:<br>\n"
              << "<select name=\"category\" required>\n"
              << "<option value=\"\">-- Select --</option>\n";
    for (const auto& cat : Config::VALID_CATEGORIES)
        std::cout << "<option value=\"" << cat << "\">" << cat << "</option>\n";
    std::cout << "</select></label><br><br>\n"
              << "<button type=\"submit\">Browse</button>\n"
              << "</form>\n";
}

void renderBrowseLocationForm(const std::string& errMsg = "") {
    std::cout << "<h2>Browse by Location</h2>\n";
    if (!errMsg.empty()) printError(errMsg);
    std::cout << "<form method=\"get\" action=\"/cgi-bin/Main.cgi\">\n"
              << "<input type=\"hidden\" name=\"action\" value=\"browse_location\">\n"
              << "<label>Your Longitude (&minus;180 to 180):<br>"
              << "<input type=\"number\" name=\"lon\" step=\"any\" min=\"-180\" max=\"180\" required></label><br><br>\n"
              << "<label>Your Latitude (&minus;90 to 90):<br>"
              << "<input type=\"number\" name=\"lat\" step=\"any\" min=\"-90\" max=\"90\" required></label><br><br>\n"
              << "<button type=\"submit\">Find Nearby Businesses</button>\n"
              << "</form>\n";
}

// ============================================================
// Session helpers
// A lightweight file-based session so user identity persists
// between CGI invocations.  Session tokens are stored as
// small text files in C:\inetpub\sessions\<token>.txt
// containing the username.
// ============================================================
static std::string SESSION_DIR = "C:\\inetpub\\wwwroot\\sessions\\";

std::string readCookie(const std::string& name) {
    const char* cookies = std::getenv("HTTP_COOKIE");
    if (!cookies) return "";
    std::string all(cookies);
    std::string search = name + "=";
    auto pos = all.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    auto end = all.find(';', pos);
    return (end == std::string::npos) ? all.substr(pos) : all.substr(pos, end - pos);
}

// Emit a Set-Cookie header BEFORE Content-Type (must be called before printHeader)
void setSessionCookie(const std::string& token) {
    // std::cout << "Set-Cookie: session=" << token << "; Path=/; HttpOnly\r\n"; - Broken, rewrite when server is fixed to print html document.
}

void clearSessionCookie() {
    // std::cout << "Set-Cookie: session=; Path=/; Max-Age=0\r\n";
}

std::string getSessionUser() {
    std::string token = readCookie("session");
    if (token.empty()) return "";
    // Sanitise token: only alphanumeric allowed
    for (char c : token) if (!std::isalnum(c)) return "";
    std::string path = SESSION_DIR + token + ".txt";
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::string user; std::getline(f, user); return user;
}

std::string createSession(const std::string& username) {
    // Simple token: base on username + a hash of time
    // For production use a cryptographic random token, but this suffices for demo
    std::ostringstream oss;
    oss << std::hex << (std::hash<std::string>{}(username) ^ (size_t)std::time(nullptr));
    std::string token = oss.str();
    // sanitise
    for (char& c : token) if (!std::isalnum(c)) c = 'x';

    std::ofstream f(SESSION_DIR + token + ".txt");
    if (f.is_open()) { f << username; f.close(); }
    return token;
}

void destroySession() {
    std::string token = readCookie("session");
    if (token.empty()) return;
    for (char c : token) if (!std::isalnum(c)) return;
    std::string path = SESSION_DIR + token + ".txt";
    std::remove(path.c_str());
}

// ============================================================
// Main CGI entry point
// ============================================================
int main() {
    // Read DB URI from environment
    char* uri_env = nullptr;
    size_t len = 0;
    //std::cout << "Content-Type: text/html\r\n\r\n";
#ifdef _WIN32
    errno_t err2 = _dupenv_s(&uri_env, &len, "MAINSTREET_DB_URI");
    if (err2 || uri_env == nullptr) {

        std::cout << "<html><body><p>Fatal: MAINSTREET_DB_URI environment variable not set.</p></body></html>\n";
        return 1;
    }
    std::string DB_URI(uri_env);
    free(uri_env);
#else
    const char* uri_ptr = std::getenv("MAINSTREET_DB_URI");
    if (!uri_ptr) {    
        std::cout << "<html><body><p>Fatal: MAINSTREET_DB_URI environment variable not set.</p></body></html>\n";
        return 1;
    }
    std::string DB_URI(uri_ptr);
#endif

    // Parse CGI params (GET or POST)
    auto params = getCgiParams();
    std::string action = params.count("action") ? params["action"] : "";

    // Session / user state
    std::string currentUser = params.count("js_session_user") ? params["js_session_user"] : "";
    
    // Validate it's sane (alphanumeric + underscore only)
    for (char c : currentUser)
        if (!std::isalnum(c) && c != '_') { currentUser = ""; break; }

    // Helper to log into BusinessManager when we have a session user.
    // BusinessManager expects login() to be called, but in CGI mode we
    // need to restore the logged-in state from the session.  We pass an
    // empty password and rely on a trust-restore path if your
    // BusinessManager supports it; if not, the simplest approach is to
    // store credentials in the session file (not done here for security).
    // Instead, we expose currentUser for display and guard actions, and
    // call manager.login() only on actual login action.

    BusinessManager manager(DB_URI, DB_NAME);

    // If session user exists, re-authenticate silently using stored session
    // (requires BusinessManager to have a restoreSession(username) method,
    // OR you store the hashed token in the DB).  For now we call loginByUsername
    // if it exists; otherwise you must add it to BusinessManager.
    if (!currentUser.empty()) {
        manager.restoreSession(currentUser);   // <-- Add this method to BusinessManager
    }

    // -------------------------------------------------------
    // Route: handle actions that emit Set-Cookie BEFORE html
    // -------------------------------------------------------
    if (action == "login") {
        std::string username = params.count("username") ? params["username"] : "";
        std::string password = params.count("password") ? params["password"] : "";
        std::string verr;

        bool ok = validateUsername(username, verr);

        if (ok) {
            // Attempt login via BusinessManager
            // We capture success by checking isLoggedIn() after the call
            manager.login(username, password);
            if (manager.isLoggedIn()) {
                std::string token = createSession(username);
                setSessionCookie(token);
                printHeader();
                printNav(username);
                std::cout << "<p>Welcome back, " << htmlEscape(username) << "! You are now logged in.</p>\n"
                          << "<p><a href=\"/cgi-bin/Main.cgi?action=browse\">Browse Businesses</a></p>\n";
                printFooter();
                return 0;
            } else {
                printHeader();
                printNav("");
                renderLoginForm("Invalid username or password.");
                printFooter();
                return 0;
            }
        } else {
            printHeader();
            printNav("");
            renderLoginForm(verr);
            printFooter();
            return 0;
        }
    }

    if (action == "logout") {
        destroySession();
        clearSessionCookie();
        printHeader();
        printNav("");
        std::cout << "<p>You have been logged out.</p>\n";
        printFooter();
        return 0;
    }

    if (action == "register") {
        std::string username = params.count("username") ? params["username"] : "";
        std::string password = params.count("password") ? params["password"] : "";
        std::string email    = params.count("email")    ? params["email"]    : "";
        std::string verr;

        if (!validateUsername(username, verr)) {
            printHeader(); printNav(currentUser); renderRegisterForm(verr); printFooter(); return 0;
        }
        if (!validatePassword(password, verr)) {
            printHeader(); printNav(currentUser); renderRegisterForm(verr); printFooter(); return 0;
        }

        manager.registerUser(username, password, email);

        printHeader();
        printNav(currentUser);
        std::cout << "<p>Registration submitted. Please <a href=\"/cgi-bin/Main.cgi?action=login_form\">login</a>.</p>\n";
        printFooter();
        return 0;
    }

    // -------------------------------------------------------
    // All remaining routes - no Set-Cookie needed
    // -------------------------------------------------------
    printHeader();
    printNav(currentUser);

    if (action.empty() || action == "home") {
        renderHomePage();
    }
    else if (action == "help") {
        renderHelpPage();
    }
    else if (action == "login_form") {
        renderLoginForm();
    }
    else if (action == "register_form") {
        renderRegisterForm();
    }
    else if (action == "browse") {
        renderBrowseMenu();
    }
    else if (action == "browse_all") {
        std::vector<Business> businesses = manager.getAllBusinesses();
        std::vector<BusinessWithDistance> bwds;
        for (const auto& b : businesses) bwds.emplace_back(b, 0.0);
        std::cout << "<h2>All Businesses</h2>\n";
        renderBusinessTable(bwds, false);
    }
    else if (action == "browse_category_form") {
        renderBrowseCategoryForm();
    }
    else if (action == "browse_category") {
        std::string cat = params.count("category") ? params["category"] : "";
        std::string verr;
        if (!validateCategory(cat, verr)) { printError(verr); renderBrowseCategoryForm(); }
        else {
            std::vector<Business> businesses = manager.getBusinessesByCategory(cat);
            std::vector<BusinessWithDistance> bwds;
            for (const auto& b : businesses) bwds.emplace_back(b, 0.0);
            std::cout << "<h2>Businesses in Category: " << htmlEscape(cat) << "</h2>\n";
            renderBusinessTable(bwds, false);
        }
    }
    else if (action == "browse_rating") {
        std::vector<Business> businesses = manager.getBusinessesByRating();
        std::vector<BusinessWithDistance> bwds;
        for (const auto& b : businesses) bwds.emplace_back(b, 0.0);
        std::cout << "<h2>Businesses Sorted by Rating (Highest First)</h2>\n";
        renderBusinessTable(bwds, false);
    }
    else if (action == "browse_location_form") {
        renderBrowseLocationForm();
    }
    else if (action == "browse_location") {
        std::string latStr = params.count("lat") ? params["lat"] : "";
        std::string lonStr = params.count("lon") ? params["lon"] : "";
        std::string verr;
        double userLat = 0.0, userLon = 0.0;
        bool ok = true;

        try { userLat = std::stod(latStr); } catch (...) { ok = false; verr = "Invalid latitude."; }
        if (ok) try { userLon = std::stod(lonStr); } catch (...) { ok = false; verr = "Invalid longitude."; }
        if (ok && !validateLatitude(userLat, verr))   ok = false;
        if (ok && !validateLongitude(userLon, verr))  ok = false;

        if (!ok) { renderBrowseLocationForm(verr); }
        else {
            std::vector<Business> businesses = manager.getBusinessesByLocation(userLon, userLat);
            std::vector<BusinessWithDistance> bwds;
            for (const auto& b : businesses) {
                double dist = calculateDistance(userLat, userLon, b.latitude, b.longitude);
                bwds.emplace_back(b, dist);
            }
            std::cout << "<h2>Businesses Near You (Sorted by Distance)</h2>\n";
            renderBusinessTable(bwds, true);
        }
    }
    else if (action == "view_details") {
        std::string id = params.count("id") ? params["id"] : "";
        if (id.empty()) { printError("No business ID specified."); renderBrowseMenu(); }
        else renderBusinessDetails(manager, id);
    }
    else if (action == "add_business_form") {
        if (currentUser.empty()) { printError("You must be logged in to add a business."); renderLoginForm(); }
        else renderAddBusinessForm();
    }
    else if (action == "add_business") {
        if (currentUser.empty()) { printError("You must be logged in."); renderLoginForm(); }
        else {
            std::string name  = params.count("name")        ? params["name"]        : "";
            std::string cat   = params.count("category")    ? params["category"]    : "";
            std::string desc  = params.count("description") ? params["description"] : "";
            std::string lonS  = params.count("longitude")   ? params["longitude"]   : "";
            std::string latS  = params.count("latitude")    ? params["latitude"]    : "";
            std::string deal  = params.count("deal")        ? params["deal"]        : "";
            std::string verr;
            bool ok = true;
            double lon = 0.0, lat = 0.0;

            if (!validateBusinessName(name, verr)) ok = false;
            if (ok && !validateCategory(cat, verr)) ok = false;
            if (ok && !validateDescription(desc, verr)) ok = false;
            if (ok) { try { lon = std::stod(lonS); } catch (...) { ok=false; verr="Invalid longitude."; } }
            if (ok) { try { lat = std::stod(latS); } catch (...) { ok=false; verr="Invalid latitude."; } }
            if (ok && !validateLongitude(lon, verr)) ok = false;
            if (ok && !validateLatitude(lat, verr))  ok = false;

            if (!ok) { renderAddBusinessForm(verr); }
            else {
                manager.addBusiness(name, cat, desc, lon, lat, deal);
                std::cout << "<p>Business submitted successfully.</p>\n"
                          << "<p><a href=\"/cgi-bin/Main.cgi?action=browse_all\">View all businesses</a></p>\n";
            }
        }
    }
    else if (action == "add_review_form") {
        if (currentUser.empty()) { printError("You must be logged in to add a review."); renderLoginForm(); }
        else {
            std::string prefill = params.count("id") ? params["id"] : "";
            renderAddReviewForm(prefill);
        }
    }
    else if (action == "add_review") {
        if (currentUser.empty()) { printError("You must be logged in."); renderLoginForm(); }
        else {
            std::string bid     = params.count("business_id") ? params["business_id"] : "";
            std::string ratingS = params.count("rating")      ? params["rating"]      : "";
            std::string comment = params.count("comment")     ? params["comment"]     : "";
            std::string verr;
            bool ok = true;
            int rating = 0;

            if (bid.empty()) { ok=false; verr="Business ID is required."; }
            if (ok) { try { rating = std::stoi(ratingS); } catch (...) { ok=false; verr="Invalid rating."; } }
            if (ok && !validateRating(rating, verr)) ok = false;

            if (!ok) { renderAddReviewForm(bid, verr); }
            else {
                manager.addOrEditReview(bid, rating, comment);
                std::cout << "<p>Review submitted.</p>\n"
                          << "<p><a href=\"/cgi-bin/Main.cgi?action=view_details&id=" << htmlEscape(bid)
                          << "\">Back to business</a></p>\n";
            }
        }
    }
    else if (action == "bookmarks") {
        if (currentUser.empty()) { printError("You must be logged in to view bookmarks."); renderLoginForm(); }
        else {
            std::cout << "<h2>Your Bookmarks</h2>\n";
            manager.displayBookmarks();   // BusinessManager prints to stdout - that's fine in CGI text/html
        }
    }
    else if (action == "toggle_bookmark") {
        if (currentUser.empty()) { printError("You must be logged in to manage bookmarks."); renderLoginForm(); }
        else {
            std::string id = params.count("id") ? params["id"] : "";
            if (id.empty()) { printError("No business ID provided."); }
            else {
                manager.toggleBookmark(id);
                std::cout << "<p>Bookmark updated.</p>\n"
                          << "<p><a href=\"/cgi-bin/Main.cgi?action=view_details&id=" << htmlEscape(id)
                          << "\">Back to business</a></p>\n";
            }
        }
    }
    else if (action == "export_csv") {
        std::string id = params.count("id") ? params["id"] : "";
        if (id.empty()) { printError("No business ID provided."); }
        else {
            std::vector<Business> all = manager.getAllBusinesses();
            Business* found = nullptr;
            for (auto& b : all) if (b.id == id) { found = &b; break; }
            if (!found) { printError("Business not found: " + id); }
            else {
                std::vector<Review> reviews = manager.getReviewsForBusiness(id);
                std::string webPath = exportBusinessToCSV(*found, reviews);
                if (webPath.empty()) {
                    printError("Could not create CSV file. Ensure C:\\inetpub\\exports\\ exists and is writable.");
                } else {
                    std::cout << "<p>CSV export created: <a href=\"" << webPath << "\">" << webPath << "</a></p>\n"
                              << "<p>(" << reviews.size() << " review(s) exported)</p>\n";
                }
            }
        }
    }
    else {
        printError("Unknown action: " + action);
        renderHomePage();
    }

    printFooter();
    return 0;
}