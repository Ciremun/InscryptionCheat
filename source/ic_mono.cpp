#include <string.h>

#include "ic_util.hpp"
#include "ic_mono.hpp"

bool mono_initialized = false;

unsigned char get_BloodCost_original_bytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x38 };
void *get_BloodCost_code_start = 0;

unsigned char get_BonesCost_original_bytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x38 };
void *get_BonesCost_code_start = 0;

unsigned char infinite_health_original_bytes[] = { 0x89, 0x46, 0x10 };
void *infinite_health_code_start = 0;

// Instant Win
unsigned char life_manager_ctor_original_bytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x18 };
void *life_manager_ctor_code_start = 0;
void *life_manager_ctor_jump_back = 0;
void *life_manager_instance = 0;
void *life_manager_vtable = 0;

void print_life_manager_instance()
{
    IC_INFO_FMT("new life_manager_instance: 0x%X", (uintptr_t)life_manager_instance);
}

__declspec(naked) void return_zero_cost()
{
    __asm {
        xor eax, eax
        ret
    }
}

__declspec(naked) void snitch_life_manager_instance()
{
    __asm {
        mov [life_manager_instance], ecx
#ifndef NDEBUG
        call print_life_manager_instance
#endif // NDEBUG
        // original code
        push ebp
        mov ebp,esp
        sub esp,0x18
        // jump back
        jmp [life_manager_ctor_jump_back]
    }
}

void _cdecl find_code_starts(void *assembly, void *domain)
{
    if (get_BloodCost_code_start     &&
        get_BonesCost_code_start     &&
        infinite_health_code_start   &&
        life_manager_ctor_code_start &&
        life_manager_vtable)
    {
        mono_initialized = true;
        return;
    }

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

    if (!infinite_health_code_start)
    {
        void* table_info = mono_image_get_table_info(image, 2);
        int rows = mono_table_info_get_rows(table_info);
        for (int i = 0; i < rows; i++)
        {
            uint32_t cols[6];
            mono_metadata_decode_row(table_info, i, cols, 6);
            const char* ns = mono_metadata_string_heap(image, cols[2]);
            if (!ns[0])
            {
                const char* name = mono_metadata_string_heap(image, cols[1]);
                if (!infinite_health_code_start &&
                    memcmp(name, "<ShowDamageSequence>d__23", 25) == 0)
                {
                    void *cls = mono_class_get(image, i + 1 | 0x02000000);
                    if (!cls) return;
                    void *code_start = get_code_start(domain, cls, "MoveNext");
                    if (code_start)
                        infinite_health_code_start = (void *)((uintptr_t)code_start + 0x2FF);
                    IC_INFO_FMT("infinite_health_code_start: 0x%X",
                        (uintptr_t)infinite_health_code_start);
                }
            }
            if (infinite_health_code_start)
                break;
        }
    }

    if (!life_manager_ctor_code_start || !life_manager_vtable)
    {
        void *lifemanager_class = mono_class_from_name_case(image, "DiskCardGame", "LifeManager");
        if (!lifemanager_class) return;
        if (!life_manager_ctor_code_start)
        {
            life_manager_ctor_code_start = get_code_start(domain, lifemanager_class, ".ctor");
            life_manager_ctor_jump_back = (void *)((uintptr_t)life_manager_ctor_code_start + 6);
            IC_INFO_FMT("life_manager_ctor_code_start: 0x%X", (uintptr_t)life_manager_ctor_code_start);
        }
        if (!life_manager_vtable)
        {
            life_manager_vtable = mono_class_vtable(domain, lifemanager_class);
            IC_INFO_FMT("life manager vtable: 0x%X", (uintptr_t)life_manager_vtable);
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
    mono_image_get_table_info       = (MONO_IMAGE_GET_TABLE_INFO)       GetProcAddress(hMono, "mono_image_get_table_info");
    mono_table_info_get_rows        = (MONO_TABLE_INFO_GET_ROWS)        GetProcAddress(hMono, "mono_table_info_get_rows");
    mono_metadata_decode_row        = (MONO_METADATA_DECODE_ROW)        GetProcAddress(hMono, "mono_metadata_decode_row");
    mono_metadata_string_heap       = (MONO_METADATA_STRING_HEAP)       GetProcAddress(hMono, "mono_metadata_string_heap");
    mono_class_get                  = (MONO_CLASS_GET)                  GetProcAddress(hMono, "mono_class_get");
    mono_class_vtable               = (MONO_CLASS_VTABLE)               GetProcAddress(hMono, "mono_class_vtable");

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

    return mono_initialized;
}
