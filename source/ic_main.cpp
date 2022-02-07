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
#include "ic_mono.hpp"

#define IC_ENABLED ImVec4(1.00f, 1.00f, 1.00f, 1.00f)
#define IC_DISABLED ImVec4(0.50f, 0.50f, 0.50f, 1.00f)
#define IC_UNAVAILABLE ImVec4(0.588f, 0.012f, 0.102f, 1.00f)

typedef long(__stdcall* present)(IDXGISwapChain*, UINT, UINT);

HINSTANCE dll_handle;
HANDLE g_process;

bool g_continue = true;
bool g_instant_win = false;
bool g_infinite_health = false;
bool g_free_cards = false;

uintptr_t g_unity_player_dll_base = 0;
uintptr_t g_view_matrix_struct_address = 0;
uintptr_t g_duel_struct_address = 0;

extern void *get_BloodCost_code_start;
extern void *get_BonesCost_code_start;

struct ViewMatrix
{
    float x;
    float y;
    float z;
    float unknown_1;
    float unknown_2;
    float rot;
    float unknown_4;
    float rot_2;
};

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
            if (!init_mono())
                IC_ERROR("init_mono failed");
            init = true;
        }
        else
            return p_present(p_swap_chain, sync_interval, flags);
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(160.0f, 200.0f), ImGuiCond_Once);
    ImGui::Begin("ic.dll", &g_continue);

    if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Duel"))
        {
            // TODO:
            // [ ] inf card health
            // [ ] inf card attack
            // [ ] add sigil

            const auto MaybeCheckbox = [](const char* label, bool cond, bool &enabled, auto body)
            {
                if (cond)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, enabled ? IC_ENABLED : IC_DISABLED);
                    if (ImGui::Checkbox(label, &enabled))
                        body();
                    ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleColor(ImGuiCol_Text, IC_UNAVAILABLE);
                    ImGui::Checkbox(label, &enabled);
                    ImGui::PopStyleColor();
                    ImGui::PopItemFlag();
                }
            };

            ImGui::PushStyleColor(ImGuiCol_Text, g_instant_win ? IC_ENABLED : IC_DISABLED);
            ImGui::Checkbox("Instant Win", &g_instant_win);
            ImGui::PopStyleColor();

            MaybeCheckbox("Infinite Health", 1, g_infinite_health, [](){

            });

            MaybeCheckbox("Free Cards", get_BloodCost_code_start && get_BonesCost_code_start, g_free_cards, [](){
                if (g_free_cards)
                {
                    detour_32(get_BloodCost_code_start, return_zero_cost, sizeof(get_BloodCost_original_bytes));
                    detour_32(get_BonesCost_code_start, return_zero_cost, sizeof(get_BonesCost_original_bytes));
                }
                else
                {
                    IC_ERROR_IF(!internal_memory_patch(get_BloodCost_code_start, get_BloodCost_original_bytes, sizeof(get_BloodCost_original_bytes)),
                        "Couldn't write get_BloodCost_original_bytes");
                    IC_ERROR_IF(!internal_memory_patch(get_BonesCost_code_start, get_BonesCost_original_bytes, sizeof(get_BonesCost_original_bytes)),
                        "Couldn't write get_BonesCost_original_bytes");
                }
            });

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("View Matrix"))
        {
            if (g_view_matrix_struct_address)
            {
                ImGui::SliderFloat("X",  (float *)(g_view_matrix_struct_address + offsetof(ViewMatrix, x)), -32.0f, 32.0f, "%.3f");
                ImGui::SliderFloat("Y",  (float *)(g_view_matrix_struct_address + offsetof(ViewMatrix, y)), -32.0f, 32.0f, "%.3f");
                ImGui::SliderFloat("Z",  (float *)(g_view_matrix_struct_address + offsetof(ViewMatrix, z)), -32.0f, 32.0f, "%.3f");
                ImGui::SliderFloat("R1", (float *)(g_view_matrix_struct_address + offsetof(ViewMatrix, rot)), -1.0f, 1.0f, "%.3f");
                ImGui::SliderFloat("R2", (float *)(g_view_matrix_struct_address + offsetof(ViewMatrix, rot_2)), -1.0f, 1.0f, "%.3f");
                ImGui::SliderFloat("R3", (float *)(g_view_matrix_struct_address + view_matrix_rot_bottom_offset), -1.0f, 1.0f, "%.3f");
            }
            else
            {
                float temp = 0.0f;
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleColor(ImGuiCol_Text, IC_UNAVAILABLE);
                ImGui::SliderFloat("X",  &temp, -32.0f, 32.0f, "%.3f");
                ImGui::SliderFloat("Y",  &temp, -32.0f, 32.0f, "%.3f");
                ImGui::SliderFloat("Z",  &temp, -32.0f, 32.0f, "%.3f");
                ImGui::SliderFloat("R1", &temp, -1.0f, 1.0f, "%.3f");
                ImGui::SliderFloat("R2", &temp, -1.0f, 1.0f, "%.3f");
                ImGui::SliderFloat("R3", &temp, -1.0f, 1.0f, "%.3f");
                ImGui::PopStyleColor();
                ImGui::PopItemFlag();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

        if (ImGui::IsItemActivated())
        {
            // NOTE(Ciremun): struct base derived from Z address
            g_view_matrix_struct_address =
                internal_multi_level_pointer_dereference(g_process, g_unity_player_dll_base + view_matrix_base_offset, view_matrix_struct_offsets);
            if (g_view_matrix_struct_address)
                g_view_matrix_struct_address -= 8;
            IC_INFO_FMT("g_view_matrix_struct_address: 0x%X", g_view_matrix_struct_address);
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = io.WantCaptureMouse;

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
#ifndef NDEBUG
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
#endif // NDEBUG

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

    g_process = GetCurrentProcess();
    g_unity_player_dll_base = GetModuleBaseAddress("UnityPlayer.dll");
    IC_ERROR_IF(g_unity_player_dll_base == 0, "Couldn't get module's base address");

    int current_part = get_current_part(g_process);
    int cycles = 0;
    bool previous_instant_win_value = g_instant_win;

    while (1)
    {
        if (!g_continue) break;

        if (cycles == 300)
        {
            current_part = get_current_part(g_process);
            cycles = 0;
        }

        const auto write_to_duel_struct = [](uintptr_t duel_struct_addr, uintptr_t offset, uint8_t new_val, auto compare_func) {
            uint8_t actual_val;
            if (internal_memory_read(g_process, duel_struct_addr + offset, &actual_val)
                && compare_func(actual_val))
                internal_memory_write(duel_struct_addr + offset, &new_val);
        };

        if (g_instant_win || g_infinite_health)
        {
            g_duel_struct_address = get_current_duel_struct_address(g_process, g_unity_player_dll_base, current_part);
            if (g_duel_struct_address)
            {
                if (g_instant_win)
                    write_to_duel_struct(g_duel_struct_address, damage_dealt_offset, 16, [](uint8_t val){ return val != 16; });
                if (g_infinite_health)
                    write_to_duel_struct(g_duel_struct_address, damage_taken_offset, 0, [](uint8_t val){ return val > 0; });
            }
        }

        if (!g_instant_win && previous_instant_win_value)
        {
            g_duel_struct_address = get_current_duel_struct_address(g_process, g_unity_player_dll_base, current_part);
            if (g_duel_struct_address)
                write_to_duel_struct(g_duel_struct_address, damage_dealt_offset, 0, [](uint8_t val){ return val == 16; });
        }

        previous_instant_win_value = g_instant_win;
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

#ifndef NDEBUG
    FreeConsole();
#endif // NDEBUG

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
