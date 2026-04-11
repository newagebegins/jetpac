#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

#define global_variable static
#define local_persist static
#define internal static

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint32 bool32;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef size_t memory_index;

typedef float real32;
typedef double real64;

#include <float.h>
#define Real32Max FLT_MAX

#define Pi32 3.14159265358979f
#define Tau32 6.283185307179586f
#define DegreesToRadians (Tau32/360.0f)
#define RadiansToDegrees (360.0f/Tau32)

#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#define Kilobytes(X) (1024ULL*(X))
#define Megabytes(X) (1024ULL*Kilobytes(X))
#define Gigabytes(X) (1024ULL*Megabytes(X))
#define Terabytes(X) (1024ULL*Gigabytes(X))

#define ArrayCount(A) (sizeof(A)/sizeof(A[0]))

#if GAME_SLOW
#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0; }
#else
#define Assert
#endif

#define InvalidCodePath Assert(!"Invalid code path")
#define InvalidDefaultCase default: { InvalidCodePath } break

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) void *name(char *FilePath)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

struct game_memory
{
    void *PermanentStorage;
    uint32 PermanentStorageSize;

    void *TransientStorage;
    uint32 TransientStorageSize;

    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
};

struct game_button
{
    bool32 IsDown;
    bool32 JustWentDown;
};

struct game_input
{
    real32 dt;

    union
    {
        struct
        {
            game_button Left;
            game_button Right;
            game_button Up;
            game_button Down;
            game_button Action;
        };

        game_button Buttons[5];
    };
};

#define BITMAP_BYTES_PER_PIXEL 4
struct game_bitmap
{
    int32 Width;
    int32 Height;
    int32 Pitch;
    void *Pixels;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, game_bitmap *Backbuffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#endif
