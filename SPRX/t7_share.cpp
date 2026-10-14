#include "headers.hpp"
#include <dirent.h>
#include "t7_share.hpp"

#include <orbis/Net.h>

struct OrbisNetSockaddrIn
{
    uint8_t sin_len;
    uint8_t sin_family;
    uint16_t sin_port;
    OrbisNetInAddr sin_addr;
    char sin_zero[8];
};

#include <atomic>
#include <sys/stat.h>

namespace T7Share
{
constexpr uint16_t  kPort               = 28330;
constexpr uintptr_t kNetworkMode        = 0xA6DBA98;
constexpr uintptr_t kClientSessions     = 0xA605850;
constexpr size_t    kClientSessionSize  = 11272;
constexpr size_t    kSessionActive      = 64;
constexpr size_t    kSessionHostAddress = 136;
constexpr size_t    kSessionHostType    = 144;
constexpr size_t    kSessionHostBlob    = 153;
constexpr size_t    kSessionHostHeard   = 9076;
constexpr uintptr_t kTaskList           = 0xCD68BF0;
constexpr uintptr_t kLaunchTask         = 0x15D70B0;
constexpr size_t    kTaskDefinition     = 8;
constexpr size_t    kTaskTimeout        = 44;
constexpr int       kTaskWalk           = 512;
constexpr size_t    kHostBlobSize       = 37;
constexpr size_t    kHostSlotSize       = 6;
constexpr int       kMaxHosts           = 3;
constexpr size_t    kGameLobby          = 1;
constexpr int32_t   kNetworkLan         = 1;
constexpr int32_t   kAddressLoopback    = 2;
constexpr int32_t   kAddressIp          = 4;

constexpr int      kMaxEntries          = 768;
constexpr size_t   kRelMax              = 160;
constexpr size_t   kPathMax             = kT7SharePath + kT7ShareName + kRelMax + 32;
constexpr size_t   kChunk               = 256 * 1024;
constexpr int      kMaxConnections      = 4;
constexpr int      kWalkDepth           = 3;
constexpr uint64_t kMatchBytesPerSecond = 10ull * 1024 * 1024;
constexpr int      kSocketWaitUs        = 2000000;
constexpr int      kSocketBuffers[]     = { 1024 * 1024, 512 * 1024, 256 * 1024 };
constexpr int      kQuietRounds         = 10;
constexpr uint64_t kConnectWaitUs       = 2500000;
constexpr uint64_t kConnectManyUs       = 1500000;
constexpr uint64_t kConnectStepUs       = 20000;
constexpr uint64_t kDeafHostUs          = 600000000;
constexpr uint64_t kSnapshotUs          = 250000;
constexpr uint64_t kWrapEveryUs         = 1000000;
constexpr uint64_t kFailureShownUs      = 6000000;
constexpr uint64_t kAwayUs              = 3000000;
constexpr uint64_t kSettleUs            = 400000;
constexpr uint64_t kLaunchGraceUs       = 90000000;
constexpr uint64_t kFreshListUs         = 500000;
constexpr int      kListTries           = 6;
constexpr uint64_t kListRetryUs         = 500000;

enum Phase
{
    kIdle,
    kContacting,
    kListing,
    kDownloading,
    kInstalling,
    kReady,
    kFailed,
};

struct Role
{
    bool     lan;
    bool     host;
    bool     client;
    uint32_t hostIp;
    int      hostCount;
    uint32_t hosts[kMaxHosts];
};

struct Hosted
{
    bool serving;
    bool custom;
    char map[kT7ShareName];
    char mapRoot[kT7SharePath];
    int  modCount;
    char mods[kT7ShareMods][kT7ShareName];
    char modRoots[kT7ShareMods][kT7SharePath];
};

struct Served
{
    uint64_t size;
    int32_t  kind;
    char     rel[kRelMax];
    char     path[kT7SharePath + kRelMax + 2];
};

struct Entry
{
    uint64_t size;
    int32_t  kind;
    bool     wanted;
    char     rel[kRelMax];
};

struct Session
{
    bool     active;
    bool     needMap;
    bool     listed;
    bool     fetched;
    uint32_t hostIp;
    uint32_t reached;
    int      hostCount;
    uint32_t hosts[kMaxHosts];
    int      modCount;
    int      newMods;
    char     map[kT7ShareName];
    char     mods[kT7ShareMods][kT7ShareName];
    char     current[kRelMax];
    char     error[160];
};

struct Stream
{
    OrbisNetId                 socket;
    const std::atomic<bool>* stop;
    int                      have;
    int                      at;
    char                     held[4096];
};

struct Pace
{
    uint64_t since;
    uint64_t bytes;
};

struct Spin
{
    std::atomic<bool>& flag;

    explicit Spin(std::atomic<bool>& taken) : flag(taken)
    {
        while (flag.exchange(true, std::memory_order_acquire))
            sceKernelUsleep(50);
    }

    ~Spin()
    {
        flag.store(false, std::memory_order_release);
    }
};

static uintptr_t g_base = 0;
static bool      g_ready = false;

static Hosted            g_hosted{};
static std::atomic<bool> g_hostedLock{false};
static std::atomic<bool> g_serve{false};
static std::atomic<bool> g_hostInLevel{false};
static std::atomic<int>  g_connections{0};
static std::atomic<bool> g_hostedWanted{false};
static std::atomic<uint32_t> g_hostedVersion{0};
static uint64_t          g_hostedAt = 0;

static Session               g_session{};
static std::atomic<bool>     g_textLock{false};
static std::atomic<bool>     g_evaluating{false};
static std::atomic<bool>     g_lastHold{false};
static std::atomic<int>      g_phase{kIdle};
static std::atomic<bool>     g_busy{false};
static std::atomic<bool>     g_stop{false};
static std::atomic<uint64_t> g_total{0};
static std::atomic<uint64_t> g_done{0};
static std::atomic<int>      g_fileAt{0};
static std::atomic<int>      g_fileCount{0};
static std::atomic<bool>     g_wrapped{false};
static std::atomic<bool>     g_mapsChanged{false};
static std::atomic<uint64_t> g_failedAt{0};
static uint64_t              g_wrapTriedAt = 0;
static uint64_t              g_awaySince = 0;
static char                  g_pendingMap[kT7ShareName] = "";
static uint32_t              g_pendingHost = 0;
static uint64_t              g_pendingSince = 0;
static uint64_t              g_heldAt = 0;
static uint64_t              g_keptAt = 0;
static uint64_t              g_keptRest = 0;
static std::atomic<uint32_t> g_deafHost{0};
static std::atomic<uint64_t> g_deafSince{0};
static bool                  g_overriding = false;
static uint32_t              g_loggedHost = 0;

static bool EqualsNoCase(const char* a, const char* b)
{
    for (;; ++a, ++b)
    {
        const char x = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
        const char y = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;

        if (x != y)
            return false;

        if (!x)
            return true;
    }
}

static bool EndsWith(const char* text, const char* tail)
{
    const size_t n = strlen(text);
    const size_t m = strlen(tail);
    return n >= m && strcmp(text + n - m, tail) == 0;
}

static bool Private(uint32_t address)
{
    const uint8_t* const b = (const uint8_t*)&address;
    return b[0] == 10 || (b[0] == 172 && (b[1] & 0xF0) == 16) || (b[0] == 192 && b[1] == 168) ||
           (b[0] == 169 && b[1] == 254);
}

static bool SafeName(const char* name, size_t length)
{
    if (!length || name[0] == '.')
        return false;

    for (size_t i = 0; i < length; ++i)
    {
        const char c = name[i];

        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' ||
              c == '-'))
            return false;
    }

    return true;
}

static bool SafeRel(const char* rel)
{
    if (strlen(rel) >= kRelMax)
        return false;

    int parts = 0;

    for (const char* at = rel;;)
    {
        const char* end = at;

        while (*end && *end != '/')
            ++end;

        if (!SafeName(at, (size_t)(end - at)) || ++parts > kWalkDepth + 1)
            return false;

        if (!*end)
            return true;

        at = end + 1;
    }
}

static bool SafeFolder(const char* name)
{
    size_t n = 0;

    for (; name[n]; ++n)
    {
        const unsigned char c = (unsigned char)name[n];

        if (n >= kT7ShareName - 1 || c < 0x20 || c >= 0x7F || c == '"' || c == '\\' || c == '/' || c == ':' ||
            c == '*' || c == '?')
            return false;
    }

    return n > 0 && name[0] != '.' && name[0] != ' ' && name[n - 1] != ' ';
}

static bool ReadMapName(char* out, size_t size)
{
    if (!T7Lua_DvarString("ui_mapname", out, size) || !out[0] || strlen(out) >= kT7ShareName)
    {
        out[0] = 0;
        return false;
    }

    for (char* c = out; *c; ++c)
    {
        if (*c >= 'A' && *c <= 'Z')
            *c = (char)(*c + 32);

        if (!((*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9') || *c == '_'))
        {
            out[0] = 0;
            return false;
        }
    }

    return true;
}

static bool Usable(uint32_t address)
{
    const uint8_t* const b = (const uint8_t*)&address;
    return Private(address) && b[3] != 0 && b[3] != 255;
}

static int HostAddresses(const uint8_t* blob, uint32_t* out)
{
    int count = 0;

    for (int pass = 0; pass < 2 && count == 0; ++pass)
    {
        const size_t step = pass == 0 ? kHostSlotSize : 1;

        for (size_t at = 0; at + sizeof(uint32_t) <= kHostBlobSize && count < kMaxHosts; at += step)
        {
            uint32_t address;
            memcpy(&address, blob + at, sizeof(address));

            bool known = !Usable(address);

            for (int i = 0; i < count && !known; ++i)
                known = out[i] == address;

            if (!known)
                out[count++] = address;
        }
    }

    return count;
}

static void LogHost(int32_t type, const uint8_t* handle, const uint8_t* blob, const Role& role)
{
    char raw[kHostBlobSize * 2 + 1];
    char found[kMaxHosts * 16 + 8] = "none";
    size_t used = 0;

    for (size_t i = 0; i < kHostBlobSize; ++i)
        snprintf(raw + i * 2, 3, "%02X", blob[i]);

    for (int i = 0; i < role.hostCount; ++i)
    {
        const uint8_t* const b = (const uint8_t*)&role.hosts[i];
        used += (size_t)snprintf(found + used, sizeof(found) - used, "%s%u.%u.%u.%u", i ? ", " : "", b[0], b[1], b[2],
                                 b[3]);
    }

    T7Log_Write("[Share] LAN game lobby: host address type %d, handle %u.%u.%u.%u, LAN address %s, raw %s", type,
                handle[0], handle[1], handle[2], handle[3], found, raw);
}

static Role ReadRole()
{
    Role role{};

    if (!g_ready)
        return role;

    role.lan = *(const int32_t*)(g_base + kNetworkMode) == kNetworkLan;

    const uintptr_t session = g_base + kClientSessions + kClientSessionSize * kGameLobby;

    if (!role.lan || *(const int32_t*)(session + kSessionActive) <= 0)
    {
        g_loggedHost = 0;
        return role;
    }

    const int32_t type = *(const int32_t*)(session + kSessionHostType);
    const uint8_t* const blob = (const uint8_t*)(session + kSessionHostBlob);

    role.host = type == kAddressLoopback;

    if (type == kAddressIp)
    {
        role.hostCount = HostAddresses(blob, role.hosts);
        role.hostIp = role.hostCount > 0 ? role.hosts[0] : 0;
        role.client = role.hostCount > 0;
    }

    uint32_t mark = 2166136261u ^ (uint32_t)type;

    for (size_t i = 0; i < kHostBlobSize; ++i)
        mark = (mark ^ blob[i]) * 16777619u;

    if (mark != g_loggedHost)
    {
        g_loggedHost = mark;
        LogHost(type, (const uint8_t*)(session + kSessionHostAddress), blob, role);
    }

    return role;
}

static void SetWait(OrbisNetId socket, int microseconds)
{
    sceNetSetsockopt(socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_RCVTIMEO, &microseconds, sizeof(microseconds));
    sceNetSetsockopt(socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_SNDTIMEO, &microseconds, sizeof(microseconds));
}

static void SetBlocking(OrbisNetId socket, bool blocking)
{
    int value = blocking ? 0 : 1;
    sceNetSetsockopt(socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &value, sizeof(value));
}

static void Tune(OrbisNetId socket)
{
    int on = 1;
    sceNetSetsockopt(socket, SCE_NET_IPPROTO_TCP, SCE_NET_TCP_NODELAY, &on, sizeof(on));

    const int options[] = { SCE_NET_SO_SNDBUF, SCE_NET_SO_RCVBUF };

    for (const int option : options)
    {
        for (int size : kSocketBuffers)
        {
            if (sceNetSetsockopt(socket, SCE_NET_SOL_SOCKET, option, &size, sizeof(size)) >= 0)
                break;
        }
    }
}

static bool Waited(int result)
{
    return result == SCE_NET_ERROR_EWOULDBLOCK || result == SCE_NET_ERROR_ETIMEDOUT || result == SCE_NET_ERROR_EINTR;
}

static int Receive(Stream& stream, void* out, int size)
{
    for (int quiet = 0; quiet < kQuietRounds;)
    {
        if (stream.stop && stream.stop->load())
            return -1;

        const int got = sceNetRecv(stream.socket, out, (size_t)size, 0);

        if (got >= 0)
            return got;

        if (!Waited(got))
            return -1;

        ++quiet;
    }

    return -1;
}

static bool ReadLine(Stream& stream, char* out, size_t size)
{
    size_t used = 0;

    for (;;)
    {
        if (stream.at >= stream.have)
        {
            const int got = Receive(stream, stream.held, (int)sizeof(stream.held));

            if (got <= 0)
                return false;

            stream.have = got;
            stream.at = 0;
        }

        const char c = stream.held[stream.at++];

        if (c == '\n')
        {
            out[used] = 0;
            return true;
        }

        if (c == '\r')
            continue;

        if (used + 1 >= size)
            return false;

        out[used++] = c;
    }
}

static int ReadBody(Stream& stream, void* out, int size)
{
    if (stream.at < stream.have)
    {
        const int held = stream.have - stream.at;
        const int taken = held < size ? held : size;
        memcpy(out, stream.held + stream.at, (size_t)taken);
        stream.at += taken;
        return taken;
    }

    return Receive(stream, out, size);
}

static bool SendAll(Stream& stream, const void* data, size_t size)
{
    const char* at = (const char*)data;

    for (int quiet = 0; size > 0;)
    {
        if (stream.stop && stream.stop->load())
            return false;

        const int sent = sceNetSend(stream.socket, at, size, 0);

        if (sent > 0)
        {
            at += sent;
            size -= (size_t)sent;
            quiet = 0;
            continue;
        }

        if (!Waited(sent) || ++quiet >= kQuietRounds)
            return false;
    }

    return true;
}

static bool SendText(Stream& stream, const char* format, ...)
{
    char text[512];
    va_list args;
    va_start(args, format);
    const int n = vsnprintf(text, sizeof(text), format, args);
    va_end(args);

    return n > 0 && (size_t)n < sizeof(text) && SendAll(stream, text, (size_t)n);
}

static void Walk(const char* root, const char* rel, int depth, int kind, Served* out, int* count)
{
    char folder[kT7SharePath + kRelMax + 2];

    if (rel[0])
        snprintf(folder, sizeof(folder), "%s/%s", root, rel);
    else
        snprintf(folder, sizeof(folder), "%s", root);

    const int fd = sceKernelOpen(folder, O_RDONLY | O_DIRECTORY, 0);

    if (fd < 0)
        return;

    constexpr int kListing = 0x8000;
    char* const listing = (char*)malloc(kListing);

    while (listing)
    {
        const int n = sceKernelGetdents(fd, listing, kListing);

        if (n <= 0)
            break;

        for (int at = 0; at + 8 <= n;)
        {
            const struct dirent* const entry = (const struct dirent*)(listing + at);

            if (entry->d_reclen == 0)
                break;

            at += entry->d_reclen;

            if (entry->d_fileno == 0 || !SafeName(entry->d_name, strlen(entry->d_name)))
                continue;

            char child[kRelMax];
            const int length = rel[0] ? snprintf(child, sizeof(child), "%s/%s", rel, entry->d_name)
                                      : snprintf(child, sizeof(child), "%s", entry->d_name);

            if (length <= 0 || (size_t)length >= sizeof(child))
                continue;

            if (entry->d_type == DT_DIR)
            {
                if (depth < kWalkDepth)
                    Walk(root, child, depth + 1, kind, out, count);

                continue;
            }

            if (EndsWith(child, ".part") || *count >= kMaxEntries)
                continue;

            Served& file = out[*count];
            OrbisKernelStat st;
            snprintf(file.path, sizeof(file.path), "%s/%s", root, child);

            if (sceKernelStat(file.path, &st) != 0 || S_ISDIR(st.st_mode) || st.st_size < 0)
                continue;

            file.size = (uint64_t)st.st_size;
            file.kind = kind;
            snprintf(file.rel, sizeof(file.rel), "%s", child);
            ++*count;
        }
    }

    free(listing);
    sceKernelClose(fd);
}

static void Throttle(Pace& pace, size_t sent)
{
    if (!g_hostInLevel.load(std::memory_order_relaxed))
    {
        pace.since = 0;
        return;
    }

    const uint64_t now = sceKernelGetProcessTime();

    if (!pace.since || now - pace.since > 1000000)
    {
        pace.since = now;
        pace.bytes = 0;
    }

    pace.bytes += sent;

    const uint64_t due = pace.bytes * 1000000 / kMatchBytesPerSecond;
    const uint64_t spent = now - pace.since;

    if (due > spent)
        sceKernelUsleep((unsigned int)(due - spent));
}

static bool SendFile(Stream& stream, const Served& file, uint64_t offset, char* buffer, Pace& pace, uint64_t* total)
{
    const int fd = sceKernelOpen(file.path, O_RDONLY, 0);
    OrbisKernelStat st;

    if (fd < 0 || sceKernelFstat(fd, &st) != 0 || (uint64_t)st.st_size != file.size || offset > file.size)
    {
        if (fd >= 0)
            sceKernelClose(fd);

        SendText(stream, "ERR that file changed\n");
        return false;
    }

    bool sent = SendText(stream, "DATA %llu\n", (unsigned long long)(file.size - offset));

    while (sent && offset < file.size)
    {
        const uint64_t left = file.size - offset;
        const size_t want = left < kChunk ? (size_t)left : kChunk;
        const int64_t got = sceKernelPread(fd, buffer, want, (off_t)offset);

        if (got <= 0 || !SendAll(stream, buffer, (size_t)got))
        {
            sent = false;
            break;
        }

        offset += (uint64_t)got;
        *total += (uint64_t)got;
        Throttle(pace, (size_t)got);
    }

    sceKernelClose(fd);
    return sent;
}

static void Answer(Stream& stream, Served* files, char* buffer)
{
    const uint32_t version = g_hostedVersion.load();
    g_hostedWanted.store(true);

    for (uint64_t waited = 0; waited < kFreshListUs && g_hostedVersion.load() == version; waited += 10000)
        sceKernelUsleep(10000);

    Hosted hosted;

    {
        Spin lock(g_hostedLock);
        hosted = g_hosted;
    }

    if (!g_serve.load() || !hosted.serving)
    {
        SendText(stream, "BO3C 1 NO\n");
        return;
    }

    int count = 0;

    if (hosted.custom)
        Walk(hosted.mapRoot, "", 0, -1, files, &count);

    for (int k = 0; k < hosted.modCount; ++k)
        Walk(hosted.modRoots[k], "", 0, k, files, &count);

    bool sent = SendText(stream, "BO3C 1 OK\n") &&
                SendText(stream, "map %s %d\n", hosted.map[0] ? hosted.map : "-", hosted.custom ? 1 : 0);

    for (int k = 0; sent && k < hosted.modCount; ++k)
        sent = SendText(stream, "mod %s\n", hosted.mods[k]);

    for (int i = 0; sent && i < count; ++i)
        sent = SendText(stream, "file %d %d %llu %s\n", i, files[i].kind, (unsigned long long)files[i].size,
                        files[i].rel);

    sent = sent && SendText(stream, "end\n");

    T7Log_Write("[Share] sent the list for %s: %d file(s), %d mod(s)", hosted.map, count, hosted.modCount);

    Pace pace{};
    char line[256];
    uint64_t total = 0;
    int asked = 0;
    const uint64_t began = sceKernelGetProcessTime();

    while (sent && ReadLine(stream, line, sizeof(line)))
    {
        if (strncmp(line, "GET ", 4) != 0)
            break;

        ++asked;

        char* at = line + 4;
        const long id = strtol(at, &at, 10);
        const uint64_t offset = strtoull(at, &at, 10);

        if (!g_serve.load())
        {
            SendText(stream, "ERR the host stopped sharing\n");
            break;
        }

        if (id < 0 || id >= count)
        {
            SendText(stream, "ERR no such file\n");
            break;
        }

        sent = SendFile(stream, files[id], offset, buffer, pace, &total);
    }

    if (asked > 0)
    {
        const double seconds = (double)(sceKernelGetProcessTime() - began) / 1000000.0;
        T7Log_Write("[Share] sent %d file(s), %llu bytes in %.1f s (%.2f MB/s)%s", asked, (unsigned long long)total, seconds,
                    seconds > 0.0 ? (double)total / seconds / 1048576.0 : 0.0, sent ? "" : ", then the link dropped");
    }
}

static void* Serve(void* argument)
{
    Stream* const stream = (Stream*)malloc(sizeof(Stream));
    Served* const files = (Served*)malloc(sizeof(Served) * kMaxEntries);
    char* const buffer = (char*)malloc(kChunk);
    const OrbisNetId socket = (OrbisNetId)(intptr_t)argument;

    if (stream && files && buffer)
    {
        memset(stream, 0, sizeof(*stream));
        stream->socket = socket;

        char line[64];

        if (ReadLine(*stream, line, sizeof(line)) && strcmp(line, "BO3C 1 LIST") == 0)
            Answer(*stream, files, buffer);
    }

    free(stream);
    free(files);
    free(buffer);
    sceNetSocketClose(socket);
    g_connections.fetch_sub(1);
    return nullptr;
}

static OrbisNetId Listen()
{
    const OrbisNetId socket = sceNetSocket("bo3c_share", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);

    if (socket < 0)
        return -1;

    int on = 1;
    sceNetSetsockopt(socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_REUSEADDR, &on, sizeof(on));
    Tune(socket);

    OrbisNetSockaddrIn address{};
    address.sin_len = sizeof(address);
    address.sin_family = SCE_NET_AF_INET;
    address.sin_port = sceNetHtons(kPort);
    address.sin_addr.s_addr = SCE_NET_INADDR_ANY;

    if (sceNetBind(socket, (const OrbisNetSockaddr*)&address, sizeof(address)) < 0 || sceNetListen(socket, 4) < 0)
    {
        sceNetSocketClose(socket);
        return -1;
    }

    SetBlocking(socket, false);
    return socket;
}

static void* Server(void*)
{
    OrbisNetId listener = -1;

    for (;;)
    {
        if (!g_serve.load())
        {
            if (listener >= 0)
            {
                sceNetSocketClose(listener);
                listener = -1;
                T7Log_Write("[Share] no longer hosting a LAN lobby: stopped listening");
            }

            sceKernelUsleep(500000);
            continue;
        }

        if (listener < 0)
        {
            listener = Listen();

            if (listener < 0)
            {
                sceKernelUsleep(3000000);
                continue;
            }

            T7Log_Write("[Share] hosting a LAN lobby: listening on port %d", (int)kPort);
        }

        OrbisNetSockaddrIn from{};
        OrbisNetSocklen_t length = sizeof(from);
        const OrbisNetId peer = sceNetAccept(listener, (OrbisNetSockaddr*)&from, &length);

        if (peer < 0)
        {
            sceKernelUsleep(100000);
            continue;
        }

        if (!Private(from.sin_addr.s_addr) || g_connections.load() >= kMaxConnections)
        {
            sceNetSocketClose(peer);
            continue;
        }

        SetBlocking(peer, true);
        SetWait(peer, kSocketWaitUs);
        g_connections.fetch_add(1);

        OrbisPthread thread;

        if (scePthreadCreate(&thread, nullptr, Serve, (void*)(intptr_t)peer, "BO3-Customs Share Send") != 0)
        {
            g_connections.fetch_sub(1);
            sceNetSocketClose(peer);
            continue;
        }

        scePthreadDetach(thread);
    }

    return nullptr;
}

static void UpdateHosted(const Role& role)
{
    g_hostInLevel.store(T7Maps_InLevel(), std::memory_order_relaxed);

    if (!role.lan || !role.host)
    {
        g_serve.store(false);
        return;
    }

    const uint64_t now = sceKernelGetProcessTime();

    if (g_serve.load() && !g_hostedWanted.exchange(false) && now - g_hostedAt < kSnapshotUs)
        return;

    g_hostedAt = now;

    static Hosted next;
    memset(&next, 0, sizeof(next));
    next.serving = true;
    ReadMapName(next.map, sizeof(next.map));
    next.custom = next.map[0] && T7Maps_ShareMap(next.map, next.mapRoot, sizeof(next.mapRoot));
    next.modCount = T7Maps_ShareMods(next.mods, next.modRoots, kT7ShareMods);

    {
        Spin lock(g_hostedLock);
        g_hosted = next;
    }

    g_hostedVersion.fetch_add(1);
    g_serve.store(true);
}

static bool Fail(const char* format, ...)
{
    char text[160];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);

    {
        Spin lock(g_textLock);
        snprintf(g_session.error, sizeof(g_session.error), "%s", text);
    }

    g_failedAt.store(g_stop.load() ? 0 : sceKernelGetProcessTime());
    g_phase.store(kFailed);
    T7Log_Write("[Share] %s: %s", g_session.map, text);
    return false;
}

static OrbisNetId Connect(uint32_t address, uint64_t limit)
{
    const OrbisNetId socket = sceNetSocket("bo3c_fetch", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);

    if (socket < 0)
        return -1;

    Tune(socket);

    OrbisNetSockaddrIn host{};
    host.sin_len = sizeof(host);
    host.sin_family = SCE_NET_AF_INET;
    host.sin_port = sceNetHtons(kPort);
    host.sin_addr.s_addr = address;

    SetBlocking(socket, false);

    bool connected = false;
    bool refused = false;

    for (uint64_t waited = 0; !connected && !refused && waited < limit && !g_stop.load(); waited += kConnectStepUs)
    {
        int problem = 0;
        OrbisNetSocklen_t size = sizeof(problem);

        if (waited && sceNetGetsockopt(socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_ERROR, &problem, &size) >= 0 && problem != 0)
        {
            refused = true;
            break;
        }

        const int result = sceNetConnect(socket, (const OrbisNetSockaddr*)&host, sizeof(host));

        if (result >= 0 || result == SCE_NET_ERROR_EISCONN)
            connected = true;
        else if (result == SCE_NET_ERROR_EINPROGRESS || result == SCE_NET_ERROR_EALREADY || Waited(result))
            sceKernelUsleep((unsigned int)kConnectStepUs);
        else
            refused = true;
    }

    if (!connected)
    {
        sceNetSocketClose(socket);
        return -1;
    }

    SetBlocking(socket, true);
    SetWait(socket, kSocketWaitUs);
    return socket;
}

static void MakeParents(const char* path)
{
    char folder[kPathMax];
    snprintf(folder, sizeof(folder), "%s", path);

    for (char* at = folder + 1; *at; ++at)
    {
        if (*at != '/')
            continue;

        *at = 0;
        sceKernelMkdir(folder, 0777);
        *at = '/';
    }
}

static bool Writable(const char* root, const char* kind)
{
    char path[kPathMax];
    snprintf(path, sizeof(path), "%s/%s/bo3customs-write-test.part", root, kind);
    MakeParents(path);

    const int fd = sceKernelOpen(path, O_WRONLY | O_CREAT | O_TRUNC, 0777);

    if (fd < 0)
        return false;

    sceKernelClose(fd);
    sceKernelUnlink(path);
    return true;
}

static const char* FileName(const char* rel)
{
    const char* const slash = strrchr(rel, '/');
    return slash ? slash + 1 : rel;
}

static bool Marker(const Entry& entry)
{
    const char* const name = FileName(entry.rel);

    if (entry.kind < 0)
    {
        char zone[kT7ShareName + 4];
        snprintf(zone, sizeof(zone), "%s.ff", g_session.map);
        return EqualsNoCase(name, zone);
    }

    return EqualsNoCase(name, "core_mod.ff") || EqualsNoCase(name, "zm_mod.ff") || EqualsNoCase(name, "mp_mod.ff") ||
           EqualsNoCase(name, "cp_mod.ff");
}

static bool Fetch(Stream& stream, int id, const Entry& entry, const char* target, char* buffer)
{
    OrbisKernelStat st;

    if (sceKernelStat(target, &st) == 0 && (uint64_t)st.st_size == entry.size)
    {
        g_done.fetch_add(entry.size);
        return true;
    }

    char part[kPathMax];

    if ((size_t)snprintf(part, sizeof(part), "%s.part", target) >= sizeof(part))
        return Fail("the path of %s is too long", entry.rel);

    uint64_t offset = 0;

    if (sceKernelStat(part, &st) == 0)
    {
        if (st.st_size > 0 && (uint64_t)st.st_size <= entry.size)
            offset = (uint64_t)st.st_size;
        else
            sceKernelUnlink(part);
    }

    MakeParents(target);

    const int fd = sceKernelOpen(part, O_WRONLY | O_CREAT |
                                           (offset ? O_APPEND : O_TRUNC), 0777);

    if (fd < 0)
        return Fail("this console could not write %s", entry.rel);

    g_done.fetch_add(offset);

    const char* problem = nullptr;

    if (offset < entry.size)
    {
        char line[128];
        uint64_t left = 0;

        if (!SendText(stream, "GET %d %llu\n", id, (unsigned long long)offset) || !ReadLine(stream, line, sizeof(line)))
            problem = "the host stopped answering";
        else if (strncmp(line, "DATA ", 5) != 0 || (left = strtoull(line + 5, nullptr, 10)) != entry.size - offset)
            problem = "the host could not send a file";

        while (!problem && left > 0)
        {
            const int want = left < kChunk ? (int)left : (int)kChunk;
            const int got = ReadBody(stream, buffer, want);

            if (got <= 0)
            {
                problem = "the host stopped sending";
                break;
            }

            for (int written = 0; written < got;)
            {
                const int64_t n = sceKernelWrite(fd, buffer + written, (size_t)(got - written));

                if (n <= 0)
                {
                    problem = "this console ran out of room";
                    break;
                }

                written += (int)n;
            }

            left -= (uint64_t)got;
            g_done.fetch_add((uint64_t)got);
        }
    }

    sceKernelFsync(fd);
    sceKernelClose(fd);

    if (problem)
        return Fail("%s", g_stop.load() ? "the download was cancelled" : problem);

    sceKernelUnlink(target);

    if (sceKernelRename(part, target) < 0)
        return Fail("this console could not save %s", entry.rel);

    return true;
}

static bool ReadList(Stream& stream, Entry* entries, int* count, char* hostMap, bool* custom,
                     char mods[][kT7ShareName], int* modCount)
{
    char line[512];

    if (!SendText(stream, "BO3C 1 LIST\n") || !ReadLine(stream, line, sizeof(line)))
        return Fail("the host did not answer");

    if (strcmp(line, "BO3C 1 OK") != 0)
        return Fail("the host is not sharing its files right now");

    while (ReadLine(stream, line, sizeof(line)))
    {
        if (strcmp(line, "end") == 0)
            return true;

        if (strncmp(line, "map ", 4) == 0)
        {
            char* const space = strchr(line + 4, ' ');

            if (space)
            {
                *space = 0;
                *custom = space[1] == '1';
            }

            snprintf(hostMap, kT7ShareName, "%s", line + 4);
        }
        else if (strncmp(line, "mod ", 4) == 0)
        {
            if (*modCount >= kT7ShareMods || !SafeFolder(line + 4))
                return Fail("the host sent a mod name this console cannot use");

            snprintf(mods[(*modCount)++], kT7ShareName, "%s", line + 4);
        }
        else if (strncmp(line, "file ", 5) == 0)
        {
            char* at = line + 5;
            const long id = strtol(at, &at, 10);
            const long kind = strtol(at, &at, 10);
            const uint64_t size = strtoull(at, &at, 10);

            while (*at == ' ')
                ++at;

            if (id != *count || *count >= kMaxEntries || kind < -1 || kind >= *modCount || !SafeRel(at))
                return Fail("the host sent a file list this console cannot use");

            Entry& entry = entries[(*count)++];
            entry.size = size;
            entry.kind = (int32_t)kind;
            entry.wanted = false;
            snprintf(entry.rel, sizeof(entry.rel), "%s", at);
        }
    }

    return Fail("%s", g_stop.load() ? "the download was cancelled" : "the host's file list was cut off");
}

static bool Exchange(Stream& stream, Entry* entries, char* buffer, bool lastTry)
{
    Session& session = g_session;
    char hostMap[kT7ShareName] = "";
    char mods[kT7ShareMods][kT7ShareName];
    bool custom = false;
    int count = 0;
    int modCount = 0;

    g_phase.store(kListing);

    if (!ReadList(stream, entries, &count, hostMap, &custom, mods, &modCount))
        return false;

    if (session.needMap && !EqualsNoCase(hostMap, session.map))
    {
        if (lastTry)
        {
            Fail("the host is on another map");
            g_failedAt.store(0);
        }

        return !lastTry;
    }

    if (session.needMap && !custom)
    {
        T7Log_Write("[Share] %s is not a custom map on the host, so there is nothing to download for it", hostMap);

        Spin lock(g_textLock);
        session.needMap = false;
    }

    bool missing[kT7ShareMods];
    int newMods = 0;

    for (int k = 0; k < modCount; ++k)
    {
        missing[k] = !T7Maps_HasMod(mods[k]);
        newMods += missing[k] ? 1 : 0;
    }

    uint64_t total = 0;
    int files = 0;

    for (int i = 0; i < count; ++i)
    {
        Entry& entry = entries[i];
        entry.wanted = entry.kind < 0 ? session.needMap : missing[entry.kind];

        if (entry.wanted)
        {
            total += entry.size;
            ++files;
        }
    }

    {
        Spin lock(g_textLock);
        session.modCount = modCount;
        session.newMods = newMods;
        session.listed = true;
        session.fetched = files > 0;

        for (int k = 0; k < modCount; ++k)
            snprintf(session.mods[k], kT7ShareName, "%s", mods[k]);
    }

    T7Log_Write("[Share] the host runs %s with %d mod(s); %d file(s) to download (%llu bytes)", hostMap, modCount, files,
                (unsigned long long)total);

    if (files > 0)
    {
        char mapsRoot[kT7SharePath];
        char modsRoot[kT7SharePath];
        T7Maps_ShareRoot(false, mapsRoot, sizeof(mapsRoot));
        T7Maps_ShareRoot(true, modsRoot, sizeof(modsRoot));

        if (session.needMap && !Writable(mapsRoot, "usermaps"))
            snprintf(mapsRoot, sizeof(mapsRoot), "%s", "/data/BO3-Customs");

        if (newMods > 0 && !Writable(modsRoot, "mods"))
            snprintf(modsRoot, sizeof(modsRoot), "%s", "/data/BO3-Customs");

        g_total.store(total);
        g_done.store(0);
        g_fileCount.store(files);
        g_fileAt.store(0);
        g_phase.store(kDownloading);

        const uint64_t began = sceKernelGetProcessTime();

        T7Log_Write("[Share] maps go to %s, mods go to %s", mapsRoot, modsRoot);
        Notify("BO3 Customs\nDownloading %s from the host", session.needMap ? session.map : "the host's mods");

        for (int pass = 0; pass < 2; ++pass)
        {
            for (int i = 0; i < count; ++i)
            {
                const Entry& entry = entries[i];

                if (!entry.wanted || Marker(entry) != (pass == 1))
                    continue;

                if (g_stop.load())
                {
                    Fail("the download was cancelled");
                    return false;
                }

                const int number = g_fileAt.fetch_add(1) + 1;

                {
                    Spin lock(g_textLock);
                    snprintf(session.current, sizeof(session.current), "%s", FileName(entry.rel));
                }

                T7Log_Write("[Share] file %d of %d: %s (%llu bytes)", number, files, entry.rel,
                            (unsigned long long)entry.size);

                char target[kPathMax];

                if (entry.kind < 0)
                    snprintf(target, sizeof(target), "%s/usermaps/%s/%s", mapsRoot, session.map, entry.rel);
                else
                    snprintf(target, sizeof(target), "%s/mods/%s/%s", modsRoot, mods[entry.kind], entry.rel);

                if (!Fetch(stream, i, entry, target, buffer))
                    return false;
            }
        }

        const double seconds = (double)(sceKernelGetProcessTime() - began) / 1000000.0;
        T7Log_Write("[Share] %d file(s) are here: %llu bytes in %.1f s (%.2f MB/s)", files, (unsigned long long)total,
                    seconds, seconds > 0.0 ? (double)total / seconds / 1048576.0 : 0.0);
    }

    SendText(stream, "BYE\n");
    g_phase.store(kInstalling);
    return false;
}

static OrbisNetId Reach(Session& session)
{
    const uint64_t limit = session.hostCount > 1 ? kConnectManyUs : kConnectWaitUs;

    for (int i = -1; i < session.hostCount && !g_stop.load(); ++i)
    {
        const uint32_t address = i < 0 ? session.reached : session.hosts[i];

        if (!address || (i >= 0 && address == session.reached))
            continue;

        const OrbisNetId socket = Connect(address, limit);
        const uint8_t* const b = (const uint8_t*)&address;

        if (socket >= 0)
        {
            if (session.reached != address)
                T7Log_Write("[Share] the host's file share answered at %u.%u.%u.%u", b[0], b[1], b[2], b[3]);

            session.reached = address;
            return socket;
        }

        T7Log_Write("[Share] no answer from %u.%u.%u.%u port %d", b[0], b[1], b[2], b[3], (int)kPort);
    }

    return -1;
}

static void* Fetcher(void*)
{
    Session& session = g_session;
    Stream* const stream = (Stream*)malloc(sizeof(Stream));
    Entry* const entries = (Entry*)malloc(sizeof(Entry) * kMaxEntries);
    char* const buffer = (char*)malloc(kChunk);

    const bool deaf = !session.needMap && g_deafHost.load() == session.hostIp &&
                      sceKernelGetProcessTime() - g_deafSince.load() < kDeafHostUs;

    if (deaf)
    {
        Fail("the host did not answer a moment ago");
    }
    else if (stream && entries && buffer)
    {
        for (int attempt = 1; attempt <= kListTries; ++attempt)
        {
            memset(stream, 0, sizeof(*stream));
            stream->stop = &g_stop;
            stream->socket = Reach(session);

            if (stream->socket < 0)
            {
                g_deafHost.store(session.hostIp);
                g_deafSince.store(sceKernelGetProcessTime());
                Fail("this console could not reach the host's file share");
                break;
            }

            g_deafHost.store(0);

            const bool again = Exchange(*stream, entries, buffer, attempt == kListTries);
            sceNetSocketClose(stream->socket);

            if (!again)
                break;

            sceKernelUsleep((unsigned int)kListRetryUs);

            if (g_stop.load())
            {
                Fail("the download was cancelled");
                break;
            }
        }
    }
    else
    {
        Fail("this console is out of memory");
    }

    free(stream);
    free(entries);
    free(buffer);

    if (g_phase.load() == kFailed && session.needMap && !g_stop.load() && g_failedAt.load() != 0)
        Notify("BO3 Customs\nCould not download %s: %s", session.map, session.error);

    g_busy.store(false);
    return nullptr;
}

static void Start(const Role& role, const char* map)
{
    Session& session = g_session;

    {
        Spin lock(g_textLock);
        memset(&session, 0, sizeof(session));
        session.active = true;
        session.hostIp = role.hostIp;
        session.hostCount = role.hostCount;
        memcpy(session.hosts, role.hosts, sizeof(session.hosts));
        snprintf(session.map, sizeof(session.map), "%s", map);
        session.needMap = !T7Maps_HasMap(map) && !T7Maps_EngineHasMap(map);
    }

    g_total.store(0);
    g_done.store(0);
    g_fileAt.store(0);
    g_fileCount.store(0);
    g_stop.store(false);
    g_phase.store(kContacting);
    g_busy.store(true);

    const uint8_t* const b = (const uint8_t*)&role.hostIp;
    T7Log_Write("[Share] joined %u.%u.%u.%u on %s (%s)", b[0], b[1], b[2], b[3], map,
                session.needMap ? "this console does not have that map" : "the map is here");

    OrbisPthread thread;

    if (scePthreadCreate(&thread, nullptr, Fetcher, nullptr, "BO3-Customs Share Fetch") != 0)
    {
        g_busy.store(false);
        Fail("the download could not start");
        return;
    }

    scePthreadDetach(thread);
}

static void Finish()
{
    Session& session = g_session;

    if (session.fetched)
    {
        if (!T7Maps_Refresh())
            return;

        g_mapsChanged.store(true);
    }

    if (session.needMap && !T7Maps_HasMap(session.map))
    {
        Fail("the files arrived but the map is not complete");
        Notify("BO3 Customs\nCould not install %s", session.map);
        return;
    }

    if (session.listed)
    {
        T7Maps_SetSessionMods(session.mods, session.modCount);
        g_overriding = true;
    }

    g_phase.store(kReady);

    if (session.fetched)
        Notify("BO3 Customs\nDownloaded %s from the host", session.needMap ? session.map : "the host's mods");
}

static void KeepJoinAlive(uint64_t now)
{
    if (!g_keptAt)
    {
        g_keptAt = now;
        g_keptRest = 0;
    }

    const uint64_t passed = now - g_keptAt + g_keptRest;
    const int32_t whole = (int32_t)(passed / 1000);

    g_keptRest = passed % 1000;
    g_keptAt = now;

    for (size_t lobby = 0; whole > 0 && lobby < 2; ++lobby)
    {
        const uintptr_t session = g_base + kClientSessions + kClientSessionSize * lobby;

        if (*(const int32_t*)(session + kSessionActive) > 0 && *(const int32_t*)(session + kSessionHostType) == kAddressIp)
            *(int32_t*)(session + kSessionHostHeard) += whole;
    }

    uintptr_t task = *(const uintptr_t*)(g_base + kTaskList);

    for (int walked = 0; task > 0x10000 && walked < kTaskWalk; ++walked)
    {
        if (*(const uintptr_t*)(task + kTaskDefinition) == g_base + kLaunchTask &&
            *(const int32_t*)(task + kTaskTimeout) != -1)
        {
            *(int32_t*)(task + kTaskTimeout) = -1;
            T7Log_Write("[Share] the launch step now waits as long as the download takes");
        }

        task = *(const uintptr_t*)task;
    }
}

static bool Evaluate(bool frame)
{
    bool taken = false;

    if (!g_evaluating.compare_exchange_strong(taken, true))
        return g_lastHold.load();

    const Role role = ReadRole();

    if (frame)
        UpdateHosted(role);

    char map[kT7ShareName] = "";
    const bool client = role.client && ReadMapName(map, sizeof(map));
    Session& session = g_session;
    bool hold = false;

    if (!client)
    {
        const uint64_t now = sceKernelGetProcessTime();

        if (!g_awaySince)
            g_awaySince = now;

        if (now - g_awaySince > kAwayUs)
        {
            if (session.active)
            {
                session.active = false;
                g_stop.store(true);
                T7Log_Write("[Share] left the host's lobby");
            }

            if (g_overriding)
            {
                g_overriding = false;
                T7Maps_SetSessionMods(nullptr, -1);
            }

            if (!g_busy.load() && g_phase.load() != kFailed)
                g_phase.store(kIdle);

            g_pendingSince = 0;
            g_heldAt = 0;
            g_keptAt = 0;
        }
    }
    else
    {
        const uint64_t now = sceKernelGetProcessTime();
        g_awaySince = 0;

        if (!session.active || session.hostIp != role.hostIp || strcmp(session.map, map) != 0)
        {
            if (!g_pendingSince || g_pendingHost != role.hostIp || strcmp(g_pendingMap, map) != 0)
            {
                g_pendingSince = now;
                g_pendingHost = role.hostIp;
                snprintf(g_pendingMap, sizeof(g_pendingMap), "%s", map);
            }

            if (g_busy.load())
            {
                g_stop.store(true);
            }
            else if (now - g_pendingSince >= kSettleUs)
            {
                g_pendingSince = 0;
                Start(role, map);
            }

            hold = true;
        }
        else
        {
            g_pendingSince = 0;

            if (frame && !g_busy.load() && g_phase.load() == kInstalling)
                Finish();

            const int phase = g_phase.load();
            hold = g_busy.load() || (phase != kReady && phase != kFailed);
        }

        if (frame)
        {
            if (hold)
                g_heldAt = now;

            if (hold || (g_heldAt && now - g_heldAt < kLaunchGraceUs && !T7Maps_InLevel()))
                KeepJoinAlive(now);
            else
                g_keptAt = 0;
        }
    }

    g_lastHold.store(hold);
    g_evaluating.store(false);
    return hold;
}

static void Size(char* out, size_t size, uint64_t bytes)
{
    const double megabytes = (double)bytes / (1024.0 * 1024.0);

    if (megabytes >= 1024.0)
        snprintf(out, size, "%.2f GB", megabytes / 1024.0);
    else if (megabytes >= 1.0)
        snprintf(out, size, "%.0f MB", megabytes);
    else
        snprintf(out, size, "%.0f KB", (double)bytes / 1024.0);
}

static bool ImportBound(const void* function)
{
    OrbisKernelVirtualQueryInfo info;

    if (sceKernelVirtualQuery((void*)&ImportBound, 0, &info, sizeof(info)) < 0)
        return true;

    const uintptr_t start = (uintptr_t)info.unk01;
    const uintptr_t end = (uintptr_t)info.unk02;
    const uintptr_t stub = (uintptr_t)function;

    if (stub < 0x10000)
        return false;

    if (stub < start || stub + 6 > end || ((const uint8_t*)stub)[0] != 0xFF || ((const uint8_t*)stub)[1] != 0x25)
        return true;

    const uintptr_t slot = stub + 6 + (intptr_t)*(const int32_t*)(stub + 2);

    if (!RangeReadable(slot, 8))
        return false;

    const uintptr_t target = *(const uintptr_t*)slot;
    return target >= 0x10000 && (target < start || target >= end);
}
}

void T7Share_Install(uintptr_t base)
{
    using namespace T7Share;

    static bool installed = false;

    if (installed || !base)
        return;

    installed = true;
    g_base = base;

    if (!RangeReadable(base + kNetworkMode, 4) || !RangeReadable(base + kClientSessions, kClientSessionSize * 2) ||
        !RangeReadable(base + kTaskList, sizeof(uintptr_t)))
    {
        T7Log_Write("[Share] off: the lobby data is not where this build expects it");
        return;
    }

    if (!ImportBound((const void*)&sceNetSocket) || !ImportBound((const void*)&sceNetConnect) ||
        !ImportBound((const void*)&sceNetAccept) || !ImportBound((const void*)&sceNetRecv) ||
        !ImportBound((const void*)&sceNetSend))
    {
        T7Log_Write("[Share] off: the network library is not loaded in this process");
        return;
    }

    OrbisPthread thread;

    if (scePthreadCreate(&thread, nullptr, Server, nullptr, "BO3-Customs Share") != 0)
    {
        T7Log_Write("[Share] off: the server thread did not start");
        return;
    }

    scePthreadDetach(thread);
    g_ready = true;
}

void T7Share_Tick()
{
    if (T7Share::g_ready)
        T7Share::Evaluate(true);
}

float T7Share_Query(const char* what)
{
    using namespace T7Share;

    if (!g_ready || !what)
        return 0.0f;

    if (strcmp(what, "hold") == 0)
        return g_wrapped.load() && Evaluate(false) ? 1.0f : 0.0f;

    if (strcmp(what, "wrapped") == 0)
    {
        if (!g_wrapped.exchange(true))
            T7Log_Write("[Share] the lobby's launch step now waits for downloads");

        return 1.0f;
    }

    if (strcmp(what, "plain") == 0)
        g_wrapped.store(false);

    return 0.0f;
}

bool T7Share_Wants(const char* map)
{
    using namespace T7Share;

    if (!g_ready || !map || !g_wrapped.load())
        return false;

    Evaluate(false);

    const Role role = ReadRole();

    if (!role.client || T7Maps_HasMap(map) || T7Maps_EngineHasMap(map))
        return false;

    const Session& session = g_session;

    if (session.active && session.hostIp == role.hostIp && EqualsNoCase(session.map, map))
        return session.needMap && g_phase.load() != kFailed;

    return true;
}

void T7Share_LobbyState(uintptr_t L)
{
    using namespace T7Share;

    if (!g_ready || !L || T7Lua_IsUiState(L) || !ReadRole().client)
        return;

    const uint64_t now = sceKernelGetProcessTime();

    if (g_wrapTriedAt && now - g_wrapTriedAt < kWrapEveryUs)
        return;

    g_wrapTriedAt = now;
    T7Lua_RunShareVm(L);
}

bool T7Share_Status(T7ShareStatus* status)
{
    using namespace T7Share;

    static uint64_t lastAt = 0;
    static uint64_t lastDone = 0;
    static double   speed = 0.0;

    memset(status, 0, sizeof(*status));

    if (!g_ready)
        return false;

    const int phase = g_phase.load();
    const uint64_t now = sceKernelGetProcessTime();

    Spin lock(g_textLock);
    const Session& session = g_session;

    if (phase != kDownloading)
    {
        lastAt = 0;
        speed = 0.0;
    }

    if (phase == kFailed)
    {
        const uint64_t at = g_failedAt.load();

        if ((!session.needMap && session.newMods == 0) || !at || now - at > kFailureShownUs)
            return false;

        snprintf(status->title, sizeof(status->title), "%s", "COULD NOT DOWNLOAD FROM THE HOST");
        snprintf(status->name, sizeof(status->name), "%s", session.needMap ? session.map : "the host's mods");
        snprintf(status->detail, sizeof(status->detail), "%s", session.error);
        status->failed = true;
        return true;
    }

    if (!session.active || phase == kIdle || phase == kReady)
        return false;

    if (!session.needMap && session.newMods == 0)
        return false;

    snprintf(status->title, sizeof(status->title), "%s", "DOWNLOADING FROM THE HOST");

    if (session.needMap && session.newMods > 0)
        snprintf(status->name, sizeof(status->name), "%s and %d mod%s", session.map, session.newMods,
                 session.newMods == 1 ? "" : "s");
    else if (session.needMap)
        snprintf(status->name, sizeof(status->name), "%s", session.map);
    else
        snprintf(status->name, sizeof(status->name), "%d mod%s", session.newMods, session.newMods == 1 ? "" : "s");

    if (phase == kContacting || phase == kListing)
    {
        status->name[0] = 0;
        snprintf(status->detail, sizeof(status->detail), "%s", "Asking the host for its files...");
        return true;
    }

    const uint64_t total = g_total.load();
    const uint64_t done = g_done.load();

    if (phase == kInstalling)
    {
        snprintf(status->detail, sizeof(status->detail), "%s", "Installing...");
        status->fraction = 1.0f;
        return true;
    }

    if (lastAt && now > lastAt && done >= lastDone)
    {
        const double instant = (double)(done - lastDone) * 1000000.0 / (double)(now - lastAt);
        speed = speed > 0.0 ? speed * 0.8 + instant * 0.2 : instant;
    }

    lastAt = now;
    lastDone = done;

    char have[24];
    char all[24];
    char rate[24];
    Size(have, sizeof(have), done);
    Size(all, sizeof(all), total);
    Size(rate, sizeof(rate), (uint64_t)speed);

    if (speed >= 1024.0 && total > done)
    {
        const uint64_t seconds = (uint64_t)((double)(total - done) / speed);

        if (seconds >= 90)
            snprintf(status->detail, sizeof(status->detail), "%s of %s  -  %s/s  -  about %d min left", have, all, rate,
                     (int)((seconds + 30) / 60));
        else
            snprintf(status->detail, sizeof(status->detail), "%s of %s  -  %s/s  -  about %d sec left", have, all, rate,
                     (int)seconds);
    }
    else
    {
        snprintf(status->detail, sizeof(status->detail), "%s of %s", have, all);
    }

    snprintf(status->file, sizeof(status->file), "File %d of %d: %s", g_fileAt.load(), g_fileCount.load(),
             session.current);
    status->fraction = total ? (float)((double)done / (double)total) : 0.0f;
    return true;
}

void T7Share_Cancel()
{
    using namespace T7Share;

    if (!g_ready || !g_session.active)
        return;

    const int phase = g_phase.load();

    if (phase == kReady || phase == kFailed || phase == kIdle)
        return;

    T7Log_Write("[Share] the player cancelled the download");
    g_stop.store(true);

    if (!g_busy.load())
        Fail("the download was cancelled");
}

bool T7Share_TakeMapsChanged()
{
    return T7Share::g_mapsChanged.exchange(false);
}
