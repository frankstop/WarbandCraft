// Shared-memory protocol between mhw-bridge (Windows DLL inside MHW, running under
// CrossOver/Wine) and mc-bridge (Fabric mod, native macOS JVM).
//
// Transport: one file-backed mapping. Wine maps file views with MAP_SHARED, so both
// processes see the same physical pages (verified under CrossOver on Apple Silicon):
//     macOS : /tmp/mhwmc/bridge.shm
//     Wine  : Z:\tmp\mhwmc\bridge.shm
//
// Everything is little-endian. Blocks that are written while the other side may be
// reading use a seqlock: the writer bumps `seq` to an odd value, writes the payload,
// then bumps it to the next even value. Readers retry if `seq` was odd or changed.
//
// The Java mirror of these offsets lives in mc-bridge: dev.mhwmc.bridge.client.shm.Protocol.
// Keep both files in sync.
#pragma once
#include <stdint.h>

#define WBMC_MAGIC 0x434D4257u  /* bytes "WBMC" */
#define WBMC_VERSION 1u
#define WBMC_SHM_SIZE (8u * 1024u * 1024u)

#define WBMC_OFF_HEADER   0x000000u
#define WBMC_OFF_STATE    0x000100u  /* WbmcGameState, written by MHW          */
#define WBMC_OFF_CONTROL  0x000800u  /* WbmcControl,   written by Minecraft    */
#define WBMC_OFF_HUNTER   0x000A00u  /* WbmcHunterEvents, written by MHW              */
#define WBMC_OFF_CMD      0x001000u  /* WbmcCmdBlock,  debug/RE command mailbox */
#define WBMC_OFF_CMD_RESP 0x002000u  /* command response payload                */
#define WBMC_CMD_RESP_MAX (0x100000u - WBMC_OFF_CMD_RESP)
#define WBMC_OFF_RAYS     0x100000u  /* WbmcRayHeader + rays + hits (terrain queries)  */
#define WBMC_OFF_ENTITIES 0x200000u  /* WbmcEntityTable, written by MHW                */
#define WBMC_OFF_DAMAGE   0x280000u  /* WbmcDamageQueue, written by Minecraft          */
#define WBMC_OFF_HAZARDS  0x290000u  /* WbmcHazardTable, written by MHW                */

#pragma pack(push, 4)

typedef struct WbmcHeader {
    uint32_t magic;               /* 0x00 */
    uint32_t version;             /* 0x04 */
    uint32_t size;                /* 0x08 */
    uint32_t reserved0;           /* 0x0C */
    volatile uint64_t mhwHeartbeat;  /* 0x10 incremented once per MHW frame (Present) */
    volatile uint64_t mcHeartbeat;   /* 0x18 incremented once per Minecraft frame      */
    volatile uint32_t mhwPid;     /* 0x20 */
    volatile uint32_t mcPid;      /* 0x24 */
    volatile uint64_t mhwStartMs; /* 0x28 unix ms when the DLL attached */
    volatile uint64_t mcStartMs;  /* 0x30 unix ms when the mod attached */
    /* Hot reload of mhwbridge_core.dll: a client bumps coreReloadReq after copying a new
     * build to <game>\mhwbridge\core_next.dll; the dinput8 loader swaps the core and sets
     * coreReloadAck = coreReloadReq. coreGeneration counts successful loads. */
    volatile uint32_t coreReloadReq;  /* 0x38 */
    volatile uint32_t coreReloadAck;  /* 0x3C */
    volatile uint32_t coreGeneration; /* 0x40 */
    volatile int32_t coreStatus;      /* 0x44 1 = running, 0 = not loaded, <0 = load error */
    /* Switching control between the games (F8 on either side). */
    volatile uint32_t mcSwitchReq;    /* 0x48 MHW -> MC: F8 pressed in MHW, give control back to Minecraft */
    volatile uint32_t mhwFocusReq;    /* 0x4C MC -> MHW: bring MHW's window to the front */
} WbmcHeader;

/* WbmcGameState.flags */
#define WBMC_STATE_CAMERA_VALID   (1u << 0)
#define WBMC_STATE_PLAYER_VALID   (1u << 1)
#define WBMC_STATE_WINDOW_VALID   (1u << 2)
#define WBMC_STATE_CAM_OVERRIDDEN (1u << 3) /* MHW rendered this frame with the Minecraft camera */
#define WBMC_STATE_MATRICES_VALID (1u << 4)
#define WBMC_STATE_WINDOW_FOCUSED (1u << 5)
#define WBMC_STATE_COMPOSITING    (1u << 6) /* MHW is drawing Minecraft's frames itself */

typedef struct WbmcGameState {
    volatile uint32_t seq;        /* 0x00 seqlock */
    uint32_t flags;               /* 0x04 WBMC_STATE_* */
    uint64_t frame;               /* 0x08 MHW frame counter */
    float camPos[3];              /* 0x10 camera eye, MHW world units */
    float camTarget[3];           /* 0x1C camera look-at point */
    float camUp[3];               /* 0x28 */
    float fovYDeg;                /* 0x34 vertical field of view, degrees */
    float nearZ;                  /* 0x38 */
    float farZ;                   /* 0x3C */
    float aspect;                 /* 0x40 */
    float unitsPerMeter;          /* 0x44 MHW units per meter (100 if MHW uses cm) */
    float playerPos[3];           /* 0x48 hunter position */
    float playerQuat[4];          /* 0x54 hunter rotation (x,y,z,w) */
    int32_t winX, winY, winW, winH; /* 0x64 game client area in screen coordinates */
    uint32_t bbW, bbH;            /* 0x74 swapchain back buffer size */
    uint32_t stageId;             /* 0x7C current zone id (sPlayer+0xAED0: 504 Training Area, 101 Ancient Forest...), 0 if unknown */
    float view[16];               /* 0x80 view matrix as the game stores it (if known) */
    float proj[16];               /* 0xC0 projection matrix as the game stores it */
} WbmcGameState;                  /* 0x100 */

/* WbmcControl.flags */
#define WBMC_CTRL_OVERRIDE_CAMERA (1u << 0) /* drive the MHW camera from camPos/camTarget/fov */
#define WBMC_CTRL_MOVE_HUNTER     (1u << 1) /* hunter stands in for the Minecraft player at hunterPos (takes its hits) */
#define WBMC_CTRL_HIDE_HUNTER     (1u << 2)
#define WBMC_CTRL_CAPTURE_DEPTH   (1u << 3) /* reserved */
#define WBMC_CTRL_COMPOSITE       (1u << 4) /* draw Minecraft's frames (frames.shm) into MHW's frame */
#define WBMC_CTRL_NO_DEPTH_TEST   (1u << 5) /* debug: composite without occlusion */
#define WBMC_CTRL_DEBUG_DEPTH     (1u << 6) /* debug: show MHW's depth buffer instead */
#define WBMC_CTRL_NO_RELIGHT      (1u << 7) /* debug: no MHW lighting on Minecraft pixels */

typedef struct WbmcControl {
    volatile uint32_t seq;        /* 0x00 seqlock */
    uint32_t flags;               /* 0x04 WBMC_CTRL_* */
    uint64_t mcFrame;             /* 0x08 */
    float camPos[3];              /* 0x10 desired camera eye, MHW units */
    float camTarget[3];           /* 0x1C desired look-at point */
    float camUp[3];               /* 0x28 */
    float fovYDeg;                /* 0x34 desired vertical FOV */
    float hunterPos[3];           /* 0x38 */
    uint32_t poseLag;             /* 0x44 game frames between applying a pose and presenting it */
    uint32_t depthIndex;          /* 0x48 which MHW depth buffer is the scene depth (0/1) */
    float lightGain;              /* 0x4C relighting: light = lightMin + ambientLum * lightGain (0 = default) */
    float lightMin;               /* 0x50 */
    float fogStrength;            /* 0x54 0 = default */
    float hunterYawDeg;           /* 0x58 hunter facing (Minecraft yaw, degrees) when MOVE_HUNTER */
    uint32_t epoch; /* native mission generation; extension in the reserved control block */
} WbmcControl;                    /* 0x5C */

/* Hits taken by the hunter while it stands in for the Minecraft player (MOVE_HUNTER):
 * monotonic counters, so Minecraft applies the difference since its last read. */
typedef struct WbmcHunterEvents {
    volatile uint32_t seq;        /* 0x00 seqlock */
    uint32_t hitCount;            /* 0x04 */
    float totalDamage;            /* 0x08 MHW HP lost, summed */
    float lastDamage;             /* 0x0C */
    float lastHitFrom[3];         /* 0x10 position of the monster closest to the hunter at the last hit */
    float hunterMaxHp;            /* 0x1C */
    uint64_t lastHitFrame;        /* 0x20 */
    uint32_t lastHitKind;         /* 0x28 WBMC_ENT_* of the monster closest to the hunter at the last hit */
    float totalStatusDamage;      /* 0x2C MHW HP lost to status damage (poison, fire...: < 1 HP per frame), summed */
} WbmcHunterEvents;               /* 0x30 */

/* Debug/RE command mailbox. The client fills cmd/argLen/args, then increments reqSeq.
 * The DLL executes it, fills status/respLen and the payload at WBMC_OFF_CMD_RESP, then
 * sets respSeq = reqSeq. */
enum WbmcCmd {
    WBMC_CMD_PING       = 1, /* -> text info */
    WBMC_CMD_READ       = 2, /* {u64 addr, u32 len} -> bytes */
    WBMC_CMD_WRITE      = 3, /* {u64 addr, u32 len, bytes} -> nothing */
    WBMC_CMD_READ_MANY  = 4, /* {u32 count, u32 len, u64 addrs[count]} -> u8 ok[count], then count*len bytes */
    WBMC_CMD_SCAN       = 5, /* {u64 start, u64 end, u32 patLen, u32 maxResults, u32 flags, u8 pat[patLen], u8 mask[patLen]} -> u64[]
                                (status -3: cut short by a core reload, results partial) */
    WBMC_CMD_SCAN_FLOAT = 6, /* {u64 start, u64 end, f32 lo, f32 hi, u32 align, u32 maxResults, u32 flags} -> u64[] (status -3 as SCAN) */
    WBMC_CMD_QUERY      = 7, /* {u64 addr} -> {u64 base, u64 allocBase, u64 size, u32 state, u32 protect, u32 type} */
    WBMC_CMD_MODULES    = 8, /* -> repeated {u64 base, u32 size, u16 nameLen, char name[nameLen]} */
    WBMC_CMD_RAYCAST    = 9, /* {f32 start[3], f32 end[3], u32 flags} -> WbmcRayHit, run on the game thread */
    WBMC_CMD_FIRE_SHELL = 10, /* {u32 slingerShellIndex, f32 origin[3], f32 target[3]} -> u64 shell, game thread */
    WBMC_CMD_MONSTER    = 11, /* {u64 uEnemy*, i32 condition (-1 none), i32 action index in set 1 (-1 none)}, game thread:
                                 activates the condition (3 = sleep) and launches the action. Status -11 = no such live
                                 monster, -2 bad condition, -3 bad action index */
};

/* scan flags: which memory to scan */
#define WBMC_SCAN_IMAGE   (1u << 0)
#define WBMC_SCAN_PRIVATE (1u << 1)
#define WBMC_SCAN_MAPPED  (1u << 2)
#define WBMC_SCAN_EXEC_ONLY (1u << 3)

typedef struct WbmcCmdBlock {
    volatile uint32_t reqSeq;     /* 0x00 */
    volatile uint32_t respSeq;    /* 0x04 */
    uint32_t cmd;                 /* 0x08 */
    uint32_t argLen;              /* 0x0C */
    int32_t status;               /* 0x10 0 = ok, negative = error */
    uint32_t respLen;             /* 0x14 */
    uint8_t args[0x1000 - 0x18];  /* 0x18 */
} WbmcCmdBlock;

/* Ray queries: Minecraft asks MHW's own collision system for terrain. The client writes
 * `count` WbmcRay entries, then bumps reqSeq. The DLL runs them on the game thread (a few
 * hundred per frame), fills WbmcRayHit entries and sets respSeq = reqSeq when done. */
#define WBMC_MAX_RAYS 8192
#define WBMC_RAYS_CAMERA_FILTER (1u << 0) /* use the camera's collision attribute filter */
#define WBMC_RAYS_CUSTOM_FILTER (1u << 1) /* use filterA/B/C (Param::setAttr arguments) */

typedef struct WbmcRay {
    float start[3];
    float end[3];
} WbmcRay;                        /* 24 bytes */

typedef struct WbmcRayHit {
    float pos[3];                 /* hit position (valid if hit != 0) */
    float normal[3];              /* surface normal at the hit */
    uint32_t hit;                 /* 0 = no hit, otherwise the raw return value */
    uint32_t attr;                /* collision attribute bits of the surface that was hit */
} WbmcRayHit;                     /* 32 bytes */

typedef struct WbmcRayHeader {
    volatile uint32_t reqSeq;     /* 0x00 */
    volatile uint32_t respSeq;    /* 0x04 */
    uint32_t count;               /* 0x08 rays in this batch (<= WBMC_MAX_RAYS) */
    uint32_t flags;               /* 0x0C WBMC_RAYS_* */
    volatile uint32_t processed;  /* 0x10 progress, written by MHW */
    uint32_t filterA;             /* 0x14 with WBMC_RAYS_CUSTOM_FILTER: Param::setAttr(a, b, c) */
    uint32_t filterB;             /* 0x18 */
    uint32_t filterC;             /* 0x1C */
    /* WbmcRay rays[WBMC_MAX_RAYS] at 0x20, WbmcRayHit hits[WBMC_MAX_RAYS] after them */
} WbmcRayHeader;

#define WBMC_RAYS_OFF_RAYS 0x20u
#define WBMC_RAYS_OFF_HITS (WBMC_RAYS_OFF_RAYS + WBMC_MAX_RAYS * 24u)

/* Hittable MHW entities (monsters etc.), republished every frame. Each gets an invisible
 * proxy entity in Minecraft with the same hitbox, so Minecraft attacks can target it. */
#define WBMC_MAX_ENTITIES 1024
#define WBMC_ENT_LARGE_MONSTER 1u
#define WBMC_ENT_SMALL_MONSTER 2u
#define WBMC_ENT_OTHER         3u

typedef struct WbmcEntity {
    uint64_t id;                  /* 0x00 MHW object address; stable while the entity lives */
    uint32_t kind;                /* 0x08 WBMC_ENT_* */
    uint32_t emId;                /* 0x0C species id if known */
    float pos[3];                 /* 0x10 origin (feet), MHW world units */
    float quat[4];                /* 0x1C rotation (x,y,z,w) */
    float boxCenter[3];           /* 0x2C hitbox center, world units */
    float boxHalf[3];             /* 0x38 hitbox half size: entity axes (x side, y up, z forward), or world axes if flags bit1 */
    float hp;                     /* 0x44 */
    float maxHp;                  /* 0x48 */
    uint32_t flags;               /* 0x4C bit0 dead/capturing, bit1 box is world-aligned (ignore quat) */
    char name[48];                /* 0x50 class or display name, NUL-terminated */
} WbmcEntity;                     /* 0x80 */

typedef struct WbmcEntityTable {
    volatile uint32_t seq;        /* 0x00 seqlock */
    uint32_t count;               /* 0x04 */
    uint64_t frame;               /* 0x08 */
    WbmcEntity entities[WBMC_MAX_ENTITIES]; /* 0x10 */
} WbmcEntityTable;

/* Minecraft -> MHW: hits to apply. Single-producer ring: Minecraft writes ring[write % N]
 * then bumps `write`; MHW applies entries until `read == write`. */
#define WBMC_DAMAGE_RING 256
typedef struct WbmcDamage {
    uint64_t id;                  /* target WbmcEntity.id */
    float amount;                 /* MHW HP to remove */
    float hitPos[3];              /* where the hit landed (world units) */
    uint32_t flags;               /* WBMC_DAMAGE_* */
    uint32_t reserved;
} WbmcDamage;                     /* 0x20 */

#define WBMC_DAMAGE_CRITICAL (1u << 0)
#define WBMC_DAMAGE_OUTWARD  (1u << 1) /* explosion/projectile: hitPos is the body point nearest the blow; the
                                          hunter's slinger shot comes from outside the body at that point */

typedef struct WbmcDamageQueue {
    volatile uint32_t write;      /* 0x00 */
    volatile uint32_t read;       /* 0x04 */
    uint32_t reserved[2];         /* 0x08 */
    WbmcDamage ring[WBMC_DAMAGE_RING]; /* 0x10 */
} WbmcDamageQueue;

/* MHW -> Minecraft: live attacks of MHW monsters that affect Minecraft's world (a
 * fireball's flames light TNT), republished every frame. */
#define WBMC_MAX_HAZARDS 64
#define WBMC_HAZARD_FIRE (1u << 0)

typedef struct WbmcHazard {
    uint64_t id;                  /* 0x00 MHW object address (the attack's shell) */
    uint64_t ownerId;             /* 0x08 WbmcEntity.id of the monster that made it, 0 if unknown */
    float pos[3];                 /* 0x10 center, MHW world units */
    float radius;                 /* 0x1C reach around pos, MHW world units */
    uint32_t flags;               /* 0x20 WBMC_HAZARD_* */
    uint32_t serial;              /* 0x24 new for each shell seen: a freed shell's address can be reused */
    uint32_t reserved[2];         /* 0x28 */
} WbmcHazard;                     /* 0x30 */

typedef struct WbmcHazardTable {
    volatile uint32_t seq;        /* 0x00 seqlock */
    uint32_t count;               /* 0x04 */
    uint64_t frame;               /* 0x08 */
    WbmcHazard hazards[WBMC_MAX_HAZARDS]; /* 0x10 */
} WbmcHazardTable;

/* ------------------------------------------------------------------------------------
 * Frame passthrough: Minecraft's rendered frames, composited by the DLL into MHW's own frame
 * (depth-tested against MHW's depth buffer, with the exact camera pose MHW rendered).
 * Separate file so the control file stays small:
 *     macOS : /tmp/mhwmc/frames.shm        Wine : Z:\tmp\mhwmc\frames.shm
 * Layout: WbmcFramesHeader at 0, then WBMC_FRAME_SLOTS slots of WBMC_FRAME_SLOT_SIZE bytes.
 * Slot = WbmcFrameHeader + world RGBA8 + world depth (float32, OpenGL window depth [0,1])
 *        + GUI RGBA8, each width*height, rows bottom-up (OpenGL order), premultiplied alpha.
 * Minecraft writes a slot, then publishes the frame's pose in WbmcControl (mcFrame = poseId),
 * so by the time MHW renders a pose, the matching pixels are already available. */
#define WBMC_FRAMES_MAGIC 0x524D4257u  /* "MHMR" */
#define WBMC_FRAME_MAX_W 1920u
#define WBMC_FRAME_MAX_H 1200u
#define WBMC_FRAME_SLOTS 3u
#define WBMC_FRAME_HDR 0x100u
#define WBMC_FRAME_SLOT_SIZE (WBMC_FRAME_HDR + WBMC_FRAME_MAX_W * WBMC_FRAME_MAX_H * 12u)
#define WBMC_FRAMES_FILE_SIZE (0x1000u + WBMC_FRAME_SLOTS * WBMC_FRAME_SLOT_SIZE)

typedef struct WbmcFramesHeader {
    uint32_t magic;               /* 0x00 */
    uint32_t version;             /* 0x04 */
    volatile uint32_t latestSlot; /* 0x08 slot of the newest complete frame */
    uint32_t reserved;            /* 0x0C */
    volatile uint64_t latestFrameId; /* 0x10 */
} WbmcFramesHeader;

typedef struct WbmcFrameHeader {
    volatile uint32_t seq;        /* 0x00 odd while Minecraft writes the slot */
    uint32_t width;               /* 0x04 */
    uint32_t height;              /* 0x08 */
    uint32_t flags;               /* 0x0C bit0 world layer valid, bit1 GUI layer valid */
    uint64_t frameId;             /* 0x10 Minecraft frame counter */
    uint64_t poseId;              /* 0x18 matches WbmcControl.mcFrame for this frame's camera */
    float mcNear;                 /* 0x20 Minecraft projection near/far, blocks */
    float mcFar;                  /* 0x24 */
    float fovYDeg;                /* 0x28 */
    float aspect;                 /* 0x2C */
} WbmcFrameHeader;

#pragma pack(pop)

#ifdef __cplusplus
static_assert(sizeof(WbmcHeader) == 0x50, "header size");
static_assert(sizeof(WbmcGameState) == 0x100, "state size");
static_assert(sizeof(WbmcControl) == 0x60, "control size");
static_assert(sizeof(WbmcHunterEvents) == 0x30, "hunter events size");
static_assert(sizeof(WbmcCmdBlock) == 0x1000, "cmd size");
static_assert(sizeof(WbmcRayHeader) == 0x20, "ray header size");
static_assert(sizeof(WbmcRay) == 24 && sizeof(WbmcRayHit) == 32, "ray sizes");
static_assert(sizeof(WbmcEntity) == 0x80, "entity size");
static_assert(sizeof(WbmcDamage) == 0x20, "damage size");
static_assert(sizeof(WbmcFrameHeader) <= WBMC_FRAME_HDR, "frame header size");
static_assert(WBMC_OFF_ENTITIES + sizeof(WbmcEntityTable) <= WBMC_OFF_DAMAGE, "entity table fits");
static_assert(WBMC_OFF_DAMAGE + sizeof(WbmcDamageQueue) <= WBMC_OFF_HAZARDS, "damage queue fits");
static_assert(sizeof(WbmcHazard) == 0x30, "hazard size");
static_assert(WBMC_OFF_HAZARDS + sizeof(WbmcHazardTable) <= WBMC_SHM_SIZE, "hazard table fits");
#endif
