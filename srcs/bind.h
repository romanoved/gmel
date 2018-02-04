#pragma once

#include <gnumake.h>

int gmel_memfunc_table_setup(void);
char* gmel_memfunc_call(char* func_name, int argc, char** argv);
char* gmel_popen_bind(char* func_name, int argc, char** argv);
char* gmel_bind(char* func_name, int argc, char** argv);
char* gmel_bind_r(char* func_name, int argc, char** argv);
char* gmel_bind_args(char* func_name, int argc, char** argv);
char* gmel_mft_register(char* func_name, int argc, char** argv);
char* gmel_print_mft(char* func_name, int argc, char** argv);
