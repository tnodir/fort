#pragma once

#include <stdint.h>
#include <string>

void __mem_leak_check(size_t& memLeakCount, uint64_t& memLeakSize, bool verbose, const char* fileDumperName, bool append);
void __static_mem_check(size_t&  memCount, uint64_t& memSize, bool verbose, const char* fileDumperName, bool append);
void __start(bool verbose, const char* fileDumperName, bool append);
void __stop();
void __stop_and_free_all_mem();
void __program_started();