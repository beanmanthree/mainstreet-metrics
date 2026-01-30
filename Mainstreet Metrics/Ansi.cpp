#include "ansi.hpp"

namespace Ansi {

	std::string fg(Color c) {
		return std::string(CSI) + std::to_string(static_cast<int>(c)) + 'm';
	}

	std::string bg(BgColor c) {
		return std::string(CSI) + std::to_string(static_cast<int>(c)) + 'm';
	}

	std::string fg256(int idx) {
		return std::string(CSI) + "38;5;" + std::to_string(idx) + 'm';
	}

	std::string bg256(int index) {
		return std::string(CSI) + "48;5;" + std::to_string(index) + 'm';
	}

	std::string fgRgb(int r, int g, int b) {
		return std::string(CSI) + "38;2;" + std::to_string(r) + ';' + std::to_string(g) + ';' + std::to_string(b) + 'm';
	}

	std::string bgRgb(int r, int g, int b) {
		return std::string(CSI) + "48;2;" + std::to_string(r) + ';' + std::to_string(g) + ';' + std::to_string(b) + 'm';
	}

	std::string moveUp(int n) {
		return std::string(CSI) + std::to_string(n) + 'A';
	}

	std::string moveDown(int n) {
		return std::string(CSI) + std::to_string(n) + 'B';
	}

	std::string moveForward(int n) {
		return std::string(CSI) + std::to_string(n) + 'C';
	}

	std::string moveBack(int n) {
		return std::string(CSI) + std::to_string(n) + 'D';
	}

	std::string moveTo(int r, int c) {
		return std::string(CSI) + std::to_string(r) + ';' + std::to_string(c) + 'H';
	}

	std::string saveCursor() {
		return std::string(CSI) + 's';
	}

	std::string restoreCursor() {
		return std::string(CSI) + 'u';
	}

}