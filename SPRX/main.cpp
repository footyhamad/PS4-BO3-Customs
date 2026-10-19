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

#ifndef BO3_CUSTOMS_DIAG_STAGE
#define BO3_CUSTOMS_DIAG_STAGE 0
#endif

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

static void* start_thread(void*)
{
   const uintptr_t base = WaitForBlackOps3();

   if (!base)
   {
       Notify("BO3 Customs: BO3 v1.33 was not detected after 10 minutes");
       return nullptr;
   }

   // Never install offsets/hooks into an executable whose BO3 v1.33
   // prologues do not match. A title string alone is not a build check.
   if (!T7Maps_BuildSupported(base))
   {
       Notify("BO3 Customs: startup stopped; BO3 v1.33 signatures did not match");
       return nullptr;
   }

#if BO3_CUSTOMS_DIAG_STAGE == 3
   Notify("DIAG 3 PASS: BO3 title and v1.33 signatures recognized; no hooks installed");
   return nullptr;
#endif

   // The game executable and static strings appear before the frontend is
   // initialized. Wait until its UI/Lua runtime is live, then let startup settle
   // before patching any code so we do not race BO3's early initialization.
   bool frontendReady = false;
   for (int attempt = 0; attempt < 6000; ++attempt)
   {
       if (T7Lua_RuntimeReady(base))
       {
           frontendReady = true;
           break;
       }

       sceKernelUsleep(100 * 1000);
   }

   if (!frontendReady)
   {
       Notify("BO3 Customs: frontend did not become ready; hooks were not installed");
       return nullptr;
   }

   sceKernelUsleep(1500 * 1000);

   if (!T7Maps_BuildSupported(base) || !T7Lua_RuntimeReady(base))
   {
       Notify("BO3 Customs: startup state changed; hooks were not installed");
       return nullptr;
   }

#if BO3_CUSTOMS_DIAG_STAGE == 4
   Notify("DIAG 4 PASS: frontend runtime became ready; no hooks installed");
   return nullptr;
#endif

   const char* rand = sceKernelGetFsSandboxRandomWord();

   char path[256 * 2];
   snprintf(path, sizeof(path), "/%s/common/lib/libSceNotification.sprx", rand);

   bool is_ps5 = false;
   int fd = sceKernelOpen(path, O_RDONLY, 0);

   if (fd >= 0)
   {
       is_ps5 = true;
       sceKernelClose(fd);
   }

   if (!is_ps5)
   {
       for (const char* const drive : k_driveRoots)
       {
           char mnt[128];
           int len = snprintf(mnt, sizeof(mnt), "/mnt/%s", drive);

           if (len < 0 || static_cast<size_t>(len) >= sizeof(mnt))
               continue;

           char sandboxPath[128];
           len = snprintf(sandboxPath, sizeof(sandboxPath), "/%s", drive);

           if (len < 0 || static_cast<size_t>(len) >= sizeof(sandboxPath))
               continue;
           
           jbc_mount_in_sandbox(mnt, drive);
       }
   }

#if BO3_CUSTOMS_DIAG_STAGE == 5
    Notify("DIAG 5 PASS: filesystem sandbox/mount stage completed; no hooks installed");
    return nullptr;
#endif

    T7Log_Install(base);
#if BO3_CUSTOMS_DIAG_STAGE == 6
    Notify("DIAG 6 PASS: T7Log_Install returned; no later modules installed");
    return nullptr;
#endif
    T7Maps_Install(base);
#if BO3_CUSTOMS_DIAG_STAGE == 7
    Notify("DIAG 7 PASS: T7Maps_Install returned; map_count=%d", T7Maps_MapCount());
    return nullptr;
#endif

    T7MapImages_Install(base);
#if BO3_CUSTOMS_DIAG_STAGE == 8
    Notify("DIAG 8 PASS: T7MapImages_Install returned; no later modules installed");
    return nullptr;
#endif

    T7Lua_Install(base);
#if BO3_CUSTOMS_DIAG_STAGE == 9
    Notify("DIAG 9 PASS: T7Lua_Install returned; no later modules installed");
    return nullptr;
#endif

    T7Kbm_Install(base);
#if BO3_CUSTOMS_DIAG_STAGE == 10
    Notify("DIAG 10 PASS: T7Kbm_Install returned; no later modules installed");
    return nullptr;
#endif

    T7Builtins_Install(base);
#if BO3_CUSTOMS_DIAG_STAGE == 11
    Notify("DIAG 11 PASS: T7Builtins_Install returned; no later modules installed");
    return nullptr;
#endif

    if (T7Maps_MapCount() >= 0)
        T7Share_Install(base);
#if BO3_CUSTOMS_DIAG_STAGE == 12
    Notify("DIAG 12 PASS: share service startup returned; frame detour not installed");
    return nullptr;
#endif

    T7Log_Write("[Maps] %d custom map(s) found", T7Maps_MapCount());

    if (RangeReadable(base + kComFrame, sizeof(k_comFrame)) &&
        memcmp((const void*)(base + kComFrame), k_comFrame, sizeof(k_comFrame)) == 0)
    {
        Detour_Attach(&g_frameDetour, (uint64_t)(base + kComFrame), (void*)Frame_h, &g_frameOriginal);
    }

    if (T7Maps_MapCount() < 0)
        Notify("BO3 Customs Mod loaded!\nCreated by ItsJokerZz. This game build is not supported - it needs version 1.33");
    else
    {
        char buff[256];
        if (T7Maps_ModCount() > 0)
            snprintf(buff, sizeof(buff), "BO3 Customs Mod loaded!\nCreated by ItsJokerZz.\nFound %d custom map(s) and %d mod(s)", T7Maps_MapCount(), T7Maps_ModCount());
        else
            snprintf(buff, sizeof(buff), "BO3 Customs Mod loaded!\nCreated by ItsJokerZz.\nFound %d custom map(s)", T7Maps_MapCount());
        Notify(buff);
    }

    return nullptr;
}

namespace
{
std::atomic<bool> g_pluginStarted{false};

int StartPlugin()
{
#if BO3_CUSTOMS_DIAG_STAGE == 1
    return 0;
#elif BO3_CUSTOMS_DIAG_STAGE == 2
    bool expected = false;
    if (!g_pluginStarted.compare_exchange_strong(expected, true))
        return 0;
    Notify("DIAG 2 PASS: full-project PRX entrypoint and notification executed; no thread or hooks");
    return 0;
#else
    bool expected = false;
    if (!g_pluginStarted.compare_exchange_strong(expected, true))
        return 0;

    Notify("BO3 Customs plugin started; waiting for BO3 v1.33");

    OrbisPthread thread{};
    const int createResult = scePthreadCreate(&thread, nullptr, start_thread, nullptr, "BO3-Customs Init");
    if (createResult != 0)
    {
        g_pluginStarted.store(false);
        Notify("BO3 Customs: failed to start initialization thread (%d)", createResult);
        return createResult;
    }

    const int detachResult = scePthreadDetach(thread);
    if (detachResult != 0)
    {
        Notify("BO3 Customs: failed to detach initialization thread (%d)", detachResult);
        return detachResult;
    }

    return 0;
#endif
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
