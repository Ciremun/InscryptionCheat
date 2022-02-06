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

HINSTANCE dll_handle;
HANDLE g_process;

bool g_continue = true;
bool g_instant_win = false;
bool g_infinite_health = false;
bool g_zero_blood_cost = false;

uintptr_t g_unity_player_dll_base = 0;
uintptr_t g_view_matrix_struct_address = 0;

unsigned char get_BloodCost_original_bytes[6] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x38 };
void *get_BloodCost_code_start = 0;

__declspec(naked) void zero_blood_cost()
{
    __asm {
        mov eax, 0
        ret
    }
}

int detour_32(void *src, void *dst, int len)
{
    DWORD oldProtect;
    if (VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldProtect) == 0)
        return 0;

    memset(src, 0x90, len);

    uintptr_t relative_address = (uintptr_t)dst - (uintptr_t)src - 5;

    *(BYTE *)src = 0xE9;

    *(uintptr_t *)((uintptr_t)src + 1) = relative_address;

    if (VirtualProtect(src, len, oldProtect, &oldProtect) == 0)
        return 0;

    return 1;
}

struct ViewMatrix
{
    float x;
    float y;
    float z;
    float rot;
    float unknown_1;
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
            // [ ] free cards
            // [ ] inf card health
            // [ ] inf card attack
            // [ ] add sigil
            ImGui::Checkbox("Instant Win", &g_instant_win);
            ImGui::Checkbox("Infinite Health", &g_infinite_health);
            if (ImGui::Checkbox("Zero Blood Cost", &g_zero_blood_cost))
            {
                if (g_zero_blood_cost)
                    detour_32(get_BloodCost_code_start, zero_blood_cost, 6);
                else
                    memcpy(get_BloodCost_code_start, get_BloodCost_original_bytes, 6);
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("View Matrix"))
        {
            if (g_view_matrix_struct_address)
            {
                // NOTE(Ciremun): struct base derived from Z address
                ImGui::SliderFloat("X",  (float *)(g_view_matrix_struct_address - offsetof(ViewMatrix, z)), -32.0f, 32.0f, "%.3f");
                ImGui::SliderFloat("Y",  (float *)(g_view_matrix_struct_address - offsetof(ViewMatrix, y)), -32.0f, 32.0f, "%.3f");
                ImGui::SliderFloat("Z",  (float *)(g_view_matrix_struct_address + offsetof(ViewMatrix, x)), -32.0f, 32.0f, "%.3f");
                ImGui::SliderFloat("R1", (float *)(g_view_matrix_struct_address + offsetof(ViewMatrix, rot)), -1.0f, 1.0f, "%.3f");
                ImGui::SliderFloat("R2", (float *)(g_view_matrix_struct_address + offsetof(ViewMatrix, rot_2)), -1.0f, 1.0f, "%.3f");
                ImGui::SliderFloat("R3", (float *)(g_view_matrix_struct_address + view_matrix_rot_bottom_offset), -1.0f, 1.0f, "%.3f");
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

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

typedef void* (__cdecl *MONO_GET_ROOT_DOMAIN)(void);
typedef void* (__cdecl *MONO_THREAD_ATTACH)(void *domain);
typedef void* (__cdecl *MONO_ASSEMBLY_GET_IMAGE)(void *assembly);
typedef void* (__cdecl *MONO_CLASS_FROM_NAME_CASE)(void *image, char *name_space, char *name);
typedef void* (__cdecl *MONO_CLASS_GET_METHOD_FROM_NAME)(void *klass, char *methodname, int paramcount);
typedef void* (__cdecl *MONO_COMPILE_METHOD)(void *method);
typedef void* (__cdecl *MONO_JIT_INFO_TABLE_FIND)(void *domain, void *addr);
typedef void* (__cdecl *MONO_JIT_INFO_GET_CODE_START)(void *jitinfo);
typedef void (__cdecl *MONO_THREAD_DETACH)(void *monothread);
typedef void (__cdecl *GFunc)          (void *data, void *user_data);
typedef int (__cdecl *MONO_ASSEMBLY_FOREACH)(GFunc func, void *user_data);

MONO_GET_ROOT_DOMAIN            mono_get_root_domain;
MONO_THREAD_ATTACH              mono_thread_attach;
MONO_ASSEMBLY_GET_IMAGE         mono_assembly_get_image;
MONO_CLASS_FROM_NAME_CASE       mono_class_from_name_case;
MONO_CLASS_GET_METHOD_FROM_NAME mono_class_get_method_from_name;
MONO_COMPILE_METHOD             mono_compile_method;
MONO_JIT_INFO_TABLE_FIND        mono_jit_info_table_find;
MONO_JIT_INFO_GET_CODE_START    mono_jit_info_get_code_start;
MONO_THREAD_DETACH              mono_thread_detach;
MONO_ASSEMBLY_FOREACH           mono_assembly_foreach;

void _cdecl AssemblyEnumerator(void *assembly, void *domain)
{
    void* image                = mono_assembly_get_image(assembly);                            if (!image) return;
    void* class_               = mono_class_from_name_case(image, "DiskCardGame", "CardInfo"); if (!class_) return;
    void* method               = mono_class_get_method_from_name(class_, "get_BloodCost", -1); if (!method) return;
    void* compiled_method_addr = mono_compile_method(method);                                  if (!compiled_method_addr) return;
    void* jit_info             = mono_jit_info_table_find(domain, compiled_method_addr);       if (!jit_info) return;
    get_BloodCost_code_start   = mono_jit_info_get_code_start(jit_info);
}

int init_mono()
{
    HMODULE hMono = GetModuleHandleA("mono-2.0-bdwgc.dll");
    CHECK(hMono != NULL);

    mono_get_root_domain            = (MONO_GET_ROOT_DOMAIN)            GetProcAddress(hMono, "mono_get_root_domain");
    mono_thread_attach              = (MONO_THREAD_ATTACH)              GetProcAddress(hMono, "mono_thread_attach");
    mono_assembly_get_image         = (MONO_ASSEMBLY_GET_IMAGE)         GetProcAddress(hMono, "mono_assembly_get_image");
    mono_class_from_name_case       = (MONO_CLASS_FROM_NAME_CASE)       GetProcAddress(hMono, "mono_class_from_name_case");
    mono_class_get_method_from_name = (MONO_CLASS_GET_METHOD_FROM_NAME) GetProcAddress(hMono, "mono_class_get_method_from_name");
    mono_compile_method             = (MONO_COMPILE_METHOD)             GetProcAddress(hMono, "mono_compile_method");
    mono_jit_info_table_find        = (MONO_JIT_INFO_TABLE_FIND)        GetProcAddress(hMono, "mono_jit_info_table_find");
    mono_jit_info_get_code_start    = (MONO_JIT_INFO_GET_CODE_START)    GetProcAddress(hMono, "mono_jit_info_get_code_start");
    mono_thread_detach              = (MONO_THREAD_DETACH)              GetProcAddress(hMono, "mono_thread_detach");
    mono_assembly_foreach           = (MONO_ASSEMBLY_FOREACH)           GetProcAddress(hMono, "mono_assembly_foreach");

    void* domain = mono_get_root_domain();
    if (!domain)
    {
        ERR("couldn't get root domain");
        return 0;
    }

    void* mono_selfthread = mono_thread_attach(domain);
    if (!mono_selfthread)
    {
        ERR("couldn't attach thread");
        return 0;
    }

    mono_assembly_foreach((GFunc)AssemblyEnumerator, domain);

    mono_thread_detach(mono_selfthread);

    return get_BloodCost_code_start != 0;
}

int WINAPI main()
{
#ifndef NDEBUG
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
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
    IF(g_unity_player_dll_base == 0, "Couldn't get module's base address");

    int current_part = get_current_part(g_process);
    int cycles = 0;
    bool previous_instant_win_value = g_instant_win;
    bool previous_infinite_health_value = g_infinite_health;

    init_mono();

    while (1)
    {
        if (!g_continue) break;
        if (cycles == 300)
        {
            current_part = get_current_part(g_process);
            cycles = 0;
        }

        g_view_matrix_struct_address =
            internal_multi_level_pointer_dereference(g_process, g_unity_player_dll_base + view_matrix_base_offset, view_matrix_struct_offsets);

        auto write_to_duel_struct = [](uintptr_t duel_struct_addr, uintptr_t offset, uint8_t new_val, auto compare_func) {
            uint8_t actual_val;
            if (internal_memory_read(g_process, duel_struct_addr + offset, &actual_val)
                && compare_func(actual_val))
                internal_memory_write(duel_struct_addr + offset, &new_val);
        };

        if (g_instant_win || g_infinite_health)
        {
            uintptr_t duel_struct_address = get_current_duel_struct_address(g_process, g_unity_player_dll_base, current_part);
            if (duel_struct_address != 0)
            {
                if (g_instant_win)
                    write_to_duel_struct(duel_struct_address, damage_dealt_offset, 16, [](uint8_t val){ return val != 16; });
                if (g_infinite_health)
                    write_to_duel_struct(duel_struct_address, damage_taken_offset, 0, [](uint8_t val){ return val > 0; });
            }
        }
        if (!g_instant_win && previous_instant_win_value)
        {
            uintptr_t duel_struct_address = get_current_duel_struct_address(g_process, g_unity_player_dll_base, current_part);
            if (duel_struct_address != 0)
                write_to_duel_struct(duel_struct_address, damage_dealt_offset, 0, [](uint8_t val){ return val == 16; });
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
