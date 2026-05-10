#ifndef GAME_H
#define GAME_H

#include "platform.h"
#include "intrinsics.h"
#include "my_math.h"
#include "random.h"

struct memory_arena
{
    void *Memory;
    memory_index Size;
    memory_index Used;
    int32 TempCount;
};

inline memory_arena
MakeArena(void *Memory, memory_index Size)
{
    memory_arena Arena = {};
    Arena.Memory = Memory;
    Arena.Size = Size;
    return(Arena);
}

struct temp_memory
{
    memory_arena *Arena;
    memory_index Used;
};

inline temp_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temp_memory Result = {Arena, Arena->Used};
    ++Arena->TempCount;
    return(Result);
}

inline void
EndTemporaryMemory(temp_memory TempMem)
{
    TempMem.Arena->Used = TempMem.Used;
    --TempMem.Arena->TempCount;
}

inline void
CheckArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

inline void *
PushSize(memory_arena *Arena, memory_index Size)
{
    Assert(Arena->Used + Size <= Arena->Size);
    void *Result = (uint8 *)Arena->Memory + Arena->Used;
    Arena->Used += Size;
    return(Result);
}

#define PushStruct(Arena, type) (type *)PushSize(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize(Arena, (Count)*sizeof(type))

inline memory_index
GetRemainingSize(memory_arena *Arena)
{
    Assert(Arena->Size >= Arena->Used);
    memory_index Result = Arena->Size - Arena->Used;
    return(Result);
}

inline memory_arena
SubArena(memory_arena *Parent, memory_index Size = 0)
{
    if(Size == 0)
    {
        Size = GetRemainingSize(Parent);
    }
    memory_arena Arena = MakeArena(PushSize(Parent, Size), Size);
    return(Arena);
}

inline void
ClearArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
    Arena->Used = 0;
}

internal void
ZeroMemory(void *Memory, memory_index Size)
{
    uint8 *Dest = (uint8 *)Memory;
    while(Size--)
    {
        *Dest++ = 0;
    }
}

enum bitmap_id
{
    Bitmap_None,
    Bitmap_JetmanWalking,
    Bitmap_JetmanFlying,
    Bitmap_Font,
    Bitmap_Laser,
    Bitmap_Part,
    Bitmap_Fuel,
    Bitmap_Flame,
    Bitmap_Asteroid,
    Bitmap_Face,
    Bitmap_Ground,
    Bitmap_Explosion,
    Bitmap_Lives,

    Bitmap_Count,
};

struct bitmap_info
{
    s32 FrameCount;
    s32 FrameWidth;
    s32 FrameHeight;
    s32 OffsetY;
};

struct atlas
{    
    game_bitmap GroundBitmaps[3];
    game_bitmap JetmanWalkingBitmaps[3];
    game_bitmap JetmanFlyingBitmaps[3];
    game_bitmap ExplosionBitmaps[5];
    game_bitmap PartBitmaps[6];
    game_bitmap AsteroidBitmaps[3];
    game_bitmap FlameBitmaps[3];
    game_bitmap FaceBitmaps[3];
    game_bitmap LaserBitmap;
    game_bitmap FuelBitmap;
    game_bitmap FontBitmap;
    game_bitmap LivesBitmap;

    bitmap_info Infos[Bitmap_Count];

    memory_arena Arena;
    game_bitmap Bitmap;
};

#include "render_group.h"

enum part_state
{
    PartState_Uninstalled,
    PartState_Grabbed,
    PartState_Installing,
    PartState_Installed,
};

struct part
{
    part_state State;
    v2 P;
    v2 V;
};

#define PLAYER_DIM_X 2*TILE_SIZE
#define PLAYER_DIM_Y 3*TILE_SIZE
#define PLAYER_DIM V2i(PLAYER_DIM_X, PLAYER_DIM_Y)

struct player
{
    v2 P;
    v2 V;
    int32 DirX;
    bool32 IsFlying;
    int32 FrameIndex;
    real32 FrameTimer;
    real32 ShotCooldown;
};

enum enemy_type
{
    EnemyType_Asteroid,
    EnemyType_Face,

    EnemyType_Count,
};

struct enemy
{
    bool32 IsActive;
    bool32 EnteredPlayfield;
    v2 P;
    v2 V;
    color Color;
    real32 FrameTimer;
    int32 FrameIndex;
};

struct explosion
{
    bool32 IsActive;
    v2 P;
    real32 FrameTimer;
    int32 FrameIndex;
    color Color;
};

#define TILE_COUNT_Y 22
#define TILE_COUNT_X 32
#define TILE_SIZE 8

struct laser
{
    bool32 IsActive;
    int32 Y;
    int32 DirX;
    real32 DistanceTraveled;
    real32 HeadX;
    real32 TailX;
    color Color;
};

#define HIT_DURATION 0.5f
#define START_DURATION 1.5f

#define ROCKET_X (21*TILE_SIZE)

#define PLAYFIELD_DIM_X (TILE_COUNT_X*TILE_SIZE)
#define PLAYFIELD_DIM_Y (TILE_COUNT_Y*TILE_SIZE)

#define ROCKET_MAX_Y ((TILE_COUNT_Y+4)*TILE_SIZE)
#define ROCKET_MIN_Y TILE_SIZE

#define PART_DIM_X (2*TILE_SIZE)
#define PART_DIM_Y (2*TILE_SIZE)

#define ENEMY_DIM_X 16
#define ENEMY_DIM_Y 10

enum sim_state
{
    SimState_Start,
    SimState_Play,
    SimState_Hit,
    SimState_TakeOff,
    SimState_Landing,
};

struct world
{
    random_series Series;

    player Player;
    uint32 Lives;

    part Parts[9];
    int32 InstalledPartsCount;

    real32 RocketBlinkTimer;
    color RocketBlinkColor;
    real32 RocketY;
    real32 FlameFrameTimer;
    int32 FlameFrameIndex;

    enemy Enemies[5];
    enemy_type EnemyType;

    explosion Explosions[32];

    sim_state SimState;
    real32 Timer;

    laser Lasers[4];
};

enum meta_phase
{
    MetaPhase_Play,
    MetaPhase_GameOver,
};

struct game_state
{
    bool32 IsInitialized;

    memory_arena MainArena;

    meta_phase MetaPhase;
    uint32 Score;
    world *World;
    memory_arena WorldArena;

    uint8 Tiles[TILE_COUNT_Y+1][TILE_COUNT_X];
    uint8 TileColors[TILE_COUNT_Y+1][TILE_COUNT_X];

    atlas *Atlas;
};

struct transient_state
{
    bool32 IsInitialized;
    memory_arena TranArena;
};

global_variable debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;

#endif
