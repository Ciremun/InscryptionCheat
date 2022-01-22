#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "minhook/minhook.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include <d3d11.h>

#include <stdint.h>

#include "ic_core.hpp"
#include "ic_util.hpp"
#include "ic_offsets.hpp"

typedef long(__stdcall* present)(IDXGISwapChain*, UINT, UINT);

bool g_continue = true;
bool g_instant_win = false;
bool g_infinite_health = false;
HINSTANCE dll_handle;

present p_present;
present p_present_target;
bool get_present_pointer()
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = GetForegroundWindow();
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* swap_chain;
    ID3D11Device* device;

    const D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(
        NULL, 
        D3D_DRIVER_TYPE_HARDWARE, 
        NULL, 
        0, 
        feature_levels, 
        2, 
        D3D11_SDK_VERSION, 
        &sd, 
        &swap_chain, 
        &device, 
        nullptr, 
        nullptr) == S_OK)
    {
        void** p_vtable = *reinterpret_cast<void***>(swap_chain);
        swap_chain->Release();
        device->Release();
        //context->Release();
        p_present_target = (present)p_vtable[8];
        return true;
    }
    return false;
}

WNDPROC oWndProc;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

bool init = false;
HWND window = NULL;
float viewport_width = 0;
ID3D11Device* p_device = NULL;
ID3D11DeviceContext* p_context = NULL;
ID3D11RenderTargetView* mainRenderTargetView = NULL;
static long __stdcall detour_present(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags) {
    if (!init) {
        if (SUCCEEDED(p_swap_chain->GetDevice(__uuidof(ID3D11Device), (void**)&p_device)))
        {
            p_device->GetImmediateContext(&p_context);
            DXGI_SWAP_CHAIN_DESC sd;
            p_swap_chain->GetDesc(&sd);
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            p_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            p_device->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = NULL;
            io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
            ImGui_ImplWin32_Init(window);
            ImGui_ImplDX11_Init(p_device, p_context);
            viewport_width = ImGui::GetMainViewport()->Size.x;
            init = true;
        }
        else
            return p_present(p_swap_chain, sync_interval, flags);
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();

    ImGui::Begin("", &g_continue);
    ImGui::Checkbox("Instant Win", &g_instant_win);
    ImGui::Checkbox("Infinite Health", &g_infinite_health);
    ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();

    p_context->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return p_present(p_swap_chain, sync_interval, flags);
}

DWORD __stdcall EjectThread(LPVOID lpParameter) {
    (void)lpParameter;
    Sleep(100);
    FreeLibraryAndExitThread(dll_handle, 0);
}

int WINAPI main()
{

    if (!get_present_pointer()) 
    {
        return 1;
    }

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK)
    {
        return 1;
    }

    if (MH_CreateHook(reinterpret_cast<void**>(p_present_target), &detour_present, reinterpret_cast<void**>(&p_present)) != MH_OK) {
        return 1;
    }

    if (MH_EnableHook(p_present_target) != MH_OK) {
        return 1;
    }

    HANDLE process_handle = GetCurrentProcess();

    const uintptr_t unity_player_dll_base = GetModuleBaseAddress("UnityPlayer.dll");
    IF(unity_player_dll_base == 0, "Couldn't get module's base address");

    int current_part = get_current_part(process_handle);
    int cycles = 0;
    bool previous_instant_win_value = g_instant_win;
    bool previous_infinite_health_value = g_infinite_health;

    while (1)
    {
        if (!g_continue) break;
        if (cycles == 300)
        {
            current_part = get_current_part(process_handle);
            cycles = 0;
        }
        // TODO: cleanup, lambda with compare function?
        if (g_instant_win || g_infinite_health)
        {
            uintptr_t duel_struct_address = get_current_duel_struct_address(process_handle, unity_player_dll_base, current_part);
            if (duel_struct_address != 0)
            {
                if (g_instant_win)
                {
                    uint8_t actual_value;
                    uint8_t new_value = 16;
                    if (internal_memory_read(process_handle, duel_struct_address + damage_dealt_offset, &actual_value)
                        && actual_value != 16)
                        internal_memory_write(duel_struct_address + damage_dealt_offset, &new_value);
                }
                if (g_infinite_health)
                {
                    uint8_t actual_value;
                    uint8_t new_value = 0;
                    if (internal_memory_read(process_handle, duel_struct_address + damage_taken_offset, &actual_value)
                        && actual_value > 0)
                        internal_memory_write(duel_struct_address + damage_taken_offset, &new_value);
                }
            }
        }
        if (!g_instant_win && previous_instant_win_value)
        {
            uintptr_t duel_struct_address = get_current_duel_struct_address(process_handle, unity_player_dll_base, current_part);
            if (duel_struct_address != 0)
            {
                uint8_t actual_value;
                uint8_t new_value = 0;
                if (internal_memory_read(process_handle, duel_struct_address + damage_dealt_offset, &actual_value)
                    && actual_value == 16)
                    internal_memory_write(duel_struct_address + damage_dealt_offset, &new_value);
            }
        }
        previous_instant_win_value = g_instant_win;
        previous_infinite_health_value = g_infinite_health;
        Sleep(200);
        cycles++;
    }

    if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK) {
        return 1;
    }
    if (MH_Uninitialize() != MH_OK) {
        return 1;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = NULL; }
    if (p_context) { p_context->Release(); p_context = NULL; }
    if (p_device) { p_device->Release(); p_device = NULL; }
    SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(oWndProc));

    CreateThread(0, 0, EjectThread, 0, 0, 0);

    return 0;
}

BOOL __stdcall DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
    (void)lpReserved;
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        dll_handle = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, NULL, 0, NULL);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {

    }
    return TRUE;
}
