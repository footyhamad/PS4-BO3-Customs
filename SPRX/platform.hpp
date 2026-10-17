#pragma once

typedef int (*sceKernelDebugOutText_t)(int32_t dbg_channel, const char* text);
inline sceKernelDebugOutText_t g_sceKernelDebugOutText = nullptr;

uint64_t GetBaseAddress();

bool RangeReadable(uintptr_t addr, size_t span);

bool SafeStrStr(uintptr_t addr, const char* target, size_t maxScan = 64);

void Notify(const char* fmt, ...);

bool IsPs5();

const char* DebugSink();

void DebugOut(const char* text);
