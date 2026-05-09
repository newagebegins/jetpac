internal void
DrawRect(game_bitmap *Bitmap, int32 MinX, int32 MinY, int32 MaxX, int32 MaxY,
         v3 Color = {1.0f, 1.0f, 1.0f})
{
    if(MinX < 0)
    {
        MinX = 0;
    }
    if(MinY < 0)
    {
        MinY = 0;
    }
    if(MaxX > Bitmap->Width)
    {
        MaxX = Bitmap->Width;
    }
    if(MaxY > Bitmap->Height)
    {
        MaxY = Bitmap->Height;
    }
    
    uint8 *DestRow = (uint8 *)Bitmap->Pixels + MinY*Bitmap->Pitch + MinX*BITMAP_BYTES_PER_PIXEL;

    for(int32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        uint32 *Dest = (uint32*)DestRow;
        for(int32 X = MinX;
            X < MaxX;
            ++X)
        {
            uint32 R = (uint32)(255.0f*Color.r + 0.5f);
            uint32 G = (uint32)(255.0f*Color.g + 0.5f);
            uint32 B = (uint32)(255.0f*Color.b + 0.5f);
            uint32 Color32 = (R << 16) | (G << 8) | B;
            *Dest = Color32;
            Dest++;
        }
        DestRow += Bitmap->Pitch;
    }
}

internal void
DrawBitmap(game_bitmap *Output, game_bitmap *Bitmap, int32 MinX, int32 MinY,
           bool32 FlipX = false, v3 Color = {1.0f, 1.0f, 1.0f},
           v2 UVOffset = {0.0f, 0.0f}, v2 UVScale = {1.0f, 1.0f})
{
    int32 MaxX = MinX + FloorReal32ToInt32(UVScale.x*(r32)Bitmap->Width);
    int32 MaxY = MinY + FloorReal32ToInt32(UVScale.y*(r32)Bitmap->Height);

    int32 ClipMinX = 0;
    int32 ClipMaxX = Output->Width;
    int32 ClipMinY = 0;
    int32 ClipMaxY = Output->Height;

    int32 AdvanceX = FloorReal32ToInt32(UVOffset.x*(r32)Bitmap->Width);
    int32 AdvanceY = FloorReal32ToInt32(UVOffset.y*(r32)Bitmap->Height);

    if(MinX < ClipMinX)
    {
        AdvanceX = ClipMinX - MinX;
        MinX = ClipMinX;
    }
    if(MinY < ClipMinY)
    {
        AdvanceY = ClipMinY - MinY;
        MinY = ClipMinY;
    }
    if(MaxX > ClipMaxX)
    {
        MaxX = ClipMaxX;
    }
    if(MaxY > ClipMaxY)
    {
        MaxY = ClipMaxY;
    }
    
    uint8 *DestRow = (uint8 *)Output->Pixels + MinY*Output->Pitch + MinX*BITMAP_BYTES_PER_PIXEL;
    uint8 *SourceRow = (uint8 *)Bitmap->Pixels + AdvanceY*Bitmap->Pitch;

    int32 SourceAdvance = 1;
    if(FlipX)
    {
        SourceRow += Bitmap->Pitch - BITMAP_BYTES_PER_PIXEL;
        SourceRow -= AdvanceX*BITMAP_BYTES_PER_PIXEL;
        SourceAdvance = -1;
    }
    else
    {
        SourceRow += AdvanceX*BITMAP_BYTES_PER_PIXEL;
    }

    for(int32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        uint32 *Source = (uint32 *)SourceRow;
        uint32 *Dest = (uint32*)DestRow;
        for(int32 X = MinX;
            X < MaxX;
            ++X)
        {
            uint32 Dest32 = *Dest;
            uint32 Source32 = *Source;

            real32 Inv255 = 1.0f / 255.0f;

            real32 SB = (real32)(Source32 & 0xFF) * Inv255;
            real32 SG = (real32)((Source32 >> 8) & 0xFF) * Inv255;
            real32 SR = (real32)((Source32 >> 16) & 0xFF) * Inv255;
            uint32 SA = (Source32 >> 24) & 0xFF;

            SR *= Color.r;
            SG *= Color.g;
            SB *= Color.b;

            if(SA)
            {
                *Dest = ((uint32)(SB*255.0f + 0.5f) |
                         ((uint32)(SG*255.0f + 0.5f) << 8) |
                         ((uint32)(SR*255.0f + 0.5f) << 16));
            }

            Dest++;
            Source += SourceAdvance;
        }
        SourceRow += Bitmap->Pitch;
        DestRow += Output->Pitch;
    }
}

internal void
DrawBitmapPortion(game_bitmap *DestBitmap,
                  game_bitmap *SourceBitmap,
                  int32 DestMinX, int32 DestMinY,
                  int32 SourceMinX, int32 SourceMinY,
                  int32 Width, int32 Height,
                  v3 Color = {1.0f, 1.0f, 1.0f})
{
    int32 DestMaxX = DestMinX + Width;
    int32 DestMaxY = DestMinY + Height;
    int32 SourceMaxX = SourceMinX + Width;
    int32 SourceMaxY = SourceMinY + Height;

    Assert(DestMinX >= 0);
    Assert(DestMinY >= 0);
    Assert(DestMaxX <= DestBitmap->Width);
    Assert(DestMaxY <= DestBitmap->Height);
    Assert(SourceMinX >= 0);
    Assert(SourceMinY >= 0);
    Assert(SourceMaxX <= SourceBitmap->Width);
    Assert(SourceMaxY <= SourceBitmap->Height);
    
    uint8 *DestRow = (uint8 *)DestBitmap->Pixels + DestMinY*DestBitmap->Pitch + DestMinX*BITMAP_BYTES_PER_PIXEL;
    uint8 *SourceRow = (uint8 *)SourceBitmap->Pixels + SourceMinY*SourceBitmap->Pitch + SourceMinX*BITMAP_BYTES_PER_PIXEL;

    for(int32 Y = DestMinY;
        Y < DestMaxY;
        ++Y)
    {
        uint32 *Source = (uint32 *)SourceRow;
        uint32 *Dest = (uint32*)DestRow;
        for(int32 X = DestMinX;
            X < DestMaxX;
            ++X)
        {
            uint32 Dest32 = *Dest;
            uint32 Source32 = *Source;

            real32 Inv255 = 1.0f / 255.0f;

            real32 SB = (real32)(Source32 & 0xFF) * Inv255;
            real32 SG = (real32)((Source32 >> 8) & 0xFF) * Inv255;
            real32 SR = (real32)((Source32 >> 16) & 0xFF) * Inv255;
            uint32 SA = (Source32 >> 24) & 0xFF;

            SR *= Color.r;
            SG *= Color.g;
            SB *= Color.b;

            if(SA)
            {
                *Dest = ((uint32)(SB*255.0f + 0.5f) |
                         ((uint32)(SG*255.0f + 0.5f) << 8) |
                         ((uint32)(SR*255.0f + 0.5f) << 16));
            }

            ++Dest;
            ++Source;
        }
        SourceRow += SourceBitmap->Pitch;
        DestRow += DestBitmap->Pitch;
    }
}

internal void
DrawString(game_bitmap *DestBitmap, game_bitmap *FontBitmap, char *String,
           int32 DestMinX, int32 DestMinY, v3 Color)
{
    int32 DestX = DestMinX;
    int32 SourceY = 0;
    for(char *Letter = String;
        *Letter;
        ++Letter)
    {
        int32 SourceX = (*Letter - ' ')*TILE_SIZE;
        DrawBitmapPortion(DestBitmap, FontBitmap,
                          DestX, DestMinY,
                          SourceX, SourceY,
                          TILE_SIZE, TILE_SIZE, Color);
        DestX += TILE_SIZE;
    }
}

internal void
PushClear(render_group *Group, color Color)
{
    render_entry_base *Base = PushStruct(&Group->Arena, render_entry_base);
    Base->ID = RenderEntry_Clear;

    render_entry_clear *Entry = PushStruct(&Group->Arena, render_entry_clear);
    Entry->Color = Color;
}

internal void
PushRect(render_group *Group, int32 MinX, int32 MinY, int32 MaxX, int32 MaxY,
         color Color = Color_White, bool32 WrapX = true)
{
    render_entry_base *Base = PushStruct(&Group->Arena, render_entry_base);
    Base->ID = RenderEntry_Rect;

    render_entry_rect *Entry = PushStruct(&Group->Arena, render_entry_rect);
    Entry->MinX = MinX;
    Entry->MinY = MinY;
    Entry->MaxX = MaxX;
    Entry->MaxY = MaxY;
    Entry->Color = Color;
    Entry->WrapX = WrapX;
}

internal void
PushRect(render_group *Group, rectangle2i Rect, color Color = Color_White, bool32 WrapX = true)
{
    PushRect(Group, Rect.MinX, Rect.MinY, Rect.MaxX, Rect.MaxY, Color, WrapX);
}

internal void
PushBitmap(render_group *Group, game_bitmap *Bitmap, int32 MinX, int32 MinY,
           bool32 Flip = false, color Color = Color_White, bool32 WrapX = true,
           v2 UVOffset = {0.0f, 0.0f}, v2 UVScale = {1.0f, 1.0f})
{
    render_entry_base *Base = PushStruct(&Group->Arena, render_entry_base);
    Base->ID = RenderEntry_Bitmap;

    render_entry_bitmap *Entry = PushStruct(&Group->Arena, render_entry_bitmap);
    Entry->Bitmap = Bitmap;
    Entry->MinX = MinX;
    Entry->MinY = MinY;
    Entry->Flip = Flip;
    Entry->Color = Color;
    Entry->UVOffset = UVOffset;
    Entry->UVScale = UVScale;

    if(WrapX)
    {
        if(MinX < 0)
        {
            MinX += Group->OutputBitmap->Width;
            PushBitmap(Group, Bitmap, MinX, MinY, Flip, Color, false, UVOffset, UVScale);
        }
        else if(MinX + Bitmap->Width > Group->OutputBitmap->Width)
        {
            MinX -= Group->OutputBitmap->Width;
            PushBitmap(Group, Bitmap, MinX, MinY, Flip, Color, false, UVOffset, UVScale);
        }
    }
}

internal void
PushString(render_group *Group, char *String, int32 X, int32 Y, color Color)
{
    int32 DestX = X;
    for(char *Letter = String;
        *Letter;
        ++Letter)
    {
        int32 SourceX = (*Letter - ' ')*TILE_SIZE;
        v2 UVOffset = {(r32)SourceX/Group->FontBitmap->Width, 0.0f};
        v2 UVScale = {(r32)TILE_SIZE/(r32)Group->FontBitmap->Width, 1.0f};
        PushBitmap(Group, Group->FontBitmap, DestX, Y,
                   false, Color, false, UVOffset, UVScale);
        DestX += TILE_SIZE;
    }
}

inline v3
ColorUintToV3(uint32 Color)
{
    v3 Result;
    real32 Inv255 = 1.0f / 255.0f;
    Result.r = (real32)((Color >> 16) & 0xFF)*Inv255;
    Result.g = (real32)((Color >> 8) & 0xFF)*Inv255;
    Result.b = (real32)((Color >> 0) & 0xFF)*Inv255;
    return(Result);
}

internal render_group *
AllocateRenderGroup(memory_arena *Arena, memory_index PushBufferSize, game_bitmap *OutputBitmap,
                    game_bitmap *FontBitmap)
{
    render_group *Group = PushStruct(Arena, render_group);
    Group->Arena = SubArena(Arena, PushBufferSize);
    Group->OutputBitmap = OutputBitmap;

    Group->Palette[Color_Black] = ColorUintToV3(0x000000);
    Group->Palette[Color_Blue] = ColorUintToV3(0x0000C0);
    Group->Palette[Color_Red] = ColorUintToV3(0xC00000);
    Group->Palette[Color_Magenta] = ColorUintToV3(0xC000C0);
    Group->Palette[Color_Green] = ColorUintToV3(0x00C000);
    Group->Palette[Color_Cyan] = ColorUintToV3(0x00C0C0);
    Group->Palette[Color_Yellow] = ColorUintToV3(0xC0C000);
    Group->Palette[Color_White] = ColorUintToV3(0xFFFFFF);
    Group->Palette[Color_BrightBlue] = ColorUintToV3(0x0000FF);
    Group->Palette[Color_BrightRed] = ColorUintToV3(0xFF0000);
    Group->Palette[Color_BrightMagenta] = ColorUintToV3(0xFF00FF);
    Group->Palette[Color_BrightGreen] = ColorUintToV3(0x00FF00);
    Group->Palette[Color_BrightCyan] = ColorUintToV3(0x00FFFF);
    Group->Palette[Color_BrightYellow] = ColorUintToV3(0xFFFF00);

    Group->FontBitmap = FontBitmap;

    return(Group);
}

internal void
RenderGroupToOutput(render_group *Group)
{
    render_entry_base *Base = (render_entry_base *)Group->Arena.Memory;
    render_entry_base *End = (render_entry_base *)((uint8 *)Group->Arena.Memory + Group->Arena.Used);
    while(Base < End)
    {
        switch(Base->ID)
        {
            case RenderEntry_Clear:
            {
                render_entry_clear *Entry = (render_entry_clear *)(Base + 1);
                DrawRect(Group->OutputBitmap, 0, 0, Group->OutputBitmap->Width, Group->OutputBitmap->Height, Group->Palette[Entry->Color]);
                Base = (render_entry_base *)(Entry + 1);
            } break;

            case RenderEntry_Rect:
            {
                render_entry_rect *Entry = (render_entry_rect *)(Base + 1);
                v3 Color = Group->Palette[Entry->Color];
                DrawRect(Group->OutputBitmap, Entry->MinX, Entry->MinY, Entry->MaxX, Entry->MaxY, Color);
                if(Entry->WrapX)
                {
                    if(Entry->MinX < 0)
                    {
                        Entry->MinX += Group->OutputBitmap->Width;
                        Entry->MaxX += Group->OutputBitmap->Width;
                        DrawRect(Group->OutputBitmap, Entry->MinX, Entry->MinY, Entry->MaxX, Entry->MaxY, Color);
                    }
                    else if(Entry->MaxX > Group->OutputBitmap->Width)
                    {
                        Entry->MaxX -= Group->OutputBitmap->Width;
                        Entry->MinX -= Group->OutputBitmap->Width;
                        DrawRect(Group->OutputBitmap, Entry->MinX, Entry->MinY, Entry->MaxX, Entry->MaxY, Color);
                    }
                }
                Base = (render_entry_base *)(Entry + 1);
            } break;

            case RenderEntry_Bitmap:
            {
                render_entry_bitmap *Entry = (render_entry_bitmap *)(Base + 1);
                v3 Color = Group->Palette[Entry->Color];
                DrawBitmap(Group->OutputBitmap, Entry->Bitmap, Entry->MinX, Entry->MinY, Entry->Flip, Color, Entry->UVOffset, Entry->UVScale);
                Base = (render_entry_base *)(Entry + 1);
            } break;

            InvalidDefaultCase;
        }
    }
}
