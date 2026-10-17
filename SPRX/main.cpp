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

static void* start_thread(void*)
{
   const uintptr_t base = WaitForBlackOps3();

   if (!base)
   {
       Notify("BO3 Customs: BO3 v1.33 was not detected after 10 minutes");
       return nullptr;
   }

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

    T7Log_Install(base);
    T7Maps_Install(base);
    T7MapImages_Install(base);
    T7Lua_Install(base);
    T7Kbm_Install(base);
    T7Builtins_Install(base);

    if (T7Maps_MapCount() >= 0)
        T7Share_Install(base);

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
