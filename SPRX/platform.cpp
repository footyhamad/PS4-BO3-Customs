#include "headers.hpp"
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

namespace {
    uint64_t g_execStart = 0;
    uint64_t g_execEnd = 0;
    uint64_t g_moduleEnd = 0;

    enum NotificationType {
        kNotificationRequest = 0,
        kNotificationRequestWithIcon = 1,
    };

    using SendNotification_t = int32_t (*)(int32_t, OrbisNotificationRequest*, size_t, int32_t);
    SendNotification_t g_sendNotification = nullptr;
    bool g_notificationReady = false;

    constexpr int kSinkNone = 0;
    constexpr int kSinkPs5 = 1;
    constexpr int kSinkPs4 = 2;
    constexpr int kKernelLogText = 7;
    constexpr size_t kKernelLogChunk = 900;
    constexpr uintptr_t kKernelSpan = 32u * 1024 * 1024;

    int g_ps5 = -1;
    int g_sink = -1;
}

extern "C" {
    int mdbg_service(int command, void* arg1, void* arg2);
    const char* sceKernelGetFsSandboxRandomWord();
}

uint64_t GetBaseAddress() {
    static uint64_t cached = 0;

    if (cached)
        return cached;

    OrbisKernelVirtualQueryInfo info;
    void* address = nullptr;

    while (sceKernelVirtualQuery(address, 0, &info, sizeof(info)) >= 0) {
        const uintptr_t start = (uintptr_t)info.unk01;
        const uintptr_t end = (uintptr_t)info.unk02;

        if (end <= start)
            break;

        address = (void*)end;

        if (info.unk04 != 5 || strcmp(info.name, "executable") != 0)
            continue;

        cached = start;
        g_execStart = start;
        g_execEnd = end;
        g_moduleEnd = end;

        OrbisKernelVirtualQueryInfo next;
        uintptr_t probe = end;

        for (int hop = 0; hop < 16; ++hop) {
            if (sceKernelVirtualQuery((void*)probe, 0, &next, sizeof(next)) < 0)
                break;

            const uintptr_t ns = (uintptr_t)next.unk01;
            const uintptr_t ne = (uintptr_t)next.unk02;

            if (ne <= ns || ns > g_moduleEnd || (next.unk04 & 1) == 0)
                break;

            g_moduleEnd = ne;
            probe = ne;
        }

        return cached;
    }

    return 0;
}

static uintptr_t ReadableEnd(uintptr_t addr, size_t need) {
    if (addr < 0x10000 || addr >= 0x0000800000000000ULL || addr + need < addr)
        return 0;

    OrbisKernelVirtualQueryInfo info;
    uintptr_t end = 0;
    uintptr_t probe = addr;

    for (int hop = 0; hop < 8; ++hop) {
        if (sceKernelVirtualQuery((void*)probe, 0, &info, sizeof(info)) < 0)
            break;

        if ((info.unk04 & 1) == 0)
            break;

        const uintptr_t start = (uintptr_t)info.unk01;
        const uintptr_t stop = (uintptr_t)info.unk02;

        if (stop <= start || probe < start || probe >= stop || (end && start > end))
            break;

        end = stop;
        probe = stop;

        if (end >= addr + need)
            break;
    }

    return end;
}

bool RangeReadable(uintptr_t addr, size_t span) {
    if (!addr || addr + span < addr)
        return false;

    const uintptr_t end = ReadableEnd(addr, span);
    return end != 0 && (addr + span) <= end;
}

bool SafeStrStr(uintptr_t addr, const char* target, size_t maxScan) {
    if (addr < 0x10000 || addr >= 0x0000800000000000ULL || !target)
        return false;

    const size_t length = strlen(target);

    if (!length || length > maxScan)
        return false;

    const uintptr_t end = ReadableEnd(addr, maxScan);

    if (!end || end <= addr)
        return false;

    size_t available = (size_t)(end - addr);

    if (available > maxScan)
        available = maxScan;

    if (available < length)
        return false;

    const char* const text = (const char*)addr;

    for (size_t i = 0; i + length <= available; ++i) {
        if (text[i] == 0)
            return false;

        size_t j = 0;

        while (j < length && text[i + j] == target[j])
            ++j;

        if (j == length)
            return true;
    }

    return false;
}

void Notify(const char* fmt, ...) {
    if (!g_notificationReady) {
        g_notificationReady = true;

        void* address = nullptr;

        if (sceKernelDlsym(0x2001, "sceKernelSendNotificationRequest", &address) == 0 && address)
            g_sendNotification = (SendNotification_t)address;
    }

    OrbisNotificationRequest request{};

    va_list va;
    va_start(va, fmt);
    vsnprintf(request.message, sizeof(request.message), fmt, va);
    va_end(va);

    request.type = NotificationRequest;
    request.targetId = -1;
    request.useIconImageUri = 1;
    snprintf(request.iconUri, sizeof(request.iconUri), "%s", "cxml://psnotification/tex_icon_ribbon");

    if (g_sendNotification)
        g_sendNotification(0, &request, sizeof(request), 0);
    else
        sceKernelSendNotificationRequest(0, &request, sizeof(request), 0);
}

bool IsPs5() {
    if (g_ps5 >= 0)
        return g_ps5 == 1;

    OrbisKernelStat st;
    const char* const word = sceKernelGetFsSandboxRandomWord();
    bool found = false;

    if (word && word[0]) {
        char path[160];
        snprintf(path, sizeof(path), "/%s/common/lib/libSceNotification.sprx", word);
        found = sceKernelStat(path, &st) == 0;
    }

    if (!found)
        found = sceKernelStat("/system/common/lib/libSceNotification.sprx", &st) == 0;

    g_ps5 = found ? 1 : 0;
    return found;
}

static uintptr_t ImportAddress(uintptr_t import, uintptr_t ownStart, uintptr_t ownEnd) {
    if (import >= ownStart && import + 6 <= ownEnd) {
        const uint8_t* const stub = (const uint8_t*)import;

        if (stub[0] != 0xFF || stub[1] != 0x25)
            return 0;

        import = *(const uintptr_t*)(import + 6 + (intptr_t)*(const int32_t*)(stub + 2));
    }

    return (import >= ownStart && import < ownEnd) ? 0 : import;
}

static bool KernelLogBound() {
    OrbisKernelVirtualQueryInfo info;

    if (sceKernelVirtualQuery((void*)&KernelLogBound, 0, &info, sizeof(info)) < 0)
        return false;

    const uintptr_t start = (uintptr_t)info.unk01;
    const uintptr_t end = (uintptr_t)info.unk02;
    const uintptr_t log = ImportAddress((uintptr_t)&mdbg_service, start, end);
    const uintptr_t kernel = ImportAddress((uintptr_t)&sceKernelUsleep, start, end);

    return log && kernel && (log > kernel ? log - kernel : kernel - log) < kKernelSpan;
}

static int Sink() {
    if (g_sink >= 0)
        return g_sink;

    int sink = kSinkNone;

    if (IsPs5()) {
        if (KernelLogBound())
            sink = kSinkPs5;
    } else {
        void* address = nullptr;

        if (sceKernelDlsym(0x2001, "sceKernelDebugOutText", &address) == 0 && address) {
            g_sceKernelDebugOutText = (sceKernelDebugOutText_t)address;
            sink = kSinkPs4;
        }
    }

    g_sink = sink;
    return sink;
}

const char* DebugSink() {
    const int sink = Sink();
    return sink == kSinkPs5 ? "the PS5 kernel log" : sink == kSinkPs4 ? "the PS4 debug output" : nullptr;
}

void DebugOut(const char* text) {
    if (!text || !text[0])
        return;

    const int sink = Sink();

    if (sink == kSinkNone)
        return;

    for (size_t left = strlen(text); left;) {
        const size_t take = left < kKernelLogChunk ? left : kKernelLogChunk;
        char line[kKernelLogChunk + 32];

        snprintf(line, sizeof(line), "%s[BO3-Customs] %.*s%s", sink == kSinkPs5 ? "<118>" : "", (int)take, text,
                 text[take - 1] == '\n' ? "" : "\n");

        if (sink == kSinkPs5)
            mdbg_service(kKernelLogText, line, nullptr);
        else
            g_sceKernelDebugOutText(0, line);

        text += take;
        left -= take;
    }
}
