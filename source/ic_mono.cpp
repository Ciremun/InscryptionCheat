#include "ic_util.hpp"
#include "ic_mono.hpp"

unsigned char get_BloodCost_original_bytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x38 };
void *get_BloodCost_code_start = 0;

unsigned char get_BonesCost_original_bytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x38 };
void *get_BonesCost_code_start = 0;

__declspec(naked) void return_zero_cost()
{
    __asm {
        xor eax, eax
        ret
    }
}

// diskcardgame.lifemanager.showdamagesequence d__23MoveNext ? offset 2ff inf health

void _cdecl find_code_starts(void *assembly, void *domain)
{
    if (get_BloodCost_code_start && get_BonesCost_code_start)
        return;

    const auto get_code_start = [](void* domain, void* class_, char *method_name) -> void*
    {
        void* method = mono_class_get_method_from_name(class_, method_name, -1);
        if (!method) return 0;

        void* compiled_method_addr = mono_compile_method(method);
        if (!compiled_method_addr) return 0;

        void* jit_info = mono_jit_info_table_find(domain, compiled_method_addr);
        if (!jit_info) return 0;

        return mono_jit_info_get_code_start(jit_info);
    };

    void* image = mono_assembly_get_image(assembly);
    if (!image) return;

    if (!get_BloodCost_code_start || !get_BonesCost_code_start)
    {
        void* class_ = mono_class_from_name_case(image, "DiskCardGame", "CardInfo");
        if (!class_) return;

        if (!get_BloodCost_code_start)
        {
            get_BloodCost_code_start = get_code_start(domain, class_, "get_BloodCost");
            IC_INFO_FMT("get_BloodCost_code_start: 0x%X", (uintptr_t)get_BloodCost_code_start);
        }

        if (!get_BonesCost_code_start)
        {
            get_BonesCost_code_start = get_code_start(domain, class_, "get_BonesCost");
            IC_INFO_FMT("get_BonesCost_code_start: 0x%X", (uintptr_t)get_BonesCost_code_start);
        }
    }
}

int init_mono()
{
    HMODULE hMono = GetModuleHandleA("mono-2.0-bdwgc.dll");
    if (hMono == NULL)
    {
        IC_WINAPI_ERROR();
        return 0;
    }

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
        IC_ERROR("couldn't get root domain");
        return 0;
    }

    void* mono_selfthread = mono_thread_attach(domain);
    if (!mono_selfthread)
    {
        IC_ERROR("couldn't attach thread");
        return 0;
    }

    mono_assembly_foreach((GFunc)find_code_starts, domain);

    mono_thread_detach(mono_selfthread);

    return get_BloodCost_code_start != 0;
}
