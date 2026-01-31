#include "Ansi.hpp"

std::string Ansi::fg(Color c) {
	return std::string(CSI) + std::to_string(static_cast<int>(c)) + 'm';
}

std::string Ansi::bg(BgColor c) {
	return std::string(CSI) + std::to_string(static_cast<int>(c)) + 'm';
}

std::string Ansi::fg256(int idx) {
	return std::string(CSI) + "38;5;" + std::to_string(idx) + 'm';
}

std::string Ansi::bg256(int index) {
	return std::string(CSI) + "48;5;" + std::to_string(index) + 'm';
}

std::string Ansi::fgRgb(int r, int g, int b) {
	return std::string(CSI) + "38;2;" + std::to_string(r) + ';' + std::to_string(g) + ';' + std::to_string(b) + 'm';
}

std::string Ansi::bgRgb(int r, int g, int b) {
	return std::string(CSI) + "48;2;" + std::to_string(r) + ';' + std::to_string(g) + ';' + std::to_string(b) + 'm';
}

std::string Ansi::moveUp(int n) {
	return std::string(CSI) + std::to_string(n) + 'A';
}

std::string Ansi::moveDown(int n) {
	return std::string(CSI) + std::to_string(n) + 'B';
}

std::string Ansi::moveForward(int n) {
	return std::string(CSI) + std::to_string(n) + 'C';
}

std::string Ansi::moveBack(int n) {
	return std::string(CSI) + std::to_string(n) + 'D';
}

std::string Ansi::moveTo(int r, int c) {
	return std::string(CSI) + std::to_string(r) + ';' + std::to_string(c) + 'H';
}

std::string Ansi::saveCursor() {
	return std::string(CSI) + 's';
}

std::string Ansi::restoreCursor() {
	return std::string(CSI) + 'u';
}