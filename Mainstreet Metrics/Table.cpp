#include <iostream>
#include <stdexcept>
#include <numeric> // std::accumulate

#include "Ansi.hpp" // Table styling

#include "Table.hpp"

std::vector<TableCell> Table::createRow(const std::vector<std::string>& rowData, const std::vector<std::string>& rowStyling) {
	// Create and fill table cells
	std::vector<TableCell> row;

	if (rowStyling.empty()) {
		// No styling is given
		for (const std::string& data : rowData) row.emplace_back(data);
	}
	else {
		for (size_t i = 0; i < width; ++i) row.emplace_back(rowData[i], rowStyling[i]);
	}

	return row;
}

Table::Table(
	const std::vector<std::string>& headerTitles,
	const std::vector<size_t>& columnWidths,
	char rowSeparator,
	char columnSeparator,
	const std::vector<std::string>& styling
) :
	width(headerTitles.size()),
	columnWidths(columnWidths),
	rowSeparator(rowSeparator),
	columnSeparator(columnSeparator) {
	size_t width = headerTitles.size();

	// Make sure that widths all match
	if (width != columnWidths.size()) throw std::invalid_argument("Header titles and header widths must be equal length.");
	if (!styling.empty() && width != styling.size()) throw std::invalid_argument("Header titles and header widths must be equal length,");

	// Initialize header
	header = createRow(headerTitles, styling);
}

void Table::print(const std::vector<Highlight>& highlights) {
	/***
	* Purpose: Formats the text in the cells by truncating or padding.
	* Parameters: The text in the cell and the desired width.
	* Result: The formatted string.
	***/
	auto formatText = [](const std::string& text, size_t width) -> std::string {
		// To truncate
		if (text.length() > width) {

			// Too small for dots
			if (width <= 2) return text.substr(0, width);

			// Truncate using two dots
			return text.substr(0, width - 2) + "..";
		}

		// Pad with spaces
		return text + std::string(width - text.length(), ' ');
		};

	/***
	* Purpose: Gets the highlighted style for a coordinate.
	* Parameters: The coordinate we want to find the style of.
	* Result: The styles located inside that coordinate.
	***/
	auto getStyle = [&highlights](size_t x, size_t y) -> std::string {
		std::string res = "";
		for (const Highlight& h : highlights) {

			// Check bounds
			if (x >= h.x0 && x < h.x1 && y >= h.y0 && y < h.y1) res += h.style;
		}
		return res;
		};

	// Row bar that separates rows
	std::string rowBar(std::accumulate(columnWidths.begin(), columnWidths.end(), width + 1), rowSeparator);

	// Loop through and print all rows and the header
	for (size_t y = 0; y <= rows.size(); ++y) {
		for (size_t x = 0; x < width; ++x) {

			// Set content and style (when y is 0, it is the header)
			std::string content = (y == 0) ? header[x].value : rows[y - 1][x].value;
			std::string style = ((y == 0) ? header[x].style : rows[y - 1][x].style) + getStyle(x, y - 1);
			std::cout << Ansi::Reset;
			std::cout << style << columnSeparator;
			std::cout << formatText(content, columnWidths[x]);
		}

		// Separate rows
		std::cout << columnSeparator << '\n';
		std::cout << Ansi::Reset << rowBar << '\n';
	}
}

void Table::addRow(const std::vector<std::string>& rowData, const std::vector<std::string>& rowStyling) {
	// Make sure that widths all match
	if (rowData.size() != width) throw std::invalid_argument("Row data size must match column count.");
	if (!rowStyling.empty() && rowStyling.size() != width) throw std::invalid_argument("Row data size must match row style size");

	rows.push_back(createRow(rowData, rowStyling));
}