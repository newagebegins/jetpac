#include "platform.h"
#include "atlas.h"
#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1)
struct bitmap_file_header
{
    uint16 Id;
    uint32 FileSize;
    uint32 Reserved;
    uint32 PixelsOffset;
};

struct dib_header
{
    uint32 HeaderSize;
    int32 BitmapWidth;
    int32 BitmapHeight;
    uint16 ColorPlanesCount;
    uint16 BitsPerPixel;
    uint32 Compression;
    uint32 ImageSize;
    int32 HorizontalResolution;
    int32 VerticalResolution;
    uint32 PaletteColorsCount;
    uint32 ImportantColorsCount;
    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
    uint32 AlphaMask;
};
#pragma pack(pop)

internal void *
ReadEntireFile(char *FilePath)
{
    FILE *File = fopen(FilePath, "rb");
    Assert(File);
    fseek(File, 0, SEEK_END);
    u32 FileSize = ftell(File);
    void *Result = malloc(FileSize);
    Assert(Result);
    fseek(File, 0, SEEK_SET);
    size_t ReadCount = fread(Result, FileSize, 1, File);
    Assert(ReadCount == 1);
    fclose(File);
    return(Result);
}

internal game_bitmap
LoadBMP(char *FilePath)
{
    game_bitmap Result;
    void *FileContents = ReadEntireFile(FilePath);
    bitmap_file_header *FileHeader = (bitmap_file_header *)FileContents;
    Assert(FileHeader->Id == 0x4D42);
    dib_header *DIBHeader = (dib_header *)((uint8 *)FileContents + sizeof(bitmap_file_header));
    Assert(DIBHeader->Compression == 3);
    Assert(DIBHeader->BitsPerPixel == BITMAP_BYTES_PER_PIXEL*8);
    Result.Width = DIBHeader->BitmapWidth;
    Result.Height = DIBHeader->BitmapHeight;
    Result.Pitch = DIBHeader->BitmapWidth*BITMAP_BYTES_PER_PIXEL;
    Result.Pixels = (uint32 *)((uint8 *)FileContents + FileHeader->PixelsOffset);
    return(Result);
}

internal void
CopyBitmap(atlas *Atlas, game_bitmap *Bitmap, s32 DestX, s32 DestY)
{
    u8 *DestRow = (u8 *)Atlas->Pixels + DestY*ATLAS_PITCH + DestX*BITMAP_BYTES_PER_PIXEL;
    u8 *SourceRow = (u8 *)Bitmap->Pixels;

    for(s32 Y = 0;
        Y < Bitmap->Height;
        ++Y)
    {
        u32 *Dest = (u32 *)DestRow;
        u32 *Source = (u32 *)SourceRow;

        for(s32 X = 0;
            X < Bitmap->Width;
            ++X)
        {
            *Dest++ = *Source++;
        }

        DestRow += ATLAS_PITCH;
        SourceRow += Bitmap->Pitch;
    }
}

inline void
CopyBitmaps(atlas *Atlas, bitmap_id ID, game_bitmap *Bitmaps, s32 BitmapCount, s32 *DestY, s32 FrameWidth = 0, s32 FrameHeight = 0)
{
    bitmap_info *Info = Atlas->Infos + ID;
    Info->FrameWidth = FrameWidth ? FrameWidth : Bitmaps[0].Width;
    Info->FrameHeight = FrameHeight ? FrameHeight : Bitmaps[0].Height;
    Info->OffsetY = *DestY;

    u32 FrameCountX = (BitmapCount * Bitmaps[0].Width) / Info->FrameWidth;
    u32 FrameCountY = Bitmaps[0].Height / Info->FrameHeight;
    Info->FrameCount = FrameCountX * FrameCountY;

    for(s32 BitmapIndex = 0;
        BitmapIndex < BitmapCount;
        ++BitmapIndex)
    {
        s32 DestX = BitmapIndex*Info->FrameWidth;
        CopyBitmap(Atlas, Bitmaps + BitmapIndex, DestX, *DestY);
    }

    *DestY += Info->FrameHeight;
}

#if OUTPUT_BMP
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif

int main(void)
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

    GroundBitmaps[0] = LoadBMP("assets/ground0.bmp");
    GroundBitmaps[1] = LoadBMP("assets/ground1.bmp");
    GroundBitmaps[2] = LoadBMP("assets/ground2.bmp");

    JetmanWalkingBitmaps[0] = LoadBMP("assets/jetman0.bmp");
    JetmanWalkingBitmaps[1] = LoadBMP("assets/jetman1.bmp");
    JetmanWalkingBitmaps[2] = LoadBMP("assets/jetman2.bmp");

    JetmanFlyingBitmaps[0] = LoadBMP("assets/jetman3.bmp");
    JetmanFlyingBitmaps[1] = LoadBMP("assets/jetman4.bmp");
    JetmanFlyingBitmaps[2] = LoadBMP("assets/jetman5.bmp");

    ExplosionBitmaps[0] = LoadBMP("assets/explosion0.bmp");
    ExplosionBitmaps[1] = LoadBMP("assets/explosion1.bmp");
    ExplosionBitmaps[2] = LoadBMP("assets/explosion2.bmp");
    ExplosionBitmaps[3] = LoadBMP("assets/explosion3.bmp");
    ExplosionBitmaps[4] = LoadBMP("assets/explosion4.bmp");

    PartBitmaps[0] = LoadBMP("assets/part0.bmp");
    PartBitmaps[1] = LoadBMP("assets/part1.bmp");
    PartBitmaps[2] = LoadBMP("assets/part2.bmp");
    PartBitmaps[3] = LoadBMP("assets/part3.bmp");
    PartBitmaps[4] = LoadBMP("assets/part4.bmp");
    PartBitmaps[5] = LoadBMP("assets/part5.bmp");

    FuelBitmap = LoadBMP("assets/fuel.bmp");

    FlameBitmaps[0] = LoadBMP("assets/rocket_flame0.bmp");
    FlameBitmaps[1] = LoadBMP("assets/rocket_flame1.bmp");
    FlameBitmaps[2] = LoadBMP("assets/rocket_flame2.bmp");

    AsteroidBitmaps[0] = LoadBMP("assets/asteroid0.bmp");
    AsteroidBitmaps[1] = LoadBMP("assets/asteroid1.bmp");
    AsteroidBitmaps[2] = LoadBMP("assets/asteroid2.bmp");

    LaserBitmap = LoadBMP("assets/laser.bmp");
    FontBitmap = LoadBMP("assets/font.bmp");
    LivesBitmap = LoadBMP("assets/lives.bmp");

    FaceBitmaps[0] = LoadBMP("assets/face0.bmp");
    FaceBitmaps[1] = LoadBMP("assets/face1.bmp");
    FaceBitmaps[2] = LoadBMP("assets/face2.bmp");

    atlas Atlas_ = {};
    atlas *Atlas = &Atlas_;
    Atlas->Infos = (bitmap_info *)malloc(ATLAS_INFOS_SIZE);
    Atlas->Pixels = malloc(ATLAS_PIXELS_SIZE);
    Assert(Atlas->Infos);
    Assert(Atlas->Pixels);

    ZeroStruct(Atlas->Infos[0]);

    s32 DestY = 0;

    CopyBitmaps(Atlas, Bitmap_Ground, GroundBitmaps, ArrayCount(GroundBitmaps), &DestY);
    CopyBitmaps(Atlas, Bitmap_JetmanWalking, JetmanWalkingBitmaps, ArrayCount(JetmanWalkingBitmaps), &DestY);
    CopyBitmaps(Atlas, Bitmap_JetmanFlying, JetmanFlyingBitmaps, ArrayCount(JetmanFlyingBitmaps), &DestY);
    CopyBitmaps(Atlas, Bitmap_Explosion, ExplosionBitmaps, ArrayCount(ExplosionBitmaps), &DestY);
    CopyBitmaps(Atlas, Bitmap_Part, PartBitmaps, ArrayCount(PartBitmaps), &DestY);
    CopyBitmaps(Atlas, Bitmap_Asteroid, AsteroidBitmaps, ArrayCount(AsteroidBitmaps), &DestY);
    CopyBitmaps(Atlas, Bitmap_Flame, FlameBitmaps, ArrayCount(FlameBitmaps), &DestY);
    CopyBitmaps(Atlas, Bitmap_Face, FaceBitmaps, ArrayCount(FaceBitmaps), &DestY);

    CopyBitmaps(Atlas, Bitmap_Laser, &LaserBitmap, 1, &DestY);
    CopyBitmaps(Atlas, Bitmap_Fuel, &FuelBitmap, 1, &DestY);
    CopyBitmaps(Atlas, Bitmap_Font, &FontBitmap, 1, &DestY, 8, 8);
    CopyBitmaps(Atlas, Bitmap_Lives, &LivesBitmap, 1, &DestY);

    Assert(DestY <= ATLAS_HEIGHT);

#if OUTPUT_BMP
    stbi_flip_vertically_on_write(1); 
    stbi_write_bmp("atlas.bmp", ATLAS_WIDTH, ATLAS_HEIGHT, 4, Atlas->Pixels);
#endif

    FILE *File = fopen("jetpac.atls", "wb");
    Assert(File);

    atlas_header Header = {};
    Header.Width = ATLAS_WIDTH;
    Header.Height = ATLAS_HEIGHT;
    Header.InfosSize = ATLAS_INFOS_SIZE;
    Header.PixelsSize = ATLAS_PIXELS_SIZE;
    Header.InfosOffset = sizeof(Header);
    Header.PixelsOffset = Header.InfosOffset + ATLAS_INFOS_SIZE;

    size_t WrittenCount;

    WrittenCount = fwrite(&Header, sizeof(Header), 1, File);
    Assert(WrittenCount == 1);

    WrittenCount = fwrite(Atlas->Infos, ATLAS_INFOS_SIZE, 1, File);
    Assert(WrittenCount == 1);

    WrittenCount = fwrite(Atlas->Pixels, ATLAS_PIXELS_SIZE, 1, File);
    Assert(WrittenCount == 1);

    return(0);
}
