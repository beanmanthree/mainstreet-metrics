#pragma once

#include <string>
#include <vector>


/***
* Purpose: Represent the four possible navigation directions used to move between UI elements in the menu.
* Parameters: None.
* Result: A strongly-typed enumeration of directions.
***/
enum class Direction {
    Up,
    Down,
    Left,
    Right
};

/***
* Purpose: Represent a single UI element with a display name and screen position.
* Parameters: None.
* Result: A lightweight data structure describing a menu element.
***/
struct UIElement {
    std::string name; // Text displayed on element.
    size_t x, y; // Screen coordinates

    UIElement(std::string n, size_t x, size_t y) :
        name(n), x(x), y(y) {}
};

/***
* Purpose: Manage a collection of UI elements, handle navigation between them,
*          and render the menu to the terminal.
* Parameters: None.
* Result: A controller object for menu-based user interfaces.
***/
class MenuManager {
private:
    std::vector<std::unique_ptr<UIElement>> elements;
    UIElement* current = nullptr;
    std::string selectStyle, normalStyle;
    char border;

public:

    MenuManager(std::string selectStyle, std::string normalStyle, char border);
    
    /***
    * Purpose: Add a new UI element to the menu at a given screen position.
    * Parameters: The element name and its x and y coordinates.
    * Result: The element is stored and may become the selected element.
    ***/
    void addElement(std::string name, size_t x, size_t y);

    /***
    * Purpose: Retrieve the name of the currently selected UI element.
    * Parameters: None.
    * Result: The name of the selected element, or an empty string if none exists.
    ***/
    std::string getSelectedName() const;

    /***
    * Purpose: Change the currently selected UI element based on a navigation direction.
    * Parameters: The direction in which to navigate.
    * Result: The closest valid UI element in that direction becomes selected.
    ***/
    void navigate(Direction dir);

    /***
    * Purpose: Draw all UI elements to the terminal, highlighting the selected one.
    * Parameters: None.
    * Result: The menu is rendered visually using ANSI escape codes.
    ***/
    void draw() const;
};