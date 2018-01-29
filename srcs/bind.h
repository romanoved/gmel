#pragma once

int gmel_memfunc_table_setup();
char* gmel_memfunc_call(char* func_name, int argc, char** argv);
char* gmel_popen_bind(char* func_name, int argc, char** argv);
char* gmel_bind(char* func_name, int argc, char** argv);
char* gmel_bind_r(char* func_name, int argc, char** argv);
