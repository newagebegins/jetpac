internal void
DrawRect(game_bitmap *Bitmap, int32 MinX, int32 MinY, int32 MaxX, int32 MaxY,
         v4 Color = {1.0f, 1.0f, 1.0f, 1.0f})
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
           v4 Color = {1.0f, 1.0f, 1.0f, 1.0f},
           v2 UVOffset = {0.0f, 0.0f}, v2 UVScale = {1.0f, 1.0f})
{
    int32 MaxX = MinX + FloorReal32ToInt32(AbsoluteValue(UVScale.x)*(r32)Bitmap->Width);
    int32 MaxY = MinY + FloorReal32ToInt32(UVScale.y*(r32)Bitmap->Height);

    int32 ClipMinX = 0;
    int32 ClipMaxX = Output->Width;
    int32 ClipMinY = 0;
    int32 ClipMaxY = Output->Height;

    s32 SourceStartX = FloorReal32ToInt32(UVOffset.x*(r32)Bitmap->Width);
    s32 SourceStartY = FloorReal32ToInt32(UVOffset.y*(r32)Bitmap->Height);
    s32 SourceAdvanceX = (s32)SignOf(UVScale.x);
    s32 SourceAdvanceY = (s32)SignOf(UVScale.y);

    if(MinX < ClipMinX)
    {
        SourceStartX += SourceAdvanceX*(ClipMinX - MinX);
        MinX = ClipMinX;
    }
    if(MinY < ClipMinY)
    {
        SourceStartY += SourceAdvanceY*(ClipMinY - MinY);
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

    if(SourceStartX >= Bitmap->Width)
    {
        SourceStartX = Bitmap->Width - 1;
    }
    else if(SourceStartX < 0)
    {
        SourceStartX = 0;
    }

    Assert(SourceStartY >= 0);
    Assert(SourceStartY < Bitmap->Height);

    uint8 *DestRow = (uint8 *)Output->Pixels + MinY*Output->Pitch + MinX*BITMAP_BYTES_PER_PIXEL;
    uint8 *SourceRow = (uint8 *)Bitmap->Pixels + SourceStartY*Bitmap->Pitch + SourceStartX*BITMAP_BYTES_PER_PIXEL;

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
            Source += SourceAdvanceX;
        }
        SourceRow += Bitmap->Pitch;
        DestRow += Output->Pitch;
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
PushBitmap(render_group *Group, bitmap_id ID, int32 MinX, int32 MinY,
           u32 FrameIndex = 0, bool32 MirrorX = false, color Color = Color_White,
           bool32 WrapX = true, s32 FrameOffsetX = 0, s32 VisibleWidthArg = -1)
{
    render_entry_base *Base = PushStruct(&Group->Arena, render_entry_base);
    Base->ID = RenderEntry_Bitmap;

    bitmap_info Info_ = GetBitmapInfo(Group->Atlas, ID);
    bitmap_info *Info = &Info_;
    game_bitmap *Bitmap = GetBitmap(Group->Atlas, ID, FrameIndex);

    Assert(FrameOffsetX >= 0);
    Assert(FrameOffsetX <= Info->FrameWidth);

    s32 VisibleWidth = VisibleWidthArg;
    if(VisibleWidth < 0)
    {
        VisibleWidth = Info->FrameWidth - FrameOffsetX;
    }
    Assert(VisibleWidth >= 0);
    Assert(VisibleWidth <= (Info->FrameWidth - FrameOffsetX));

    s32 OffsetX;
    s32 OffsetY = Info->OffsetY;

    v2 UVScale = V2((r32)VisibleWidth*Info->InvFullWidth,
                    (r32)Info->FrameHeight*Info->InvFullHeight);

    // TODO(slava): Remove this when we start using atlas
    s32 FakeFrameIndex = (ID == Bitmap_Font) ? FrameIndex : 0;

    if(MirrorX)
    {
        OffsetX = (FakeFrameIndex + 1)*Info->FrameWidth - 1 - FrameOffsetX;
        UVScale.x = -UVScale.x;
    }
    else
    {
        OffsetX = FakeFrameIndex*Info->FrameWidth + FrameOffsetX;
    }

    v2 UVOffset = {(r32)OffsetX*Info->InvFullWidth,
                   (r32)OffsetY*Info->InvFullHeight};

    render_entry_bitmap *Entry = PushStruct(&Group->Arena, render_entry_bitmap);
    Entry->Bitmap = Bitmap;
    Entry->DimX = VisibleWidth;
    Entry->DimY = Info->FrameHeight;
    Entry->MinX = MinX;
    Entry->MinY = MinY;
    Entry->Color = Group->Palette[Color];
    Entry->UVOffset = UVOffset;
    Entry->UVScale = UVScale;

    if(WrapX)
    {
        int32 MaxX = MinX + VisibleWidth;
        if(MinX < 0)
        {
            MinX += Group->OutputBitmap->Width;
            PushBitmap(Group, ID, MinX, MinY, FrameIndex, MirrorX, Color, false, FrameOffsetX, VisibleWidthArg);
        }
        else if(MaxX > Group->OutputBitmap->Width)
        {
            MinX -= Group->OutputBitmap->Width;
            PushBitmap(Group, ID, MinX, MinY, FrameIndex, MirrorX, Color, false, FrameOffsetX, VisibleWidthArg);
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
        u32 FrameIndex = *Letter - ' ';
        PushBitmap(Group, Bitmap_Font, DestX, Y, FrameIndex,
                   false, Color, false);
        DestX += TILE_SIZE;
    }
}

inline v4
ColorUInt32ToV4(uint32 Color)
{
    v4 Result;
    real32 Inv255 = 1.0f / 255.0f;
    Result.r = (real32)((Color >> 16) & 0xFF)*Inv255;
    Result.g = (real32)((Color >> 8) & 0xFF)*Inv255;
    Result.b = (real32)((Color >> 0) & 0xFF)*Inv255;
    Result.a = 1.0f;
    return(Result);
}

internal render_group *
AllocateRenderGroup(memory_arena *Arena, memory_index PushBufferSize, game_bitmap *OutputBitmap,
                    atlas *Atlas)
{
    render_group *Group = PushStruct(Arena, render_group);
    Group->Arena = SubArena(Arena, PushBufferSize);
    Group->OutputBitmap = OutputBitmap;

    Group->Palette[Color_Black] = ColorUInt32ToV4(0x000000);
    Group->Palette[Color_Blue] = ColorUInt32ToV4(0x0000C0);
    Group->Palette[Color_Red] = ColorUInt32ToV4(0xC00000);
    Group->Palette[Color_Magenta] = ColorUInt32ToV4(0xC000C0);
    Group->Palette[Color_Green] = ColorUInt32ToV4(0x00C000);
    Group->Palette[Color_Cyan] = ColorUInt32ToV4(0x00C0C0);
    Group->Palette[Color_Yellow] = ColorUInt32ToV4(0xC0C000);
    Group->Palette[Color_White] = ColorUInt32ToV4(0xFFFFFF);
    Group->Palette[Color_BrightBlue] = ColorUInt32ToV4(0x0000FF);
    Group->Palette[Color_BrightRed] = ColorUInt32ToV4(0xFF0000);
    Group->Palette[Color_BrightMagenta] = ColorUInt32ToV4(0xFF00FF);
    Group->Palette[Color_BrightGreen] = ColorUInt32ToV4(0x00FF00);
    Group->Palette[Color_BrightCyan] = ColorUInt32ToV4(0x00FFFF);
    Group->Palette[Color_BrightYellow] = ColorUInt32ToV4(0xFFFF00);

    Group->FontBitmap = &Atlas->FontBitmap;
    Group->Atlas = Atlas;

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
                v4 Color = Group->Palette[Entry->Color];
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
                DrawBitmap(Group->OutputBitmap, Entry->Bitmap, Entry->MinX, Entry->MinY, Entry->Color, Entry->UVOffset, Entry->UVScale);
                Base = (render_entry_base *)(Entry + 1);
            } break;

            InvalidDefaultCase;
        }
    }
}
