#include "window/window.hpp"
#include "bot/bot.hpp"
int main() {
	//initialize the window
	//window::init();
	//while (true) {
	//	window::run();
	//}
	for (int i = 0; i < 100; i++) {
		bot::run();
	}
	bot::run();
}