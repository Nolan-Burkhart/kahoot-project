#include <Windows.h>

#include "window/window.hpp"
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	//initialize the window
	window::init();
	while (true) {
		window::run();
	}
}