#include "window/window.hpp"
int main() {
	//initialize the window
	window::init();
	while (true) {
		window::run();
	}
}