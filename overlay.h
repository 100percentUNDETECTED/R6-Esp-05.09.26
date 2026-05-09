#pragma once
#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <iostream>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class c_overlay {
private:
    HWND m_hwnd = nullptr;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_device_context = nullptr;
    IDXGISwapChain* m_swap_chain = nullptr;
    ID3D11RenderTargetView* m_render_target_view = nullptr;
    bool m_menu_visible = false;
    int m_width = 0;
    int m_height = 0;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
        if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    bool init_d3d() {
        DXGI_SWAP_CHAIN_DESC sd{};
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = m_hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createDeviceFlags = 0;
        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
        if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_swap_chain, &m_device, &featureLevel, &m_device_context) != S_OK)
            return false;

        ID3D11Texture2D* pBackBuffer;
        m_swap_chain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        m_device->CreateRenderTargetView(pBackBuffer, nullptr, &m_render_target_view);
        pBackBuffer->Release();
        return true;
    }

public:
    ~c_overlay() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (m_render_target_view) { m_render_target_view->Release(); m_render_target_view = nullptr; }
        if (m_swap_chain) { m_swap_chain->Release(); m_swap_chain = nullptr; }
        if (m_device_context) { m_device_context->Release(); m_device_context = nullptr; }
        if (m_device) { m_device->Release(); m_device = nullptr; }
        DestroyWindow(m_hwnd);
        UnregisterClassW(L"R6ExternalOverlay", GetModuleHandle(nullptr));
    }

    bool create(bool demo_mode = false) {
        HWND game_hwnd = nullptr;
        RECT rect;
        if (!demo_mode) {
            game_hwnd = FindWindowW(nullptr, L"Rainbow Six");
            if (!game_hwnd) {
                std::cout << "[!] Waiting for Rainbow Six window..." << std::endl;
                while (!(game_hwnd = FindWindowW(nullptr, L"Rainbow Six"))) Sleep(1000);
            }
            GetWindowRect(game_hwnd, &rect);
        } else {
            rect.left = 0; rect.top = 0;
            rect.right = GetSystemMetrics(SM_CXSCREEN);
            rect.bottom = GetSystemMetrics(SM_CYSCREEN);
        }

        m_width = rect.right - rect.left;
        m_height = rect.bottom - rect.top;

        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"R6ExternalOverlay", nullptr };
        RegisterClassExW(&wc);

        m_hwnd = CreateWindowExW(WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, wc.lpszClassName, L"Overlay", WS_POPUP, rect.left, rect.top, m_width, m_height, nullptr, nullptr, wc.hInstance, nullptr);
        SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

        MARGINS margins = { -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(m_hwnd, &margins);

        if (!init_d3d()) return false;

        ShowWindow(m_hwnd, SW_SHOWDEFAULT);
        UpdateWindow(m_hwnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(m_hwnd);
        ImGui_ImplDX11_Init(m_device, m_device_context);

        return true;
    }

    bool process_messages() {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) return false;
        }
        return true;
    }

    void begin_frame() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void end_frame() {
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_device_context->OMSetRenderTargets(1, &m_render_target_view, nullptr);
        m_device_context->ClearRenderTargetView(m_render_target_view, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        m_swap_chain->Present(1, 0);
    }

    void toggle_menu() {
        m_menu_visible = !m_menu_visible;
        long ex_style = GetWindowLong(m_hwnd, GWL_EXSTYLE);
        if (m_menu_visible) {
            ex_style &= ~WS_EX_TRANSPARENT;
        } else {
            ex_style |= WS_EX_TRANSPARENT;
        }
        SetWindowLong(m_hwnd, GWL_EXSTYLE, ex_style);
        SetForegroundWindow(m_hwnd);
    }

    bool menu_visible() const { return m_menu_visible; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    void draw_text(float x, float y, const char* text, ImU32 color) {
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(x, y), color, text);
    }

    void draw_line(float x1, float y1, float x2, float y2, ImU32 color, float thickness = 1.0f) {
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, thickness);
    }

    void draw_filled_rect(float x, float y, float w, float h, ImU32 color) {
        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color);
    }

    void draw_cornered_box(float x, float y, float w, float h, ImU32 color, float thickness = 1.0f) {
        float line_w = w / 4.0f;
        float line_h = h / 4.0f;
        draw_line(x, y, x, y + line_h, color, thickness);
        draw_line(x, y, x + line_w, y, color, thickness);
        draw_line(x + w - line_w, y, x + w, y, color, thickness);
        draw_line(x + w, y, x + w, y + line_h, color, thickness);
        draw_line(x, y + h - line_h, x, y + h, color, thickness);
        draw_line(x, y + h, x + line_w, y + h, color, thickness);
        draw_line(x + w - line_w, y + h, x + w, y + h, color, thickness);
        draw_line(x + w, y + h - line_h, x + w, y + h, color, thickness);
    }

    void draw_actor_esp(float head_x, float head_y, float foot_x, float foot_y, float distance, bool draw_box, bool draw_snap) {
        float h = foot_y - head_y;
        float w = h * 0.45f;
        float x = head_x - w / 2.0f;
        float y = head_y;

        ImU32 color = IM_COL32(255, 255, 0, 255); // Yellow >40m
        if (distance < 15.0f) color = IM_COL32(255, 0, 0, 255); // Red <15m
        else if (distance < 40.0f) color = IM_COL32(255, 165, 0, 255); // Orange <40m

        if (draw_box) {
            draw_cornered_box(x, y, w, h, color, 1.5f);
            draw_filled_rect(x - 6.0f, y, 4.0f, h, IM_COL32(0, 0, 0, 255));
            draw_filled_rect(x - 5.0f, y + 1.0f, 2.0f, h - 2.0f, IM_COL32(0, 255, 0, 255));
        }

        if (draw_snap) {
            draw_line(m_width / 2.0f, (float)m_height, foot_x, foot_y, IM_COL32(255, 255, 255, 100), 1.0f);
        }

        char dist_str[32];
        snprintf(dist_str, sizeof(dist_str), "[%.1fm]", distance);
        draw_text(x + w / 2.0f - ImGui::CalcTextSize(dist_str).x / 2.0f, y + h + 2.0f, dist_str, IM_COL32(255, 255, 255, 255));
    }
};
