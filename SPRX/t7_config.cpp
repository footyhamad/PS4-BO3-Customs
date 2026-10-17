#include "headers.hpp"
#include "t7_config.hpp"
#include "t7_log.hpp"

#include <atomic>
#include <fcntl.h>

namespace T7Config
{
static const char k_folder[]      = "/data/BO3-Customs";
static const char k_path[]        = "/data/BO3-Customs/config.cfg";
static const char k_temp[]        = "/data/BO3-Customs/config.cfg.tmp";
static const char k_oldBinds[]    = "/data/BO3-Customs/kbm.cfg";
static const char k_oldGraphics[] = "/data/BO3-Customs/graphics.cfg";

constexpr size_t kMaxText = 32768;

static std::atomic<bool> g_busy{false};
static bool              g_migrated = false;
static char              g_current[kMaxText];
static char              g_text[kMaxText];

struct Lock
{
    Lock()
    {
        while (g_busy.exchange(true, std::memory_order_acquire))
            sceKernelUsleep(100);
    }

    ~Lock()
    {
        g_busy.store(false, std::memory_order_release);
    }
};

static int64_t ReadAll(const char* path, char* out, size_t size)
{
    const int fd = sceKernelOpen(path, O_RDONLY, 0);

    if (fd < 0)
        return -1;

    size_t used = 0;

    while (used < size - 1)
    {
        const int64_t got = sceKernelRead(fd, out + used, size - 1 - used);

        if (got <= 0)
            break;

        used += (size_t)got;
    }

    sceKernelClose(fd);
    out[used] = 0;
    return (int64_t)used;
}

static bool WriteAll(const char* text, size_t length)
{
    sceKernelMkdir(k_folder, 0777);

    const int fd = sceKernelOpen(k_temp, O_WRONLY | O_CREAT | O_TRUNC, 0777);

    if (fd < 0)
        return false;

    const int64_t put = length ? sceKernelWrite(fd, text, length) : 0;
    sceKernelClose(fd);

    if (put != (int64_t)length || sceKernelRename(k_temp, k_path) < 0)
    {
        sceKernelUnlink(k_temp);
        return false;
    }

    return true;
}

static void Put(size_t* used, const char* data, size_t length)
{
    if (*used + length >= sizeof(g_text))
        return;

    memcpy(g_text + *used, data, length);
    *used += length;
    g_text[*used] = 0;
}

static void PutLine(size_t* used, const char* data, size_t length)
{
    Put(used, data, length);
    Put(used, "\n", 1);
}

static void PutBlock(size_t* used, const char* header, const char* lines)
{
    PutLine(used, header, strlen(header));

    const size_t length = strlen(lines);
    Put(used, lines, length);

    if (length && lines[length - 1] != '\n')
        Put(used, "\n", 1);
}

static void Migrate()
{
    if (g_migrated)
        return;

    g_migrated = true;

    OrbisKernelStat st;

    if (sceKernelStat(k_path, &st) == 0)
        return;

    static char binds[kMaxText / 2];
    static char graphics[4096];
    const int64_t bindsLength = ReadAll(k_oldBinds, binds, sizeof(binds));
    const int64_t graphicsLength = ReadAll(k_oldGraphics, graphics, sizeof(graphics));

    if (bindsLength < 0 && graphicsLength < 0)
        return;

    size_t used = 0;
    g_text[0] = 0;

    if (bindsLength > 0)
        PutBlock(&used, "// keyboard and mouse", binds);

    if (graphicsLength > 0)
    {
        PutLine(&used, "// graphics", 11);

        for (char* line = graphics; line && *line;)
        {
            char* const next = strchr(line, '\n');

            if (next)
                *next = 0;

            size_t length = strlen(line);

            while (length && (line[length - 1] == '\r' || line[length - 1] == ' '))
                line[--length] = 0;

            if (length && line[0] != '/' && strchr(line, ' '))
            {
                Put(&used, "set ", 4);
                PutLine(&used, line, length);
            }

            line = next ? next + 1 : nullptr;
        }
    }

    if (!WriteAll(g_text, used))
    {
        T7Log_Write("[Config] config.cfg could not be written; kbm.cfg and graphics.cfg stay");
        return;
    }

    if (bindsLength >= 0)
        sceKernelUnlink(k_oldBinds);

    if (graphicsLength >= 0)
        sceKernelUnlink(k_oldGraphics);

    T7Log_Write("[Config] kbm.cfg and graphics.cfg are now config.cfg");
}
}

bool T7Config_Read(char* out, size_t size)
{
    using namespace T7Config;

    Lock lock;
    Migrate();
    return ReadAll(k_path, out, size) >= 0;
}

bool T7Config_Save(const char* section, bool (*owns)(const char* line), const char* lines)
{
    using namespace T7Config;

    Lock lock;
    Migrate();

    if (ReadAll(k_path, g_current, sizeof(g_current)) < 0)
        g_current[0] = 0;

    char header[96];
    snprintf(header, sizeof(header), "// %s", section);

    size_t used = 0;
    bool placed = false;
    g_text[0] = 0;

    for (char* line = g_current; *line;)
    {
        char* const end = strchr(line, '\n');
        size_t length = end ? (size_t)(end - line) : strlen(line);

        if (end)
            *end = 0;

        while (length && line[length - 1] == '\r')
            line[--length] = 0;

        if (strcmp(line, header) == 0 || owns(line))
        {
            if (!placed)
            {
                PutBlock(&used, header, lines);
                placed = true;
            }
        }
        else
        {
            PutLine(&used, line, length);
        }

        if (!end)
            break;

        line = end + 1;
    }

    if (!placed)
        PutBlock(&used, header, lines);

    return WriteAll(g_text, used);
}
