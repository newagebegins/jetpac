#if !defined(RENDER_GROUP_H)
#define RENDER_GROUP_H

enum color
{
    Color_Black,
    Color_Blue,
    Color_Red,
    Color_Magenta,
    Color_Green,
    Color_Cyan,
    Color_Yellow,
    Color_White,

    Color_BrightBlue,
    Color_BrightRed,
    Color_BrightMagenta,
    Color_BrightGreen,
    Color_BrightCyan,
    Color_BrightYellow,

    Color_Count,
};

enum render_entry_id
{
    RenderEntry_Clear,
    RenderEntry_Rect,
    RenderEntry_Bitmap,
    RenderEntry_String,
};

struct render_entry_base
{
    render_entry_id ID;
};

struct render_entry_clear
{
    color Color;
};

struct render_entry_rect
{
    int32 MinX, MinY;
    int32 MaxX, MaxY;
    color Color;
    bool32 WrapX;
};

struct render_entry_bitmap
{
    game_bitmap *Bitmap;
    s32 DimX, DimY;
    s32 MinX, MinY;
    v4 Color;
    v2 UVOffset;
    v2 UVScale;
};

struct render_group
{
    memory_arena Arena;
    game_bitmap *OutputBitmap;
    v4 Palette[Color_Count];
    game_bitmap *FontBitmap;
    atlas *Atlas;
};

#endif
