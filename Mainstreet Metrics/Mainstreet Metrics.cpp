#include <iostream>

#include "Table.hpp"

int main() {

    Table businessInfo({ "Category", "Name", "Short Description", "Rating", "Distance" }, { 10, 16, 32, 6, 8 }, '-', '|', std::vector<std::string>(5, static_cast<std::string>(Ansi::Inverse)));
    businessInfo.addRow({ "Food", "Chipotle", "Tasty mexican food.", "9.3", "1.2km" });
    businessInfo.print();
    return 0;

}