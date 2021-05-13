#ifndef WINDOW_HPP_
#define WINDOW_HPP_

/* directx 11 include */
#include <d3d11.h>

/* windows includes */
#include <tchar.h>
#include <cstdint>

namespace window {
	bool init();
	void run();
	void exit();

	LONG_PTR wnd_proc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param); //stupid windows naming styling :(
	void update_window_pos(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);

	extern ID3D11Device* d3d_device;
	extern ID3D11DeviceContext* d3d_context;
	extern IDXGISwapChain* d3d_swapchain;
	extern ID3D11RenderTargetView* d3d_view;

}

#endif // !WINDOW_HPP_
