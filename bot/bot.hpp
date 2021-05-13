#ifndef BOT_HPP_
#define BOT_HPP_

/* directx 11 include */
#include <d3d11.h>

/* windows includes */
#include <tchar.h>
#include <cstdint>
#include <string>

namespace bot {
	void run(std::string game_code, std::string name, int waitfor);
	extern uint64_t go_time;
	extern int bots_connected;
}

#endif // !BOT_HPP_
