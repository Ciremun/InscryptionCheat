#pragma once

#include <windows.h>

int init_mono();
void return_zero_cost();
void _cdecl find_code_starts(void *assembly, void *domain);

typedef void* (__cdecl *MONO_GET_ROOT_DOMAIN)(void);
typedef void* (__cdecl *MONO_THREAD_ATTACH)(void *domain);
typedef void* (__cdecl *MONO_ASSEMBLY_GET_IMAGE)(void *assembly);
typedef void* (__cdecl *MONO_CLASS_FROM_NAME_CASE)(void *image, char *name_space, char *name);
typedef void* (__cdecl *MONO_CLASS_GET_METHOD_FROM_NAME)(void *klass, char *methodname, int paramcount);
typedef void* (__cdecl *MONO_COMPILE_METHOD)(void *method);
typedef void* (__cdecl *MONO_JIT_INFO_TABLE_FIND)(void *domain, void *addr);
typedef void* (__cdecl *MONO_JIT_INFO_GET_CODE_START)(void *jitinfo);
typedef void  (__cdecl *MONO_THREAD_DETACH)(void *monothread);
typedef void  (__cdecl *GFunc)(void *data, void *user_data);
typedef int   (__cdecl *MONO_ASSEMBLY_FOREACH)(GFunc func, void *user_data);

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
