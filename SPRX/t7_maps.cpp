#include "headers.hpp"
#include <dirent.h>
#ifndef DT_DIR
#define DT_DIR 4
#endif
#include "t7_maps.hpp"
#include "t7_mapimages.hpp"
#include "t7_lua.hpp"

#include <atomic>
#ifndef SCE_KERNEL_PRIO_FIFO_HIGHEST
#define SCE_KERNEL_PRIO_FIFO_HIGHEST 256
#endif

#ifdef _DEBUG
extern "C" {
    int sceKernelInstallExceptionHandler(int signum, void (*handler)(int, void*));
    int sceKernelRaiseException(OrbisPthread thread, int signum);
    int T7Unity_InstallExceptionHandler(int signum, void (*handler)(int, void*));
    int T7Unity_RaiseException(OrbisPthread thread, int signum);
}
#endif

namespace T7Maps
{
constexpr uintptr_t kFileOpen     = 0xF29960;
constexpr uintptr_t kMapExists    = 0xF10650;
constexpr uintptr_t kIsMapValid   = 0xDECEB0;
constexpr uintptr_t kDlcBitForMap = 0xDC5480;
constexpr uint32_t  kBaseContentBit = 2;
constexpr uintptr_t kMapContentGate = 0xD8FC8F;
constexpr uintptr_t kMapContentBase = 0xD8FCBF;
constexpr uint8_t   kMapContentReturn = 0x06;
constexpr uint8_t   kMapContentJump = 0x2C;
constexpr uintptr_t kMapTable      = 0xBC05500;
constexpr uintptr_t kMapTableCount = 0xBC054F4;
constexpr size_t    kMapTableStride = 16;
constexpr int32_t   kMapTableMax   = 2048;
constexpr uintptr_t kLoadXAssets  = 0x85B5C0;
constexpr uintptr_t kCodeTexture  = 0xBD3FD0;
constexpr uintptr_t kBindTexture  = 0xBDE8C0;
constexpr uintptr_t kImageBlack   = 0x73C8A90;
constexpr uintptr_t kImageClear   = 0x73C8AC0;
constexpr uintptr_t kBlockJob     = 0x7F9EA0;
constexpr uintptr_t kReadRing     = 0x57E79D8;
constexpr uintptr_t kZoneFolder   = 0xCD5CB60;
constexpr uintptr_t kSessionModes = 0xBC20268;

constexpr uintptr_t kGdtMapsTable = 0xE8CA90;
constexpr uintptr_t kPathRemap    = 0xF29E50;
constexpr uintptr_t kLinkXAsset   = 0x857A10;
constexpr uintptr_t kLinkXAssetEnd = 0x858234;
constexpr uintptr_t kZonePriority = 0x858DC0;
constexpr uintptr_t kPatchZones   = 0x866090;
constexpr uintptr_t kZoneRecords  = 0x5D45D70;
constexpr uintptr_t kZoneRecordCount = 0x5D45D68;
constexpr size_t    kZoneRecordStride = 176;
constexpr uintptr_t kXPakOpen     = 0x10EAD90;
constexpr uintptr_t kLoadXFile    = 0x7FD560;
constexpr uintptr_t kLoadLevelZones = 0xF0B560;
constexpr uintptr_t kRenderSync     = 0xF3C270;
constexpr uintptr_t kKeepAssetUse   = 0x35E7DF0;
constexpr uintptr_t kAssetHash      = 0x57E9710;
constexpr int       kAssetHashSize  = 152576;
constexpr uintptr_t kAssetEntries   = 0x587E710;
constexpr size_t    kAssetEntrySize = 32;
constexpr size_t    kAssetEntryInUse = 17;
constexpr size_t    kAssetEntryNext = 20;
constexpr uintptr_t kKillServer     = 0xF3FCB0;
constexpr uintptr_t kFrontendMovies = 0x15C2E80;
constexpr uintptr_t kPlayMovie      = 0x746640;
constexpr int32_t   kRawFileAsset   = 47;
constexpr uint32_t  kUnloadFlag     = 0x100;
constexpr uint64_t  kModLoadWaitUs  = 60000000;
constexpr uintptr_t kDemoStart    = 0x10E3A10;
constexpr uintptr_t kGetDvarString = 0x57CB50;
constexpr uintptr_t kScrAddString = 0x791870;
constexpr uintptr_t kScrVmState   = 0x3453C90;
constexpr uintptr_t kScriptStrings = 0x33C3848;
constexpr uint32_t  kFsGameHash   = 0x5BF97A95;
constexpr uintptr_t kPlayViewmodelFx = 0x5B8EF0;
constexpr uintptr_t kClientFxIndex = 0x54ACC0;
constexpr uintptr_t kClientFxDefs = 0x1B0B4E0;
constexpr uintptr_t kScrGetConstString = 0x78BD10;
constexpr uintptr_t kScrError     = 0x78BE10;
constexpr uintptr_t kScrErrorText = 0x3448A80;
constexpr uintptr_t kScrErrorBuffer = 0x35852B0;
constexpr int32_t   kClientFxSlots = 1024;
constexpr uintptr_t kTexturesLoaded = 0xA56D90;
constexpr uintptr_t kGpuFxQueue     = 0x8B5A40;
constexpr uintptr_t kScrVmTop       = kScrVmState + 0x20;
constexpr uintptr_t kStreamStarted  = 0x8FD66C0;
constexpr uintptr_t kStreamDone     = 0x8FD66BE;
constexpr uintptr_t kStreamPending  = 0xD32AB84;
constexpr uintptr_t kStreamOverride = 0xD3EA37C;
constexpr int32_t   kVarInteger     = 7;
constexpr uint64_t  kTextureWaitUs  = 180000000;
#ifdef _DEBUG
constexpr uintptr_t kStreamLevelLoad = 0x8FD66BD;
constexpr uintptr_t kImagePool       = 0x45863A0;
constexpr uint64_t  kImagePoolSize   = 45979;
constexpr uintptr_t kPartsRequired   = 0x8F17580;
constexpr uintptr_t kPartsWanted     = 0x8F22980;
constexpr uintptr_t kPartsInMemory   = 0x8F0C180;
constexpr int       kPartWords       = 5748;
constexpr uintptr_t kMeshRequired    = 0x8F44580;
constexpr uintptr_t kMeshWanted      = 0x8F50580;
constexpr uintptr_t kMeshInMemory    = 0x8F46580;
constexpr int       kMeshWords       = 1024;
constexpr uintptr_t kSoundLoaded     = 0xD4B147C;
constexpr uintptr_t kSoundSlots      = 0xD48BE60;
constexpr uintptr_t kSoundSlotSize   = 4720;
constexpr int       kSoundSlotCount  = 32;
constexpr uintptr_t kStreamBlobs     = 0x90396C0;
constexpr int       kStreamBlobCount = 350;
constexpr uintptr_t kBlobGenerations = 0x903E350;
constexpr uintptr_t kBlobTick        = 0x160F3B8;
constexpr uintptr_t kBlobMegabytes   = 0x903E370;
constexpr uintptr_t kStreamPool      = 0x8BFEA28;
constexpr uintptr_t kOpenMenuRow       = 0x6341CE0;
constexpr uintptr_t kCloseMenuRow      = 0x6341D00;
constexpr uintptr_t kFreezeControlsRow = 0x6341E60;
constexpr uintptr_t kUiVisibilityRow   = 0x63424E0;
constexpr uintptr_t kGetPlayersRow     = 0x6CF9450;
constexpr uintptr_t kScrGetInt         = 0x78FC30;
constexpr uintptr_t kGEntities         = 0x66EAFE0;
constexpr uintptr_t kGEntitySize       = 1264;
constexpr uintptr_t kGEntityClient     = 592;
constexpr uintptr_t kClientControls    = 94212;
constexpr uint32_t  kControlsFrozen    = 4;
constexpr uint32_t  kScriptWatchLogs   = 150;
constexpr uint64_t  kHeartbeatUs       = 2000000;
constexpr uint32_t  kHeartbeats        = 90;
constexpr uintptr_t kThreadHandles     = 0xC97ED00;
constexpr uintptr_t kCodeEnd           = 0x1520000;
constexpr int       kStallSignal       = 30;
constexpr int       kStallThreads      = 14;
constexpr uint64_t  kStallUs           = 4000000;
constexpr uint64_t  kStallGapUs        = 5000000;
constexpr uint32_t  kStallRounds       = 3;
constexpr uint32_t  kStallArmFrames    = 120;
constexpr uint64_t  kStallAnswerUs     = 300000;
constexpr int       kStallRawWords     = 64;
constexpr int       kStallStackWords   = 8192;
constexpr int       kStallFrames       = 32;
constexpr int       kStallReturns      = 24;
constexpr uintptr_t kMainReturn        = 0xDF;
constexpr uintptr_t kThreadReturn      = 0xF3C131;
constexpr uintptr_t kRootScanBytes     = 64 * 1024;
constexpr uintptr_t kFrameScanBytes    = 256 * 1024;
constexpr int       kWalkFrames        = 48;
constexpr uintptr_t kKernelSpan        = 32u << 20;
constexpr size_t    kReportBytes       = 256 * 1024;
constexpr uint32_t  kDvarReadsLogged   = 64;
constexpr uintptr_t kGpuFrameLabel     = 0x6E56A20;
constexpr uintptr_t kFrontendData      = 0x74FC850;
constexpr uintptr_t kFrameDataA        = 0x74FCB20;
constexpr uintptr_t kFrameDataB        = 0x7A62550;
constexpr uintptr_t kFrameLabelWanted  = 0xF6DE0;
constexpr uintptr_t kDispatch          = 0xBD9F80;
constexpr size_t    kContextCompute    = 6992;
constexpr size_t    kContextDeferred   = 15872;
constexpr size_t    kContextImmediate  = 15864;
constexpr size_t    kCommandsIssued    = 16;
constexpr size_t    kCommandsLabel     = 24;
constexpr uint32_t  kDispatchesKept    = 512;
#endif
constexpr uint64_t  kGateLogUs       = 5000000;
constexpr uint64_t  kGateLogSlowUs   = 20000000;

constexpr int32_t kLevelCommonFlag = 0x200;

constexpr int32_t  kScriptParseTree       = 54;
constexpr uint32_t kUsermapZoneFlags      = 0x100;
constexpr uint32_t kUsermapScriptPriority = 1200;

constexpr int    kMaxPackages   = 128;
constexpr int    kMaxFiles      = 1536;
constexpr int    kMaxZoneCall   = 32;
constexpr int    kMaxListed     = 96;
constexpr size_t kNameMax       = 56;
constexpr size_t kListedNameMax = 96;

static const char k_levelCommon[] = "zm_levelcommon";
static const char k_soundFolder[] = "/app0/zone/";

constexpr uintptr_t kZoneNames       = 0x5D268F0;
constexpr int       kZoneNameSlots   = 65;
constexpr size_t    kZoneNameStride  = 80;
constexpr size_t    kZoneStateAt     = 72;
constexpr int32_t   kModeCommonFlags = 0x40;
constexpr uint32_t  kModMarker       = 0x80000000u;
constexpr uint32_t  kLocalizedZone   = 0x8000000u;
constexpr uint32_t  kPoolFlagMask    = 0x7FFFFFFu;
constexpr uint32_t  kFirstPoolEnd    = 0x200u;
constexpr int32_t   kModZoneFlags    = (int32_t)(kModMarker | (uint32_t)kModeCommonFlags);
constexpr int32_t   kLoadWithoutFree = 0x40000000;
constexpr uint64_t  kModPriority     = 1150;
constexpr int       kModZoneKinds    = 4;
constexpr int       kMaxMods         = 48;
constexpr int       kMaxStack        = 6;
constexpr int       kMaxStackFiles   = 160;
constexpr uint32_t  kModRankShift    = 29;
constexpr uint32_t  kModRankMax      = 3;
constexpr int32_t   kKeyValuePairs   = 55;
constexpr int32_t   kBgCache         = 85;
constexpr int       kExpandedZones   = 32;
constexpr int       kMaxModListed    = 256;
constexpr int       kMaxScriptClaims = 512;
constexpr int       kMaxInterned     = 384;
constexpr size_t    kInternedMax     = 128;

static const char* const k_modZones[kModZoneKinds]   = { "core_mod", "zm_mod", "mp_mod", "cp_mod" };
static const char* const k_modeCommons[kModZoneKinds] = { "", "zm_common", "mp_common", "cp_common" };
static const char k_chosenModFile[] = "/data/BO3-Customs/active_mod.txt";

struct Package
{
    char name[kNameMax];
    char folder[224];
    char alternate[224];
    char preview[kListedNameMax];
    bool map;
};

struct Movie
{
    char name[kListedNameMax];
    char file[kListedNameMax];
    char folder[224];
    int package;
};

struct File
{
    char rel[128];
    char path[224];
    int package;
};

struct ZoneInfo
{
    const char* name;
    int32_t allocFlags;
    int32_t freeFlags;
    int32_t allocSlot;
    int32_t freeSlot;
    uint64_t buffer;
    uint64_t bufferSize;
};

struct Mod
{
    char name[kNameMax];
    char folder[224];
    char alternate[224];
    char title[96];
    char ugc[40];
    char internal[kNameMax];
    bool zones[kModZoneKinds];
};

using Hook_t = uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                            double, double, double, double, double, double, double, double);

static uintptr_t g_base = 0;
static Package   g_packages[kMaxPackages];
static int       g_packageCount = 0;
static File      g_files[kMaxFiles];
static int       g_fileCount = 0;
static Movie     g_movies[64];
static int       g_movieCount = 0;
static int       g_levelCommon = -1;
static bool      g_buildMatched = false;
static int       g_listedMaps = 0;
static bool      g_hooksInstalled = false;
static bool      g_rankLowered = false;

static Mod               g_mods[kMaxMods];
static int               g_modCount = 0;
struct StackMod
{
    Mod  mod;
    File files[kMaxStackFiles];
    int  fileCount;
    char zones[kModZoneKinds][24];
};

struct ScriptClaim
{
    char base[kInternedMax];
    int  owner;
};

static StackMod                g_stack[kMaxStack];
static ScriptClaim             g_scriptClaims[kMaxScriptClaims];
static int                     g_scriptClaimCount = 0;
static char                    g_interned[kMaxInterned][kInternedMax];
static int                     g_internedCount = 0;
static std::atomic<bool>       g_claimLock{false};
static int                     g_stackCount = 0;
static char                    g_chosen[kMaxStack][kNameMax];
static int                     g_chosenCount = 0;
static char                    g_loaded[kMaxStack][kNameMax];
static int                     g_loadedCount = 0;
static char                    g_sessionMods[kMaxStack][kNameMax];
static int                     g_sessionModCount = -1;
static std::atomic<uintptr_t>  g_loadingZone{0};
static std::atomic<uint64_t>   g_plainZoneThread{0};
static std::atomic<bool>       g_applyWanted{false};
static std::atomic<bool>       g_prefetchWanted{false};
static bool                    g_reloadHeld = false;
static bool                    g_reloadForced = false;
static bool                    g_refreshOnClose = false;
static int                     g_readyZones = -1;
static char                    g_unshown[kMaxStack][kNameMax];
static int                     g_unshownCount = 0;
static bool                    g_frontendReloadReady = false;
static bool                    g_quietReload = false;
static bool                    g_reloadCheck = false;
static uint64_t                g_reloadSince = 0;
static int                     g_waitFrom = 0;
static char                    g_uiMods[kMaxMods][kNameMax];
static int                     g_uiModCount = 0;
static char                    g_waitZones[32][kNameMax];
static int                     g_waitZoneCount = 0;
static std::atomic<uint64_t>   g_loadingThread{0};
static char                    g_xpakNames[32][kNameMax + 8];
static std::atomic<uint32_t>   g_xpakNameAt{0};
static std::atomic<bool> g_modsLuaDirty{true};

static std::atomic<int32_t> g_listReaders{0};
static std::atomic<bool>    g_listWriting{false};

struct ListReader
{
    ListReader()
    {
        for (;;)
        {
            g_listReaders.fetch_add(1);

            if (!g_listWriting.load())
                return;

            g_listReaders.fetch_sub(1);

            while (g_listWriting.load())
                sceKernelUsleep(100);
        }
    }

    ~ListReader()
    {
        g_listReaders.fetch_sub(1);
    }
};

static void BeginListWrite()
{
    g_listWriting.store(true);

    while (g_listReaders.load() != 0)
        sceKernelUsleep(100);
}

static void EndListWrite()
{
    g_listWriting.store(false);
}

static Detour g_openDetour{};
static void*  g_openOriginal = nullptr;
static Detour g_existsDetour{};
static void*  g_existsOriginal = nullptr;
static Detour g_validDetour{};
static void*  g_validOriginal = nullptr;
static Detour g_dlcBitDetour{};
static void*  g_dlcBitOriginal = nullptr;
static Detour g_loadDetour{};
static void*  g_loadOriginal = nullptr;
static Detour g_blockDetour{};
static void*  g_blockOriginal = nullptr;
static Detour g_codeTextureDetour{};
static void*  g_codeTextureOriginal = nullptr;
static Detour g_mapsTableDetour{};
static void*  g_mapsTableOriginal = nullptr;
static Detour g_pathRemapDetour{};
static void*  g_pathRemapOriginal = nullptr;
static Detour g_linkDetour{};
static void*  g_linkOriginal = nullptr;
static Detour g_priorityDetour{};
static void*  g_priorityOriginal = nullptr;
static Detour g_patchZonesDetour{};
static void*  g_patchZonesOriginal = nullptr;
static Detour g_xpakOpenDetour{};
static void*  g_xpakOpenOriginal = nullptr;
static Detour g_loadXFileDetour{};
static void*  g_loadXFileOriginal = nullptr;
static Detour g_levelZonesDetour{};
static void*  g_levelZonesOriginal = nullptr;
static Detour g_playMovieDetour{};
static void*  g_playMovieOriginal = nullptr;
static Detour g_demoStartDetour{};
static void*  g_demoStartOriginal = nullptr;
static Detour g_getDvarStringDetour{};
static void*  g_getDvarStringOriginal = nullptr;
static Detour g_viewmodelFxDetour{};
static void*  g_viewmodelFxOriginal = nullptr;
static Detour g_texturesDetour{};
static void*  g_texturesOriginal = nullptr;
static Detour g_gpuFxQueueDetour{};
static void*  g_gpuFxQueueOriginal = nullptr;

static std::atomic<bool>     g_usermapLevel{false};
static std::atomic<uint64_t> g_textureWaitStart{0};
static std::atomic<bool>     g_texturesForced{false};
static std::atomic<uint64_t> g_gateLogged{0};
static std::atomic<uint32_t> g_gateLogs{0};
#ifdef _DEBUG
static void*                 g_openMenuOriginal = nullptr;
static void*                 g_closeMenuOriginal = nullptr;
static void*                 g_freezeOriginal = nullptr;
static void*                 g_uiVisibilityOriginal = nullptr;
static void*                 g_getPlayersOriginal = nullptr;
static std::atomic<uint32_t> g_watchLogs{0};
static std::atomic<uint64_t> g_heartbeatAt{0};
static std::atomic<uint32_t> g_heartbeats{0};
static std::atomic<uint32_t> g_serverCalls{0};
static std::atomic<uint32_t> g_clientFrames{0};
static std::atomic<uint64_t> g_levelStart{0};
static std::atomic<uint64_t> g_lastFrameAt{0};
static std::atomic<uint32_t> g_levelFrames{0};
static int32_t               g_lastFrozen[4] = { -1, -1, -1, -1 };

struct StallSample
{
    int64_t  mcontext;
    uint64_t rip;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t words;
    uint64_t raw[kStallRawWords];
    uint64_t stack[kStallStackWords];
};

using InstallHandler_t = int (*)(int, void (*)(int, void*));
using RaiseException_t = int (*)(OrbisPthread, int);

static const char* const k_stallThreads[kStallThreads] = {
    "0_Main", "2_Backend", "1_Worker0", "1_Worker1", "1_Worker2", "3_Server", "Database",
    "Stream", "StreamAsync", "Sound Mix", "Sound Buffer", "Video Decode", "Sound Decode", "Netchan",
};

static StallSample            g_stallSample;
static std::atomic<int>       g_stallState{0};
static std::atomic<uintptr_t> g_stallTarget{0};
static uintptr_t              g_sprxStart = 0;
static uintptr_t              g_sprxEnd = 0;
static RaiseException_t       g_raiseException = nullptr;
static std::atomic<uintptr_t> g_mainStackProbe{0};
static char                   g_report[kReportBytes];
static std::atomic<size_t>    g_reportUsed{0};
static uint64_t               g_dvarReadKeys[kDvarReadsLogged];
static std::atomic<uint32_t>  g_dvarReads{0};

struct Dispatch
{
    uintptr_t commands;
    uintptr_t label;
    uintptr_t caller;
    uintptr_t shader;
    uint32_t  number;
    uint32_t  x;
    uint32_t  y;
    uint32_t  z;
    uint32_t  frame;
};

static const uint8_t          k_dispatch[] = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54, 0x53,
                                               0x50, 0x41, 0x89, 0xCF, 0x41, 0x89, 0xD4, 0x41, 0x89, 0xF5, 0x48, 0x89, 0xFB };
static Detour                 g_dispatchDetour{};
static void*                  g_dispatchOriginal = nullptr;
static Dispatch               g_dispatches[kDispatchesKept];
static std::atomic<uint32_t>  g_dispatchCount{0};
static std::atomic<uint32_t>  g_spawnersDropped{0};
#endif
static std::atomic<uint64_t> g_scriptLinker{0};

static bool     g_inLevel = false;

static const uint8_t k_fileOpen[]  = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55,
                                       0x41, 0x54, 0x53, 0x48, 0x83, 0xEC, 0x58, 0x48, 0x8B, 0x05 };
static const uint8_t k_mapExists[] = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54,
                                       0x53, 0x49, 0x89, 0xFE, 0x4D, 0x85, 0xF6, 0x0F, 0x84 };
static const uint8_t k_loadZones[] = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                       0x53, 0x48, 0x81, 0xEC, 0xC8, 0x0D, 0x00, 0x00, 0x48, 0x8B, 0x05 };
static const uint8_t k_isMapValid[] = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                        0x53, 0x50, 0x49, 0x89, 0xFC, 0x49, 0x8B, 0x74, 0x24, 0x50, 0x49, 0x3B,
                                        0x74, 0x24, 0x48 };
static const uint8_t k_dlcBitForMap[] = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                          0x53, 0x50, 0x49, 0x89, 0xFC, 0x45, 0x31, 0xED, 0x41, 0xBE, 0x00, 0x00,
                                          0x00, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x49, 0x8B, 0x74, 0x24, 0x50 };
static const uint8_t k_mapContentGate[] = { 0x31, 0xC0, 0xEB, 0x00, 0x31, 0xC0, 0xEB, 0x02 };
static const uint8_t k_mapContentBase[] = { 0x4C, 0x8B, 0x7D, 0xB8, 0x41, 0x89, 0xC6, 0x45, 0x85, 0xF6, 0x0F, 0x84 };
static const uint8_t k_blockJob[]   = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                        0x53, 0x48, 0x83, 0xE4, 0xE0, 0x48, 0x81, 0xEC, 0x20, 0x12, 0x00, 0x00,
                                        0x48, 0x8B, 0x0D };
static const uint8_t k_codeTexture[] = { 0x41, 0x89, 0xC8, 0x45, 0x89, 0xC1, 0x4B, 0x8D, 0x04, 0x49, 0x80, 0xBC,
                                         0x47, 0x80, 0x17, 0x00, 0x00, 0x00, 0x74, 0x54, 0x44, 0x0F, 0xB7, 0x84,
                                         0x47, 0x82, 0x17, 0x00, 0x00, 0x48, 0x8B, 0x0D };
static const uint8_t k_mapsTable[]   = { 0x55, 0x48, 0x89, 0xE5, 0x53, 0x48, 0x83, 0xEC, 0x18, 0x48, 0x8B, 0x1D };
static const uint8_t k_pathRemap[]   = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                         0x53, 0x48, 0x81, 0xEC, 0xF8, 0x05, 0x00, 0x00, 0x48, 0x8B, 0x05 };
static const uint8_t k_linkXAsset[]   = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                         0x53, 0x48, 0x83, 0xEC, 0x38, 0x48, 0x8B, 0x05 };
static const uint8_t k_zonePriority[] = { 0x55, 0x48, 0x89, 0xE5, 0x53, 0x50, 0x8D, 0x0C, 0xFD, 0x00, 0x00, 0x00,
                                          0x00, 0xB8, 0x1B, 0x01, 0x00, 0x00, 0xBB, 0xE9, 0x03, 0x00, 0x00 };
static const uint8_t k_patchZones[]   = { 0x81, 0xE7, 0xFF, 0xFF, 0xFF, 0x17, 0x31, 0xC0, 0x83, 0xFF, 0x1F, 0x7F, 0x42 };
static const uint8_t k_xpakOpen[]     = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                          0x53, 0x48, 0x81, 0xEC, 0xF8, 0x05, 0x00, 0x00 };
static const uint8_t k_loadXFile[]    = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                          0x53, 0x48, 0x81, 0xEC, 0x48, 0x2B, 0x00, 0x00 };
static const uint8_t k_loadLevelZones[] = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54, 0x53,
                                            0x48, 0x83, 0xE4, 0xE0, 0x48, 0x81, 0xEC, 0xA0, 0x04, 0x00, 0x00 };
static const uint8_t k_playMovie[]    = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                          0x53, 0x48, 0x81, 0xEC, 0xB8, 0x08, 0x00, 0x00 };
static const uint8_t k_killServer[]   = { 0x55, 0x48, 0x89, 0xE5, 0x53, 0x50, 0xC6, 0x05, 0x50, 0x19, 0x16, 0x0B,
                                          0x01 };
static const uint8_t k_demoStart[]    = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                         0x53, 0x48, 0x83, 0xE4, 0xE0, 0x48, 0x81, 0xEC, 0x40, 0x0C, 0x00, 0x00 };
static const uint8_t k_getDvarString[] = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                          0x53, 0x50, 0x41, 0x89, 0xFE, 0x4C, 0x8D, 0x25 };
static const uint8_t k_playViewmodelFx[] = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
                                            0x53, 0x48, 0x83, 0xEC, 0x18, 0x48, 0x8B, 0x05 };
static const uint8_t k_gpuFxQueue[] = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54, 0x53,
                                       0x48, 0x83, 0xEC, 0x28, 0x4C, 0x8B, 0x15, 0x58, 0xAE, 0xD4, 0x00, 0x49, 0x8B };
static const uint8_t k_texturesLoaded[] = { 0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x54, 0x53, 0x48,
                                           0x83, 0xEC, 0x10, 0x4C, 0x8B, 0x35 };

struct SignaturePatch
{
    uintptr_t      site;
    const uint8_t* expect;
    size_t         expectLength;
    size_t         offset;
    const char*    what;
};

static const uint8_t k_signatureHeader[] = { 0x85, 0xDB, 0x75, 0x09, 0x83, 0xBD, 0x10, 0xD6, 0xFF, 0xFF, 0x01, 0x74, 0x0A,
                                             0x81, 0x05, 0x51, 0xA4, 0xFE, 0x04, 0x00, 0x40, 0x00, 0x00,
                                             0x48, 0x8D, 0x3D };
static const uint8_t k_signaturePatch[]  = { 0x85, 0xDB, 0x75, 0x06, 0x83, 0x7D, 0x90, 0x01, 0x74, 0x0A,
                                             0x81, 0x05, 0xA6, 0xA0, 0xFE, 0x04, 0x00, 0x40, 0x00, 0x00,
                                             0x0F, 0xB6, 0x05 };
static const uint8_t k_nop10[]           = { 0x66, 0x2E, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const SignaturePatch k_signaturePatches[] =
{
    { 0x7FF2A4, k_signatureHeader, sizeof(k_signatureHeader), 13, "zone header signature" },
    { 0x7FF652, k_signaturePatch,  sizeof(k_signaturePatch),  10, "patch file signature" },
};

struct PenaltySite
{
    uintptr_t site;
    uint8_t   opcode[4];
    size_t    operand;
};

static const PenaltySite k_placeholderPenalties[] =
{
    { 0x857D47, { 0x41, 0x8D, 0x8E }, 3 },
    { 0x857D5A, { 0x41, 0x81, 0xC5 }, 3 },
    { 0x857DF9, { 0x41, 0x8D, 0x8C, 0x24 }, 4 },
    { 0x857E13, { 0x41, 0x81, 0xC7 }, 3 },
    { 0x857EC8, { 0x41, 0x8D, 0x96 }, 3 },
    { 0x857EDB, { 0x41, 0x81, 0xC7 }, 3 },
    { 0x857F79, { 0x41, 0x8D, 0x8C, 0x24 }, 4 },
    { 0x857F93, { 0x41, 0x81, 0xC7 }, 3 },
};

constexpr int32_t kPlaceholderPenalty    = -1000;
constexpr int32_t kPlaceholderPenaltyLow = -2000;

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

static bool IsToken(const char* name)
{
    size_t n = 0;

    for (; name[n]; ++n)
    {
        const char c = name[n];

        if (n >= kNameMax - 1 || !((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'))
            return false;
    }

    return n > 0;
}

static bool IsZoneFile(const char* name)
{
    return EndsWith(name, ".ff") || EndsWith(name, ".fd") || EndsWith(name, ".xpak") || EndsWith(name, ".sabl") ||
           EndsWith(name, ".sabs");
}

static int ListFolder(const char* folder, bool folders, char names[][kListedNameMax], int capacity)
{
    const int fd = sceKernelOpen(folder, O_RDONLY | O_DIRECTORY, 0);

    if (fd < 0)
        return 0;

    static char buffer[0x10000];
    int count = 0;

    for (;;)
    {
        const int n = sceKernelGetdents(fd, buffer, sizeof(buffer));

        if (n <= 0)
            break;

        for (int at = 0; at + 8 <= n;)
        {
            const struct dirent* const entry = (const struct dirent*)(buffer + at);

            if (entry->d_reclen == 0)
                break;

            at += entry->d_reclen;

            if (entry->d_name[0] == '.')
                continue;

            if ((entry->d_type == DT_DIR) != folders || strlen(entry->d_name) >= kListedNameMax)
                continue;

            if (count < capacity)
                snprintf(names[count++], kListedNameMax, "%s", entry->d_name);
        }
    }

    sceKernelClose(fd);
    return count;
}

static void AddFile(const char* rel, const char* folder, int package)
{
    for (int i = 0; i < g_fileCount; ++i)
    {
        if (strcmp(g_files[i].rel, rel) == 0)
        {
            return;
        }
    }

    if (g_fileCount >= kMaxFiles)
        return;

    File& file = g_files[g_fileCount++];
    snprintf(file.rel, sizeof(file.rel), "%s", rel);
    snprintf(file.path, sizeof(file.path), "%s/%s", folder, rel);
    file.package = package;
}

static void ScanPackage(const char* folder, int package)
{
    const char* const name = g_packages[package].name;
    static char files[kMaxListed][kListedNameMax];
    static char languages[16][kListedNameMax];

    const int fileCount = ListFolder(folder, false, files, kMaxListed);

    for (int i = 0; i < fileCount; ++i)
    {
        if (IsZoneFile(files[i]) && strstr(files[i], name))
            AddFile(files[i], folder, package);
    }

    char sounds[224];
    snprintf(sounds, sizeof(sounds), "%s/snd", folder);

    const int languageCount = ListFolder(sounds, true, languages, 16);

    for (int l = 0; l < languageCount; ++l)
    {
        char language[224];
        snprintf(language, sizeof(language), "%s/%s", sounds, languages[l]);

        const int bankCount = ListFolder(language, false, files, kMaxListed);

        for (int i = 0; i < bankCount; ++i)
        {
            if (IsZoneFile(files[i]) && strstr(files[i], name))
            {
                char rel[128];
                snprintf(rel, sizeof(rel), "snd/%s/%s", languages[l], files[i]);
                AddFile(rel, folder, package);
            }
        }
    }
}

static int AddPackage(const char* name, bool map, const char* folder, const char* alternate)
{
    if (g_packageCount >= kMaxPackages)
        return -1;

    for (int i = 0; i < g_packageCount; ++i)
    {
        if (EqualsNoCase(name, g_packages[i].name))
            return -1;
    }

    Package& package = g_packages[g_packageCount];
    snprintf(package.name, sizeof(package.name), "%s", name);
    snprintf(package.folder, sizeof(package.folder), "%s", folder);
    snprintf(package.alternate, sizeof(package.alternate), "%s", alternate);
    package.preview[0] = 0;
    package.map = map;
    return g_packageCount++;
}

static bool FindPackageFile(const Package& package, const char* rel, char* out, size_t size)
{
    OrbisKernelStat st;
    snprintf(out, size, "%s/%s", package.folder, rel);

    if (sceKernelStat(out, &st) == 0)
        return true;

    if (!package.alternate[0])
        return false;

    snprintf(out, size, "%s/%s", package.alternate, rel);
    return sceKernelStat(out, &st) == 0;
}

static void ScanUsermaps(const char* data)
{
    char root[224];
    snprintf(root, sizeof(root), "%s/usermaps", data);

    static char folders[kMaxPackages][kListedNameMax];
    const int count = ListFolder(root, true, folders, kMaxPackages);

    for (int i = 0; i < count; ++i)
    {
        char folder[224];
        char zoneFolder[224];
        char zone[256];
        OrbisKernelStat st;

        snprintf(folder, sizeof(folder), "%s/%s", root, folders[i]);
        snprintf(zoneFolder, sizeof(zoneFolder), "%s/zone", folder);

        const bool valid = IsToken(folders[i]);
        snprintf(zone, sizeof(zone), "%s/%s.ff", folder, folders[i]);
        const bool atRoot = valid && sceKernelStat(zone, &st) == 0;
        snprintf(zone, sizeof(zone), "%s/%s.ff", zoneFolder, folders[i]);
        const bool inZone = valid && !atRoot && sceKernelStat(zone, &st) == 0;

        if (!atRoot && !inZone)
            continue;

        const int package = atRoot ? AddPackage(folders[i], true, folder, zoneFolder)
                                   : AddPackage(folders[i], true, zoneFolder, folder);

        if (package < 0)
            continue;

        ScanPackage(g_packages[package].folder, package);
        ScanPackage(g_packages[package].alternate, package);
    }
}

static void ScanSharedZones(const char* data)
{
    char folder[224];
    snprintf(folder, sizeof(folder), "%s/zone", data);

    static char files[kMaxListed][kListedNameMax];
    const int count = ListFolder(folder, false, files, kMaxListed);

    for (int i = 0; i < count; ++i)
    {
        if (!EndsWith(files[i], ".ff"))
            continue;

        char zone[kListedNameMax];
        snprintf(zone, sizeof(zone), "%s", files[i]);
        zone[strlen(zone) - 3] = 0;

        if (!IsToken(zone))
            continue;

        bool localized = false;

        if (strlen(zone) > 3 && zone[2] == '_')
        {
            char original[kListedNameMax];
            snprintf(original, sizeof(original), "%s.ff", zone + 3);

            for (int j = 0; j < count && !localized; ++j)
                localized = strcmp(files[j], original) == 0;
        }

        if (localized)
            continue;

        const int package = AddPackage(zone, false, folder, "");

        if (package < 0)
            continue;

        ScanPackage(folder, package);

        if (strcmp(zone, k_levelCommon) == 0)
            g_levelCommon = package;
    }
}

static const char* Relative(const char* path)
{
    const char* const folder = (const char*)(g_base + kZoneFolder);
    const size_t n = strnlen(folder, 260);

    if (n && n < 260 && strncmp(path, folder, n) == 0 && (path[n] == '/' || path[n] == '\\'))
        return path + n + 1;

    if (strncmp(path, k_soundFolder, sizeof(k_soundFolder) - 1) == 0)
        return path + sizeof(k_soundFolder) - 1;

    return nullptr;
}

static bool LanguageShape(const char* rel, char* out, size_t size)
{
    static const char* const k_languages[] = { "all", "bp", "ct", "cz", "du", "ea", "en", "es", "fr",
                                               "ge", "it", "ja", "ko", "nl", "pl", "po", "pt", "ru", "sp" };
    constexpr size_t kCount = sizeof(k_languages) / sizeof(k_languages[0]);

    if (strncmp(rel, "snd/", 4) == 0)
    {
        const char* const slash = strchr(rel + 4, '/');

        if (slash == nullptr)
            return false;

        const size_t length = (size_t)(slash - (rel + 4));

        for (size_t i = 0; i < kCount; ++i)
        {
            if (strlen(k_languages[i]) != length || strncmp(rel + 4, k_languages[i], length) != 0)
                continue;

            char tail[128];
            snprintf(tail, sizeof(tail), "%s", slash + 1);
            char* const dot = strchr(tail, '.');

            if (dot == nullptr)
                return false;

            const char* const rest = strchr(dot + 1, '.');

            if (rest == nullptr)
                return false;

            *dot = 0;
            snprintf(out, size, "snd/*/%s.*%s", tail, rest);
            return true;
        }

        return false;
    }

    const char* const underscore = strchr(rel, '_');

    if (underscore == nullptr)
        return false;

    const size_t length = (size_t)(underscore - rel);

    for (size_t i = 0; i < kCount; ++i)
    {
        if (strlen(k_languages[i]) == length && strncmp(rel, k_languages[i], length) == 0)
        {
            snprintf(out, size, "*%s", underscore);
            return true;
        }
    }

    return false;
}

static int LanguageCompanion(const char* rel)
{
    char wanted[128];

    if (!LanguageShape(rel, wanted, sizeof(wanted)))
        return -1;

    for (int i = 0; i < g_fileCount; ++i)
    {
        char have[128];

        if (LanguageShape(g_files[i].rel, have, sizeof(have)) && strcmp(have, wanted) == 0)
            return i;
    }

    return -1;
}

static int FindMod(const char* name)
{
    for (int i = 0; name && name[0] && i < g_modCount; ++i)
    {
        if (EqualsNoCase(g_mods[i].name, name))
            return i;
    }

    return -1;
}

static bool IsModZoneName(const char* name)
{
    if (!name)
        return false;

    if (name[0] == 'm' && name[1] >= '2' && name[1] <= '9' && name[2] == '_')
        name += 3;

    for (int k = 0; k < kModZoneKinds; ++k)
    {
        if (EqualsNoCase(name, k_modZones[k]))
            return true;
    }

    return false;
}

static bool StackName(const char* name, int* index, char* inner, size_t size)
{
    if (!name || strchr(name, '/'))
        return false;

    const char* at = name;
    char language[4] = "";

    if (at[0] >= 'a' && at[0] <= 'z' && at[1] >= 'a' && at[1] <= 'z' && at[2] == '_' && at[3] == 'm' &&
        at[4] >= '2' && at[4] <= '9' && at[5] == '_')
    {
        language[0] = at[0];
        language[1] = at[1];
        language[2] = '_';
        language[3] = 0;
        at += 3;
    }

    if (at[0] != 'm' || at[1] < '2' || at[1] > '9' || at[2] != '_' || !at[3])
        return false;

    *index = at[1] - '1';
    snprintf(inner, size, "%s%s", language, at + 3);
    return true;
}

static int ZoneKind(const char* inner, bool* localized)
{
    const char* base = inner;
    *localized = false;

    if (base[0] && base[1] && base[2] == '_' && !IsModZoneName(base))
    {
        base += 3;
        *localized = true;
    }

    for (int k = 0; k < kModZoneKinds; ++k)
    {
        if (EqualsNoCase(base, k_modZones[k]))
            return k;
    }

    return -1;
}

static void AddStackFile(StackMod& entry, const char* rel, const char* folder)
{
    for (int i = 0; i < entry.fileCount; ++i)
    {
        if (strcmp(entry.files[i].rel, rel) == 0)
            return;
    }

    if (entry.fileCount >= kMaxStackFiles)
        return;

    File& file = entry.files[entry.fileCount++];
    snprintf(file.rel, sizeof(file.rel), "%s", rel);
    snprintf(file.path, sizeof(file.path), "%s/%s", folder, rel);
    file.package = -1;
}

static void ScanStackFolder(StackMod& entry, const char* folder)
{
    if (!folder[0])
        return;

    static char files[kMaxModListed][kListedNameMax];
    static char languages[24][kListedNameMax];

    const int fileCount = ListFolder(folder, false, files, kMaxModListed);

    for (int i = 0; i < fileCount; ++i)
    {
        if (IsZoneFile(files[i]))
            AddStackFile(entry, files[i], folder);
    }

    char sounds[224];
    snprintf(sounds, sizeof(sounds), "%s/snd", folder);

    const int languageCount = ListFolder(sounds, true, languages, 24);

    for (int l = 0; l < languageCount; ++l)
    {
        char language[224];
        snprintf(language, sizeof(language), "%s/%s", sounds, languages[l]);

        const int bankCount = ListFolder(language, false, files, kMaxModListed);

        for (int i = 0; i < bankCount; ++i)
        {
            if (IsZoneFile(files[i]))
            {
                char rel[128];
                snprintf(rel, sizeof(rel), "snd/%s/%s", languages[l], files[i]);
                AddStackFile(entry, rel, folder);
            }
        }
    }
}

static void SetStack(const char (*names)[kNameMax], int count)
{
    g_stackCount = 0;

    for (int i = 0; i < count && g_stackCount < kMaxStack; ++i)
    {
        const int index = FindMod(names[i]);

        if (index < 0)
            continue;

        StackMod& entry = g_stack[g_stackCount];
        entry.mod = g_mods[index];
        entry.fileCount = 0;

        for (int k = 0; k < kModZoneKinds; ++k)
        {
            if (g_stackCount == 0)
                snprintf(entry.zones[k], sizeof(entry.zones[k]), "%s", k_modZones[k]);
            else
                snprintf(entry.zones[k], sizeof(entry.zones[k]), "m%d_%s", g_stackCount + 1, k_modZones[k]);
        }

        ScanStackFolder(entry, entry.mod.folder);
        ScanStackFolder(entry, entry.mod.alternate);
        T7Log_Write("[Mods] mod %d is %s from %s (%d files)", g_stackCount + 1, entry.mod.name, entry.mod.folder,
                    entry.fileCount);
        ++g_stackCount;
    }
}

static const File* StackFileIn(const StackMod& entry, const char* rel, bool companion)
{
    for (int i = 0; i < entry.fileCount; ++i)
    {
        if (strcmp(entry.files[i].rel, rel) == 0)
            return &entry.files[i];
    }

    if (!companion || strncmp(rel, "snd/all/", 8) == 0)
        return nullptr;

    char wanted[128];

    if (!LanguageShape(rel, wanted, sizeof(wanted)))
        return nullptr;

    const File* any = nullptr;

    for (int i = 0; i < entry.fileCount; ++i)
    {
        char have[128];

        if (strncmp(entry.files[i].rel, "snd/all/", 8) == 0 || !LanguageShape(entry.files[i].rel, have, sizeof(have)) ||
            strcmp(have, wanted) != 0)
            continue;

        if (strncmp(entry.files[i].rel, "en_", 3) == 0 || strncmp(entry.files[i].rel, "snd/en/", 7) == 0)
            return &entry.files[i];

        if (!any)
            any = &entry.files[i];
    }

    return any;
}

static const File* StackFile(const char* rel)
{
    if (g_stackCount == 0)
        return nullptr;

    int index = -1;
    char inner[128];

    if (StackName(rel, &index, inner, sizeof(inner)))
        return index > 0 && index < g_stackCount ? StackFileIn(g_stack[index], inner, true) : nullptr;

    for (int i = 0; i < g_stackCount; ++i)
    {
        if (const File* const file = StackFileIn(g_stack[i], rel, false))
            return file;
    }

    for (int i = 0; i < g_stackCount; ++i)
    {
        if (const File* const file = StackFileIn(g_stack[i], rel, true))
            return file;
    }

    return nullptr;
}

static uint64_t FileOpen_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                           double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    const char* const path = (const char*)a1;

    if (path)
    {
        const char* const rel = Relative(path);

        if (rel)
        {
            ListReader reading;
            int found = -1;
            const File* const mod = StackFile(rel);

            if (mod)
            {
                a1 = (uint64_t)mod->path;
            }
            else
            {
                for (int i = 0; i < g_fileCount; ++i)
                {
                    if (strcmp(g_files[i].rel, rel) == 0)
                    {
                        found = i;
                        break;
                    }
                }

                if (found < 0)
                    found = LanguageCompanion(rel);

                if (found >= 0)
                {
                    a1 = (uint64_t)g_files[found].path;
                }
            }
        }
    }

    return ((Hook_t)g_openOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static int MapPackage(const char* name)
{
    for (int i = 0; i < g_packageCount; ++i)
    {
        if (g_packages[i].map && EqualsNoCase(name, g_packages[i].name))
            return i;
    }

    return -1;
}

static uint64_t MapExists_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                            double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    const char* const map = (const char*)a1;

    if (map && *map)
    {
        ListReader reading;
        const int package = MapPackage(map);

        if (package >= 0)
            return 1;
    }

    return ((Hook_t)g_existsOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static uint64_t IsMapValid_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                             double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    const uint64_t at = *(uint64_t*)(a1 + 80);

    if (at < *(uint64_t*)(a1 + 72) && (*(uint32_t*)at & 0xF) == 4 && *(uint64_t*)(at + 8))
    {
        const uint64_t string = *(uint64_t*)(at + 8);
        ListReader reading;
        const int package = (*(uint64_t*)(string + 8) & 0x3FFFFFFFFFFFFFFFull) < kNameMax
                                ? MapPackage((const char*)(string + 20))
                                : -1;

        if (package >= 0)
        {
            const uint64_t top = *(uint64_t*)(a1 + 72);
            *(uint32_t*)top = 1;
            *(uint32_t*)(top + 8) = 1;
            *(uint64_t*)(a1 + 72) = top + 16;
            return 1;
        }
    }

    return ((Hook_t)g_validOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static const char* LuaStringArgument(uint64_t state)
{
    const uint64_t at = *(uint64_t*)(state + 80);

    if (at >= *(uint64_t*)(state + 72) || (*(uint32_t*)at & 0xF) != 4 || !*(uint64_t*)(at + 8))
        return nullptr;

    const uint64_t string = *(uint64_t*)(at + 8);
    return (*(uint64_t*)(string + 8) & 0x3FFFFFFFFFFFFFFFull) < kNameMax ? (const char*)(string + 20) : nullptr;
}

static uint64_t PushLuaNumber(uint64_t state, float value)
{
    const uint64_t top = *(uint64_t*)(state + 72);
    *(float*)(top + 8) = value;
    *(uint32_t*)top = 3;
    *(uint64_t*)(state + 72) = top + 16;
    return 1;
}

static uint64_t DlcBitForMap_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                               double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    static const char k_ours[] = "bo3customs:";
    const char* asked = LuaStringArgument(a1);

    if (asked && strncmp(asked, k_ours, sizeof(k_ours) - 1) == 0)
        return PushLuaNumber(a1, T7Share_Query(asked + sizeof(k_ours) - 1));

    if (asked)
    {
        T7Share_LobbyState(a1);
        asked = LuaStringArgument(a1);
    }

    if (asked)
    {
        bool have = false;

        {
            ListReader reading;
            have = MapPackage(asked) >= 0;
        }

        if (have || T7Share_Wants(asked))
            return PushLuaNumber(a1, (float)kBaseContentBit);
    }

    return ((Hook_t)g_dlcBitOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

struct ClaimLock
{
    ClaimLock()
    {
        while (g_claimLock.exchange(true, std::memory_order_acquire))
            ;
    }

    ~ClaimLock() { g_claimLock.store(false, std::memory_order_release); }
};

static const char* InternName(const char* text)
{
    if (strlen(text) >= kInternedMax)
        return nullptr;

    for (int i = 0; i < g_internedCount; ++i)
    {
        if (strcmp(g_interned[i], text) == 0)
            return g_interned[i];
    }

    if (g_internedCount >= kMaxInterned)
        return nullptr;

    char* const slot = g_interned[g_internedCount++];
    snprintf(slot, kInternedMax, "%s", text);
    return slot;
}

static void ResetScriptClaims()
{
    ClaimLock locked;
    g_scriptClaimCount = 0;
}

static int StackIndexOfZone(const char* zone)
{
    int index = -1;
    char inner[kNameMax];

    if (StackName(zone, &index, inner, sizeof(inner)))
        return index;

    bool localized = false;
    return ZoneKind(zone, &localized) >= 0 ? 0 : -1;
}

static bool ScriptBase(const char* name, char* base, size_t size)
{
    const size_t n = strlen(name);

    if (n < 5 || n + 4 > size || name[n - 4] != '.' ||
        (!EqualsNoCase(name + n - 3, "gsc") && !EqualsNoCase(name + n - 3, "csc")))
        return false;

    for (size_t i = 0; i < n - 4; ++i)
        base[i] = (name[i] >= 'A' && name[i] <= 'Z') ? (char)(name[i] + 32) : name[i];

    base[n - 4] = 0;
    return true;
}

static void ClaimStackScript(uintptr_t header, int index)
{
    const char* const name = *(const char* const*)header;
    char base[kInternedMax];

    if (!name || !RangeReadable((uintptr_t)name, 1) || !ScriptBase(name, base, sizeof(base)))
        return;

    int owner = -1;
    const char* hidden = nullptr;

    {
        ClaimLock locked;

        for (int i = 0; i < g_scriptClaimCount && owner < 0; ++i)
        {
            if (strcmp(g_scriptClaims[i].base, base) == 0)
                owner = g_scriptClaims[i].owner;
        }

        if (owner < 0)
        {
            if (g_scriptClaimCount < kMaxScriptClaims)
            {
                snprintf(g_scriptClaims[g_scriptClaimCount].base, kInternedMax, "%s", base);
                g_scriptClaims[g_scriptClaimCount].owner = index;
                ++g_scriptClaimCount;
            }

            return;
        }

        if (owner >= index)
            return;

        char renamed[kInternedMax];
        snprintf(renamed, sizeof(renamed), "m%d_%s", index + 1, name);
        hidden = InternName(renamed);

        if (hidden)
            *(const char**)header = hidden;
    }

    if (hidden)
        T7Log_Write("[Mods] mod %d (%s): %s is hidden, mod %d (%s) owns both sides of %s", index + 1,
                    g_stack[index].mod.name, name, owner + 1, g_stack[owner].mod.name, base);
    else
        T7Log_Write("[Mods] mod %d (%s): no room to hide %s, mod %d owns %s", index + 1, g_stack[index].mod.name,
                    name, owner + 1, base);
}

static void NoteUiFile(uintptr_t header, int index)
{
    const char* const name = *(const char* const*)header;

    if (!name || !RangeReadable((uintptr_t)name, 4) || name[0] != 'u' || name[1] != 'i')
        return;

    const size_t n = strlen(name);

    if (n < 8 || !EqualsNoCase(name + n - 4, ".lua"))
        return;

    const char* const mod = g_stack[index].mod.name;
    bool added = false;

    {
        ClaimLock locked;
        bool known = false;

        for (int i = 0; i < g_uiModCount && !known; ++i)
            known = EqualsNoCase(g_uiMods[i], mod);

        if (!known && g_uiModCount < kMaxMods)
        {
            snprintf(g_uiMods[g_uiModCount++], kNameMax, "%s", mod);
            added = true;
        }
    }

    if (added)
        T7Log_Write("[Mods] %s has menu files (%s)", mod, name);
}

static void RenameStackAsset(uint64_t asset)
{
    if (g_loadingThread.load(std::memory_order_acquire) != (uint64_t)scePthreadSelf())
        return;

    const int32_t type = *(const int32_t*)asset;

    if (type != kKeyValuePairs && type != kBgCache && type != kScriptParseTree && type != kRawFileAsset)
        return;

    const char* const zone = (const char*)g_loadingZone.load(std::memory_order_acquire);

    if (!zone || !RangeReadable((uintptr_t)zone, 1))
        return;

    ListReader reading;
    const int index = StackIndexOfZone(zone);
    const uintptr_t header = *(const uintptr_t*)(asset + 8);

    if (index < 0 || index >= g_stackCount || !header || !RangeReadable(header, sizeof(uintptr_t)))
        return;

    if (type == kRawFileAsset)
    {
        NoteUiFile(header, index);
        return;
    }

    if (g_stackCount < 2)
        return;

    if (type == kScriptParseTree)
    {
        ClaimStackScript(header, index);
        return;
    }

    int parsed = -1;
    char inner[kNameMax];

    if (index == 0 || !StackName(zone, &parsed, inner, sizeof(inner)))
        return;

    bool localized = false;
    const char* const name = *(const char* const*)header;

    if (ZoneKind(inner, &localized) < 0 || !name || !RangeReadable((uintptr_t)name, 1) || !EqualsNoCase(name, inner))
        return;

    const char* slot = nullptr;

    {
        ClaimLock locked;
        slot = InternName(zone);
    }

    if (!slot)
        return;

    *(const char**)header = slot;
    T7Log_Write("[Mods] mod %d (%s): %s %s is now %s", index + 1, g_stack[index].mod.name,
                type == kBgCache ? "precache list" : "key/value list", inner, slot);
}

static uint64_t LinkXAsset_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                             double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    bool owner = false;

    if (a1 && g_stackCount > 0)
        RenameStackAsset(a1);

    if (g_usermapLevel.load(std::memory_order_relaxed) && a1 && *(const int32_t*)a1 == kScriptParseTree)
    {
        uint64_t idle = 0;
        owner = g_scriptLinker.compare_exchange_strong(idle, (uint64_t)scePthreadSelf(), std::memory_order_acq_rel);
    }

    const uint64_t result = ((Hook_t)g_linkOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);

    if (owner)
        g_scriptLinker.store(0, std::memory_order_release);

    return result;
}

static uint64_t ZonePriority_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                               double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    if (((uint32_t)a1 & kModMarker) != 0)
    {
        const uint64_t rank = ((uint32_t)a1 >> kModRankShift) & kModRankMax;
        const uint64_t priority = kModPriority + 2 * (kModRankMax - rank) + 1;
        return ((uint32_t)a1 & kLocalizedZone) != 0 ? priority - 1 : priority;
    }

    const uint64_t priority = ((Hook_t)g_priorityOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);

    if ((uint32_t)a1 != kUsermapZoneFlags || g_scriptLinker.load(std::memory_order_acquire) == 0)
        return priority;

    const uintptr_t from = (uintptr_t)__builtin_return_address(0) - g_base;

    if (from <= kLinkXAsset || from >= kLinkXAssetEnd ||
        g_scriptLinker.load(std::memory_order_relaxed) != (uint64_t)scePthreadSelf())
        return priority;

    return kUsermapScriptPriority;
}

static uint64_t PatchZones_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                             double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    if (((uint32_t)a1 & kModMarker) != 0)
        return 0;

    return ((Hook_t)g_patchZonesOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

using LoadXFile_t = uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                 double, double, double, double, double, double, double, double);

static uint64_t LoadXFile_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7,
                            uint64_t a8, double x0, double x1, double x2, double x3, double x4, double x5, double x6,
                            double x7)
{
    const uintptr_t zone = g_loadingZone.exchange((uintptr_t)a3, std::memory_order_acq_rel);
    const uint64_t thread = g_loadingThread.exchange((uint64_t)scePthreadSelf(), std::memory_order_acq_rel);
    const uint64_t result = ((LoadXFile_t)g_loadXFileOriginal)(a1, a2, a3, a4, a5, a6, a7, a8, x0, x1, x2, x3, x4, x5,
                                                               x6, x7);
    g_loadingThread.store(thread, std::memory_order_release);
    g_loadingZone.store(zone, std::memory_order_release);
    return result;
}

static bool LanguagePrefixed(const char* name)
{
    static const char* const k_prefixes[] = { "bp", "ea", "en", "es", "fr", "ge", "it", "ja", "po", "ru", "sc", "tc",
                                              "ct", "cz", "du", "ko", "nl", "pl", "pt", "sp" };

    if (!name[0] || !name[1] || name[2] != '_')
        return false;

    for (const char* const prefix : k_prefixes)
    {
        if (name[0] == prefix[0] && name[1] == prefix[1])
            return true;
    }

    return false;
}

static uint64_t XPakOpen_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                           double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    const char* const pak = (const char*)a1;
    const char* const zone = (const char*)g_loadingZone.load(std::memory_order_acquire);
    int index = -1;
    char inner[kNameMax];

    if (pak && zone && g_loadingThread.load(std::memory_order_acquire) == (uint64_t)scePthreadSelf() &&
        RangeReadable((uintptr_t)pak, 1) && StackName(zone, &index, inner, sizeof(inner)))
    {
        ListReader reading;
        char file[kNameMax + 8];
        snprintf(file, sizeof(file), "%s.xpak", pak);

        if (index > 0 && index < g_stackCount && strlen(pak) < kNameMax && StackFileIn(g_stack[index], file, false))
        {
            char* const name = g_xpakNames[g_xpakNameAt.fetch_add(1) % 32];

            if (LanguagePrefixed(pak))
                snprintf(name, kNameMax + 8, "%c%c_m%d_%s", pak[0], pak[1], index + 1, pak + 3);
            else
                snprintf(name, kNameMax + 8, "m%d_%s", index + 1, pak);

            T7Log_Write("[Mods] mod %d (%s): xpak %s opens as %s", index + 1, g_stack[index].mod.name, pak, name);
            a1 = (uint64_t)name;
        }
    }

    return ((Hook_t)g_xpakOpenOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static bool AsksForFsGame(uint32_t inst)
{
    if (inst > 1)
        return false;

    const uintptr_t vm = g_base + kScrVmState + 0x8A38 * (uintptr_t)inst;

    if (*(const uint32_t*)(vm + 0x34) < 1)
        return false;

    const uintptr_t top = *(const uintptr_t*)(vm + 0x20);

    if (!top)
        return false;

    const uint32_t value = *(const uint32_t*)top;
    const uint32_t type = *(const uint32_t*)(top + 8);

    if (type == 5 || type == 7)
        return value == kFsGameHash;

    if (type != 2 || !value)
        return false;

    const uintptr_t table = *(const uintptr_t*)(g_base + kScriptStrings);
    return table && EqualsNoCase((const char*)(table + 28 * (uintptr_t)value + 4), "fs_game");
}

#ifdef _DEBUG
static void DescribeScriptValue(uintptr_t slot, char* out, size_t size)
{
    if (!slot || !RangeReadable(slot, 12))
    {
        snprintf(out, size, "(nothing)");
        return;
    }

    const uint32_t value = *(const uint32_t*)slot;
    const uint32_t type = *(const uint32_t*)(slot + 8);
    const uintptr_t table = *(const uintptr_t*)(g_base + kScriptStrings);

    if (type == 2 && value && table)
        snprintf(out, size, "\"%.120s\"", (const char*)(table + 28 * (uintptr_t)value + 4));
    else if (type == 5 || type == 7)
        snprintf(out, size, "hash %08X", value);
    else
        snprintf(out, size, "type %u value %u", type, value);
}

static bool FirstDvarRead(uint64_t key)
{
    const uint32_t count = g_dvarReads.load(std::memory_order_acquire);

    for (uint32_t i = 0; i < count && i < kDvarReadsLogged; ++i)
    {
        if (g_dvarReadKeys[i] == key)
            return false;
    }

    if (count >= kDvarReadsLogged)
        return false;

    g_dvarReadKeys[count] = key;
    g_dvarReads.store(count + 1, std::memory_order_release);
    return true;
}
#endif

static uint64_t GetDvarString_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                                double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
#ifdef _DEBUG
    const uint32_t inst = (uint32_t)a1;
    const uintptr_t vm = g_base + kScrVmState + 0x8A38 * (uintptr_t)(inst <= 1 ? inst : 0);
    const uintptr_t argument = inst <= 1 && *(const uint32_t*)(vm + 0x34) >= 1 ? *(const uintptr_t*)(vm + 0x20) : 0;
    const bool watch = g_usermapLevel.load(std::memory_order_relaxed) && argument &&
                       FirstDvarRead(((uint64_t)inst << 40) | ((uint64_t)*(const uint32_t*)(argument + 8) << 32) |
                                     *(const uint32_t*)argument);
    char name[160] = "";

    if (watch)
        DescribeScriptValue(argument, name, sizeof(name));
#endif

    if (g_usermapLevel.load(std::memory_order_relaxed) && AsksForFsGame((uint32_t)a1))
    {
        ((void (*)(uint32_t, const char*))(g_base + kScrAddString))((uint32_t)a1, "usermaps");
#ifdef _DEBUG
        if (watch)
            T7Log_Write("[Dvar] %s script read GetDvarString(%s) = \"usermaps\" (answered by BO3-Customs)", inst == 1 ? "client" : "server",
                        name);
#endif
        return 0;
    }

    const uint64_t result = ((Hook_t)g_getDvarStringOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);

#ifdef _DEBUG
    if (watch)
    {
        char value[160];
        DescribeScriptValue(*(const uintptr_t*)(vm + 0x20), value, sizeof(value));
        T7Log_Write("[Dvar] %s script read GetDvarString(%s) = %s", inst == 1 ? "client" : "server", name, value);
    }
#endif

    return result;
}

static void ScriptError(uint32_t inst, const char* text)
{
    uint64_t* const pending = (uint64_t*)(g_base + kScrErrorText) + 14 * (uintptr_t)inst + 2;

    if (!*pending)
    {
        char* const buffer = (char*)(g_base + kScrErrorBuffer);
        snprintf(buffer, 1024, "%s", text);
        *pending = (uint64_t)buffer;
    }

    *(uint8_t*)(g_base + kScrVmState + 0x8A38 * (uintptr_t)inst + 42) = 0;
    ((void (*)(uint32_t))(g_base + kScrError))(inst);
}

static uint64_t PlayViewmodelFx_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                                  double x0, double x1, double x2, double x3, double x4, double x5, double x6,
                                  double x7)
{
    const uint32_t inst = (uint32_t)a1;
    const uint32_t id = ((uint32_t (*)(uint32_t, uint32_t))(g_base + kScrGetConstString))(inst, 1);
    const uintptr_t strings = *(const uintptr_t*)(g_base + kScriptStrings);
    const char* const name = id && strings ? (const char*)(strings + 28 * (uintptr_t)id + 4) : nullptr;
    const int32_t index = ((int32_t (*)(const char*))(g_base + kClientFxIndex))(name);

    if (index <= 0 || index >= kClientFxSlots || !*(const uint64_t*)(g_base + kClientFxDefs + 16 * (uintptr_t)index))
    {
        char text[256];
        snprintf(text, sizeof(text), "PlayViewmodelFX %s is not precached by the linker", name ? name : "");
        ScriptError(inst, text);
        return 0;
    }

    return ((Hook_t)g_viewmodelFxOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

#ifdef _DEBUG
static void Appendf(char* out, size_t size, size_t* used, const char* format, ...)
{
    if (*used + 1 >= size)
        return;

    va_list args;
    va_start(args, format);
    const int wrote = vsnprintf(out + *used, size - *used, format, args);
    va_end(args);

    if (wrote > 0)
        *used = *used + (size_t)wrote < size ? *used + (size_t)wrote : size - 1;
}
#endif

static void LogStreamGate(uint64_t waited, bool details)
{
#ifdef _DEBUG
    const uint32_t* const required = (const uint32_t*)(g_base + kPartsRequired);
    const uint32_t* const wanted = (const uint32_t*)(g_base + kPartsWanted);
    const uint32_t* const inMemory = (const uint32_t*)(g_base + kPartsInMemory);
    const uintptr_t pool = *(const uintptr_t*)(g_base + kImagePool);
    const bool poolReadable = pool && RangeReadable(pool, kImagePoolSize * 304);
    uint32_t requiredParts = 0;
    uint32_t requiredLoaded = 0;
    uint32_t wantedParts = 0;
    uint32_t wantedLoaded = 0;
    uint64_t requiredBytes = 0;
    uint64_t loadedBytes = 0;
    uint32_t missing[65] = {};
    uint32_t shown[12];
    int shownCount = 0;

    for (int w = 0; w < kPartWords; ++w)
    {
        requiredParts += (uint32_t)__builtin_popcount(required[w]);
        requiredLoaded += (uint32_t)__builtin_popcount(required[w] & inMemory[w]);
        wantedParts += (uint32_t)__builtin_popcount(wanted[w]);
        wantedLoaded += (uint32_t)__builtin_popcount(wanted[w] & inMemory[w]);

        for (uint32_t bits = poolReadable ? required[w] : 0; bits; bits &= bits - 1)
        {
            const uint32_t bit = (uint32_t)w * 32 + (uint32_t)__builtin_ctz(bits);
            const uint64_t image = bit >> 2;
            const uint64_t part = bit & 3;

            if (image >= kImagePoolSize)
                continue;

            const uintptr_t entry = pool + 304 * image;
            const uint64_t size = *(const uint64_t*)(entry + 24 + 40 * part);
            const uint64_t flags = *(const uint64_t*)(entry + 32 + 40 * part);

            requiredBytes += size;

            if (inMemory[w] & (1u << (bit & 31)))
            {
                loadedBytes += size;
                continue;
            }

            ++missing[(flags & 0x80) ? (flags & 0x3F) : 64];

            if (shownCount < 12)
                shown[shownCount++] = bit;
        }
    }

    const uint32_t* const meshNeed = (const uint32_t*)(g_base + kMeshRequired);
    const uint32_t* const meshWant = (const uint32_t*)(g_base + kMeshWanted);
    const uint32_t* const meshIn = (const uint32_t*)(g_base + kMeshInMemory);
    uint32_t meshRequired = 0;
    uint32_t meshRequiredLoaded = 0;
    uint32_t meshWanted = 0;
    uint32_t meshWantedLoaded = 0;

    for (int w = 0; w < kMeshWords; ++w)
    {
        meshRequired += (uint32_t)__builtin_popcount(meshNeed[w]);
        meshRequiredLoaded += (uint32_t)__builtin_popcount(meshNeed[w] & meshIn[w]);
        meshWanted += (uint32_t)__builtin_popcount(meshWant[w]);
        meshWantedLoaded += (uint32_t)__builtin_popcount(meshWant[w] & meshIn[w]);
    }

    uint32_t blobCount[5] = {};
    uint32_t blobLoaded[5] = {};
    uint64_t blobBytes[5] = {};
    uint64_t blobLoadedBytes[5] = {};

    for (int i = 0; i < kStreamBlobCount; ++i)
    {
        const uintptr_t entry = g_base + kStreamBlobs + 56 * (uintptr_t)i;
        const uint32_t state = *(const uint32_t*)(entry + 32);

        if (!state)
            continue;

        const uint32_t category = *(const uint8_t*)(entry + 37) < 5 ? *(const uint8_t*)(entry + 37) : 3;
        const uintptr_t item = *(const uintptr_t*)(entry + 48);
        const uint64_t bytes = item >= 0x10000 && item < 0x800000000000ull ? *(const uint32_t*)(item + 52) : 0;

        ++blobCount[category];
        blobBytes[category] += bytes;

        if (state >= 2)
        {
            ++blobLoaded[category];
            blobLoadedBytes[category] += bytes;
        }
    }

    static const char* const k_blobNames[5] = { "probe", "volume", "sun shadow tree", "other", "sky" };
    char text[1400];
    size_t used = 0;

    Appendf(text, sizeof(text), &used, "[Stream] %llu s: level load %u, started %u, done %u | required image parts %u of %u in memory "
            "(%llu of %llu MB), wanted %u of %u | required mesh parts %u of %u, wanted %u of %u | sound banks %.3f loaded | blobs:",
            (unsigned long long)(waited / 1000000), *(const uint8_t*)(g_base + kStreamLevelLoad),
            *(const uint8_t*)(g_base + kStreamStarted), *(const uint8_t*)(g_base + kStreamDone), requiredLoaded,
            requiredParts, (unsigned long long)(loadedBytes >> 20), (unsigned long long)(requiredBytes >> 20), wantedLoaded,
            wantedParts, meshRequiredLoaded, meshRequired, meshWantedLoaded, meshWanted,
            (double)*(const float*)(g_base + kSoundLoaded));

    for (int c = 0; c < 5; ++c)
    {
        if (blobCount[c])
            Appendf(text, sizeof(text), &used, " %s %u of %u (%llu of %llu MB)", k_blobNames[c], blobLoaded[c], blobCount[c],
                    (unsigned long long)(blobLoadedBytes[c] >> 20), (unsigned long long)(blobBytes[c] >> 20));
    }

    T7Log_Write("%s", text);

    used = 0;
    Appendf(text, sizeof(text), &used, "[Stream]   required parts not in memory, by xpak:");

    for (int x = 0; x < 64; ++x)
    {
        if (missing[x])
            Appendf(text, sizeof(text), &used, " %d x%u", x, missing[x]);
    }

    if (missing[64])
        Appendf(text, sizeof(text), &used, " none x%u", missing[64]);

    if (requiredLoaded < requiredParts)
        T7Log_Write("%s", text);

    used = 0;
    Appendf(text, sizeof(text), &used, "[Stream]   sound bank slots:");

    for (int i = 0; i < kSoundSlotCount; ++i)
    {
        const uintptr_t slot = g_base + kSoundSlots + kSoundSlotSize * (uintptr_t)i;

        if (*(const uint64_t*)slot)
            Appendf(text, sizeof(text), &used, " [%d] state %u, %d/%d", i, *(const uint32_t*)(slot + 4716),
                    *(const int32_t*)(slot + 4680), *(const int32_t*)(slot + 4684));
    }

    T7Log_Write("%s", text);

    if (!details)
        return;

    for (int i = 0; i < shownCount; ++i)
    {
        const uint64_t image = shown[i] >> 2;
        const uint64_t part = shown[i] & 3;
        const uintptr_t entry = pool + 304 * image;
        const char* const name = *(const char* const*)(entry + 288);
        const uint64_t flags = *(const uint64_t*)(entry + 32 + 40 * part);

        T7Log_Write("[Stream]   not in memory: image %llu '%s' part %llu, %llu bytes, %s %llu at offset 0x%llX, key %llX",
                    (unsigned long long)image, name && RangeReadable((uintptr_t)name, 1) ? name : "?", (unsigned long long)part,
                    (unsigned long long)*(const uint64_t*)(entry + 24 + 40 * part), (flags & 0x80) ? "xpak" : "NOT IN AN XPAK, slot",
                    (unsigned long long)(flags & 0x3F), (unsigned long long)*(const uint64_t*)(entry + 16 + 40 * part),
                    (unsigned long long)*(const uint64_t*)(entry + 8 + 40 * part));
    }
#else
    (void)waited;
    (void)details;
#endif
}

#ifdef _DEBUG
static const char* ScriptStringArgument(uint32_t index)
{
    const uint32_t id = ((uint32_t (*)(uint32_t, uint32_t))(g_base + kScrGetConstString))(0, index);
    const uintptr_t strings = *(const uintptr_t*)(g_base + kScriptStrings);
    return id && strings ? (const char*)(strings + 28 * (uintptr_t)id + 4) : "?";
}

static int PlayerFrozen(uint32_t entity)
{
    if (entity >= 4)
        return -1;

    const uintptr_t client = *(const uintptr_t*)(g_base + kGEntities + kGEntitySize * entity + kGEntityClient);

    if (!client || !RangeReadable(client + kClientControls, 4))
        return -1;

    return (*(const uint32_t*)(client + kClientControls) & kControlsFrozen) ? 1 : 0;
}

static const char* FrozenText(int frozen)
{
    return frozen == 1 ? "frozen" : frozen == 0 ? "free to move" : "not in the game";
}

static bool WatchScripts()
{
    return g_usermapLevel.load(std::memory_order_relaxed) &&
           g_watchLogs.fetch_add(1, std::memory_order_relaxed) < kScriptWatchLogs;
}

static uint64_t OpenMenu_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, double x0,
                           double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    if (WatchScripts())
        T7Log_Write("[Script] OpenMenu \"%s\" for player %u", ScriptStringArgument(0), (uint32_t)a2);

    return ((Hook_t)g_openMenuOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static uint64_t CloseMenu_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, double x0,
                            double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    if (WatchScripts())
        T7Log_Write("[Script] CloseMenu \"%s\" for player %u", ScriptStringArgument(0), (uint32_t)a2);

    return ((Hook_t)g_closeMenuOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static uint64_t UiVisibility_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, double x0,
                               double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    if (WatchScripts())
        T7Log_Write("[Script] SetClientUIVisibilityFlag \"%s\" %d for player %u", ScriptStringArgument(0),
                    ((int32_t (*)(uint32_t, uint32_t))(g_base + kScrGetInt))(0, 1), (uint32_t)a2);

    return ((Hook_t)g_uiVisibilityOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static uint64_t FreezeControls_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                                 double x0, double x1, double x2, double x3, double x4, double x5, double x6,
                                 double x7)
{
    const uint64_t result = ((Hook_t)g_freezeOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
    const uint32_t entity = (uint32_t)a2;

    if (entity < 4 && g_usermapLevel.load(std::memory_order_relaxed))
    {
        const int frozen = PlayerFrozen(entity);

        if (frozen != g_lastFrozen[entity] && WatchScripts())
        {
            g_lastFrozen[entity] = frozen;
            T7Log_Write("[Script] FreezeControls: player %u is now %s", entity, FrozenText(frozen));
        }
    }

    return result;
}

static uint64_t GetPlayers_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, double x0,
                             double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    g_serverCalls.fetch_add(1, std::memory_order_relaxed);
    return ((Hook_t)g_getPlayersOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static void CountBlobs(uint32_t count[5], uint32_t loaded[5])
{
    for (int i = 0; i < kStreamBlobCount; ++i)
    {
        const uintptr_t entry = g_base + kStreamBlobs + 56 * (uintptr_t)i;
        const uint32_t state = *(const uint32_t*)(entry + 32);

        if (!state)
            continue;

        const uint32_t category = *(const uint8_t*)(entry + 37) < 5 ? *(const uint8_t*)(entry + 37) : 3;
        ++count[category];

        if (state >= 2)
            ++loaded[category];
    }
}

static void LogBlobs(uint64_t seconds)
{
    static const char* const k_names[5] = { "probe", "volume", "sun shadow tree", "other", "sky" };
    const uint64_t tick = *(const uint64_t*)(g_base + kBlobTick);

    T7Log_Write("[Blobs] %llu s into the level: stream pool %llu MB with %llu MB free; in use by lighting: probe %u MB, volume %u MB, "
                "sun shadow tree %u MB, sky %u MB",
                (unsigned long long)seconds, (unsigned long long)(*(const uint64_t*)(g_base + kStreamPool) >> 4),
                (unsigned long long)(*(const uint64_t*)(g_base + kStreamPool + 8) >> 4),
                (uint32_t)(*(const uint64_t*)(g_base + kBlobMegabytes) >> 20),
                (uint32_t)(*(const uint64_t*)(g_base + kBlobMegabytes + 8) >> 20),
                (uint32_t)(*(const uint64_t*)(g_base + kBlobMegabytes + 16) >> 20),
                (uint32_t)(*(const uint64_t*)(g_base + kBlobMegabytes + 32) >> 20));

    for (int i = 0; i < kStreamBlobCount; ++i)
    {
        const uintptr_t entry = g_base + kStreamBlobs + 56 * (uintptr_t)i;
        const uint32_t state = *(const uint32_t*)(entry + 32);

        if (!state)
            continue;

        const uint32_t category = *(const uint8_t*)(entry + 37) < 5 ? *(const uint8_t*)(entry + 37) : 3;
        const uintptr_t item = *(const uintptr_t*)(entry + 48);
        const bool known = item >= 0x10000 && item < 0x800000000000ull && RangeReadable(item, 64);
        const uint64_t used = *(const uint64_t*)(entry + 24);
        char raw[2 * 64 + 1] = {};

        if (known)
        {
            static const char k_hex[] = "0123456789abcdef";

            for (int b = 0; b < 64; ++b)
            {
                raw[2 * b] = k_hex[*(const uint8_t*)(item + b) >> 4];
                raw[2 * b + 1] = k_hex[*(const uint8_t*)(item + b) & 15];
            }
        }

        T7Log_Write("[Blobs] %d: %s, state %u (%s), %s, last wanted %llu ticks ago, %u bytes, entry %08x %08x %016llx, item %s",
                    i, k_names[category], state, state >= 3 ? "in memory" : state == 2 ? "loading" : "not loaded",
                    *(const uint32_t*)(entry + 8) == *(const uint32_t*)(g_base + kBlobGenerations + 4 * category) ? "wanted now"
                                                                                                                 : "not wanted",
                    (unsigned long long)(tick >= used ? tick - used : 0), known ? *(const uint32_t*)(item + 52) : 0,
                    *(const uint32_t*)entry, *(const uint32_t*)(entry + 4), (unsigned long long)*(const uint64_t*)(entry + 40),
                    known ? raw : "?");
    }
}

static void NoteDroppedSpawner(uint64_t element, uint32_t time, int32_t asked)
{
    if (g_spawnersDropped.fetch_add(1, std::memory_order_relaxed))
        return;

    T7Log_Write("[GpuFx] an effect element (definition %llX) asked the GPU for %d particles at time %u; requests with nothing to spawn "
                "are left out, as the CPU particle path does, and counted in the [Frame] lines",
                (unsigned long long)element, asked, time);
}

static uint64_t Dispatch_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, double x0, double x1,
                           double x2, double x3, double x4, double x5, double x6, double x7)
{
    if (a1 && g_usermapLevel.load(std::memory_order_relaxed))
    {
        uintptr_t commands = *(const uintptr_t*)(a1 + kContextDeferred);

        if (!commands)
            commands = *(const uintptr_t*)(a1 + kContextImmediate);

        if (commands)
        {
            Dispatch& kept = g_dispatches[g_dispatchCount.fetch_add(1, std::memory_order_relaxed) % kDispatchesKept];

            kept.commands = commands;
            kept.label = *(const uintptr_t*)(commands + kCommandsLabel);
            kept.number = *(const uint32_t*)(commands + kCommandsIssued) + 1;
            kept.caller = (uintptr_t)__builtin_return_address(0);
            kept.shader = *(const uintptr_t*)(a1 + kContextCompute);
            kept.x = (uint32_t)a2;
            kept.y = (uint32_t)a3;
            kept.z = (uint32_t)a4;
            kept.frame = g_levelFrames.load(std::memory_order_relaxed);
        }
    }

    return ((Hook_t)g_dispatchOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static void ReportLine(const char* format, ...);

static void ReportDispatches()
{
    const uint32_t count = g_dispatchCount.load(std::memory_order_relaxed);
    const uint32_t kept = count < kDispatchesKept ? count : kDispatchesKept;

    if (!g_dispatchOriginal)
    {
        ReportLine("[Stall] compute jobs are not recorded: the dispatch routine did not match");
        return;
    }

    ReportLine("[Stall] %u compute jobs recorded this level, the last %u kept; %u GPU particle requests with nothing to spawn left out",
               count, kept, g_spawnersDropped.load(std::memory_order_relaxed));

    uintptr_t seen[16] = {};
    int contexts = 0;

    for (uint32_t back = 0; back < kept; ++back)
    {
        const Dispatch& job = g_dispatches[(count - 1 - back) % kDispatchesKept];
        bool known = false;

        for (int i = 0; i < contexts; ++i)
            known = known || seen[i] == job.label;

        if (known || contexts >= 16 || job.label < 0x10000 || job.label >= 0x800000000000ull || !RangeReadable(job.label, 4))
            continue;

        seen[contexts++] = job.label;

        const uint32_t done = *(const uint32_t*)job.label;
        ReportLine("[Stall] command list %llX (label %llX): the GPU has finished compute job %u; the newest sent there is %u",
                   (unsigned long long)job.commands, (unsigned long long)job.label, done, job.number);
    }

    for (uint32_t back = 0; back < kept && back < 96; ++back)
    {
        const Dispatch& job = g_dispatches[(count - 1 - back) % kDispatchesKept];
        const bool readable = job.label >= 0x10000 && job.label < 0x800000000000ull && RangeReadable(job.label, 4);
        const uint32_t done = readable ? *(const uint32_t*)job.label : 0;

        ReportLine("[Stall] compute job %u on %llX: sent from %llX, %u x %u x %u groups, shader %llX, client frame %u%s",
                   job.number, (unsigned long long)job.commands, (unsigned long long)(job.caller - g_base), job.x, job.y, job.z,
                   (unsigned long long)job.shader, job.frame, !readable ? "" : job.number > done ? ", NOT finished" : "");
    }
}

static void StallSignal(int, void* context)
{
    if (!context || (uintptr_t)scePthreadSelf() != g_stallTarget.load(std::memory_order_acquire))
        return;

    int expected = 1;

    if (!g_stallState.compare_exchange_strong(expected, 3, std::memory_order_acq_rel))
        return;

    const uint64_t* const words = (const uint64_t*)context;
    StallSample& sample = g_stallSample;
    memcpy(sample.raw, words, sizeof(sample.raw));
    sample.mcontext = -1;
    sample.words = 0;

    for (int i = 21; i + 3 < kStallRawWords; ++i)
    {
        if (words[i] == 0x43 && words[i + 3] == 0x3B)
        {
            sample.mcontext = i - 21;
            break;
        }
    }

    if (sample.mcontext >= 0)
    {
        sample.rip = words[sample.mcontext + 20];
        sample.rbp = words[sample.mcontext + 9];
        sample.rsp = words[sample.mcontext + 23];

        const uintptr_t rsp = sample.rsp & ~(uintptr_t)7;
        OrbisKernelVirtualQueryInfo info;

        if (rsp >= 0x10000 && rsp < 0x800000000000ull && sceKernelVirtualQuery((void*)rsp, 0, &info, sizeof(info)) >= 0 &&
            (info.unk04 & 1) && (uintptr_t)info.unk01 <= rsp && rsp < (uintptr_t)info.unk02)
        {
            size_t bytes = (uintptr_t)info.unk02 - rsp;

            if (bytes > sizeof(sample.stack))
                bytes = sizeof(sample.stack);

            memcpy(sample.stack, (const void*)rsp, bytes);
            sample.words = bytes / 8;
        }
    }

    g_stallState.store(2, std::memory_order_release);
}

static bool CallPrecedes(uint64_t value)
{
    if (value < g_base + 16 || value >= g_base + kCodeEnd)
        return false;

    const uint8_t* const p = (const uint8_t*)value;
    return p[-5] == 0xE8 || (p[-2] == 0xFF && ((p[-1] & 0xF8) == 0xD0 || (p[-1] & 0xF8) == 0x10)) ||
           (p[-3] == 0xFF && ((p[-2] & 0xF8) == 0x50 || p[-2] == 0x14)) || (p[-4] == 0xFF && p[-3] == 0x54) ||
           (p[-6] == 0xFF && ((p[-5] & 0xF8) == 0x90 || p[-5] == 0x15)) || (p[-7] == 0xFF && p[-6] == 0x94);
}

static void AppendAddress(char* out, size_t size, size_t* used, uint64_t value)
{
    OrbisKernelVirtualQueryInfo info;

    if (value >= g_base && value < g_base + kCodeEnd)
        Appendf(out, size, used, " %llX", (unsigned long long)(value - g_base));
    else if (value >= g_sprxStart && value < g_sprxEnd)
        Appendf(out, size, used, " sprx+%llX", (unsigned long long)(value - g_sprxStart));
    else if (value >= 0x10000 && value < 0x800000000000ull && sceKernelVirtualQuery((void*)value, 0, &info, sizeof(info)) >= 0 &&
             (uintptr_t)info.unk01 <= value && value < (uintptr_t)info.unk02)
        Appendf(out, size, used, " %.32s+%llX", info.name, (unsigned long long)(value - (uintptr_t)info.unk01));
    else
        Appendf(out, size, used, " ?%llX", (unsigned long long)value);
}

static void ReportLine(const char* format, ...)
{
    char line[2048];
    const uint64_t now = sceKernelGetProcessTime();
    const int stamp = snprintf(line, sizeof(line), "[%02u:%02u.%03u] ", (unsigned)(now / 60000000ull), (unsigned)(now / 1000000ull % 60),
                               (unsigned)(now / 1000ull % 1000));

    if (stamp <= 0)
        return;

    const size_t room = sizeof(line) - (size_t)stamp - 1;
    va_list args;
    va_start(args, format);
    const int body = vsnprintf(line + stamp, room, format, args);
    va_end(args);

    if (body < 0)
        return;

    size_t length = (size_t)stamp + ((size_t)body < room ? (size_t)body : room - 1);
    line[length++] = '\n';

    const size_t at = g_reportUsed.load(std::memory_order_relaxed);

    if (at + length > sizeof(g_report))
        return;

    memcpy(g_report + at, line, length);
    g_reportUsed.store(at + length, std::memory_order_release);
}

static void RaiseThread()
{
    const OrbisPthread self = scePthreadSelf();
    scePthreadSetprio(self, SCE_KERNEL_PRIO_FIFO_HIGHEST);

    if (scePthreadSetaffinity(self, 0x7F) != 0)
        scePthreadSetaffinity(self, 0x3F);
}

static void LogStallSample(const char* name)
{
    const StallSample& sample = g_stallSample;
    char line[1900];
    size_t used = 0;

    if (sample.mcontext < 0)
    {
        Appendf(line, sizeof(line), &used, "[Stall] %s: no registers found in its context; first words:", name);

        for (int i = 0; i < kStallRawWords; ++i)
            Appendf(line, sizeof(line), &used, " %llX", (unsigned long long)sample.raw[i]);

        ReportLine("%s", line);
        return;
    }

    Appendf(line, sizeof(line), &used, "[Stall] %s: at", name);
    AppendAddress(line, sizeof(line), &used, sample.rip);
    Appendf(line, sizeof(line), &used, " (rsp %llX, %llu KB of stack read); frame chain:", (unsigned long long)sample.rsp,
            (unsigned long long)(sample.words / 128));

    const uintptr_t rsp = sample.rsp & ~(uintptr_t)7;
    uintptr_t rbp = sample.rbp;

    for (int frame = 0; frame < kStallFrames; ++frame)
    {
        if (rbp < rsp || (rbp & 7) || (rbp - rsp) / 8 + 1 >= sample.words)
            break;

        const uint64_t at = (rbp - rsp) / 8;
        AppendAddress(line, sizeof(line), &used, sample.stack[at + 1]);

        if (sample.stack[at] <= rbp)
            break;

        rbp = sample.stack[at];
    }

    ReportLine("%s", line);

    used = 0;
    Appendf(line, sizeof(line), &used, "[Stall] %s: game calls on its stack (return@offset):", name);
    int shown = 0;

    for (uint64_t i = 0; i < sample.words && shown < kStallReturns; ++i)
    {
        if (!CallPrecedes(sample.stack[i]))
            continue;

        Appendf(line, sizeof(line), &used, " %llX@%llX", (unsigned long long)(sample.stack[i] - g_base), (unsigned long long)(i * 8));
        ++shown;
    }

    ReportLine("%s", line);
}

static bool ThreadStack(int index, uintptr_t thread, uintptr_t* low, uintptr_t* high)
{
    OrbisPthreadAttr attr;
    void* address = nullptr;
    size_t size = 0;

    if (scePthreadAttrInit(&attr) == 0)
    {
        if (scePthreadAttrGet((OrbisPthread)thread, &attr) != 0 || scePthreadAttrGetstack(&attr, &address, &size) != 0)
        {
            address = nullptr;
            size = 0;
        }

        scePthreadAttrDestroy(&attr);
    }

    const uintptr_t start = (uintptr_t)address;
    uintptr_t probe = start && size ? start + size - 8 : 0;
    const uintptr_t mainProbe = index == 0 ? g_mainStackProbe.load(std::memory_order_relaxed) : 0;

    if (mainProbe && (!probe || mainProbe < start || mainProbe >= start + size))
        probe = mainProbe;

    OrbisKernelVirtualQueryInfo info;

    if (!probe || sceKernelVirtualQuery((void*)probe, 0, &info, sizeof(info)) < 0 || !(info.unk04 & 1))
        return false;

    *low = (uintptr_t)info.unk01;
    *high = (uintptr_t)info.unk02;

    if (probe != mainProbe)
    {
        if (start > *low)
            *low = start;

        if (start + size < *high)
            *high = start + size;
    }

    return *high > *low + 64;
}

static bool InSprx(uintptr_t value)
{
    return g_sprxStart && value >= g_sprxStart && value < g_sprxEnd;
}

static uintptr_t FindChild(uintptr_t frame, uintptr_t low, uintptr_t high, bool foreign)
{
    const uintptr_t stop = frame - low > kFrameScanBytes ? frame - kFrameScanBytes : low;

    for (uintptr_t slot = frame - 16; slot >= stop; slot -= 8)
    {
        if (*(const uintptr_t*)slot != frame)
            continue;

        const uintptr_t ret = *(const uintptr_t*)(slot + 8);
        const bool code = CallPrecedes(ret) || InSprx(ret);
        const bool other = ret >= 0x10000 && ret < 0x800000000000ull && (ret < low || ret >= high) &&
                           !(ret >= g_base && ret < g_base + kCodeEnd) && !InSprx(ret);

        if (foreign ? other : code)
            return slot;
    }

    return 0;
}

static void LogStackWalk(int index, uintptr_t thread)
{
    uintptr_t low = 0;
    uintptr_t high = 0;

    if (!ThreadStack(index, thread, &low, &high))
    {
        ReportLine("[Stall] %s: its stack could not be found", k_stallThreads[index]);
        return;
    }

    const uintptr_t rootReturn = g_base + (index == 0 ? kMainReturn : kThreadReturn);
    const uintptr_t floor = high - low > kRootScanBytes * 16 ? high - kRootScanBytes * 16 : low;
    uintptr_t frame = 0;

    for (uintptr_t slot = (high - 16) & ~(uintptr_t)7; slot >= floor; slot -= 8)
    {
        const uintptr_t saved = *(const uintptr_t*)slot;

        if (*(const uintptr_t*)(slot + 8) == rootReturn && saved > slot && saved < high)
        {
            frame = slot;
            break;
        }
    }

    char line[1900];
    size_t used = 0;
    Appendf(line, sizeof(line), &used, "[Stall] %s: stack %llX-%llX, live calls from its start:", k_stallThreads[index],
            (unsigned long long)low, (unsigned long long)high);

    if (!frame)
    {
        Appendf(line, sizeof(line), &used, " start frame not found");
        ReportLine("%s", line);
        return;
    }

    for (int frames = 0; frames < kWalkFrames; ++frames)
    {
        uintptr_t child = FindChild(frame, low, high, false);
        const bool last = !child;

        if (last)
            child = FindChild(frame, low, high, true);

        if (!child)
            break;

        AppendAddress(line, sizeof(line), &used, *(const uintptr_t*)(child + 8));
        frame = child;

        if (last)
            break;
    }

    Appendf(line, sizeof(line), &used, " (innermost frame %llX, %llu bytes below the top)", (unsigned long long)frame,
            (unsigned long long)(high - frame));
    ReportLine("%s", line);
}

static void SampleStalledThreads(uint32_t round, uint64_t stalledMs)
{
    uint32_t count[5] = {};
    uint32_t loaded[5] = {};
    CountBlobs(count, loaded);

    ReportLine("[Stall] round %u: no client frame for %llu ms; %u GetPlayers calls since the last heartbeat; streamer flags %u %u; "
                "blobs loaded: probe %u of %u, volume %u of %u, sun shadow tree %u of %u, sky %u of %u",
                round, (unsigned long long)stalledMs, g_serverCalls.load(std::memory_order_relaxed),
                *(const uint8_t*)(g_base + kStreamStarted), *(const uint8_t*)(g_base + kStreamDone), loaded[0], count[0], loaded[1],
                count[1], loaded[2], count[2], loaded[4], count[4]);

    const uintptr_t label = *(const uintptr_t*)(g_base + kGpuFrameLabel);
    const uintptr_t data = *(const uintptr_t*)(g_base + kFrontendData);
    const uintptr_t other = g_base + (data == g_base + kFrameDataA ? kFrameDataB : kFrameDataA);

    if (label >= 0x10000 && label < 0x800000000000ull && RangeReadable(label, 4))
        ReportLine("[Stall] the GPU has finished frame %u; the main thread waits for frame %u (the frame being built is %u)",
                   *(const uint32_t*)label, *(const uint32_t*)(other + kFrameLabelWanted),
                   data ? *(const uint32_t*)(data + kFrameLabelWanted) : 0);

    for (int i = 0; i < kStreamBlobCount; ++i)
    {
        const uintptr_t entry = g_base + kStreamBlobs + 56 * (uintptr_t)i;
        const uint32_t state = *(const uint32_t*)(entry + 32);
        const uintptr_t item = *(const uintptr_t*)(entry + 48);

        if (!state || item < 0x10000 || item >= 0x800000000000ull || *(const uint32_t*)(item + 52) < (1u << 20))
            continue;

        const uint32_t category = *(const uint8_t*)(entry + 37) < 5 ? *(const uint8_t*)(entry + 37) : 3;

        ReportLine("[Stall] lighting item %d: category %u, %u MB, state %u, %s, %s", i, category, *(const uint32_t*)(item + 52) >> 20,
                   state, *(const uint32_t*)(entry + 8) == *(const uint32_t*)(g_base + kBlobGenerations + 4 * category) ? "wanted" : "not wanted",
                   (*(const uint8_t*)(item + 32) & 0x80) ? "streamed" : "not streamed");
    }

    if (round == 1)
        ReportDispatches();

    for (int i = 0; i < kStallThreads; ++i)
    {
        const uintptr_t thread = *(const uintptr_t*)(g_base + kThreadHandles + 8 * (uintptr_t)i);

        if (!thread)
            continue;

        LogStackWalk(i, thread);

        if (!g_raiseException)
            continue;

        g_stallTarget.store(thread, std::memory_order_release);
        g_stallState.store(1, std::memory_order_release);

        const int raised = g_raiseException((OrbisPthread)thread, kStallSignal);
        uint64_t waited = 0;

        while (raised == 0 && g_stallState.load(std::memory_order_acquire) != 2 && waited < kStallAnswerUs)
        {
            sceKernelUsleep(2000);
            waited += 2000;
        }

        int expected = 1;

        if (g_stallState.compare_exchange_strong(expected, 0, std::memory_order_acq_rel))
        {
            if (raised != 0)
                ReportLine("[Stall] %s: could not be signalled (0x%08X)", k_stallThreads[i], (uint32_t)raised);
            else
                ReportLine("[Stall] %s: did not answer within %llu ms", k_stallThreads[i], (unsigned long long)(kStallAnswerUs / 1000));

            continue;
        }

        while (g_stallState.load(std::memory_order_acquire) == 3)
            sceKernelUsleep(1000);

        LogStallSample(k_stallThreads[i]);
        g_stallState.store(0, std::memory_order_release);
    }

    g_stallTarget.store(0, std::memory_order_release);
}

static void* StallWatch(void*)
{
    RaiseThread();

    uint64_t level = 0;
    uint64_t lastRound = 0;
    uint32_t rounds = 0;

    for (;;)
    {
        sceKernelUsleep(250000);

        const uint64_t start = g_levelStart.load(std::memory_order_relaxed);

        if (start != level)
        {
            level = start;
            lastRound = 0;
            rounds = 0;
        }

        if (!start || !*(volatile bool*)&g_inLevel || !g_usermapLevel.load(std::memory_order_relaxed) || rounds >= kStallRounds ||
            g_levelFrames.load(std::memory_order_relaxed) < kStallArmFrames)
            continue;

        const uint64_t now = sceKernelGetProcessTime();
        const uint64_t last = g_lastFrameAt.load(std::memory_order_relaxed);

        if (!last || now - last < kStallUs || (lastRound && now - lastRound < kStallGapUs))
            continue;

        lastRound = now;
        SampleStalledThreads(++rounds, (now - last) / 1000);
    }

    return nullptr;
}

static void* StallMirror(void*)
{
    RaiseThread();

    size_t written = 0;

    for (;;)
    {
        sceKernelUsleep(500000);

        const size_t used = g_reportUsed.load(std::memory_order_acquire);

        while (written < used)
        {
            const char* const line = g_report + written;
            const char* const end = (const char*)memchr(line, '\n', used - written);

            if (!end)
                break;

            T7Log_Write("%.*s", (int)(end - line), line);
            written = (size_t)(end - g_report) + 1;
        }
    }

    return nullptr;
}

static uintptr_t ImportTarget(uintptr_t target)
{
    if (InSprx(target) && target + 6 <= g_sprxEnd)
    {
        const uint8_t* const stub = (const uint8_t*)target;

        if (stub[0] != 0xFF || stub[1] != 0x25)
            return 0;

        const uintptr_t slot = target + 6 + (intptr_t)*(const int32_t*)(stub + 2);
        target = RangeReadable(slot, 8) ? *(const uintptr_t*)slot : 0;
    }

    if (target < 0x10000 || target >= 0x800000000000ull || InSprx(target) || (target >= g_base && target < g_base + kCodeEnd))
        return 0;

    return target;
}

static bool Near(uintptr_t a, uintptr_t b)
{
    return (a > b ? a - b : b - a) < kKernelSpan;
}

static bool UsablePair(uintptr_t install, uintptr_t raise, uintptr_t self)
{
    return install && raise && self && install != raise && Near(install, self) && Near(raise, self);
}

static void StartStallWatch()
{
    static bool started = false;

    if (started)
        return;

    started = true;

    OrbisKernelVirtualQueryInfo info;

    if (sceKernelVirtualQuery((void*)&StallSignal, 0, &info, sizeof(info)) >= 0 && (info.unk04 & 4))
    {
        g_sprxStart = (uintptr_t)info.unk01;
        g_sprxEnd = (uintptr_t)info.unk02;
    }

    const uintptr_t self = g_sprxStart ? ImportTarget((uintptr_t)&scePthreadSelf) : 0;
    const uintptr_t unityInstall = g_sprxStart ? ImportTarget((uintptr_t)&T7Unity_InstallExceptionHandler) : 0;
    const uintptr_t unityRaise = g_sprxStart ? ImportTarget((uintptr_t)&T7Unity_RaiseException) : 0;
    const uintptr_t kernelInstall = g_sprxStart ? ImportTarget((uintptr_t)&sceKernelInstallExceptionHandler) : 0;
    const uintptr_t kernelRaise = g_sprxStart ? ImportTarget((uintptr_t)&sceKernelRaiseException) : 0;

    T7Log_Write("[Stall] this module %llX-%llX; scePthreadSelf %llX; exception functions: libkernel_unity %llX %llX, libkernel %llX %llX",
                (unsigned long long)g_sprxStart, (unsigned long long)g_sprxEnd, (unsigned long long)self,
                (unsigned long long)unityInstall, (unsigned long long)unityRaise, (unsigned long long)kernelInstall,
                (unsigned long long)kernelRaise);

    uintptr_t install = 0;
    uintptr_t raise = 0;
    const char* from = nullptr;

    if (UsablePair(unityInstall, unityRaise, self))
    {
        install = unityInstall;
        raise = unityRaise;
        from = "libkernel_unity";
    }
    else if (UsablePair(kernelInstall, kernelRaise, self))
    {
        install = kernelInstall;
        raise = kernelRaise;
        from = "libkernel";
    }

    if (install)
    {
        const int installed = ((InstallHandler_t)install)(kStallSignal, StallSignal);

        if (installed == 0)
            g_raiseException = (RaiseException_t)raise;
        else
            T7Log_Write("[Stall] installing the exception handler through %s failed (0x%08X)", from, (uint32_t)installed);
    }

    OrbisPthread thread;
    const int created = scePthreadCreate(&thread, nullptr, StallWatch, nullptr, "BO3-Customs Stall");
    const int mirrored = scePthreadCreate(&thread, nullptr, StallMirror, nullptr, "BO3-Customs Stall Log");
    const char* const state = created == 0 && mirrored == 0 ? "running" : "NOT started";

    if (g_raiseException)
        T7Log_Write("[Stall] watchdog %s (threads are sampled by a signal through %s, plus stack walks)", state, from);
    else
        T7Log_Write("[Stall] watchdog %s (threads are sampled by stack walks only)", state);
}

static void FrameHeartbeat()
{
    g_clientFrames.fetch_add(1, std::memory_order_relaxed);
    g_levelFrames.fetch_add(1, std::memory_order_relaxed);

    const uint64_t now = sceKernelGetProcessTime();
    g_lastFrameAt.store(now, std::memory_order_relaxed);

    if (!g_mainStackProbe.load(std::memory_order_relaxed))
        g_mainStackProbe.store((uintptr_t)__builtin_frame_address(0), std::memory_order_relaxed);

    if (!g_usermapLevel.load(std::memory_order_relaxed) || g_heartbeats.load(std::memory_order_relaxed) >= kHeartbeats)
        return;

    uint64_t at = g_heartbeatAt.load(std::memory_order_relaxed);

    if (!at)
    {
        g_heartbeatAt.compare_exchange_strong(at, now, std::memory_order_relaxed);
        return;
    }

    if (now - at < kHeartbeatUs || !g_heartbeatAt.compare_exchange_strong(at, now, std::memory_order_relaxed))
        return;

    g_heartbeats.fetch_add(1, std::memory_order_relaxed);

    uint32_t count[5] = {};
    uint32_t loaded[5] = {};
    CountBlobs(count, loaded);

    const uint64_t start = g_levelStart.load(std::memory_order_relaxed);

    T7Log_Write("[Frame] %llu s into the level: %u client frames and %u GetPlayers calls in %llu ms; player 0 is %s; streamer "
                "flags %u %u; blobs loaded: probe %u of %u, volume %u of %u, sun shadow tree %u of %u, sky %u of %u; %u GPU particle "
                "requests with nothing to spawn left out",
                (unsigned long long)(start ? (now - start) / 1000000 : 0), g_clientFrames.exchange(0, std::memory_order_relaxed),
                g_serverCalls.exchange(0, std::memory_order_relaxed), (unsigned long long)((now - at) / 1000),
                FrozenText(PlayerFrozen(0)), *(const uint8_t*)(g_base + kStreamStarted), *(const uint8_t*)(g_base + kStreamDone),
                loaded[0], count[0], loaded[1], count[1], loaded[2], count[2], loaded[4], count[4],
                g_spawnersDropped.load(std::memory_order_relaxed));

    const uint32_t beat = g_heartbeats.load(std::memory_order_relaxed);

    if (beat == 2 || beat == 10 || beat == 30)
        LogBlobs(start ? (now - start) / 1000000 : 0);
}

static bool WatchBuiltin(uintptr_t row, uint32_t hash, uintptr_t handler, void* hook, void** original)
{
    if (*original)
        return true;

    if (!RangeReadable(g_base + row, 32) || *(const uint32_t*)(g_base + row) != hash ||
        *(const uintptr_t*)(g_base + row + 16) != g_base + handler)
        return false;

    *original = (void*)(g_base + handler);
    *(uintptr_t*)(g_base + row + 16) = (uintptr_t)hook;
    return true;
}

static void WatchBuiltins(bool report)
{
    const struct
    {
        uintptr_t row;
        uint32_t  hash;
        uintptr_t handler;
        void*     hook;
        void**    original;
    } builtins[] =
    {
        { kOpenMenuRow, 0x4AA9CAAC, 0x9A9640, (void*)OpenMenu_h, &g_openMenuOriginal },
        { kCloseMenuRow, 0x49876A9E, 0x9A9AA0, (void*)CloseMenu_h, &g_closeMenuOriginal },
        { kFreezeControlsRow, 0xFAF8F736, 0x9ABDC0, (void*)FreezeControls_h, &g_freezeOriginal },
        { kUiVisibilityRow, 0x1B947E2A, 0x9B1640, (void*)UiVisibility_h, &g_uiVisibilityOriginal },
        { kGetPlayersRow, 0x3F10449F, 0xA4BF80, (void*)GetPlayers_h, &g_getPlayersOriginal },
    };

    for (const auto& builtin : builtins)
    {
        if (!WatchBuiltin(builtin.row, builtin.hash, builtin.handler, builtin.hook, builtin.original) && report)
            T7Log_Write("[Script] builtin %08X is not where this build expects it - not watched", builtin.hash);
    }
}
#endif

using GpuFxQueue_t = uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

static uint64_t GpuFxQueue_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7)
{
    if ((int32_t)(uint32_t)a6 <= 0)
    {
#ifdef _DEBUG
        NoteDroppedSpawner(a3, (uint32_t)a5, (int32_t)(uint32_t)a6);
#endif
        return 0;
    }

    return ((GpuFxQueue_t)g_gpuFxQueueOriginal)(a1, a2, a3, a4, a5, a6, a7);
}

static uint64_t TexturesLoaded_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                                 double x0, double x1, double x2, double x3, double x4, double x5, double x6,
                                 double x7)
{
    const uint64_t result =
        ((Hook_t)g_texturesOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);

    if (!g_usermapLevel.load(std::memory_order_relaxed))
        return result;

    const uintptr_t top = *(const uintptr_t*)(g_base + kScrVmTop);

    if (!top || !RangeReadable(top, 16) || *(const int32_t*)(top + 8) != kVarInteger)
        return result;

    int32_t* const value = (int32_t*)top;
    const uint64_t now = sceKernelGetProcessTime();
    uint64_t start = g_textureWaitStart.load(std::memory_order_relaxed);

    if (*value != 0)
    {
        if (start && !g_texturesForced.load(std::memory_order_relaxed))
            T7Log_Write("[Stream] the level's textures loaded after %llu ms",
                        (unsigned long long)((now - start) / 1000));

        g_textureWaitStart.store(0, std::memory_order_relaxed);
        return result;
    }

    if (!start)
    {
        g_textureWaitStart.store(now, std::memory_order_relaxed);
        g_gateLogged.store(now, std::memory_order_relaxed);
        g_gateLogs.store(0, std::memory_order_relaxed);
        return result;
    }

    const uint32_t logs = g_gateLogs.load(std::memory_order_relaxed);

    if (now - g_gateLogged.load(std::memory_order_relaxed) >= (logs < 12 ? kGateLogUs : kGateLogSlowUs))
    {
        g_gateLogged.store(now, std::memory_order_relaxed);
        g_gateLogs.store(logs + 1, std::memory_order_relaxed);
        LogStreamGate(now - start, logs % 6 == 1);
    }

    if (now - start < kTextureWaitUs)
        return result;

    *value = 1;

    if (!g_texturesForced.exchange(true, std::memory_order_relaxed))
    {
        T7Log_Write("[Stream] textures still loading after %llu s (streamer flags %u %u, pending %d, override %u) - "
                    "the level starts and they keep streaming in",
                    (unsigned long long)(kTextureWaitUs / 1000000), *(const uint8_t*)(g_base + kStreamStarted),
                    *(const uint8_t*)(g_base + kStreamDone), *(const int32_t*)(g_base + kStreamPending),
                    *(const uint8_t*)(g_base + kStreamOverride));
        LogStreamGate(now - start, true);
    }

    return result;
}

static uint64_t DemoStart_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                            double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    if (g_usermapLevel.load(std::memory_order_relaxed))
        return 0;

    return ((Hook_t)g_demoStartOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static bool ZoneLoaded(const char* name)
{
    const uintptr_t table = g_base + kZoneNames;

    for (int i = 1; i < kZoneNameSlots; ++i)
    {
        const uintptr_t entry = table + kZoneNameStride * (uintptr_t)i;
        const int32_t state = *(const int32_t*)(entry + kZoneStateAt);

        if ((uint32_t)(state - 1) <= 2 && EqualsNoCase((const char*)entry, name))
            return true;
    }

    return false;
}

static int LoadedModeCommon()
{
    for (int k = 1; k < kModZoneKinds; ++k)
    {
        if (ZoneLoaded(k_modeCommons[k]))
            return k;
    }

    return -1;
}

static bool ModHooksReady()
{
    return g_patchZonesOriginal != nullptr && g_priorityOriginal != nullptr;
}

static bool ModeCommonOnTop()
{
    const int32_t count = *(const int32_t*)(g_base + kZoneRecordCount);

    for (int32_t i = count - 1; i >= 0 && i < 64; --i)
    {
        const uint32_t flags = *(const uint32_t*)(g_base + kZoneRecords + kZoneRecordStride * (uintptr_t)i + 4);
        const uint32_t pooled = flags & kPoolFlagMask;

        if (pooled < kFirstPoolEnd)
            return pooled == (uint32_t)kModeCommonFlags;
    }

    return false;
}

static bool StackHooksReady()
{
    return ModHooksReady() && g_linkOriginal != nullptr && g_xpakOpenOriginal != nullptr &&
           g_loadXFileOriginal != nullptr;
}

static bool AnyModZoneLoaded()
{
    for (int i = 0; i < g_stackCount; ++i)
    {
        for (int k = 0; k < kModZoneKinds; ++k)
        {
            if (ZoneLoaded(g_stack[i].zones[k]))
                return true;
        }
    }

    for (int k = 0; k < kModZoneKinds; ++k)
    {
        if (ZoneLoaded(k_modZones[k]))
            return true;
    }

    return false;
}

static int StackEntries(int from, int mode, ZoneInfo* out, int room)
{
    int n = 0;

    for (int i = from; i < g_stackCount; ++i)
    {
        const uint32_t rank = (uint32_t)i < kModRankMax ? (uint32_t)i : kModRankMax;
        const int32_t flags = (int32_t)((uint32_t)kModZoneFlags | (rank << kModRankShift));

        for (int k = 0; k < kModZoneKinds; ++k)
        {
            if (!g_stack[i].mod.zones[k] || (k != 0 && k != mode))
                continue;

            if (n >= room)
            {
                T7Log_Write("[Mods] no room for %s in this zone load", g_stack[i].zones[k]);
                continue;
            }

            out[n].name = g_stack[i].zones[k];
            out[n].allocFlags = flags;
            out[n].freeFlags = kLoadWithoutFree;
            out[n].allocSlot = 0;
            out[n].freeSlot = 0;
            out[n].buffer = 0;
            out[n].bufferSize = 0;
            ++n;
        }
    }

    return n;
}

static void SetLoaded(int count)
{
    g_loadedCount = 0;

    for (int i = 0; i < count && i < g_stackCount; ++i)
        snprintf(g_loaded[g_loadedCount++], kNameMax, "%s", g_stack[i].mod.name);

    g_modsLuaDirty.store(true);
}

static int ChosenForLoad(char (*names)[kNameMax])
{
    const int limit = StackHooksReady() ? kMaxStack : 1;
    const bool session = g_sessionModCount >= 0;
    const char (*const wanted)[kNameMax] = session ? g_sessionMods : g_chosen;
    const int wantedCount = session ? g_sessionModCount : g_chosenCount;
    int count = 0;

    for (int i = 0; i < wantedCount && count < limit; ++i)
    {
        if (FindMod(wanted[i]) >= 0)
            snprintf(names[count++], kNameMax, "%s", wanted[i]);
    }

    return count;
}

static int AddModZones(const ZoneInfo* zones, int count, ZoneInfo* out)
{
    int mode = -1;

    for (int i = 0; i < count && mode < 0; ++i)
    {
        if (!zones[i].name || zones[i].allocFlags != kModeCommonFlags)
            continue;

        for (int k = 1; k < kModZoneKinds && mode < 0; ++k)
        {
            if (EqualsNoCase(zones[i].name, k_modeCommons[k]))
                mode = k;
        }
    }

    if (mode < 0 || !ModHooksReady())
        return 0;

    for (int i = 0; i < count; ++i)
    {
        if (zones[i].name && IsModZoneName(zones[i].name))
            return 0;
    }

    char names[kMaxStack][kNameMax];
    const int wanted = ChosenForLoad(names);

    if (wanted == 0)
    {
        if (g_loadedCount > 0)
        {
            T7Log_Write("[Mods] %d mod(s) unload; %s reloads without mods", g_loadedCount, k_modeCommons[mode]);
            g_loadedCount = 0;
            g_modsLuaDirty.store(true);
        }

        ResetScriptClaims();

        if (g_chosenCount > 0)
            T7Log_Write("[Mods] the chosen mods are not installed; %s loads without them", k_modeCommons[mode]);

        return 0;
    }

    const int room = (kExpandedZones - 4 * count) / 2;

    if (room <= 0 || count + room > kMaxZoneCall)
        return 0;

    BeginListWrite();
    SetStack(names, wanted);
    EndListWrite();
    ResetScriptClaims();

    memcpy(out, zones, (size_t)count * sizeof(ZoneInfo));
    const int added = StackEntries(0, mode, out + count, room);
    SetLoaded(added > 0 ? g_stackCount : 0);

    for (int i = 0; i < added; ++i)
        T7Log_Write("[Mods] %s loads with %s", out[count + i].name, k_modeCommons[mode]);

    return added;
}

static bool FreesModZones(const ZoneInfo* zones, int count)
{
    for (int i = 0; i < count; ++i)
    {
        const uint32_t alloc = (uint32_t)zones[i].allocFlags;
        const uint32_t free = (uint32_t)zones[i].freeFlags;

        if (free & (uint32_t)kLoadWithoutFree)
            continue;

        uint32_t low = free & 0x1FF;

        if (alloc & 0x7FFFFFF)
            low = (alloc & 0x3FFFE00) ? 0 : (alloc & 0x1FF);

        if (low && (low & (0u - low)) <= (uint32_t)kModeCommonFlags)
            return true;
    }

    return false;
}

static void WaitForModZones(const ZoneInfo* zones, int count, int from)
{
    if (!g_reloadCheck)
    {
        g_waitZoneCount = 0;
        g_waitFrom = from;
    }
    else if (from < g_waitFrom)
    {
        g_waitFrom = from;
    }

    for (int i = 0; i < count && g_waitZoneCount < (int)(sizeof(g_waitZones) / sizeof(g_waitZones[0])); ++i)
    {
        bool known = !zones[i].name;

        for (int k = 0; k < g_waitZoneCount && !known; ++k)
            known = EqualsNoCase(g_waitZones[k], zones[i].name);

        if (!known)
            snprintf(g_waitZones[g_waitZoneCount++], kNameMax, "%s", zones[i].name);
    }

    if (g_waitZoneCount > 0 && !g_inLevel)
    {
        g_reloadCheck = true;
        g_reloadSince = sceKernelGetProcessTime();
    }
}

static uint64_t LoadXAssets_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                              double x0, double x1, double x2, double x3, double x4, double x5, double x6,
                              double x7)
{
    ZoneInfo withMods[kMaxZoneCall];
    const uint64_t self = (uint64_t)scePthreadSelf();

    if (a1 && (int)(uint32_t)a2 > 0 && (int)(uint32_t)a2 <= kMaxZoneCall && (g_modCount > 0 || g_loadedCount > 0) &&
        g_plainZoneThread.load(std::memory_order_acquire) != self)
    {
        const bool replacing = g_loadedCount > 0 || AnyModZoneLoaded();
        const int added = g_modCount > 0 ? AddModZones((const ZoneInfo*)a1, (int)(uint32_t)a2, withMods) : 0;

        if (added > 0)
            WaitForModZones(withMods + (uint32_t)a2, added, 0);

        if (added > 0 && replacing)
        {
            T7Log_Write("[Mods] the loaded mods unload with this zone change; the ticked mods load right after it");
            g_plainZoneThread.store(self, std::memory_order_release);
            const uint64_t result = LoadXAssets_h(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
            g_plainZoneThread.store(0, std::memory_order_release);
            LoadXAssets_h((uint64_t)(withMods + (uint32_t)a2), (uint64_t)added, a3, a4, a5, a6, x0, x1, x2, x3, x4,
                          x5, x6, x7);
            return result;
        }

        if (added > 0)
        {
            a1 = (uint64_t)withMods;
            a2 = (uint64_t)((uint32_t)a2 + (uint32_t)added);
        }
        else if (g_loadedCount > 0 && FreesModZones((const ZoneInfo*)a1, (int)(uint32_t)a2))
        {
            T7Log_Write("[Mods] %d mod(s) unload with this zone change", g_loadedCount);
            g_loadedCount = 0;
            ResetScriptClaims();
            g_modsLuaDirty.store(true);
        }
    }

    const ZoneInfo* const zones = (const ZoneInfo*)a1;
    const int count = (int)(uint32_t)a2;
    const bool zombies = (*(const uint32_t*)(g_base + kSessionModes) & 0xF) == 0;

    if (zones && count > 0 && count <= kMaxZoneCall)
    {
        ListReader reading;
        bool custom = false;
        bool level = false;

        for (int i = 0; i < count; ++i)
        {
            if (zones[i].allocFlags & 0x800000)
                g_inLevel = false;
            else if (zones[i].allocFlags & 0x100)
                g_inLevel = true;
            else if (zones[i].freeFlags & 0x100)
                g_inLevel = false;

            custom = custom || (zones[i].name && MapPackage(zones[i].name) >= 0);
            level = level || (zones[i].name && (zones[i].allocFlags & 0x100));
        }

        if (level)
        {
            g_usermapLevel.store(custom, std::memory_order_relaxed);
            g_textureWaitStart.store(0, std::memory_order_relaxed);
            g_texturesForced.store(false, std::memory_order_relaxed);
#ifdef _DEBUG
            g_watchLogs.store(0, std::memory_order_relaxed);
            g_heartbeatAt.store(0, std::memory_order_relaxed);
            g_heartbeats.store(0, std::memory_order_relaxed);
            g_serverCalls.store(0, std::memory_order_relaxed);
            g_clientFrames.store(0, std::memory_order_relaxed);
            g_levelFrames.store(0, std::memory_order_relaxed);
            g_lastFrameAt.store(0, std::memory_order_relaxed);
            g_levelStart.store(sceKernelGetProcessTime(), std::memory_order_relaxed);
            g_dvarReads.store(0, std::memory_order_release);

            g_dispatchCount.store(0, std::memory_order_relaxed);
            g_spawnersDropped.store(0, std::memory_order_relaxed);

            for (int32_t& frozen : g_lastFrozen)
                frozen = -1;
#endif
            T7Log_NewLevel();
#ifdef _DEBUG
            WatchBuiltins(true);
#endif
        }

        for (int i = 0; i < count; ++i)
            T7Log_Zone(zones[i].name, zones[i].allocFlags, zones[i].freeFlags);
    }

    int at = -1;

    if (zones && count > 0 && count <= kMaxZoneCall && zombies)
    {
        ListReader reading;

        if (g_levelCommon >= 0)
        {
            for (int i = 0; i < count; ++i)
            {
                const char* const name = zones[i].name;

                if (!name)
                    continue;

                if (EqualsNoCase(name, k_levelCommon))
                {
                    at = -1;
                    break;
                }

                if (at < 0 && MapPackage(name) >= 0)
                    at = i;
            }
        }
    }

    if (at >= 0)
    {
        T7Log_Write("[Zones] %s goes in front of %s", k_levelCommon, zones[at].name);

        ZoneInfo list[kMaxZoneCall + 1];

        memcpy(list, zones, (size_t)at * sizeof(ZoneInfo));
        list[at] = zones[at];
        list[at].name = k_levelCommon;
        list[at].allocFlags |= kLevelCommonFlag;
        memcpy(list + at + 1, zones + at, (size_t)(count - at) * sizeof(ZoneInfo));

        return ((Hook_t)g_loadOriginal)((uint64_t)list, (uint64_t)(count + 1), a3, a4, a5, a6, x0, x1, x2, x3, x4,
                                         x5, x6, x7);
    }

    return ((Hook_t)g_loadOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static int SessionModeCommon()
{
    switch (*(const uint32_t*)(g_base + kSessionModes) & 0xF)
    {
    case 0:
        return 1;
    case 1:
        return 2;
    case 2:
        return 3;
    default:
        return -1;
    }
}

static void ClearAssetsInUse()
{
    const uint32_t* const buckets = (const uint32_t*)(g_base + kAssetHash);
    uint8_t* const entries = (uint8_t*)(g_base + kAssetEntries);

    for (int i = 0; i < kAssetHashSize; ++i)
    {
        for (uint32_t j = buckets[i]; j; j = *(const uint32_t*)(entries + kAssetEntrySize * j + kAssetEntryNext))
            entries[kAssetEntrySize * j + kAssetEntryInUse] = 0;
    }
}

static void AppendMods(const char (*names)[kNameMax], int wanted, int mode)
{
    const int from = g_loadedCount;

    BeginListWrite();
    SetStack(names, wanted);
    EndListWrite();

    if (from == 0)
        ResetScriptClaims();

    ZoneInfo zones[kMaxZoneCall];
    const int count = StackEntries(from, mode, zones, kExpandedZones / 2);
    SetLoaded(g_stackCount);

    if (count <= 0)
    {
        T7Log_Write("[Mods] the newly ticked mods have nothing for %s", k_modeCommons[mode]);
        return;
    }

    for (int i = 0; i < count; ++i)
        T7Log_Write("[Mods] %s loads now (%s is already loaded)", zones[i].name, k_modeCommons[mode]);

    WaitForModZones(zones, count, from);
    LoadXAssets_h((uint64_t)zones, (uint64_t)count, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

static void ApplyModsBeforeLevel(const char* level)
{
    if ((g_modCount == 0 && g_loadedCount == 0) || !g_loadOriginal || !ModHooksReady())
        return;

    char names[kMaxStack][kNameMax];
    const int wanted = ChosenForLoad(names);
    bool same = wanted == g_loadedCount;
    bool extends = g_loadedCount <= wanted;

    for (int i = 0; i < g_loadedCount; ++i)
    {
        const bool match = i < wanted && EqualsNoCase(g_loaded[i], names[i]);
        same = same && match;
        extends = extends && match;
    }

    if (same)
        return;

    const int mode = LoadedModeCommon();

    if (mode < 0 || mode != SessionModeCommon())
    {
        T7Log_Write("[Mods] %s: the ticked mods load with this mode's common zones", level);
        return;
    }

    if (!ModeCommonOnTop())
    {
        T7Log_Write("[Mods] %s starts with the old mods: %s is not on top of its memory pool", level,
                    k_modeCommons[mode]);
        return;
    }

    if (extends && (g_loadedCount > 0 || !AnyModZoneLoaded()))
    {
        T7Log_Write("[Mods] adding %d mod(s) before %s", wanted - g_loadedCount, level);
        AppendMods(names, wanted, mode);
        return;
    }

    T7Log_Write("[Mods] reloading %s before %s so the mod changes apply (%d loaded, %d ticked)", k_modeCommons[mode],
                level, g_loadedCount, wanted);

    if (*(const int32_t*)(g_base + kKeepAssetUse) == 0)
    {
        ((void (*)())(g_base + kRenderSync))();
        ClearAssetsInUse();
    }

    ZoneInfo common{};
    common.name = k_modeCommons[mode];
    common.allocFlags = kModeCommonFlags;
    common.freeFlags = kModeCommonFlags;
    LoadXAssets_h((uint64_t)&common, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

static const char* ModChangesApply()
{
    if (!g_inLevel)
        return "when you close this menu";

    return g_levelZonesOriginal ? "when your next match starts" : "when you switch game mode or restart the game";
}

static int ZoneStateOf(const char* name)
{
    const uintptr_t table = g_base + kZoneNames;

    for (int i = 1; i < kZoneNameSlots; ++i)
    {
        const uintptr_t entry = table + kZoneNameStride * (uintptr_t)i;
        const int32_t state = *(const int32_t*)(entry + kZoneStateAt);

        if (state != 0 && EqualsNoCase((const char*)entry, name))
            return state;
    }

    return 0;
}

static bool AnyZoneLoading()
{
    const uintptr_t table = g_base + kZoneNames;

    for (int i = 1; i < kZoneNameSlots; ++i)
    {
        if (*(const int32_t*)(table + kZoneNameStride * (uintptr_t)i + kZoneStateAt) == 1)
            return true;
    }

    return false;
}

static const char* RecordZoneName(int32_t record)
{
    const uint16_t slot = *(const uint16_t*)(g_base + kZoneRecords + kZoneRecordStride * (uintptr_t)record);

    if (slot == 0 || slot >= kZoneNameSlots)
        return nullptr;

    return (const char*)(g_base + kZoneNames + kZoneNameStride * (uintptr_t)slot);
}

static void RemoveClaimsFrom(int index)
{
    ClaimLock locked;
    int kept = 0;

    for (int i = 0; i < g_scriptClaimCount; ++i)
    {
        if (g_scriptClaims[i].owner >= index)
            continue;

        if (kept != i)
            g_scriptClaims[kept] = g_scriptClaims[i];

        ++kept;
    }

    g_scriptClaimCount = kept;
}

static bool FreeModsFrom(int keep)
{
    const int32_t count = *(const int32_t*)(g_base + kZoneRecordCount);
    int32_t marked[64];
    int markedCount = 0;
    bool top = true;

    for (int32_t i = count - 1; i >= 0; --i)
    {
        if (i >= 64)
            continue;

        const uint32_t flags = *(const uint32_t*)(g_base + kZoneRecords + kZoneRecordStride * (uintptr_t)i + 4);

        if ((flags & kPoolFlagMask) >= kFirstPoolEnd)
            continue;

        if (flags & kUnloadFlag)
        {
            T7Log_Write("[Mods] a level zone is loaded, so the mods cannot unload in the menu");
            return false;
        }

        const char* const name = RecordZoneName(i);
        const int index = (flags & kModMarker) && name ? StackIndexOfZone(name) : -1;

        if (index < keep)
        {
            top = false;
            continue;
        }

        if (!top || markedCount >= 64)
        {
            T7Log_Write("[Mods] %s is not on top of its memory pool, so it cannot unload in the menu", name);
            return false;
        }

        marked[markedCount++] = i;
    }

    if (markedCount == 0)
        return true;

    for (int i = 0; i < markedCount; ++i)
        *(uint32_t*)(g_base + kZoneRecords + kZoneRecordStride * (uintptr_t)marked[i] + 4) |= kUnloadFlag;

    T7Log_Write("[Mods] unloading %d mod zone(s)", markedCount);

    ZoneInfo unload{};
    unload.freeFlags = (int32_t)kUnloadFlag;
    LoadXAssets_h((uint64_t)&unload, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    const int32_t after = *(const int32_t*)(g_base + kZoneRecordCount);
    int left = 0;

    for (int32_t i = 0; i < after && i < 64; ++i)
    {
        uint32_t* const flags = (uint32_t*)(g_base + kZoneRecords + kZoneRecordStride * (uintptr_t)i + 4);

        if ((*flags & kModMarker) && (*flags & kUnloadFlag))
        {
            *flags &= ~kUnloadFlag;
            ++left;
        }
    }

    if (left > 0)
    {
        T7Log_Write("[Mods] %d mod zone(s) did not unload; they stay until your next match", left);
        return false;
    }

    return true;
}

static bool IsUiMod(const char* name)
{
    ClaimLock locked;

    for (int i = 0; i < g_uiModCount; ++i)
    {
        if (EqualsNoCase(g_uiMods[i], name))
            return true;
    }

    return false;
}

static void ReloadFrontend(const char* why)
{
    if (g_inLevel)
        return;

    if (!g_frontendReloadReady)
    {
        T7Log_Write("[Mods] %s; the menus show it after your next match", why);
        return;
    }

    T7Log_Write("[Mods] %s; reloading the menus the way PC does after a mod change", why);

    g_quietReload = g_playMovieOriginal != nullptr;
    ((void (*)(uint64_t))(g_base + kKillServer))(0);
    g_quietReload = false;
    g_unshownCount = 0;
}

static uint64_t PlayMovie_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                            double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    const uint64_t* const movies = (const uint64_t*)(g_base + kFrontendMovies);

    if (g_quietReload && a1 && (a1 == movies[0] || a1 == movies[2]) && movies[1])
    {
        T7Log_Write("[Mods] the menu reload shows the loading spinner instead of %s", (const char*)a1);
        a1 = movies[1];
    }

    return ((Hook_t)g_playMovieOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static void LoadModeCommonForMods()
{
    uint32_t* const session = (uint32_t*)(g_base + kSessionModes);
    const uint32_t saved = *session;

    if ((saved & 0xF) != 3)
    {
        T7Log_Write("[Mods] a game mode is starting; the ticked mods load with it");
        return;
    }

    T7Log_Write("[Mods] no game mode is loaded yet; loading %s now so the ticked mods load right away",
                k_modeCommons[1]);

    ZoneInfo common{};
    common.name = k_modeCommons[1];
    common.allocFlags = kModeCommonFlags;
    common.freeFlags = kModeCommonFlags;

    *session = saved & ~0xFu;
    LoadXAssets_h((uint64_t)&common, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    *session = saved;
}

static bool IsUnshown(const char* name)
{
    for (int i = 0; i < g_unshownCount; ++i)
    {
        if (EqualsNoCase(g_unshown[i], name))
            return true;
    }

    return false;
}

static void NoteUnshown(const char* name)
{
    if (!IsUnshown(name) && g_unshownCount < kMaxStack)
        snprintf(g_unshown[g_unshownCount++], kNameMax, "%s", name);
}

static void PruneWaitZones()
{
    int kept = 0;

    for (int i = 0; i < g_waitZoneCount; ++i)
    {
        if (ZoneStateOf(g_waitZones[i]) == 0)
            continue;

        if (kept != i)
            memcpy(g_waitZones[kept], g_waitZones[i], kNameMax);

        ++kept;
    }

    g_waitZoneCount = kept;
}

static bool StackModReady(int index, int mode)
{
    if (index < 0 || index >= g_stackCount)
        return true;

    for (int k = 0; k < kModZoneKinds; ++k)
    {
        if (!g_stack[index].mod.zones[k] || (k != 0 && k != mode))
            continue;

        if (ZoneStateOf(g_stack[index].zones[k]) < 2)
            return false;
    }

    return true;
}

static void SetRefreshOnClose(bool refresh)
{
    if (g_refreshOnClose == refresh)
        return;

    g_refreshOnClose = refresh;
    g_modsLuaDirty.store(true);
}

static void PrefetchMods()
{
    if (!g_loadOriginal || !ModHooksReady() || g_inLevel)
        return;

    if (AnyZoneLoading())
    {
        g_prefetchWanted.store(true);
        return;
    }

    char names[kMaxStack][kNameMax];
    const int wanted = ChosenForLoad(names);
    int keep = 0;

    while (keep < g_loadedCount && keep < wanted && EqualsNoCase(g_loaded[keep], names[keep]))
        ++keep;

    if (keep == wanted || keep < g_loadedCount)
        return;

    const int mode = LoadedModeCommon();

    if (mode < 0)
    {
        LoadModeCommonForMods();

        for (int i = 0; i < g_loadedCount; ++i)
            NoteUnshown(g_loaded[i]);

        g_modsLuaDirty.store(true);
        return;
    }

    if (!ModeCommonOnTop())
    {
        T7Log_Write("[Mods] another zone sits above %s, so the ticked mods load when MODS closes",
                    k_modeCommons[mode]);
        g_modsLuaDirty.store(true);
        return;
    }

    T7Log_Write("[Mods] loading %d ticked mod(s) while MODS is open; the menus refresh when it closes",
                wanted - keep);
    AppendMods(names, wanted, mode);

    for (int i = keep; i < wanted; ++i)
        NoteUnshown(names[i]);
}

static void ApplyModsNow()
{
    if (!g_loadOriginal || !ModHooksReady() || g_inLevel)
        return;

    if (AnyZoneLoading())
    {
        g_applyWanted.store(true);
        return;
    }

    char names[kMaxStack][kNameMax];
    const int wanted = ChosenForLoad(names);
    int keep = 0;

    while (keep < g_loadedCount && keep < wanted && EqualsNoCase(g_loaded[keep], names[keep]))
        ++keep;

    if (keep == g_loadedCount && keep == wanted)
        return;

    const int mode = LoadedModeCommon();

    if (mode < 0)
    {
        if (wanted > 0)
            LoadModeCommonForMods();

        return;
    }

    if (!ModeCommonOnTop())
    {
        T7Log_Write("[Mods] another zone sits above the mods, so the changes apply when your next match starts");
        return;
    }

    T7Log_Write("[Mods] applying in the menu: %d kept, %d unload, %d load", keep, g_loadedCount - keep,
                wanted - keep);

    bool removedUi = false;

    for (int i = keep; i < g_loadedCount; ++i)
        removedUi = removedUi || (IsUiMod(g_loaded[i]) && !IsUnshown(g_loaded[i]));

    if (keep < g_loadedCount)
    {
        if (*(const int32_t*)(g_base + kKeepAssetUse) == 0)
        {
            ((void (*)())(g_base + kRenderSync))();
            ClearAssetsInUse();
        }

        if (!FreeModsFrom(keep))
            return;

        PruneWaitZones();
        g_loadedCount = keep;
        RemoveClaimsFrom(keep);

        BeginListWrite();
        SetStack(names, keep);
        EndListWrite();

        SetLoaded(keep);
    }

    if (wanted > keep)
        AppendMods(names, wanted, mode);

    if (removedUi && g_reloadCheck)
        g_reloadForced = true;
    else if (removedUi)
        ReloadFrontend("a mod with menu files was unloaded");
}

static uint64_t LoadLevelZones_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                                 double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    const char* const level = (const char*)a1;

    if (level && RangeReadable(a1, 1) && level[0] && !EqualsNoCase(level, "core_frontend"))
        ApplyModsBeforeLevel(level);

    return ((Hook_t)g_levelZonesOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static uint64_t BlockJob_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                           double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    const uint64_t slot = a1 ? *(uint64_t*)(a1 + 16) : 0;

    if (slot)
    {
        const uint64_t data = *(uint64_t*)(slot + 0x40020);
        const uint32_t unpacked = *(uint32_t*)(slot + 0x40028);
        const uint32_t hashed = *(uint32_t*)(slot + 0x4002C);
        const uint32_t stored = *(uint32_t*)(slot + 0x40030);
        const uint64_t ring = *(uint64_t*)(g_base + kReadRing);

        const bool outside = hashed && (data < ring || data + hashed > ring + 0xA00000);
        const bool odd = hashed > stored + 3 || unpacked > 0x40000;

        if (outside || odd)
        {
            *(uint32_t*)(slot + 0x4002C) = 0;
            *(uint32_t*)(slot + 0x40028) = 0;
        }
    }

    return ((Hook_t)g_blockOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static bool CodeImageSet(uint64_t image)
{
    return image >= 0x10000 && image < 0x0000800000000000ULL;
}

static uint64_t CodeTexture_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                              double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    const uint32_t index = (uint32_t)a4;

    if (a1 && index < 99 && !*(uint8_t*)(a1 + 6016 + 6 * (uint64_t)index) &&
        !CodeImageSet(*(uint64_t*)(a1 + 4432 + 8 * (uint64_t)index)) &&
        !CodeImageSet(*(uint64_t*)(a1 + 5224 + 8 * (uint64_t)index)))
    {
        uint64_t image = *(uint64_t*)(g_base + kImageClear);

        if (!image)
            image = *(uint64_t*)(g_base + kImageBlack);

        if (image)
        {
            const uint64_t texture = image + (*(uint8_t*)(image + 164) ? 200 : 168);
            return ((Hook_t)(g_base + kBindTexture))(a1, a2, a3, texture, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
        }
    }

    return ((Hook_t)g_codeTextureOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
}

static char g_customMapsLua[262144];

constexpr size_t kDescriptionMax = 3072;
constexpr size_t kRawDescriptionMax = 32768;

static char* ReadSmallFile(const char* path, size_t max)
{
    const int fd = sceKernelOpen(path, O_RDONLY, 0);

    if (fd < 0)
        return nullptr;

    char* const text = (char*)malloc(max + 1);
    size_t done = 0;

    while (text && done < max)
    {
        const int64_t n = sceKernelRead(fd, text + done, max - done);

        if (n <= 0)
            break;

        done += (size_t)n;
    }

    sceKernelClose(fd);

    if (!text || !done)
    {
        free(text);
        return nullptr;
    }

    text[done] = 0;
    return text;
}

static size_t Utf8(uint32_t code, char* out)
{
    if (code < 0x80)
    {
        out[0] = (char)code;
        return 1;
    }

    if (code < 0x800)
    {
        out[0] = (char)(0xC0 | (code >> 6));
        out[1] = (char)(0x80 | (code & 0x3F));
        return 2;
    }

    out[0] = (char)(0xE0 | (code >> 12));
    out[1] = (char)(0x80 | ((code >> 6) & 0x3F));
    out[2] = (char)(0x80 | (code & 0x3F));
    return 3;
}

static bool JsonString(const char* json, const char* key, char* out, size_t size)
{
    char quoted[64];
    snprintf(quoted, sizeof(quoted), "\"%s\"", key);

    const char* at = strstr(json, quoted);

    if (!at || size == 0)
        return false;

    at += strlen(quoted);

    while (*at == ' ' || *at == '\t' || *at == '\r' || *at == '\n' || *at == ':')
        ++at;

    if (*at != '"')
        return false;

    ++at;
    size_t used = 0;

    for (; *at && *at != '"' && used + 4 < size; ++at)
    {
        if (*at != '\\')
        {
            out[used++] = *at;
            continue;
        }

        ++at;

        switch (*at)
        {
        case 'n':  out[used++] = '\n'; break;
        case 't':  out[used++] = ' '; break;
        case 'r':
        case 'b':
        case 'f':  break;
        case 'u':
        {
            uint32_t code = 0;
            int digits = 0;

            for (; digits < 4; ++digits)
            {
                const char c = at[digits + 1];
                const int value = c >= '0' && c <= '9' ? c - '0' :
                                  c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                                  c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;

                if (value < 0)
                    break;

                code = code * 16 + (uint32_t)value;
            }

            at += digits;

            if (digits == 4 && code && (code < 0xD800 || code > 0xDFFF))
                used += Utf8(code, out + used);

            break;
        }
        case 0:    --at; break;
        default:   out[used++] = *at; break;
        }
    }

    out[used] = 0;
    return true;
}

static void AppendLuaString(char* out, size_t size, size_t* used, const char* text)
{
    if (*used + 3 >= size)
        return;

    out[(*used)++] = '"';

    for (const unsigned char* c = (const unsigned char*)text; *c && *used + 4 < size; ++c)
    {
        if (*c == '"' || *c == '\\')
        {
            out[(*used)++] = '\\';
            out[(*used)++] = (char)*c;
        }
        else if (*c == '\n')
        {
            out[(*used)++] = '\\';
            out[(*used)++] = 'n';
        }
        else if (*c >= 0x20)
        {
            out[(*used)++] = (char)*c;
        }
    }

    out[(*used)++] = '"';
    out[*used] = 0;
}

static void Append(char* out, size_t size, size_t* used, const char* text)
{
    const int n = snprintf(out + *used, size - *used, "%s", text);

    if (n > 0)
        *used = *used + (size_t)n < size ? *used + (size_t)n : size - 1;
}

struct BbTag
{
    char        name[16];
    bool        closing;
    bool        valued;
    const char* end;
};

static bool IsBbTag(const char* name)
{
    static const char* const k_tags[] = { "h1", "h2", "h3", "b", "i", "u", "s", "strike", "spoiler", "noparse", "code",
                                          "quote", "url", "img", "previewyoutube", "list", "olist", "*", "hr", "table",
                                          "tr", "th", "td" };

    for (const char* const tag : k_tags)
    {
        if (strcmp(tag, name) == 0)
            return true;
    }

    return false;
}

static bool ReadBbTag(const char* at, BbTag* tag)
{
    const char* p = at + 1;
    tag->closing = *p == '/';

    if (tag->closing)
        ++p;

    size_t n = 0;

    while (n + 1 < sizeof(tag->name) &&
           ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '*'))
    {
        tag->name[n++] = (char)(*p >= 'A' && *p <= 'Z' ? *p + 32 : *p);
        ++p;
    }

    tag->name[n] = 0;

    if (!n || !IsBbTag(tag->name))
        return false;

    tag->valued = *p == '=';

    for (int i = 0; i < 512 && *p && *p != '\n'; ++i, ++p)
    {
        if (*p == ']')
        {
            tag->end = p + 1;
            return true;
        }
    }

    return false;
}

static const char* FindBbClosing(const char* from, const char* name)
{
    for (const char* p = from; *p; ++p)
    {
        BbTag tag;

        if (p[0] == '[' && p[1] == '/' && ReadBbTag(p, &tag) && strcmp(tag.name, name) == 0)
            return tag.end;
    }

    return nullptr;
}

static void ExpandBbCode(const char* in, char* out, size_t size)
{
    size_t used = 0;
    int upper = 0;
    int depth = 0;
    int numbers[8] = {};

    const auto put = [&](char c)
    {
        if (used + 1 < size)
            out[used++] = c;
    };

    const auto text = [&](const char* s)
    {
        while (*s)
            put(*s++);
    };

    const auto line = [&]()
    {
        if (used && out[used - 1] != '\n')
            put('\n');
    };

    const auto paragraph = [&]()
    {
        if (!used)
            return;

        line();

        if (used < 2 || out[used - 2] != '\n')
            put('\n');
    };

    for (const char* at = in; *at;)
    {
        BbTag tag;

        if (*at == '[' && ReadBbTag(at, &tag))
        {
            const char* next = tag.end;
            const char* const name = tag.name;
            bool block = true;

            if (strcmp(name, "img") == 0 || strcmp(name, "previewyoutube") == 0)
            {
                block = false;

                if (!tag.closing && !tag.valued)
                {
                    const char* const close = FindBbClosing(next, name);
                    next = close ? close : next + strlen(next);
                }
            }
            else if (name[0] == 'h' && name[1] >= '1' && name[1] <= '3' && !name[2])
            {
                if (!tag.closing)
                {
                    paragraph();
                    ++upper;
                }
                else
                {
                    if (upper)
                        --upper;

                    line();
                }
            }
            else if (strcmp(name, "list") == 0 || strcmp(name, "olist") == 0)
            {
                if (!tag.closing)
                {
                    if (depth < 8)
                        numbers[depth] = name[0] == 'o' ? 0 : -1;

                    ++depth;
                }
                else if (depth)
                {
                    --depth;
                }

                line();
            }
            else if (strcmp(name, "*") == 0)
            {
                line();
                const int level = depth > 0 && depth <= 8 ? depth - 1 : -1;

                if (level >= 0 && numbers[level] >= 0)
                {
                    char number[16];
                    snprintf(number, sizeof(number), "%d. ", ++numbers[level]);
                    text(number);
                }
                else
                {
                    text("- ");
                }
            }
            else if (strcmp(name, "hr") == 0)
            {
                paragraph();
            }
            else if (strcmp(name, "quote") == 0 || strcmp(name, "code") == 0 || strcmp(name, "table") == 0)
            {
                line();
            }
            else if (strcmp(name, "tr") == 0)
            {
                if (tag.closing)
                    line();
            }
            else if (strcmp(name, "td") == 0 || strcmp(name, "th") == 0)
            {
                if (tag.closing)
                    text("  ");
                block = false;
            }
            else
            {
                block = false;
            }

            if (block && *next == '\n')
                ++next;

            at = next;
            continue;
        }

        const unsigned char c = (unsigned char)*at;

        if (c == 0xE2 && (unsigned char)at[1] == 0x80 && at[2])
        {
            const unsigned char d = (unsigned char)at[2];
            const char* const replacement = d == 0x98 || d == 0x99 ? "'" :
                                            d == 0x9C || d == 0x9D ? "\"" :
                                            d == 0x93 || d == 0x94 || d == 0xA2 ? "-" :
                                            d == 0xA6 ? "..." : nullptr;

            if (replacement)
            {
                text(replacement);
                at += 3;
                continue;
            }
        }

        if (c == 0xC2 && (unsigned char)at[1] == 0xA0)
        {
            put(' ');
            at += 2;
            continue;
        }

        if (c >= 0xF0)
        {
            ++at;

            while (((unsigned char)*at & 0xC0) == 0x80)
                ++at;

            continue;
        }

        if (c == '\t')
        {
            put(' ');
            ++at;
            continue;
        }

        if (c < 0x20 && c != '\n')
        {
            ++at;
            continue;
        }

        put(upper && c >= 'a' && c <= 'z' ? (char)(c - 32) : (char)c);
        ++at;
    }

    out[used] = 0;
}

static bool SeparatorLine(const char* line, size_t length)
{
    if (length < 3)
        return false;

    for (size_t i = 0; i < length; ++i)
    {
        if (line[i] != ' ' && !strchr("-=_~*#+.:|", line[i]))
            return false;
    }

    return true;
}

static void TidyLines(const char* in, char* out, size_t size)
{
    size_t used = 0;
    bool any = false;
    bool blank = false;

    for (const char* line = in; *line;)
    {
        const char* end = strchr(line, '\n');

        if (!end)
            end = line + strlen(line);

        const char* s = line;
        const char* e = end;
        line = *end ? end + 1 : end;

        while (s < e && *s == ' ')
            ++s;

        while (e > s && e[-1] == ' ')
            --e;

        if (s == e || SeparatorLine(s, (size_t)(e - s)))
        {
            blank = any;
            continue;
        }

        if (any && used + 1 < size)
            out[used++] = '\n';

        if (any && blank && used + 1 < size)
            out[used++] = '\n';

        any = true;
        blank = false;
        bool space = false;

        for (const char* p = s; p < e && used + 1 < size; ++p)
        {
            if (*p == ' ' && space)
                continue;

            space = *p == ' ';
            out[used++] = *p;
        }
    }

    out[used] = 0;
}

static void LimitText(const char* text, char* out, size_t size)
{
    const size_t length = strlen(text);

    if (length < size)
    {
        memcpy(out, text, length + 1);
        return;
    }

    size_t cut = size - 4;

    while (cut && ((unsigned char)text[cut] & 0xC0) == 0x80)
        --cut;

    for (size_t at = cut; at > 0 && cut - at < 240; --at)
    {
        if (text[at] == ' ' || text[at] == '\n')
        {
            cut = at;
            break;
        }
    }

    while (cut && (text[cut - 1] == ' ' || text[cut - 1] == '\n'))
        --cut;

    memcpy(out, text, cut);
    memcpy(out + cut, "...", 4);
}

static void FormatDescription(const char* raw, char* out, size_t size)
{
    static char expanded[kRawDescriptionMax];
    static char tidy[kRawDescriptionMax];

    ExpandBbCode(raw, expanded, sizeof(expanded));
    TidyLines(expanded, tidy, sizeof(tidy));
    LimitText(tidy, out, size);
}

static bool MoviePlayable(const char* path, char* why, size_t whySize)
{
    const int fd = sceKernelOpen(path, O_RDONLY, 0);

    if (fd < 0)
    {
        snprintf(why, whySize, "it could not be opened");
        return false;
    }

    static uint8_t data[64 * 1024];
    size_t size = 0;

    while (size < sizeof(data))
    {
        const int64_t n = sceKernelRead(fd, data + size, sizeof(data) - size);

        if (n <= 0)
            break;

        size += (size_t)n;
    }

    sceKernelClose(fd);

    const auto readVint = [&](size_t at, bool keepMarker, uint64_t* value) -> size_t
    {
        if (at >= size)
            return 0;

        const uint8_t first = data[at];
        size_t length = 1;

        for (uint8_t mask = 0x80; length <= 8 && !(first & mask); mask >>= 1)
            ++length;

        if (length > 8 || at + length > size)
            return 0;

        uint64_t result = keepMarker ? first : (first & (0xFF >> length));

        for (size_t i = 1; i < length; ++i)
            result = result << 8 | data[at + i];

        *value = result;
        return length;
    };

    char codec[32] = "";
    uint64_t width = 0;
    uint64_t height = 0;
    int profile = -1;
    int level = -1;

    size_t at = 0;
    size_t end = size;
    int tracks = 0;

    while (at < end)
    {
        uint64_t id = 0;
        uint64_t length = 0;
        const size_t idLength = readVint(at, true, &id);
        const size_t sizeLength = idLength ? readVint(at + idLength, false, &length) : 0;

        if (!idLength || !sizeLength)
            break;

        const size_t body = at + idLength + sizeLength;
        const bool unknown = length >= (1ull << (7 * sizeLength)) - 1;
        const size_t bodyEnd = unknown || body + length > end ? end : body + (size_t)length;

        if (id == 0x1F43B675 || (id == 0xAE && ++tracks > 1))
            break;

        if (id == 0x18538067 || id == 0x1654AE6B || id == 0xAE || id == 0xE0)
        {
            at = body;

            if (id == 0x18538067)
                end = bodyEnd;

            continue;
        }

        if (id == 0x86 && !codec[0])
            snprintf(codec, sizeof(codec), "%.*s", (int)(bodyEnd - body), (const char*)data + body);
        else if (id == 0xB0)
            for (size_t i = body; i < bodyEnd; ++i) width = width << 8 | data[i];
        else if (id == 0xBA)
            for (size_t i = body; i < bodyEnd; ++i) height = height << 8 | data[i];
        else if (id == 0x63A2 && bodyEnd - body >= 4 && data[body] == 1)
        {
            profile = data[body + 1];
            level = data[body + 3];
        }

        at = bodyEnd;
    }

    if (strcmp(codec, "V_MPEG4/ISO/AVC") != 0)
    {
        snprintf(why, whySize, "its video is %s, not H.264", codec[0] ? codec : "unreadable");
        return false;
    }

    if (!width || !height || width > 1920 || height > 1080)
    {
        snprintf(why, whySize, "it is %llux%llu", (unsigned long long)width, (unsigned long long)height);
        return false;
    }

    if (profile >= 0 && ((profile != 66 && profile != 77 && profile != 100) || level > 42))
    {
        snprintf(why, whySize, "it is H.264 profile %d level %d.%d", profile, level / 10, level % 10);
        return false;
    }

    snprintf(why, whySize, "H.264 %llux%llu", (unsigned long long)width, (unsigned long long)height);
    return true;
}

static void ScanMovies()
{
    static char names[64][kListedNameMax];

    for (int i = 0; i < g_packageCount; ++i)
    {
        if (!g_packages[i].map)
            continue;

        for (int place = 0; place < 2; ++place)
        {
            const char* const base = place == 0 ? g_packages[i].folder : g_packages[i].alternate;

            if (!base[0])
                continue;

            char folder[224];
            snprintf(folder, sizeof(folder), "%s/video", base);

            const int count = ListFolder(folder, false, names, 64);

            for (int n = 0; n < count; ++n)
            {
                const size_t length = strlen(names[n]);

                if (length <= 4 || !EqualsNoCase(names[n] + length - 4, ".mkv"))
                    continue;

                bool taken = false;

                for (int m = 0; m < g_movieCount && !taken; ++m)
                    taken = strlen(g_movies[m].name) == length - 4 && strncmp(g_movies[m].file, names[n], length - 4) == 0;

                if (taken || g_movieCount >= (int)(sizeof(g_movies) / sizeof(g_movies[0])))
                    continue;

                char path[320];
                char why[160];
                snprintf(path, sizeof(path), "%s/%s", folder, names[n]);

                if (!MoviePlayable(path, why, sizeof(why)))
                    continue;

                Movie& movie = g_movies[g_movieCount++];
                snprintf(movie.file, sizeof(movie.file), "%s", names[n]);
                snprintf(movie.name, sizeof(movie.name), "%.*s", (int)(length - 4), names[n]);
                snprintf(movie.folder, sizeof(movie.folder), "%s", folder);
                movie.package = i;
            }
        }
    }
}

static void FindPreview(Package& package, const char* json)
{
    char path[320];

    package.preview[0] = 0;

    if (FindPackageFile(package, "previewimage.png", path, sizeof(path)))
    {
        snprintf(package.preview, sizeof(package.preview), "previewimage.png");
        return;
    }

    char thumbnail[260];

    if (!json || !JsonString(json, "Thumbnail", thumbnail, sizeof(thumbnail)))
        return;

    const char* file = thumbnail;

    for (const char* c = thumbnail; *c; ++c)
    {
        if (*c == '/' || *c == '\\')
            file = c + 1;
    }

    const size_t length = strlen(file);

    if (length <= 4 || length >= sizeof(package.preview) || !EqualsNoCase(file + length - 4, ".png"))
        return;

    if (FindPackageFile(package, file, path, sizeof(path)))
        snprintf(package.preview, sizeof(package.preview), "%s", file);
}

static void BuildCustomMapsLua()
{
    char* const out = g_customMapsLua;
    const size_t size = sizeof(g_customMapsLua);
    static const char k_tail[] = "})\n";
    static char entry[16384];
    static char raw[kRawDescriptionMax];
    static char description[kDescriptionMax];
    size_t used = 0;

    out[0] = 0;
    Append(out, size, &used, "rawset(_G, \"BO3CustomMaps\", {\n");

    for (int i = 0; i < g_packageCount; ++i)
    {
        Package& package = g_packages[i];
        const bool zombies = strncmp(package.name, "zm_", 3) == 0;
        const bool multiplayer = strncmp(package.name, "mp_", 3) == 0;

        if (!package.map || (!zombies && !multiplayer))
            continue;

        char path[320];
        FindPackageFile(package, "workshop.json", path, sizeof(path));

        char title[160];
        snprintf(title, sizeof(title), "%s", package.name);
        raw[0] = 0;

        char* const json = ReadSmallFile(path, 64 * 1024);

        if (json)
        {
            if (!JsonString(json, "Title", title, sizeof(title)) || !title[0])
                snprintf(title, sizeof(title), "%s", package.name);

            JsonString(json, "Description", raw, sizeof(raw));
        }

        FormatDescription(raw, description, sizeof(description));
        FindPreview(package, json);
        free(json);

        char picture[320];
        char previewName[96];
        char loadingName[96];

        const bool preview = package.preview[0] != 0;
        const bool loading = FindPackageFile(package, "loadingimage.png", picture, sizeof(picture));
        snprintf(previewName, sizeof(previewName), T7_USERMAP_PREVIEW_PREFIX "%s", package.name);
        snprintf(loadingName, sizeof(loadingName), T7_USERMAP_LOADING_PREFIX "%s", package.name);

        char movieName[kNameMax + 8];
        snprintf(movieName, sizeof(movieName), "%s_load", package.name);
        bool movie = false;

        for (int m = 0; m < g_movieCount && !movie; ++m)
            movie = g_movies[m].package == i && EqualsNoCase(g_movies[m].name, movieName);

        for (int attempt = 0; attempt < 2; ++attempt)
        {
            if (attempt)
            {
                char shorter[256];
                LimitText(description, shorter, sizeof(shorter));
                snprintf(description, sizeof(description), "%s", shorter);
            }

            size_t length = 0;
            entry[0] = 0;
            Append(entry, sizeof(entry), &length, "  { id = ");
            AppendLuaString(entry, sizeof(entry), &length, package.name);
            Append(entry, sizeof(entry), &length, zombies ? ", mode = \"zm\"" : ", mode = \"mp\"");
            Append(entry, sizeof(entry), &length, ", name = ");
            AppendLuaString(entry, sizeof(entry), &length, title);
            Append(entry, sizeof(entry), &length, ", desc = ");
            AppendLuaString(entry, sizeof(entry), &length, description);

            if (preview)
            {
                Append(entry, sizeof(entry), &length, ", preview = ");
                AppendLuaString(entry, sizeof(entry), &length, previewName);
            }

            if (loading)
            {
                Append(entry, sizeof(entry), &length, ", loading = ");
                AppendLuaString(entry, sizeof(entry), &length, loadingName);
            }

            if (movie)
            {
                Append(entry, sizeof(entry), &length, ", movie = ");
                AppendLuaString(entry, sizeof(entry), &length, movieName);
            }

            Append(entry, sizeof(entry), &length, " },\n");

            if (used + length + sizeof(k_tail) < size)
            {
                memcpy(out + used, entry, length + 1);
                used += length;
                ++g_listedMaps;
                break;
            }

            if (attempt)
                T7Log_Write("[Maps] %s left out of the map list: the list is full", package.name);
        }
    }

    Append(out, size, &used, k_tail);
}

static char g_mapsTableLua[sizeof(g_customMapsLua) + 64 * 1024 + 64];

static void BuildMapsTableLua()
{
    char path[256];
    snprintf(path, sizeof(path), "%s/ui_scripts/maptable.lua", "/data/BO3-Customs");

    char* const source = ReadSmallFile(path, 64 * 1024);

    if (!source)
    {
        g_mapsTableLua[0] = 0;
        return;
    }

    snprintf(g_mapsTableLua, sizeof(g_mapsTableLua), "%s%s", g_customMapsLua, source);
    free(source);
}

static bool ModNameValid(const char* name)
{
    size_t n = 0;

    for (; name[n]; ++n)
    {
        const unsigned char c = (unsigned char)name[n];

        if (n >= kNameMax - 1 || c < 0x20 || c >= 0x7F || c == '"' || c == '\\' || c == '/' || c == ':' || c == '*' ||
            c == '?')
            return false;
    }

    return n > 0 && name[0] != '.' && name[0] != ' ';
}

static bool AllDigits(const char* text)
{
    if (!text[0])
        return false;

    for (const char* c = text; *c; ++c)
    {
        if (*c < '0' || *c > '9')
            return false;
    }

    return true;
}

static void ReadModInfo(Mod& mod)
{
    snprintf(mod.title, sizeof(mod.title), "%s", mod.name);
    snprintf(mod.ugc, sizeof(mod.ugc), "%s", mod.name);
    snprintf(mod.internal, sizeof(mod.internal), "%s", mod.name);

    char* json = nullptr;

    for (const char* const folder : { mod.folder, mod.alternate })
    {
        if (!json && folder[0])
        {
            char path[288];
            snprintf(path, sizeof(path), "%s/workshop.json", folder);
            json = ReadSmallFile(path, 64 * 1024);
        }
    }

    if (!json)
    {
        char parent[224];
        snprintf(parent, sizeof(parent), "%s", mod.folder);
        char* const slash = strrchr(parent, '/');

        if (slash && strcmp(slash, "/zone") == 0)
        {
            *slash = 0;
            char path[288];
            snprintf(path, sizeof(path), "%s/workshop.json", parent);
            json = ReadSmallFile(path, 64 * 1024);
        }
    }

    if (!json)
        return;

    char value[160];

    if (JsonString(json, "Title", value, sizeof(value)) && value[0])
        snprintf(mod.title, sizeof(mod.title), "%s", value);

    if (JsonString(json, "PublisherID", value, sizeof(value)) && AllDigits(value) && strlen(value) < sizeof(mod.ugc))
        snprintf(mod.ugc, sizeof(mod.ugc), "%s", value);

    if (JsonString(json, "FolderName", value, sizeof(value)) && value[0] && strlen(value) < sizeof(mod.internal))
        snprintf(mod.internal, sizeof(mod.internal), "%s", value);

    free(json);
}

static void ScanMods(const char* data)
{
    char root[224];
    snprintf(root, sizeof(root), "%s/mods", data);

    static char folders[kMaxMods][kListedNameMax];
    const int count = ListFolder(root, true, folders, kMaxMods);

    for (int i = 0; i < count && g_modCount < kMaxMods; ++i)
    {
        if (!ModNameValid(folders[i]) || FindMod(folders[i]) >= 0)
            continue;

        char base[224];
        char zone[224];
        snprintf(base, sizeof(base), "%s/%s", root, folders[i]);
        snprintf(zone, sizeof(zone), "%s/zone", base);

        Mod& mod = g_mods[g_modCount];
        memset(&mod, 0, sizeof(mod));
        snprintf(mod.name, sizeof(mod.name), "%s", folders[i]);

        bool atBase = false;
        bool any = false;

        for (int k = 0; k < kModZoneKinds; ++k)
        {
            char file[288];
            OrbisKernelStat st;
            snprintf(file, sizeof(file), "%s/%s.ff", base, k_modZones[k]);
            const bool here = sceKernelStat(file, &st) == 0;
            snprintf(file, sizeof(file), "%s/%s.ff", zone, k_modZones[k]);
            const bool inZone = !here && sceKernelStat(file, &st) == 0;

            mod.zones[k] = here || inZone;
            atBase = atBase || here;
            any = any || mod.zones[k];
        }

        if (!any)
            continue;

        snprintf(mod.folder, sizeof(mod.folder), "%s", atBase ? base : zone);
        snprintf(mod.alternate, sizeof(mod.alternate), "%s", atBase ? zone : base);
        ReadModInfo(mod);
        ++g_modCount;

        T7Log_Write("[Mods] found %s (%s):%s%s%s%s", mod.name, mod.title, mod.zones[0] ? " core_mod" : "",
                    mod.zones[1] ? " zm_mod" : "", mod.zones[2] ? " mp_mod" : "", mod.zones[3] ? " cp_mod" : "");
    }
}

static void ReadChosenMods()
{
    g_chosenCount = 0;

    char* const text = ReadSmallFile(k_chosenModFile, 2048);

    if (!text)
        return;

    char* line = text;

    while (*line && g_chosenCount < kMaxStack)
    {
        char* end = line;

        while (*end && *end != '\n' && *end != '\r' && *end != ',')
            ++end;

        const char stop = *end;
        *end = 0;

        size_t n = strlen(line);

        while (n > 0 && line[n - 1] == ' ')
            line[--n] = 0;

        bool duplicate = false;

        for (int i = 0; i < g_chosenCount; ++i)
            duplicate = duplicate || EqualsNoCase(g_chosen[i], line);

        if (!duplicate && ModNameValid(line))
            snprintf(g_chosen[g_chosenCount++], kNameMax, "%s", line);

        if (!stop)
            break;

        line = end + 1;
    }

    free(text);
}

static bool WriteChosenMods(const char (*names)[kNameMax], int count)
{
    const int fd = sceKernelOpen(k_chosenModFile, O_WRONLY | O_CREAT | O_TRUNC, 0777);

    if (fd < 0)
        return false;

    bool written = true;

    for (int i = 0; i < count && written; ++i)
    {
        char line[kNameMax + 2];
        const int n = snprintf(line, sizeof(line), "%s\n", names[i]);
        written = n > 0 && sceKernelWrite(fd, line, (size_t)n) == (int64_t)n;
    }

    sceKernelClose(fd);
    return written;
}

static char g_modsLua[48 * 1024];

static void BuildModsLua()
{
    char* const out = g_modsLua;
    const size_t size = sizeof(g_modsLua);
    size_t used = 0;

    out[0] = 0;
    Append(out, size, &used, "rawset(_G, \"BO3CustomMods\", {\n");

    for (int i = 0; i < g_modCount; ++i)
    {
        const Mod& mod = g_mods[i];
        char entry[1024];
        size_t length = 0;
        entry[0] = 0;
        Append(entry, sizeof(entry), &length, "  { name = ");
        AppendLuaString(entry, sizeof(entry), &length, mod.name);
        Append(entry, sizeof(entry), &length, ", title = ");
        AppendLuaString(entry, sizeof(entry), &length, mod.title);
        Append(entry, sizeof(entry), &length, ", ugc = ");
        AppendLuaString(entry, sizeof(entry), &length, mod.ugc);
        Append(entry, sizeof(entry), &length, ", internal = ");
        AppendLuaString(entry, sizeof(entry), &length, mod.internal);
        Append(entry, sizeof(entry), &length, mod.zones[0] ? ", core = true" : "");
        Append(entry, sizeof(entry), &length, mod.zones[1] ? ", zm = true" : "");
        Append(entry, sizeof(entry), &length, mod.zones[2] ? ", mp = true" : "");
        Append(entry, sizeof(entry), &length, mod.zones[3] ? ", cp = true" : "");
        Append(entry, sizeof(entry), &length, " },\n");

        if (used + length + 1024 < size)
        {
            memcpy(out + used, entry, length + 1);
            used += length;
        }
    }

    Append(out, size, &used, "})\nrawset(_G, \"BO3CustomsChosenMods\", {");

    for (int i = 0; i < g_chosenCount; ++i)
    {
        AppendLuaString(out, size, &used, g_chosen[i]);
        Append(out, size, &used, ", ");
    }

    Append(out, size, &used, "})\nrawset(_G, \"BO3CustomsChosenMod\", ");
    AppendLuaString(out, size, &used, g_chosenCount > 0 ? g_chosen[0] : "");
    Append(out, size, &used, ")\nrawset(_G, \"BO3CustomsActiveMods\", {\n");

    int first = -1;
    const int mode = LoadedModeCommon();

    for (int i = 0; i < g_loadedCount; ++i)
    {
        const int loaded = FindMod(g_loaded[i]);

        if (loaded < 0)
            continue;

        if (first < 0)
            first = loaded;

        const Mod& mod = g_mods[loaded];
        Append(out, size, &used, "  { name = ");
        AppendLuaString(out, size, &used, mod.name);
        Append(out, size, &used, ", title = ");
        AppendLuaString(out, size, &used, mod.title);
        Append(out, size, &used, ", ugc = ");
        AppendLuaString(out, size, &used, mod.ugc);
        Append(out, size, &used, ", internal = ");
        AppendLuaString(out, size, &used, mod.internal);
        Append(out, size, &used, StackModReady(i, mode) ? " },\n" : ", loading = true },\n");
    }

    Append(out, size, &used, "})\n");
    Append(out, size, &used, g_refreshOnClose ? "rawset(_G, \"BO3CustomsModsRefresh\", true)\n"
                                              : "rawset(_G, \"BO3CustomsModsRefresh\", false)\n");

    if (first < 0)
    {
        Append(out, size, &used, "rawset(_G, \"BO3CustomsActiveMod\", nil)\n");
    }
    else
    {
        const Mod& mod = g_mods[first];
        Append(out, size, &used, "rawset(_G, \"BO3CustomsActiveMod\", { name = ");
        AppendLuaString(out, size, &used, mod.name);
        Append(out, size, &used, ", title = ");
        AppendLuaString(out, size, &used, mod.title);
        Append(out, size, &used, ", ugc = ");
        AppendLuaString(out, size, &used, mod.ugc);
        Append(out, size, &used, ", internal = ");
        AppendLuaString(out, size, &used, mod.internal);
        Append(out, size, &used, " })\n");
    }

    Append(out, size, &used,
           "local ui = rawget(_G, \"BO3CustomsModsUI\")\n"
           "if ui ~= nil and ui.DataChanged ~= nil then pcall(ui.DataChanged) end\n");
}

static uint64_t MapsTable_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                            double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    const uint64_t results = ((Hook_t)g_mapsTableOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);

    if ((uint32_t)results == 1 && a1 && g_mapsTableLua[0])
        T7Lua_RunOnTop(a1, g_mapsTableLua, "=bo3customs_maptable");

    return results;
}

static bool SameNoCase(const char* a, const char* b, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        char x = a[i];
        char y = b[i];

        if (x >= 'A' && x <= 'Z')
            x = (char)(x + 32);

        if (y >= 'A' && y <= 'Z')
            y = (char)(y + 32);

        if (x != y || !x)
            return x == y && i + 1 == length;
    }

    return true;
}

static uint64_t PathRemap_h(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6,
                            double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7)
{
    const uint64_t result = ((Hook_t)g_pathRemapOriginal)(a1, a2, a3, a4, a5, a6, x0, x1, x2, x3, x4, x5, x6, x7);
    const char* const path = (const char*)a1;
    char* const out = (char*)a2;
    const char* const slash = path ? strrchr(path, '/') : nullptr;

    if (!out || !slash || slash - path < 6 || strncmp(slash - 6, "/video", 6) != 0)
        return result;

    const char* const file = slash + 1;
    const size_t length = strlen(file);

    if (length <= 4 || !SameNoCase(file + length - 4, ".mkv", 4))
        return result;

    ListReader reading;

    for (int i = 0; i < g_movieCount; ++i)
    {
        Movie& movie = g_movies[i];
        const size_t movieLength = strlen(movie.name);

        if (movieLength != length - 4 || !SameNoCase(file, movie.name, movieLength))
            continue;

        snprintf(out, 256, "%s/%s", movie.folder, movie.file);
        break;
    }

    return result;
}

static bool RemoveSignaturePenalty(uintptr_t base, const SignaturePatch& patch)
{
    const uintptr_t at = base + patch.site;

    if (!RangeReadable(at, patch.expectLength))
        return false;

    const uint8_t* const code = (const uint8_t*)at;
    const size_t tail = patch.offset + sizeof(k_nop10);
    const bool around = memcmp(code, patch.expect, patch.offset) == 0 &&
                        memcmp(code + tail, patch.expect + tail, patch.expectLength - tail) == 0;

    if (around && memcmp(code + patch.offset, k_nop10, sizeof(k_nop10)) == 0)
        return true;

    if (!around || memcmp(code + patch.offset, patch.expect + patch.offset, sizeof(k_nop10)) != 0)
        return false;

    const uintptr_t first = (at + patch.offset) & ~0x3FFFull;
    const uintptr_t last = (at + tail - 1) & ~0x3FFFull;

    if (sceKernelMprotect((const void*)first, last - first + 0x4000, 7) < 0)
        return false;

    memcpy((void*)(at + patch.offset), k_nop10, sizeof(k_nop10));
    return true;
}

static bool TreatUnknownMapsAsBase(uintptr_t base)
{
    const uintptr_t gate = base + kMapContentGate;
    const uintptr_t target = base + kMapContentBase;

    if (!RangeReadable(gate, sizeof(k_mapContentGate)) || !RangeReadable(target, sizeof(k_mapContentBase)) ||
        memcmp((const void*)target, k_mapContentBase, sizeof(k_mapContentBase)) != 0 ||
        memcmp((const void*)gate, k_mapContentGate, 3) != 0 || memcmp((const void*)(gate + 4), k_mapContentGate + 4, 4) != 0 ||
        gate + 4 + kMapContentJump != target)
        return false;

    uint8_t* const jump = (uint8_t*)(gate + 3);

    if (*jump == kMapContentJump)
        return true;

    if (*jump != kMapContentReturn || sceKernelMprotect((const void*)((uintptr_t)jump & ~0x3FFFull), 0x4000, 7) < 0)
        return false;

    *jump = kMapContentJump;
    return true;
}

static bool LowerPlaceholderRank(uintptr_t base)
{
    for (const PenaltySite& site : k_placeholderPenalties)
    {
        const uintptr_t at = base + site.site;

        if (!RangeReadable(at, site.operand + sizeof(int32_t)) || memcmp((const void*)at, site.opcode, site.operand) != 0)
            return false;

        int32_t value;
        memcpy(&value, (const void*)(at + site.operand), sizeof(value));

        if (value != kPlaceholderPenalty && value != kPlaceholderPenaltyLow)
            return false;
    }

    const uintptr_t first = (base + k_placeholderPenalties[0].site) & ~0x3FFFull;
    const PenaltySite& tail = k_placeholderPenalties[sizeof(k_placeholderPenalties) / sizeof(k_placeholderPenalties[0]) - 1];
    const uintptr_t last = (base + tail.site + tail.operand + sizeof(int32_t) - 1) & ~0x3FFFull;

    if (sceKernelMprotect((const void*)first, last - first + 0x4000, 7) < 0)
        return false;

    for (const PenaltySite& site : k_placeholderPenalties)
        memcpy((void*)(base + site.site + site.operand), &kPlaceholderPenaltyLow, sizeof(int32_t));

    return true;
}

struct Prologue
{
    uintptr_t      offset;
    const char*    what;
    const uint8_t* bytes;
    size_t         size;
};

static bool BuildMatches(uintptr_t base)
{
    static const Prologue checks[] =
    {
        { kFileOpen,    "file open",       k_fileOpen,   sizeof(k_fileOpen) },
        { kMapExists,   "map check",       k_mapExists,  sizeof(k_mapExists) },
        { kIsMapValid,  "Engine.IsMapValid", k_isMapValid, sizeof(k_isMapValid) },
        { kLoadXAssets, "DB_LoadXAssets",  k_loadZones,  sizeof(k_loadZones) },
    };

    for (const Prologue& check : checks)
    {
        const uintptr_t at = base + check.offset;

        if (!RangeReadable(at, check.size) || memcmp((const void*)at, check.bytes, check.size) != 0)
            return false;
    }

    return true;
}

static void ScanAll()
{
    g_packageCount = 0;
    g_fileCount = 0;
    g_movieCount = 0;
    g_levelCommon = -1;
    g_listedMaps = 0;
    g_modCount = 0;

    ScanUsermaps("/data/BO3-Customs");
    ScanSharedZones("/data/BO3-Customs");
    ScanMods("/data/BO3-Customs");

    for (const char* const drive : k_driveRoots)
    {
        char data[64];
        snprintf(data, sizeof(data), "/%s/BO3-Customs", drive);
        ScanUsermaps(data);
        ScanSharedZones(data);
        ScanMods(data);
    }

    g_modsLuaDirty.store(true);

    ScanMovies();
    BuildCustomMapsLua();
    BuildMapsTableLua();
}

static std::atomic<int> g_uninstallState{0};
static char g_uninstallMap[kNameMax];
static char g_uninstallRoot[224];
static char g_uninstallMessage[160];
static bool g_uninstallRemoved = false;

static int RemoveTree(const char* path, int depth)
{
    if (depth > 8)
        return 1;

    const int fd = sceKernelOpen(path, O_RDONLY | O_DIRECTORY, 0);

    if (fd < 0)
        return 1;

    size_t capacity = 0x10000;
    size_t length = 0;
    char* listing = (char*)malloc(capacity);

    while (listing)
    {
        if (capacity - length < 0x10000)
        {
            char* const grown = (char*)realloc(listing, capacity * 2);

            if (!grown)
                break;

            listing = grown;
            capacity *= 2;
        }

        const int n = sceKernelGetdents(fd, listing + length, 0x10000);

        if (n <= 0)
            break;

        length += (size_t)n;
    }

    sceKernelClose(fd);

    if (!listing)
        return 1;

    int failures = 0;

    for (size_t at = 0; at + 8 <= length;)
    {
        const struct dirent* const entry = (const struct dirent*)(listing + at);

        if (entry->d_reclen == 0)
            break;

        at += entry->d_reclen;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char child[512];

        if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >= (int)sizeof(child))
        {
            ++failures;
            continue;
        }

        if (entry->d_type == DT_DIR)
            failures += RemoveTree(child, depth + 1);
        else if (sceKernelUnlink(child) < 0)
            failures += RemoveTree(child, depth + 1);
    }

    free(listing);

    if (sceKernelRmdir(path) < 0)
        ++failures;

    return failures;
}

static void* UninstallThread(void*)
{
    const int failures = RemoveTree(g_uninstallRoot, 0);
    OrbisKernelStat st;
    const bool removed = sceKernelStat(g_uninstallRoot, &st) != 0;

    if (removed)
    {
        g_uninstallMessage[0] = 0;
        T7Log_Write("[Maps] uninstalled %s", g_uninstallMap);
    }
    else
    {
        snprintf(g_uninstallMessage, sizeof(g_uninstallMessage), "%d item%s in %s could not be deleted", failures ? failures : 1,
                 failures == 1 ? "" : "s", g_uninstallRoot);
        T7Log_Write("[Maps] uninstall of %s failed: %s", g_uninstallMap, g_uninstallMessage);
    }

    g_uninstallRemoved = removed;
    g_uninstallState.store(2);
    return nullptr;
}

static void InstallHooks(uintptr_t base)
{
    if (g_levelCommon >= 0 && !g_rankLowered)
    {
        g_rankLowered = true;
        LowerPlaceholderRank(base);
    }

    if (g_hooksInstalled)
        return;

    g_hooksInstalled = true;

    for (const SignaturePatch& patch : k_signaturePatches)
        RemoveSignaturePenalty(base, patch);

    Detour_Attach(&g_openDetour, (uint64_t)(base + kFileOpen), (void*)FileOpen_h, &g_openOriginal);
    Detour_Attach(&g_existsDetour, (uint64_t)(base + kMapExists), (void*)MapExists_h, &g_existsOriginal);
    Detour_Attach(&g_validDetour, (uint64_t)(base + kIsMapValid), (void*)IsMapValid_h, &g_validOriginal);
    Detour_Attach(&g_loadDetour, (uint64_t)(base + kLoadXAssets), (void*)LoadXAssets_h, &g_loadOriginal);

    if (RangeReadable(base + kDlcBitForMap, sizeof(k_dlcBitForMap)) &&
        memcmp((const void*)(base + kDlcBitForMap), k_dlcBitForMap, sizeof(k_dlcBitForMap)) == 0)
    {
        Detour_Attach(&g_dlcBitDetour, (uint64_t)(base + kDlcBitForMap), (void*)DlcBitForMap_h, &g_dlcBitOriginal);
    }

    TreatUnknownMapsAsBase(base);

    if (RangeReadable(base + kBlockJob, sizeof(k_blockJob)) &&
        memcmp((const void*)(base + kBlockJob), k_blockJob, sizeof(k_blockJob)) == 0)
    {
        Detour_Attach(&g_blockDetour, (uint64_t)(base + kBlockJob), (void*)BlockJob_h, &g_blockOriginal);
    }

    if (RangeReadable(base + kCodeTexture, sizeof(k_codeTexture)) &&
        memcmp((const void*)(base + kCodeTexture), k_codeTexture, sizeof(k_codeTexture)) == 0)
    {
        Detour_Attach(&g_codeTextureDetour, (uint64_t)(base + kCodeTexture), (void*)CodeTexture_h,
                      &g_codeTextureOriginal);
    }

    const struct
    {
        uintptr_t      offset;
        const uint8_t* bytes;
        size_t         size;
        Detour*        detour;
        void*          hook;
        void**         original;
    } loadingHooks[] =
    {
        { kGdtMapsTable, k_mapsTable,    sizeof(k_mapsTable),    &g_mapsTableDetour,    (void*)MapsTable_h,    &g_mapsTableOriginal },
        { kPathRemap,    k_pathRemap,    sizeof(k_pathRemap),    &g_pathRemapDetour,    (void*)PathRemap_h,    &g_pathRemapOriginal },
    };

    for (const auto& hook : loadingHooks)
    {
        if (RangeReadable(base + hook.offset, hook.size) &&
            memcmp((const void*)(base + hook.offset), hook.bytes, hook.size) == 0)
        {
            Detour_Attach(hook.detour, (uint64_t)(base + hook.offset), hook.hook, hook.original);
        }
    }

    if (RangeReadable(base + kLinkXAsset, sizeof(k_linkXAsset)) &&
        memcmp((const void*)(base + kLinkXAsset), k_linkXAsset, sizeof(k_linkXAsset)) == 0 &&
        RangeReadable(base + kZonePriority, sizeof(k_zonePriority)) &&
        memcmp((const void*)(base + kZonePriority), k_zonePriority, sizeof(k_zonePriority)) == 0)
    {
        Detour_Attach(&g_priorityDetour, (uint64_t)(base + kZonePriority), (void*)ZonePriority_h, &g_priorityOriginal);

        if (g_priorityOriginal)
            Detour_Attach(&g_linkDetour, (uint64_t)(base + kLinkXAsset), (void*)LinkXAsset_h, &g_linkOriginal);
    }

    if (RangeReadable(base + kPatchZones, sizeof(k_patchZones)) &&
        memcmp((const void*)(base + kPatchZones), k_patchZones, sizeof(k_patchZones)) == 0)
    {
        Detour_Attach(&g_patchZonesDetour, (uint64_t)(base + kPatchZones), (void*)PatchZones_h, &g_patchZonesOriginal);
    }

    if (!g_patchZonesOriginal || !g_priorityOriginal)
        T7Log_Write("[Mods] mods stay off: the zone patch or priority hook did not install");

    if (RangeReadable(base + kXPakOpen, sizeof(k_xpakOpen)) &&
        memcmp((const void*)(base + kXPakOpen), k_xpakOpen, sizeof(k_xpakOpen)) == 0)
    {
        Detour_Attach(&g_xpakOpenDetour, (uint64_t)(base + kXPakOpen), (void*)XPakOpen_h, &g_xpakOpenOriginal);
    }

    if (RangeReadable(base + kLoadXFile, sizeof(k_loadXFile)) &&
        memcmp((const void*)(base + kLoadXFile), k_loadXFile, sizeof(k_loadXFile)) == 0)
    {
        Detour_Attach(&g_loadXFileDetour, (uint64_t)(base + kLoadXFile), (void*)LoadXFile_h, &g_loadXFileOriginal);
    }

    if (RangeReadable(base + kLoadLevelZones, sizeof(k_loadLevelZones)) &&
        memcmp((const void*)(base + kLoadLevelZones), k_loadLevelZones, sizeof(k_loadLevelZones)) == 0)
    {
        Detour_Attach(&g_levelZonesDetour, (uint64_t)(base + kLoadLevelZones), (void*)LoadLevelZones_h,
                      &g_levelZonesOriginal);
    }

    if (!g_levelZonesOriginal)
        T7Log_Write("[Mods] removing a mod needs a restart: the level zone hook did not install");

    g_frontendReloadReady = RangeReadable(base + kKillServer, sizeof(k_killServer)) &&
                            memcmp((const void*)(base + kKillServer), k_killServer, sizeof(k_killServer)) == 0;

    if (!g_frontendReloadReady)
        T7Log_Write("[Mods] the menu reload call is not where this build expects it");

    if (RangeReadable(base + kPlayMovie, sizeof(k_playMovie)) &&
        memcmp((const void*)(base + kPlayMovie), k_playMovie, sizeof(k_playMovie)) == 0 &&
        RangeReadable(base + kFrontendMovies, 24))
    {
        Detour_Attach(&g_playMovieDetour, (uint64_t)(base + kPlayMovie), (void*)PlayMovie_h, &g_playMovieOriginal);
    }

    if (!g_playMovieOriginal)
        T7Log_Write("[Mods] the menu reload keeps the Zombies loading movie");

    if (!g_xpakOpenOriginal || !g_loadXFileOriginal || !g_linkOriginal)
        T7Log_Write("[Mods] only one mod at a time: the xpak, zone file or link hook did not install");

    if (RangeReadable(base + kDemoStart, sizeof(k_demoStart)) &&
        memcmp((const void*)(base + kDemoStart), k_demoStart, sizeof(k_demoStart)) == 0)
    {
        Detour_Attach(&g_demoStartDetour, (uint64_t)(base + kDemoStart), (void*)DemoStart_h, &g_demoStartOriginal);
    }

    if (RangeReadable(base + kGetDvarString, sizeof(k_getDvarString)) &&
        memcmp((const void*)(base + kGetDvarString), k_getDvarString, sizeof(k_getDvarString)) == 0)
    {
        Detour_Attach(&g_getDvarStringDetour, (uint64_t)(base + kGetDvarString), (void*)GetDvarString_h,
                      &g_getDvarStringOriginal);
    }

    if (RangeReadable(base + kPlayViewmodelFx, sizeof(k_playViewmodelFx)) &&
        memcmp((const void*)(base + kPlayViewmodelFx), k_playViewmodelFx, sizeof(k_playViewmodelFx)) == 0)
    {
        Detour_Attach(&g_viewmodelFxDetour, (uint64_t)(base + kPlayViewmodelFx), (void*)PlayViewmodelFx_h,
                      &g_viewmodelFxOriginal);
    }

    if (RangeReadable(base + kTexturesLoaded, sizeof(k_texturesLoaded)) &&
        memcmp((const void*)(base + kTexturesLoaded), k_texturesLoaded, sizeof(k_texturesLoaded)) == 0)
    {
        Detour_Attach(&g_texturesDetour, (uint64_t)(base + kTexturesLoaded), (void*)TexturesLoaded_h,
                      &g_texturesOriginal);
    }

    if (RangeReadable(base + kGpuFxQueue, sizeof(k_gpuFxQueue)) &&
        memcmp((const void*)(base + kGpuFxQueue), k_gpuFxQueue, sizeof(k_gpuFxQueue)) == 0)
    {
        Detour_Attach(&g_gpuFxQueueDetour, (uint64_t)(base + kGpuFxQueue), (void*)GpuFxQueue_h, &g_gpuFxQueueOriginal);
    }

#ifdef _DEBUG
    WatchBuiltins(false);
    StartStallWatch();

    if (RangeReadable(base + kDispatch, sizeof(k_dispatch)) &&
        memcmp((const void*)(base + kDispatch), k_dispatch, sizeof(k_dispatch)) == 0)
    {
        Detour_Attach(&g_dispatchDetour, (uint64_t)(base + kDispatch), (void*)Dispatch_h, &g_dispatchOriginal);
    }
#endif
}

bool T7Maps_BuildSupported(uintptr_t base)
{
    return base != 0 && T7Maps::BuildMatches(base);
}

void T7Maps_Install(uintptr_t base)
{
    using namespace T7Maps;

    static bool installed = false;

    if (installed || !base)
        return;

    installed = true;

    if (!BuildMatches(base))
        return;

    g_buildMatched = true;
    g_base = base;

    ScanAll();
    ReadChosenMods();

    if (g_chosenCount > 0)
        T7Log_Write("[Mods] %d mod(s) ticked, first %s", g_chosenCount, g_chosen[0]);

    InstallHooks(base);
}

bool T7Maps_Refresh()
{
    using namespace T7Maps;

    if (!g_buildMatched || !g_base || g_inLevel)
        return false;

    BeginListWrite();
    ScanAll();
    EndListWrite();

    if (g_packageCount || g_modCount)
        InstallHooks(g_base);

    return true;
}

bool T7Maps_Uninstall(const char* map, char* message, size_t size)
{
    using namespace T7Maps;

    if (!g_buildMatched || !g_base || g_inLevel)
    {
        snprintf(message, size, "maps can only be uninstalled from the menus");
        return false;
    }

    if (!map || !IsToken(map))
    {
        snprintf(message, size, "that is not the name of a map");
        return false;
    }

    int idle = 0;

    if (!g_uninstallState.compare_exchange_strong(idle, 1))
    {
        snprintf(message, size, "another map is still being uninstalled");
        return false;
    }

    char root[224] = "";

    {
        ListReader reading;
        const int package = MapPackage(map);

        for (int i = -1; package >= 0 && !root[0] && i < (int)(sizeof(k_driveRoots) / sizeof(k_driveRoots[0])); ++i)
        {
            const Package& found = g_packages[package];
            char candidate[224];

            if (i < 0)
                snprintf(candidate, sizeof(candidate), "/data/BO3-Customs/usermaps/%s", found.name);
            else
                snprintf(candidate, sizeof(candidate), "/%s/BO3-Customs/usermaps/%s", k_driveRoots[i], found.name);

            if (strcmp(candidate, found.folder) == 0 || strcmp(candidate, found.alternate) == 0)
                snprintf(root, sizeof(root), "%s", candidate);
        }
    }

    if (!root[0])
    {
        g_uninstallState.store(0);
        snprintf(message, size, "%s is not installed in a BO3-Customs usermaps folder", map);
        return false;
    }

    snprintf(g_uninstallMap, sizeof(g_uninstallMap), "%s", map);
    snprintf(g_uninstallRoot, sizeof(g_uninstallRoot), "%s", root);
    g_uninstallMessage[0] = 0;
    g_uninstallRemoved = false;

    OrbisPthread thread;

    if (scePthreadCreate(&thread, nullptr, UninstallThread, nullptr, "BO3-Customs Uninstall") != 0)
    {
        g_uninstallState.store(0);
        snprintf(message, size, "the uninstall could not start");
        return false;
    }

    scePthreadDetach(thread);
    T7Log_Write("[Maps] uninstalling %s from %s", map, root);
    message[0] = 0;
    return true;
}

int T7Maps_UninstallResult(char* map, size_t mapSize, char* message, size_t messageSize)
{
    using namespace T7Maps;

    if (g_uninstallState.load() != 2)
        return -1;

    snprintf(map, mapSize, "%s", g_uninstallMap);
    snprintf(message, messageSize, "%s", g_uninstallMessage);
    const int removed = g_uninstallRemoved ? 1 : 0;
    g_uninstallState.store(0);
    return removed;
}

int T7Maps_MapCount()
{
    return T7Maps::g_buildMatched ? T7Maps::g_listedMaps : -1;
}

bool T7Maps_InLevel()
{
    return T7Maps::g_inLevel;
}

const char* T7Maps_CustomMapsLua()
{
    return T7Maps::g_customMapsLua[0] ? T7Maps::g_customMapsLua : "rawset(_G, \"BO3CustomMaps\", {})\n";
}

bool T7Maps_MapFile(const char* map, const char* file, char* path, size_t size)
{
    using namespace T7Maps;

    ListReader reading;
    const int package = g_base && map && file ? MapPackage(map) : -1;
    return package >= 0 && FindPackageFile(g_packages[package], file, path, size);
}

const char* T7Maps_PreviewFile(const char* map)
{
    using namespace T7Maps;

    ListReader reading;
    const int package = g_base && map ? MapPackage(map) : -1;
    return package >= 0 && g_packages[package].preview[0] ? g_packages[package].preview : nullptr;
}

void T7Maps_RequestApply()
{
    using namespace T7Maps;

    if (g_reloadHeld && g_reloadCheck)
        g_reloadSince = sceKernelGetProcessTime();

    g_reloadHeld = false;
    g_applyWanted.store(true);
}

void T7Maps_RequestPrefetch()
{
    T7Maps::g_reloadHeld = true;
    T7Maps::g_prefetchWanted.store(true);
}

void T7Maps_MenusBuilt()
{
    T7Maps::g_reloadHeld = false;
}

void T7Maps_FrameWork()
{
    using namespace T7Maps;

    if (!g_base)
        return;

    if (g_inLevel)
    {
        g_reloadHeld = false;
        g_unshownCount = 0;
    }

    if (g_applyWanted.exchange(false))
        ApplyModsNow();
    else if (g_prefetchWanted.exchange(false))
        PrefetchMods();

    if (!g_reloadCheck)
    {
        SetRefreshOnClose(false);
        return;
    }

    if (g_inLevel)
    {
        g_reloadCheck = false;
        g_reloadForced = false;
        SetRefreshOnClose(false);
        return;
    }

    int ready = 0;

    for (int i = 0; i < g_waitZoneCount; ++i)
        ready += ZoneStateOf(g_waitZones[i]) >= 2 ? 1 : 0;

    if (ready != g_readyZones)
    {
        g_readyZones = ready;
        g_modsLuaDirty.store(true);
    }

    if (ready < g_waitZoneCount || AnyZoneLoading())
    {
        if (!g_reloadHeld && sceKernelGetProcessTime() - g_reloadSince > kModLoadWaitUs)
        {
            T7Log_Write("[Mods] the new mods are still loading after 60 s; their menus show after your next match");
            g_reloadCheck = false;
            g_reloadForced = false;
        }

        return;
    }

    const bool forced = g_reloadForced;
    bool ui = forced;

    for (int i = g_waitFrom; i < g_stackCount && !ui; ++i)
        ui = IsUiMod(g_stack[i].mod.name);

    if (g_reloadHeld)
    {
        SetRefreshOnClose(ui);
        return;
    }

    g_reloadCheck = false;
    g_reloadForced = false;
    SetRefreshOnClose(false);

    if (!ui)
    {
        T7Log_Write("[Mods] the new mods finished loading; none of them has menu files");
        return;
    }

    ReloadFrontend(forced ? "a mod with menu files was unloaded" : "a mod with menu files finished loading");
}

int T7Maps_SelectMod(const char* list, char* message, size_t size)
{
    using namespace T7Maps;

    if (!g_buildMatched || !g_base || !ModHooksReady())
    {
        snprintf(message, size, "Mods need the PS4 1.33 game.");
        return -1;
    }

    char names[kMaxStack][kNameMax];
    int count = 0;
    bool missing = false;
    bool trimmed = false;
    const bool off = !list || !list[0] || strcmp(list, "!off") == 0;

    for (const char* at = off ? "" : list; *at;)
    {
        const char* end = at;

        while (*end && *end != ',')
            ++end;

        const size_t length = (size_t)(end - at);
        char name[kNameMax];
        snprintf(name, sizeof(name), "%.*s", (int)(length < sizeof(name) ? length : sizeof(name) - 1), at);
        const int index = name[0] ? FindMod(name) : -1;
        bool duplicate = false;

        for (int i = 0; i < count; ++i)
            duplicate = duplicate || (index >= 0 && EqualsNoCase(names[i], g_mods[index].name));

        if (index >= 0 && !duplicate && count < kMaxStack)
            snprintf(names[count++], kNameMax, "%s", g_mods[index].name);
        else if (index >= 0 && !duplicate)
            trimmed = true;
        else if (index < 0 && name[0])
            missing = true;

        at = *end ? end + 1 : end;
    }

    if (count > 1 && !StackHooksReady())
    {
        count = 1;
        trimmed = true;
    }

    if (!WriteChosenMods(names, count))
        T7Log_Write("[Mods] the choice could not be saved to %s", k_chosenModFile);

    g_chosenCount = count;

    for (int i = 0; i < count; ++i)
        snprintf(g_chosen[i], kNameMax, "%s", names[i]);

    g_modsLuaDirty.store(true);
    T7Log_Write("[Mods] ticked: %d mod(s)%s%s", count, count > 0 ? ", first " : "", count > 0 ? names[0] : "");

    const char* const note = missing ? " Some ticked mods are no longer installed."
                             : trimmed ? " Only the first ticked mods fit." : "";
    bool same = count == g_loadedCount;

    for (int i = 0; same && i < count; ++i)
        same = EqualsNoCase(g_loaded[i], names[i]);

    if (count == 0)
    {
        if (g_loadedCount > 0)
        {
            snprintf(message, size, "Mods are off. The loaded mods unload %s.%s", ModChangesApply(), note);
            return 3;
        }

        snprintf(message, size, "Mods are off.%s", note);
        return 0;
    }

    if (same)
    {
        snprintf(message, size, "%s", count == 1 ? "The ticked mod is loaded." : "The ticked mods are loaded.");
        return 4;
    }

    bool extends = g_loadedCount < count;

    for (int i = 0; extends && i < g_loadedCount; ++i)
        extends = EqualsNoCase(g_loaded[i], names[i]);

    if (!extends || g_inLevel)
    {
        snprintf(message, size, "Your mod changes apply %s.%s", ModChangesApply(), note);
        return 3;
    }

    const int last = FindMod(names[count - 1]);
    const char* const title = count - g_loadedCount == 1 && last >= 0 ? g_mods[last].title : nullptr;

    if (title)
        snprintf(message, size, "Loading %s...%s", title, note);
    else
        snprintf(message, size, "Loading %d mods...%s", count - g_loadedCount, note);

    return 2;
}

const char* T7Maps_ModsLua()
{
    using namespace T7Maps;

    if (g_modsLuaDirty.exchange(false) || !g_modsLua[0])
        BuildModsLua();

    return g_modsLua;
}

bool T7Maps_ModsChanged()
{
    return T7Maps::g_modsLuaDirty.load(std::memory_order_relaxed);
}

int T7Maps_ModCount()
{
    return T7Maps::g_buildMatched ? T7Maps::g_modCount : 0;
}

bool T7Maps_UsingMods()
{
    return T7Maps::g_loadedCount > 0 || T7Maps::g_usermapLevel.load(std::memory_order_relaxed);
}

static_assert(T7Maps::kNameMax == kT7ShareName, "shared names use the map name size");
static_assert(sizeof(T7Maps::Mod::folder) == kT7SharePath, "shared paths use the folder size");

static void ContentRoot(const char* folder, const char* alternate, char* out, size_t size)
{
    snprintf(out, size, "%s", T7Maps::EndsWith(folder, "/zone") && alternate[0] ? alternate : folder);
}

bool T7Maps_HasMap(const char* name)
{
    using namespace T7Maps;

    if (!g_base || !name || !name[0])
        return false;

    ListReader reading;
    return MapPackage(name) >= 0;
}

bool T7Maps_EngineHasMap(const char* name)
{
    using namespace T7Maps;

    if (!g_base || !name || !RangeReadable(g_base + kMapTableCount, sizeof(int32_t)))
        return false;

    const int32_t count = *(const int32_t*)(g_base + kMapTableCount);

    if (count <= 0 || count > kMapTableMax || !RangeReadable(g_base + kMapTable, (size_t)count * kMapTableStride))
        return false;

    uint32_t hash = 5381;

    for (const unsigned char* c = (const unsigned char*)name; *c; ++c)
        hash = (uint32_t)((*c >= 'A' && *c <= 'Z') ? *c + 32 : *c) + 33 * hash;

    for (int32_t i = 0; i < count; ++i)
    {
        if (*(const uint32_t*)(g_base + kMapTable + kMapTableStride * (uintptr_t)i) == hash)
            return true;
    }

    return false;
}

bool T7Maps_HasMod(const char* name)
{
    using namespace T7Maps;

    if (!g_base || !name)
        return false;

    ListReader reading;
    return FindMod(name) >= 0;
}

void T7Maps_SetSessionMods(const char (*names)[kT7ShareName], int count)
{
    using namespace T7Maps;

    if (!names || count < 0)
    {
        if (g_sessionModCount >= 0)
            T7Log_Write("[Mods] left the host's lobby: your own ticked mods apply again");

        g_sessionModCount = -1;
        return;
    }

    int kept = 0;

    for (int i = 0; i < count && kept < kMaxStack; ++i)
        snprintf(g_sessionMods[kept++], kNameMax, "%s", names[i]);

    g_sessionModCount = kept;
    T7Log_Write("[Mods] this lobby loads the host's %d mod(s) in place of your %d ticked", kept, g_chosenCount);
}

bool T7Maps_ShareMap(const char* map, char* root, size_t size)
{
    using namespace T7Maps;

    if (!g_base || !map || !map[0])
        return false;

    ListReader reading;
    const int package = MapPackage(map);

    if (package < 0)
        return false;

    ContentRoot(g_packages[package].folder, g_packages[package].alternate, root, size);
    return true;
}

int T7Maps_ShareMods(char (*names)[kT7ShareName], char (*roots)[kT7SharePath], int capacity)
{
    using namespace T7Maps;

    if (!g_base)
        return 0;

    char wanted[kMaxStack][kNameMax];
    int count = 0;

    if (g_inLevel)
    {
        for (; count < g_loadedCount && count < kMaxStack; ++count)
            snprintf(wanted[count], kNameMax, "%s", g_loaded[count]);
    }
    else
    {
        count = ChosenForLoad(wanted);
    }

    ListReader reading;
    int listed = 0;

    for (int i = 0; i < count && listed < capacity; ++i)
    {
        const int index = FindMod(wanted[i]);

        if (index < 0)
            continue;

        snprintf(names[listed], kT7ShareName, "%s", g_mods[index].name);
        ContentRoot(g_mods[index].folder, g_mods[index].alternate, roots[listed], kT7SharePath);
        ++listed;
    }

    return listed;
}

void T7Maps_ShareRoot(bool mods, char* out, size_t size)
{
    using namespace T7Maps;

    static const char k_data[] = "/data/BO3-Customs";
    const char* const kind = mods ? "mods" : "usermaps";

    snprintf(out, size, "%s", k_data);

    if (!g_base)
        return;

    ListReader reading;
    int best = 0;

    for (int drive = -1; drive < (int)(sizeof(k_driveRoots) / sizeof(k_driveRoots[0])); ++drive)
    {
        char root[64];

        if (drive < 0)
            snprintf(root, sizeof(root), "%s", k_data);
        else
            snprintf(root, sizeof(root), "/%s/BO3-Customs", k_driveRoots[drive]);

        char prefix[96];
        const size_t length = (size_t)snprintf(prefix, sizeof(prefix), "%s/%s/", root, kind);
        int count = 0;

        if (mods)
        {
            for (int i = 0; i < g_modCount; ++i)
                count += strncmp(g_mods[i].folder, prefix, length) == 0 ? 1 : 0;
        }
        else
        {
            for (int i = 0; i < g_packageCount; ++i)
                count += g_packages[i].map && strncmp(g_packages[i].folder, prefix, length) == 0 ? 1 : 0;
        }

        if (count > best)
        {
            best = count;
            snprintf(out, size, "%s", root);
        }
    }
}

void T7Maps_FrameHeartbeat()
{
#ifdef _DEBUG
    using namespace T7Maps;

    if (g_base)
        FrameHeartbeat();
#endif
}
