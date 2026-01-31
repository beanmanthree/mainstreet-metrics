#pragma once

#include <vector>

/***
* Purpose: Stores data to manage a formatted rectangle with a custom style
* Parameters: Two size types to represent the location of the top left corner.
* Two to represent the bottom left corner.
* A string to represent the style.
***/
struct Highlight {
	// Top left corner
	size_t x0, y0;

	// Bottom right corner
	size_t x1, y1;

	// ANSI escape code style
	std::string style;

	Highlight(size_t topLeftX, size_t topLeftY, size_t bottomRightX, size_t bottomRightY, std::string style) :
		x0(topLeftX),
		y0(topLeftY),
		x1(bottomRightX),
		y1(bottomRightY),
		style(style) {}
};

/***
* Purpose: Stores data about a table cell in the table.
* Parameters: Contains text inside the style and the styling of the cell.
***/
struct TableCell {
	// Text inside the cell
	std::string value;

	// ANSI escape code style
	std::string style;

	TableCell(std::string value, std::string style = "") : value(value), style(style) {}
};

class Table {
private:
	size_t width;
	std::vector<TableCell> header;
	std::vector<size_t> columnWidths;
	std::vector<std::vector<TableCell>> rows;
	char rowSeparator;
	char columnSeparator;
	
	/***
	* Purpose: Merge a vector of info with a vector of the styles to create a vector of TableCells.
	* Parameters: A vector of the info in each cell and a vector of the style of each cell.
	* Result: The merged vector of TableCells.
	***/
	std::vector<TableCell> createRow(const std::vector<std::string>& rowData, const std::vector<std::string>& rowStyling);

public:
	Table(
		const std::vector<std::string>& headerTitles,
		const std::vector<size_t>& columnWidths,
		char rowSeparator = '#',
		char columnSeparator = '#',
		const std::vector<std::string>& styling = std::vector<std::string>()
	);

	/***
	* Purpose: Print the formatted table with highlights (custom formatting rectangle).
	* Parameters: Vector of highlights or empty vector if none are passed.
	* Result: None, prints the formatted table to the console.
	***/
	void print(const std::vector<Highlight>& highlights = std::vector<Highlight>());

	/***
	* Purpose: Adds a row to our currrent vector of rows.
	* Parameters: A vector of the data in the new row and a potential vector of the styling.
	* Result: None, affects member variables.
	***/
	void addRow(const std::vector<std::string>& rowData, const std::vector<std::string>& rowStyling = std::vector<std::string>());
};