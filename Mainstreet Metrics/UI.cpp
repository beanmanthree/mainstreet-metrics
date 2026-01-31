#include "iostream"
#include "Ansi.hpp"

#include "UI.hpp"

MenuManager::MenuManager(std::string selectStyle, std::string normalStyle, char border) : selectStyle(selectStyle), normalStyle(normalStyle), border(border) {}

void MenuManager::addElement(std::string name, size_t x, size_t y) {
    elements.push_back(std::make_unique<UIElement>(name, x, y));
    if (!current) current = elements.back().get();
}

std::string MenuManager::getSelectedName() const {
    return current ? current->name : "";
}

void MenuManager::navigate(Direction dir) {
    if (!current) return;

    UIElement* bestMatch = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (const auto& el : elements) {
        if (el.get() == current) continue;

        // Cast coordinates to signed floats for math
        float dx = (float)el->x - (float)current->x;
        float dy = (float)el->y - (float)current->y;

        // Strict Directional Filter
        // This ensures the element is at least 1 unit in the general direction
        bool inDirection = false;
        switch (dir) {
        case Direction::Up:    if (dy < 0) inDirection = true; break;
        case Direction::Down:  if (dy > 0) inDirection = true; break;
        case Direction::Left:  if (dx < 0) inDirection = true; break;
        case Direction::Right: if (dx > 0) inDirection = true; break;
        }

        if (inDirection) {
            // Simple Distance Calculation
            // We use Squared Distance to avoid the expensive sqrt() call.
            // Closest in the general direction always wins, no matter how far 'offset' it is on the other axis.
            float distSq = (dx * dx) + (dy * dy);

            if (distSq < minDistance) {
                minDistance = distSq;
                bestMatch = el.get();
            }
        }
    }

    if (bestMatch) current = bestMatch;
}

void MenuManager::draw() const {
    // Keep track of furthest down the cursor goes.
    int maxY = 0;

    for (const auto& el : elements) {
        bool isSelected = (el.get() == current);
        std::string style = isSelected ? selectStyle : normalStyle;

        // Terminal rows/cols start at 1. 
        // We map 0 -> 1, 1 -> 2, etc.
        int startX = static_cast<int>(el->x) + 1;
        int startY = static_cast<int>(el->y) + 1;

        maxY = std::max(maxY, startY + 2);

        // Draw Top Border
        std::cout << Ansi::moveTo(startY, startX);
        std::cout << style;
        for (size_t i = 0; i < el->name.length() + 2; ++i) {
            std::cout << border;
        }
        std::cout << Ansi::Reset;

        // Draw Middle (Border + Name + Border)
        std::cout << Ansi::moveTo(startY + 1, startX);
        std::cout << style << border << el->name << border << Ansi::Reset;

        // Draw Bottom Border
        std::cout << Ansi::moveTo(startY + 2, startX);
        std::cout << style;
        for (size_t i = 0; i < el->name.length() + 2; ++i) {
            std::cout << border;
        }
        std::cout << Ansi::Reset;
    }

    // Restore state to the end of the UI
    std::cout << Ansi::moveTo(maxY + 1, 1) << std::flush;
}