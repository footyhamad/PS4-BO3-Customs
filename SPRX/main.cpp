#ifdef BO3_CUSTOMS_BOOT_PROBE
#include <stddef.h>
#include <stdint.h>

// Diagnostic-only GoldHEN/OpenOrbis PRX. No game reads, notifications,
// threads, memory patches, detours, or mod initialization.
extern "C" {
__attribute__((visibility("default")))
int32_t plugin_load(int32_t argc, const char* argv[])
{
    (void)argc;
    (void)argv;
    return 0;
}

__attribute__((visibility("default")))
int32_t plugin_unload(int32_t argc, const char* argv[])
{
    (void)argc;
    (void)argv;
    return 0;
}

int module_start(size_t argc, const void* args)
{
    (void)argc;
    (void)args;
    return 0;
}

int module_stop(size_t argc, const void* args)
{
    (void)argc;
    (void)args;
    return 0;
}
}
#else
#include "headers.hpp"
#include <fcntl.h>
#include <atomic>


namespace
{
constexpr uintptr_t kTitleProbe = 0x1312346;

constexpr uintptr_t kComFrame = 0xF0B8A0;

const uint8_t k_comFrame[] = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                               0x53, 0x48, 0x83, 0xE4, 0xE0, 0x48, 0x81, 0xEC, 0x40, 0x22, 0x00, 0x00 };

Detour g_frameDetour{};
void*  g_frameOriginal = nullptr;

using Hook_t = uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                            double, double, double, double, double, double, double, double);

uint64_t Frame_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                 double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    T7Maps_FrameWork();
    T7Share_Tick();
    T7Lua_Tick();
    T7Builtins_Tick();

    return ((Hook_t)g_frameOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

bool IsBlackOps3(uintptr_t base)
{
    return SafeStrStr(base + kTitleProbe, "Multiplayer") || SafeStrStr(base + kTitleProbe, "multiProgress");
}

uintptr_t WaitForBlackOps3()
{
    for (int attempt = 0; attempt < 6000; ++attempt)
    {
        const uintptr_t base = (uintptr_t)GetBaseAddress();

        if (base && IsBlackOps3(base))
            return base;

        sceKernelUsleep(100 * 1000);
    }

    return 0;
}
}

extern "C" const char* sceKernelGetFsSandboxRandomWord();

namespace
{
const char* g_diagLogPath = nullptr;
bool g_diagLogInitialized = false;
unsigned int g_diagSequence = 0;
std::atomic_flag g_diagLogBusy = ATOMIC_FLAG_INIT;

void DiagLogInit()
{
    if (g_diagLogInitialized)
        return;

    g_diagLogInitialized = true;
    sceKernelMkdir("/data/BO3-Customs", 0777);

    const char* const primary = "/data/BO3-Customs/diagnostics.log";
    int fd = sceKernelOpen(primary, O_WRONLY | O_CREAT | O_TRUNC, 0777);

    if (fd >= 0)
    {
        g_diagLogPath = primary;
        sceKernelClose(fd);
    }
    else
    {
        const char* const fallback = "/data/bo3customs-diagnostics.log";
        fd = sceKernelOpen(fallback, O_WRONLY | O_CREAT | O_TRUNC, 0777);

        if (fd >= 0)
        {
            g_diagLogPath = fallback;
            sceKernelClose(fd);
        }
    }

    if (g_diagLogPath)
    {
        const char header[] = "BO3-Customs staged runtime diagnostics\n";
        fd = sceKernelOpen(g_diagLogPath, O_WRONLY | O_CREAT | O_APPEND, 0777);
        if (fd >= 0)
        {
            sceKernelWrite(fd, header, sizeof(header) - 1);
            sceKernelClose(fd);
        }
    }
}

void DiagLog(const char* level, int stage, const char* fmt, ...)
{
    if (!g_diagLogInitialized)
        DiagLogInit();

    char message[384];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    // The entrypoint and detached worker can log at nearly the same time.
    // Serialize append operations so each checkpoint stays on its own line.
    while (g_diagLogBusy.test_and_set(std::memory_order_acquire))
        sceKernelUsleep(50);

    char line[576];
    const int length = snprintf(line, sizeof(line), "[%04u] %-7s stage=%02d %s\n",
                                ++g_diagSequence, level ? level : "INFO", stage, message);

    if (g_diagLogPath && length > 0)
    {
        const size_t bytes = (size_t)length < sizeof(line) ? (size_t)length : sizeof(line) - 1;
        const int fd = sceKernelOpen(g_diagLogPath, O_WRONLY | O_CREAT | O_APPEND, 0777);

        if (fd >= 0)
        {
            const char* at = line;
            size_t left = bytes;
            while (left)
            {
                const int64_t wrote = sceKernelWrite(fd, at, left);
                if (wrote <= 0)
                    break;
                at += (size_t)wrote;
                left -= (size_t)wrote;
            }
            sceKernelClose(fd);
        }
    }

    g_diagLogBusy.clear(std::memory_order_release);
}

void DiagStart(int stage, const char* name)
{
    DiagLog("START", stage, "%s", name);
}

void DiagPass(int stage, const char* name)
{
    // PASS is persisted before notification delivery. The notification markers
    // distinguish a crash inside Notify() from a crash in the stage operation.
    DiagLog("PASS", stage, "%s", name);
    DiagLog("NOTIFY_START", stage, "sending stage PASS notification");
    Notify("BO3 Customs DIAG\nStage %d/13 PASS:\n%s", stage, name);
    DiagLog("NOTIFY_RETURN", stage, "stage PASS notification returned");
}

void DiagFail(int stage, const char* name)
{
    DiagLog("FAIL", stage, "%s", name);
    DiagLog("NOTIFY_START", stage, "sending stage FAIL notification");
    Notify("BO3 Customs DIAG\nStage %d/13 FAILED:\n%s", stage, name);
    DiagLog("NOTIFY_RETURN", stage, "stage FAIL notification returned");
}

void DiagPause()
{
    sceKernelUsleep(3 * 1000 * 1000);
}
}

static void* start_thread(void*)
{
    DiagLog("BOOT", 0, "initialization thread started; log=%s",
            g_diagLogPath ? g_diagLogPath : "log file unavailable");

    // Stage 1 validates that the full project's startup thread and notification
    // function can execute without touching the game or installing hooks.
    DiagStart(1, "plugin thread and notification path");
    DiagPass(1, "plugin thread and notification path returned");
    DiagPause();

    DiagStart(2, "wait for Black Ops 3 executable/title strings");
    uintptr_t base = 0;
    for (int attempt = 0; attempt < 6000; ++attempt)
    {
        base = (uintptr_t)GetBaseAddress();
        if (base && IsBlackOps3(base))
            break;

        if ((attempt % 30) == 0)
            DiagLog("WAIT", 2, "waiting for BO3 title; attempt=%d/6000 base=%p",
                    attempt, (void*)base);
        sceKernelUsleep(100 * 1000);
    }

    if (!base || !IsBlackOps3(base))
    {
        DiagFail(2, "BO3 title was not detected within 10 minutes");
        return nullptr;
    }
    DiagPass(2, "BO3 executable and title strings detected");
    DiagPause();

    DiagStart(3, "verify known BO3 v1.33 function signatures");
    if (!T7Maps_BuildSupported(base))
    {
        DiagFail(3, "BO3 v1.33 signatures did not match; no hooks attempted");
        return nullptr;
    }
    DiagPass(3, "BO3 v1.33 signature preflight passed");
    DiagPause();

    DiagStart(4, "wait for frontend UI/Lua runtime readiness");
    bool frontendReady = false;
    for (int attempt = 0; attempt < 6000; ++attempt)
    {
        if (T7Lua_RuntimeReady(base))
        {
            frontendReady = true;
            break;
        }

        if ((attempt % 30) == 0)
            DiagLog("WAIT", 4, "waiting for frontend UI/Lua; attempt=%d/6000", attempt);
        sceKernelUsleep(100 * 1000);
    }

    if (!frontendReady)
    {
        DiagFail(4, "frontend UI/Lua runtime did not become ready within 10 minutes");
        return nullptr;
    }

    // Let frontend startup settle before any hook installer executes.
    sceKernelUsleep(1500 * 1000);
    if (!T7Maps_BuildSupported(base) || !T7Lua_RuntimeReady(base))
    {
        DiagFail(4, "signature or frontend readiness changed before initialization");
        return nullptr;
    }
    DiagPass(4, "frontend UI/Lua runtime ready and revalidated");
    DiagPause();

    DiagStart(5, "filesystem sandbox detection and external-root mount calls");
    const char* const rand = sceKernelGetFsSandboxRandomWord();
    bool is_ps5 = false;

    if (rand && rand[0])
    {
        char path[256];
        snprintf(path, sizeof(path), "/%s/common/lib/libSceNotification.sprx", rand);

        const int fd = sceKernelOpen(path, O_RDONLY, 0);
        if (fd >= 0)
        {
            is_ps5 = true;
            sceKernelClose(fd);
        }
    }
    else
    {
        DiagLog("WARN", 5, "filesystem sandbox random word was null/empty; external mount scan skipped");
    }

    if (!is_ps5 && rand && rand[0])
    {
        for (const char* const drive : k_driveRoots)
        {
            char mnt[128];
            const int len = snprintf(mnt, sizeof(mnt), "/mnt/%s", drive);
            if (len < 0 || static_cast<size_t>(len) >= sizeof(mnt))
                continue;

            jbc_mount_in_sandbox(mnt, drive);
        }
    }

    DiagPass(5, "filesystem sandbox/mount stage returned");
    DiagPause();

    DiagStart(6, "T7Log_Install");
    T7Log_Install(base);
    DiagPass(6, "T7Log_Install returned");
    DiagPause();

    DiagStart(7, "T7Maps_Install and custom map/mod scan");
    T7Maps_Install(base);
    const int mapCount = T7Maps_MapCount();
    const int modCount = T7Maps_ModCount();
    DiagLog("INFO", 7, "map_count=%d mod_count=%d", mapCount, modCount);
    T7Log_Write("[Maps] %d custom map(s), %d mod(s) found", mapCount, modCount);
    DiagPass(7, "T7Maps_Install returned");
    DiagPause();

    DiagStart(8, "T7MapImages_Install");
    T7MapImages_Install(base);
    DiagPass(8, "T7MapImages_Install returned");
    DiagPause();

    DiagStart(9, "T7Lua_Install");
    T7Lua_Install(base);
    DiagPass(9, "T7Lua_Install returned");
    DiagPause();

    DiagStart(10, "T7Kbm_Install");
    T7Kbm_Install(base);
    DiagPass(10, "T7Kbm_Install returned");
    DiagPause();

    DiagStart(11, "T7Builtins_Install");
    T7Builtins_Install(base);
    DiagPass(11, "T7Builtins_Install returned");
    DiagPause();

    DiagStart(12, "T7Share_Install / background share server");
    if (T7Maps_MapCount() >= 0)
        T7Share_Install(base);
    else
        DiagLog("SKIP", 12, "map_count is negative; share server not started");
    DiagPass(12, "share service stage returned");
    DiagPause();

    DiagStart(13, "frame detour at the BO3 v1.33 frame function");
    bool frameInstalled = false;
    if (RangeReadable(base + kComFrame, sizeof(k_comFrame)) &&
        memcmp((const void*)(base + kComFrame), k_comFrame, sizeof(k_comFrame)) == 0)
    {
        frameInstalled = Detour_Attach(&g_frameDetour, (uint64_t)(base + kComFrame),
                                       (void*)Frame_h, &g_frameOriginal) != nullptr;
    }

    if (frameInstalled)
        DiagPass(13, "frame detour installed; all startup stages returned");
    else
    {
        DiagLog("WARN", 13, "frame signature mismatch or detour install failed; other stages completed");
        DiagPass(13, "stage completed, but frame detour was not installed");
    }

    DiagLog("DONE", 13, "all stages returned; maps=%d mods=%d", T7Maps_MapCount(), T7Maps_ModCount());
    Notify("BO3 Customs DIAG\nAll 13 stages returned.\nCheck diagnostics.log");
    return nullptr;
}

namespace
{
std::atomic<bool> g_pluginStarted{false};

int StartPlugin()
{
    bool expected = false;
    if (!g_pluginStarted.compare_exchange_strong(expected, true))
        return 0;

    DiagLogInit();
    DiagLog("BOOT", 0, "plugin entrypoint reached; creating init thread");

    OrbisPthread thread{};
    const int createResult = scePthreadCreate(&thread, nullptr, start_thread, nullptr, "BO3-Customs Init");
    if (createResult != 0)
    {
        DiagLog("FAIL", 0, "scePthreadCreate failed: %d", createResult);
        Notify("BO3 Customs DIAG: init thread create failed (%d)", createResult);
        g_pluginStarted.store(false);
        return createResult;
    }

    const int detachResult = scePthreadDetach(thread);
    if (detachResult != 0)
    {
        DiagLog("FAIL", 0, "scePthreadDetach failed: %d", detachResult);
        Notify("BO3 Customs DIAG: init thread detach failed (%d)", detachResult);
        return detachResult;
    }

    DiagLog("INFO", 0, "init thread detached successfully");
    return 0;
}
}

extern "C"
{
__attribute__((visibility("default")))
int32_t plugin_load(int32_t argc, const char* argv[])
{
    (void)argc;
    (void)argv;
    return StartPlugin();
}

__attribute__((visibility("default")))
int32_t plugin_unload(int32_t argc, const char* argv[])
{
    (void)argc;
    (void)argv;
    return 0;
}

int module_start(size_t argc, const void* args)
{
    (void)argc;
    (void)args;
    return StartPlugin();
}

int module_stop(size_t argc, const void* args)
{
    (void)argc;
    (void)args;
    return 0;
}
}

#endif // BO3_CUSTOMS_BOOT_PROBE
