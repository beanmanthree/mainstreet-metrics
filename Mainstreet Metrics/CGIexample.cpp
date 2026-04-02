#include <windows.h>
#include <iostream>
#include <string>
#include <map>
#include <cstdlib>
#include <sstream>
#include <iomanip>

// URL decode utility
std::string url_decode(const std::string& str)
{
    std::string ret;
    char ch;
    int ii;
    for (std::size_t i = 0; i < str.length(); i++) {
        if (str[i] == '+') {
            ret += ' ';
        }
        else if (str[i] == '%' && i + 2 < str.length()) {
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> ii) {
                ch = static_cast<char>(ii);
                ret += ch;
                i += 2;
            }
            else {
                ret += '%';
            }
        }
        else {
            ret += str[i];
        }
    }
    return ret;
}

// Parse query string into a map
std::map<std::string, std::string> parse_query(const std::string& query)
{
    std::map<std::string, std::string> params;
    std::string::size_type lastPos = 0, pos = 0;
    while ((pos = query.find('&', lastPos)) != std::string::npos) {
        std::string pair = query.substr(lastPos, pos - lastPos);
        std::string::size_type eq = pair.find('=');
        if (eq != std::string::npos) {
            params[url_decode(pair.substr(0, eq))] = url_decode(pair.substr(eq + 1));
        }
        lastPos = pos + 1;
    }
    std::string pair = query.substr(lastPos);
    std::string::size_type eq = pair.find('=');
    if (eq != std::string::npos) {
        params[url_decode(pair.substr(0, eq))] = url_decode(pair.substr(eq + 1));
    }
    return params;
}

std::string request()
{
    std::string query;
    size_t len;

    char* request_method;// = getenv("REQUEST_METHOD");
    errno_t err = _dupenv_s(&request_method, &len, "REQUEST_METHOD");

    if (request_method && std::string(request_method) == "POST") {

        char* content_length;// = getenv("CONTENT_LENGTH");
        err = _dupenv_s(&content_length, &len, "CONTENT_LENGTH");
        int len = content_length ? std::atoi(content_length) : 0;
        query.resize(len);

        //read "Name=&Category=&Description=sad&Distance=&Rating=1&action=search"
        std::cin.read(&query[0], len);
    }
    else {
        char* qs;// = getenv("QUERY_STRING");
        err = _dupenv_s(&qs, &len, "QUERY_STRING");
        if (qs) query = qs;
    }

    return query;
}

void debug_display(std::map<std::string, std::string>& params)
{
    std::cout << "<h2>" << "action: " << params["action"] << "</h2>" << std::endl;
    std::cout << "<table border=\"1\" cellpadding=\"6\">" << std::endl;
    std::cout << "<tr><th>Field</th><th>Value</th></tr>" << std::endl;

    for (const auto& [key, value] : params) {
        std::cout << "<tr><td>" << key << "</td><td>" << value << "</td></tr>" << std::endl;
    }
    std::cout << "</table>" << std::endl;
}

void handle_database(std::map<std::string, std::string>& params)
{

    if (params["action"].compare("search") == 0)
    {
    }
    else if (params["action"].compare("insert") == 0)
    {

    }
    else if (params["action"].compare("update") == 0)
    {

    }
    else if (params["action"].compare("delete") == 0)
    {

    }
    else
    {

    }
}


bool _debug = true;
int cgi()
{
    std::string query = request();

    std::map<std::string, std::string> params = parse_query(query);
    std::cout << "Content-type: text/html\r\n\r\n";
    std::cout << "<!DOCTYPE html><html lang=\"en-US\"><head><title>Form Data</title></head><body>";

    if (_debug)
        debug_display(params);
    else
        handle_database(params);

    std::cout << "</body></html>";
    return 0;
}
