#if !defined(ATLAS_H)
#define ATLAS_H

#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

#pragma pack(push, 1)

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

#define ATLAS_WIDTH 512
#define ATLAS_HEIGHT ATLAS_WIDTH
#define ATLAS_PITCH (ATLAS_WIDTH*BITMAP_BYTES_PER_PIXEL)

struct atlas
{
#define ATLAS_MAGIC_VALUE RIFF_CODE('a', 't', 'l', 's')
    u32 MagicValue;

#define ATLAS_VERSION 0
    u32 Version;

    bitmap_info Infos[Bitmap_Count];
    u32 Pixels[ATLAS_WIDTH*ATLAS_HEIGHT];
};

#pragma pack(pop)

#endif
