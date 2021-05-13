/* project includes */
#include "../bot/bot.hpp"
#include "window.hpp"

/* imgui includes */
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"

/* windows includes */
#include <thread>
#include <vector>

namespace window {
    ID3D11Device* d3d_device = nullptr;
    ID3D11DeviceContext* d3d_context = nullptr;
    IDXGISwapChain* d3d_swapchain = nullptr;
    ID3D11RenderTargetView* d3d_view = nullptr;
}

bool clicked;
float x_click;
float y_click;

std::vector<std::thread> threads;

/* window::update_window_pos()
*  drags the window across the string
*/
void window::update_window_pos(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    POINT p;
    GetCursorPos(&p);

    RECT window;
    RECT desktop;

    const POINTS points = MAKEPOINTS(l_param);

    const int x_mouse = points.x;
    const int y_mouse = points.y;

    if (x_mouse < 0 || y_mouse < 0)
        return;

    if (y_mouse < 18 && GetCapture() == wnd && !clicked)
        clicked = true;

    if (clicked) {
        GetWindowRect(wnd, &window);
        GetWindowRect(GetDesktopWindow(), &desktop);

        const int x_window = window.left + x_mouse - x_click;
        const int y_window = window.top + y_mouse - y_click;

        SetWindowPos(wnd, nullptr, x_window, y_window, 350,200, SWP_NOREDRAW);
    }

    if (GetCapture() != wnd)
        clicked = false;
}

/* window::init()
*  initializes the window
*  true = created succesfuly
*  false = an error occured
*/

bool window::init() {
    // create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, wnd_proc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("kahoot flooder"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, _T("kahoot flooder"), (WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN), 1100, 300, 350, 200, NULL, NULL, wc.hInstance, NULL);

    // initialize directx 11
    DXGI_SWAP_CHAIN_DESC swap_chain_description{};
    swap_chain_description.BufferCount = 2;
    swap_chain_description.BufferDesc.Width = 0;
    swap_chain_description.BufferDesc.Height = 0;
    swap_chain_description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_description.BufferDesc.RefreshRate.Numerator = 60;
    swap_chain_description.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_description.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_description.OutputWindow = hwnd;
    swap_chain_description.SampleDesc.Count = 1;
    swap_chain_description.SampleDesc.Quality = 0;
    swap_chain_description.Windowed = TRUE;
    swap_chain_description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    //create the device and swap chain
    uint32_t createDeviceFlags = 0;
    D3D_FEATURE_LEVEL feature_level;
    const D3D_FEATURE_LEVEL feature_level_array[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, feature_level_array, 2, D3D11_SDK_VERSION, &swap_chain_description, &d3d_swapchain, &d3d_device, &feature_level, &d3d_context) != S_OK)
        return false;

    //create back buffer
    ID3D11Texture2D* d3d_buffer;
    d3d_swapchain->GetBuffer(0, IID_PPV_ARGS(&d3d_buffer));
    d3d_device->CreateRenderTargetView(d3d_buffer, NULL, &d3d_view);
    d3d_buffer->Release();

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(d3d_device, d3d_context);

    return true;
}

/* window::run()
*  executes a frame of the window
*/
void window::run() {
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    const ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(350, 200));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar;
    {
        ImGui::Begin("kahoot spammer", (bool*)0, window_flags);
        ImGui::Text("kahoot spammer by nolan burkhart");
        ImGui::Text("Github: @Nolan-Burkhart");

        static char pin[8];
        ImGui::InputText("game pin", pin, 8);

        static char name[30];
        ImGui::InputText("custom name", name, 30);

        static int bots = 1;
        ImGui::SliderInt("number of bots", &bots, 1, 100); //max 100 sockets we can open

        const auto time_since_epoch = []() -> uint64_t { //macro i keep abusing
            return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        };

        if (threads.size() == 0 && ImGui::Button("start!")) { //if we haven't started it yet
            for (int i = 0; i < bots; i++) {
                bot::go_time = time_since_epoch() + (bots / 10) + 3; //set the time for when the bots will flood all at once rather than trickle
                threads.push_back(std::thread(bot::run, std::string(pin), std::string(name) + std::to_string(i), i * 100)); //creat a new bot thread
            }
        }

        if (threads.size() != 0) //if we have started
        {
            ImGui::Text(std::string(std::string("bots connected: ") + std::to_string(bot::bots_connected)).c_str()); //how many bots are connected
            float fraction = (float)bot::bots_connected / (float)bots; //percent of bots
            ImGui::ProgressBar(fraction); //show progress bar
            if (time_since_epoch() < bot::go_time) { //countdown
                ImGui::Text(std::string(std::string("seconds until bots hit: ") + std::to_string(bot::go_time - time_since_epoch())).c_str());
            }
            else {
                ImGui::Text("bots have landed");
            }
        }

        ImGui::End();
    }

    // render
    ImGui::Render();
    const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
    d3d_context->OMSetRenderTargets(1, &d3d_view, NULL);
    d3d_context->ClearRenderTargetView(d3d_view, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    d3d_swapchain->Present(1, 0); //present
}

/* window::exit()
*  closes the window
*/
void window::exit() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}



/* window::wnd_proc()
*  handles window input
*/
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT window::wnd_proc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param) {

    if (ImGui_ImplWin32_WndProcHandler(wnd, msg, w_param, l_param))
        return true;

    switch (msg)
    {
    case WM_LBUTTONDOWN:
        SetCapture(wnd);
        x_click = LOWORD(l_param);
        y_click = HIWORD(l_param);
        break;
    case WM_MOUSEMOVE:
        update_window_pos(wnd, msg, w_param, l_param);
        break;
    case WM_SYSCOMMAND:
        if ((w_param & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProc(wnd, msg, w_param, l_param);
}