#include "game.h"

#include "render_group.cpp"

internal void
AddExplosionAt(world *World, v2 P)
{
    player *Player = &World->Player;

    color Colors[] =
    {
        Color_BrightRed,
        Color_BrightMagenta,
        Color_BrightYellow,
    };

    for(int32 ExplosionIndex = 0;
        ExplosionIndex < ArrayCount(World->Explosions);
        ++ExplosionIndex)
    {
        explosion *Explosion = World->Explosions + ExplosionIndex;
        if(!Explosion->IsActive)
        {
            Explosion->IsActive = true;
            Explosion->P = P;
            Explosion->FrameTimer = 0.0f;
            Explosion->FrameIndex = 0;
            Explosion->Color = RandomPick(&World->Series, Colors);
            break;
        }
    }            
}

internal void
StartFlying(world *World)
{
    player *Player = &World->Player;
    Player->IsFlying = true;
    Player->FrameTimer = 0.0f;
    Player->FrameIndex = 0;

    v2 ExplosionCenter = {Player->P.x + 0.5f*PLAYER_DIM_X, Player->P.y};
    v2 ExplosionHalfDim = 0.5f*V2i(3*TILE_SIZE, 2*TILE_SIZE);
    v2 P = ExplosionCenter - ExplosionHalfDim;
    AddExplosionAt(World, P);
}

inline uint8
GetTileValue(game_state *GameState, int32 TileX, int32 TileY)
{
    Assert(0 <= TileX && TileX < TILE_COUNT_X);
    Assert(0 <= TileY && TileY < TILE_COUNT_Y+1);
    uint8 Result = GameState->Tiles[TileY][TileX];
    return(Result);
}

inline uint8
GetTileValueWrapped(game_state *GameState, int32 TileX, int32 TileY)
{
    TileX = TileX % TILE_COUNT_X;
    if(TileX < 0)
    {
        TileX += TILE_COUNT_X;
    }
    uint8 Result = GetTileValue(GameState, TileX, TileY);
    return(Result);
}

inline rectangle2i
GetLaserRect(laser *Laser)
{
    rectangle2i Result;
    int32 TailX = (int32)Laser->TailX;
    int32 HeadX = (int32)Laser->HeadX;
    if(Laser->DirX > 0)
    {
        Result.MinX = TailX;
        Result.MinY = Laser->Y;
        Result.MaxX = HeadX + 1;
        Result.MaxY = Laser->Y + 1;
    }
    else
    {
        Result.MinX = HeadX;
        Result.MinY = Laser->Y;
        Result.MaxX = TailX + 1;
        Result.MaxY = Laser->Y + 1;
    }
    return(Result);
}

inline void
ResetFuel(world *World)
{
    int32 PossibleFuelTileX[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,
                                 24,25,26,27,28,29,30};
    real32 FuelStartY = TILE_COUNT_Y*TILE_SIZE;

    for(int32 FuelIndex = 3;
        FuelIndex < ArrayCount(World->Parts);
        ++FuelIndex)
    {
        part *Part = World->Parts + FuelIndex;
        Part->State = PartState_Uninstalled;
        int32 TileX = RandomPick(&World->Series, PossibleFuelTileX);
        Part->P = {(real32)(TileX*TILE_SIZE), FuelStartY};
    }
}

inline rectangle2i
GetPlayerRect(player *Player)
{
    rectangle2i Result;
    Result.MinX = (int32)Player->P.x;
    Result.MinY = (int32)Player->P.y;
    Result.MaxX = Result.MinX + PLAYER_DIM_X;
    Result.MaxY = Result.MinY + PLAYER_DIM_Y;
    return(Result);
}

internal void
UInt32ToString(uint32 Value, char *Dest, uint32 Width)
{
    char *Start = Dest;
    uint32 DigitCount = 0;

    while(Value)
    {
        *Dest++ = '0' + (Value % 10);
        Value /= 10;
        ++DigitCount;
    }

    while(DigitCount < Width)
    {
        *Dest++ = '0';
        ++DigitCount;
    }

    *Dest = 0;

    --Dest;

    while(Start < Dest)
    {
        char Temp = *Start;
        *Start = *Dest;
        *Dest = Temp;
        ++Start;
        --Dest;
    }
}

inline real32
WrapX(real32 X)
{
    if(X >= PLAYFIELD_DIM_X)
    {
        X -= PLAYFIELD_DIM_X;
    }
    else if(X < 0.0f)
    {
        X += PLAYFIELD_DIM_X;
    }
    return(X);
}

struct try_move_x_result
{
    real32 NewX;
    bool32 Collided;
};

internal try_move_x_result
TryMoveX(game_state *GameState, v2 P, real32 Vx, int32 DimX, int32 DimY, real32 dt)
{
    try_move_x_result Result = { P.x, false };

    if(Vx != 0.0f)
    {
        Result.NewX = P.x + Vx*dt;
        int32 MinTileY = (int32)P.y/TILE_SIZE;
        int32 MaxTileY = ((int32)P.y + DimY - 1)/TILE_SIZE;
        int32 NewTileX;
        int32 HitX;

        if(Vx > 0.0f)
        {
            NewTileX = ((int32)Result.NewX + DimX - 1) / TILE_SIZE;
            HitX = NewTileX*TILE_SIZE - DimX;
        }
        else // if(Vx < 0.0f)
        {
            NewTileX = (int32)Result.NewX / TILE_SIZE;
            HitX = (NewTileX + 1)*TILE_SIZE;
        }

        for(int32 TileY = MinTileY;
            TileY <= MaxTileY;
            ++TileY)
        {
            uint8 TileValue = GetTileValueWrapped(GameState, NewTileX, TileY);
            if(TileValue)
            {
                Result.NewX = (real32)HitX;
                Result.Collided = true;
                break;
            }
        }
    }

    return(Result);
}

struct try_move_y_result
{
    real32 NewY;
    bool32 Collided;
};

internal try_move_y_result
TryMoveY(game_state *GameState, v2 P, real32 Vy, int32 DimX, int32 DimY, real32 dt)
{
    try_move_y_result Result = { P.y, false };

    if(Vy != 0.0f)
    {
        Result.NewY = P.y + Vy*dt;
        int32 MinTileX = (int32)P.x/TILE_SIZE;
        int32 MaxTileX = ((int32)P.x + DimX - 1)/TILE_SIZE;
        int32 NewTileY;
        int32 HitY;

        if(Vy > 0.0f)
        {
            NewTileY = ((int32)Result.NewY + DimY - 1) / TILE_SIZE;
            HitY = NewTileY*TILE_SIZE - DimY;
        }
        else // if(Vy < 0.0f)
        {
            NewTileY = (int32)Result.NewY / TILE_SIZE;
            HitY = (NewTileY + 1)*TILE_SIZE;
        }

        for(int32 TileX = MinTileX;
            TileX <= MaxTileX;
            ++TileX)
        {
            uint8 TileValue = GetTileValueWrapped(GameState, TileX, NewTileY);
            if(TileValue)
            {
                Result.NewY = (real32)HitY;
                Result.Collided = true;
                break;
            }
        }
    }

    return(Result);
}

internal world *
AllocateWorld(memory_arena *Arena)
{
    world *World = PushStruct(Arena, world);
    ZeroStruct(*World);

    World->Series = RandomSeries(123);

    World->InstalledPartsCount = 1;

    World->Parts[0].P = V2i(ROCKET_X, TILE_SIZE);
    World->Parts[0].State = PartState_Installed;

    World->Parts[1].P = V2i(16*TILE_SIZE, 12*TILE_SIZE);
    World->Parts[2].P = V2i(6*TILE_SIZE, 15*TILE_SIZE);

    ResetFuel(World);

    World->RocketBlinkColor = Color_BrightMagenta;
    World->RocketY = TILE_SIZE;

    World->Lives = 5;
    World->EnemyType = EnemyType_Asteroid;

#if 0
    World->SimState = SimState_TakeOff;
    World->InstalledPartsCount = 9;
#endif

    return(World);
}

internal bool32
UpdateAndRenderWorld(game_state *GameState, game_input *Input, render_group *RenderGroup, memory_arena *TranArena)
{
    bool32 GameOver = false;
    world *World = GameState->World;

    //
    // NOTE(slava): Update the player
    //

    player *Player = &World->Player;
    int32 WalkingFrames[] = {0,1,2,1};

    if(World->SimState == SimState_Play)
    {
        if(Player->IsFlying)
        {
            real32 Gravity = -150.0f;
            real32 ThrustUp = 300.0f;
            real32 ThrustSide = 300.0f;
            v2 PlayerForce = {0.0f, Gravity};

            if(Input->Up.IsDown)
            {
                PlayerForce.y += ThrustUp;
            }
            if(Input->Left.IsDown)
            {
                Player->DirX = -1;
                PlayerForce.x -= ThrustSide;
            }
            else if(Input->Right.IsDown)
            {
                Player->DirX = 1;
                PlayerForce.x += ThrustSide;
            }

            v2 Friction = -Player->V;
            PlayerForce += Friction;
            Player->V += PlayerForce*Input->dt;

            try_move_x_result TryMoveXResult =
                TryMoveX(GameState, Player->P, Player->V.x, PLAYER_DIM_X, PLAYER_DIM_Y, Input->dt);
            Player->P.x = WrapX(TryMoveXResult.NewX);
            if(TryMoveXResult.Collided)
            {
                Player->V.x = -0.5f*Player->V.x;
            }

            try_move_y_result TryMoveYResult =
                TryMoveY(GameState, Player->P, Player->V.y, PLAYER_DIM_X, PLAYER_DIM_Y, Input->dt);
            Player->P.y = TryMoveYResult.NewY;
            if(TryMoveYResult.Collided)
            {
                if(Player->V.y > 0.0f)
                {
                    if(Player->V.y < 20.0f)
                    {
                        Player->V.y = 0.0f;
                    }
                    Player->V.y = -0.5f*Player->V.y;
                }
                else
                {
                    Player->IsFlying = false;
                    Player->FrameTimer = 0.0f;
                    Player->FrameIndex = 0;
                }
            }

            Player->FrameTimer += Input->dt;
            real32 FrameDuration = 0.08f;
            if(Player->FrameTimer >= FrameDuration)
            {
                Player->FrameTimer -= FrameDuration;
                bitmap_info *Info = GameState->Atlas->Infos + Bitmap_JetmanFlying;
                Player->FrameIndex = (Player->FrameIndex + 1) % Info->FrameCount;
            }
        }
        else
        {
            if(Input->Up.IsDown)
            {
                StartFlying(World);
            }

            real32 PlayerSpeed = 48.0f;
            if(Input->Left.IsDown)
            {
                Player->DirX = -1;
                Player->V = V2(-PlayerSpeed, 0.0f);
            }
            else if(Input->Right.IsDown)
            {
                Player->DirX = 1;
                Player->V = V2(PlayerSpeed, 0.0f);
            }
            else
            {
                Player->V = {};
            }

            Player->P += Player->V*Input->dt;
            Player->P.x = WrapX(Player->P.x);

            int32 MinTileX = (int32)Player->P.x/TILE_SIZE;
            int32 MaxTileX = ((int32)Player->P.x + PLAYER_DIM_X - 1)/TILE_SIZE;
            int32 GroundTileY = ((int32)Player->P.y - 1)/TILE_SIZE;
            bool32 Grounded = false;
            for(int32 TileX = MinTileX;
                TileX <= MaxTileX;
                ++TileX)
            {
                uint8 TileValue = GetTileValueWrapped(GameState, TileX, GroundTileY);
                if(TileValue)
                {
                    Grounded = true;
                    break;
                }
            }

            if(!Grounded)
            {
                StartFlying(World);
            }

            if(Player->V.x != 0.0f)
            {
                Player->FrameTimer += Input->dt;
                real32 FrameDuration = 0.08f;
                if(Player->FrameTimer >= FrameDuration)
                {
                    Player->FrameTimer -= FrameDuration;
                    Player->FrameIndex = (Player->FrameIndex + 1) % ArrayCount(WalkingFrames);
                }
            }
        }

        if(Player->ShotCooldown > 0.0f)
        {
            Player->ShotCooldown -= Input->dt;
        }
        else
        {
            if(Input->Action.IsDown)
            {
                Player->ShotCooldown = 0.08f;

                color LaserColors[] =
                {
                    Color_White,
                    Color_BrightMagenta,
                    Color_BrightCyan,
                };

                for(int32 LaserIndex = 0;
                    LaserIndex < ArrayCount(World->Lasers);
                    ++LaserIndex)
                {
                    laser *Laser = World->Lasers + LaserIndex;
                    if(!Laser->IsActive)
                    {
                        Laser->IsActive = true;
                        Laser->Y = (int32)Player->P.y + PLAYER_DIM_Y/2;
                        Laser->DirX = Player->DirX;
                        Laser->DistanceTraveled = 0.0f;
                        real32 OffsetX = 4.0f;
                        if(Player->DirX > 0)
                        {
                            Laser->HeadX = Player->P.x + PLAYER_DIM_X + OffsetX;
                        }
                        else
                        {
                            Laser->HeadX = Player->P.x - OffsetX;
                        }
                        Laser->TailX = Laser->HeadX;
                        Laser->Color = RandomPick(&World->Series, LaserColors);
                        break;
                    }
                }
            }
        }
    }

    if(World->SimState == SimState_Play || World->SimState == SimState_Hit)
    {
#if 0
        PushRect(RenderGroup, Player->P, Player->P + Player->Dim, ColorUintToV3(COLOR_GREEN));
#endif
        bool32 Flip = Player->DirX < 0;
        bitmap_id ID;
        u32 FrameIndex;
        if(Player->IsFlying)
        {
            ID = Bitmap_JetmanFlying;
            FrameIndex = Player->FrameIndex;
        }
        else
        {
            ID = Bitmap_JetmanWalking;
            FrameIndex = WalkingFrames[Player->FrameIndex];
        }
        PushBitmap(RenderGroup, ID, (int32)Player->P.x, (int32)Player->P.y, FrameIndex, Flip);
    }

    //
    // NOTE(slava): Update lasers
    //

    for(int32 LaserIndex = 0;
        LaserIndex < ArrayCount(World->Lasers);
        ++LaserIndex)
    {
        laser *Laser = World->Lasers + LaserIndex;
        if(Laser->IsActive)
        {
            s32 MaxLaserLength = 17*TILE_SIZE;
            r32 MaxLaserDistance = (r32)(MaxLaserLength + 5*TILE_SIZE);
            // NOTE(slava): 1 tile per frame
            real32 LaserSpeed = TILE_SIZE*60.0f;
            //LaserSpeed /= 10;

            real32 StepLength = LaserSpeed*Input->dt;
            real32 dX = Laser->DirX*StepLength;

            s32 OffsetX;
            rectangle2i LaserRect;
            s32 LaserLength;

            if(Laser->DistanceTraveled < MaxLaserDistance)
            {
                Laser->DistanceTraveled += StepLength;
                real32 NewX = Laser->HeadX + dX;
                int32 NewTileX = (int32)NewX/TILE_SIZE;
                int32 TileY = (int32)Laser->Y/TILE_SIZE;
                if(GetTileValueWrapped(GameState, NewTileX, TileY))
                {
                    Laser->DistanceTraveled = MaxLaserDistance;
                    if(Laser->DirX > 0)
                    {
                        NewX = (real32)(NewTileX*TILE_SIZE - 1);
                    }
                    else
                    {
                        NewX = (real32)((NewTileX + 1)*TILE_SIZE);
                    }
                }
                Laser->HeadX = NewX;
                if(AbsoluteValue(Laser->HeadX - Laser->TailX) > MaxLaserLength)
                {
                    Laser->TailX += dX;
                }

                LaserRect = GetLaserRect(Laser);
                LaserLength = Minimum(GetWidth(LaserRect), MaxLaserLength);

                if(Laser->DirX > 0)
                {
                    OffsetX = MaxLaserLength - LaserLength;
                }
                else
                {
                    OffsetX = 0;
                }
            }
            else
            {
                Laser->TailX += dX;
                if(((Laser->DirX > 0) && (Laser->TailX >= Laser->HeadX)) ||
                   ((Laser->DirX < 0) && (Laser->TailX <= Laser->HeadX)))
                {
                    Laser->TailX = Laser->HeadX;
                    Laser->IsActive = false;
                }

                LaserRect = GetLaserRect(Laser);
                LaserLength = Minimum(GetWidth(LaserRect), MaxLaserLength);

                if(Laser->DirX > 0)
                {
                    OffsetX = 0;
                }
                else
                {
                    OffsetX = MaxLaserLength - LaserLength;
                }
            }

            PushBitmap(RenderGroup, Bitmap_Laser, LaserRect.MinX, Laser->Y, 0,
                       Laser->DirX < 0, Laser->Color, true, OffsetX, LaserLength);
        }
    }

    color EnemyColors[] =
    {
        Color_Green,
        Color_Cyan,
        Color_Magenta,
        Color_Red,
    };

    //
    // NOTE(slava): Spawn enemies
    //

    if(World->SimState == SimState_Play)
    {
        for(int32 EnemyIndex = 0;
            EnemyIndex < ArrayCount(World->Enemies);
            ++EnemyIndex)
        {
            enemy *Enemy = World->Enemies + EnemyIndex;
            if(!Enemy->IsActive)
            {
                Enemy->IsActive = true;
                Enemy->EnteredPlayfield = false;
                real32 X, DirX;
                if(RandomChoice(&World->Series, 2))
                {
                    X = -ENEMY_DIM_X;
                    DirX = 1.0f;
                }
                else
                {
                    X = (real32)PLAYFIELD_DIM_X;
                    DirX = -1.0f;
                }
                real32 MinY = (real32)(TILE_SIZE + PLAYER_DIM_Y);
                real32 MaxY = PLAYFIELD_DIM_Y - ENEMY_DIM_Y;
                real32 Y = RandomBetween(&World->Series, MinY, MaxY);
                Enemy->P = {X, Y};
                v2 Direction = Normalize(V2(DirX, RandomBetween(&World->Series, -1.0f, 1.0f)));
                real32 Speed = 67.0f;
                Enemy->V = Speed*Direction;
                Enemy->Color = RandomPick(&World->Series, EnemyColors);
                break;
            }
        }
    }

    if(World->SimState == SimState_Hit)
    {
        World->Timer += Input->dt;
        if(World->Timer >= HIT_DURATION)
        {
            World->Timer = 0.0f;

            if(World->Lives == 0)
            {
                GameOver = true;
            }
            else
            {
                --World->Lives;
                World->SimState = SimState_Start;

                if(World->InstalledPartsCount < ArrayCount(World->Parts))
                {
                    part *Part = World->Parts + World->InstalledPartsCount;
                    if(Part->State == PartState_Grabbed)
                    {
                        Part->State = PartState_Uninstalled;
                    }
                }
            }
        }
    }

    if(World->SimState == SimState_Start)
    {
        World->Timer += Input->dt;
        if(World->Timer >= START_DURATION)
        {
            World->Timer = 0.0f;
            World->SimState = SimState_Play;

            Player->P = V2i(16*TILE_SIZE, TILE_SIZE);
            Player->V = {};
            Player->DirX = 1;

            for(int32 EnemyIndex = 0;
                EnemyIndex < ArrayCount(World->Enemies);
                ++EnemyIndex)
            {
                enemy *Enemy = World->Enemies + EnemyIndex;
                Enemy->IsActive = false;
            }
        }
    }

    //
    // NOTE(slava): Update the active rocket part
    //

    if(World->InstalledPartsCount < ArrayCount(World->Parts))
    {
        real32 PartFallingSpeed = 50.0f;
        part *Part = World->Parts + World->InstalledPartsCount;

        switch(Part->State)
        {
            case PartState_Uninstalled:
            {
                real32 NewY = Part->P.y - PartFallingSpeed*Input->dt;
                int32 NewTileY = (int32)NewY/TILE_SIZE;
                int32 MinTileX = (int32)Part->P.x/TILE_SIZE;
                int32 MaxTileX = ((int32)Part->P.x + PART_DIM_X - 1)/TILE_SIZE;
                for(int32 TileX = MinTileX;
                    TileX <= MaxTileX;
                    ++TileX)
                {
                    uint8 TileValue = GetTileValueWrapped(GameState, TileX, NewTileY);
                    if(TileValue)
                    {
                        NewY = (real32)((NewTileY + 1)*TILE_SIZE);
                    }
                }
                Part->P.y = NewY;

                if(World->SimState == SimState_Play)
                {
                    rectangle2i PartRect;
                    PartRect.MinX = (int32)Part->P.x;
                    PartRect.MinY = (int32)Part->P.y;
                    PartRect.MaxX = PartRect.MinX + PART_DIM_X;
                    PartRect.MaxY = PartRect.MinY + PART_DIM_Y;
                    if(RectsOverlapWrapX(GetPlayerRect(Player), PartRect, PLAYFIELD_DIM_X))
                    {
                        Part->State = PartState_Grabbed;
                        GameState->Score += 100;
                    }
                }
            } break;

            case PartState_Grabbed:
            {
                v2 TargetP = {Player->P.x + 0.5f*PLAYER_DIM_X - 0.5f*PART_DIM_X, Player->P.y};
                real32 X0 = Part->P.x;
                real32 X1 = Part->P.x;
                if(X1 < 0.5f*PLAYFIELD_DIM_X)
                {
                    X1 += PLAYFIELD_DIM_X;
                }
                else if(X1 > 0.5f*PLAYFIELD_DIM_X)
                {
                    X1 -= PLAYFIELD_DIM_X;
                }
                real32 D0 = AbsoluteValue(X0 - TargetP.x);
                real32 D1 = AbsoluteValue(X1 - TargetP.x);
                Part->P.x = (D0 < D1) ? X0 : X1;
                Part->P = Lerp(Part->P, 0.3f, TargetP);

                real32 PartCenterX = Part->P.x + 0.5f*PART_DIM_X;
                real32 RocketCenterX = (real32)(ROCKET_X + TILE_SIZE);

                if(AbsoluteValue(PartCenterX - RocketCenterX) < TILE_SIZE)
                {
                    Part->P.x = (real32)ROCKET_X;
                    Part->State = PartState_Installing;
                }
            } break;

            case PartState_Installing:
            {
                Part->P.y -= PartFallingSpeed*Input->dt;
                real32 TargetY = (real32)TILE_SIZE;
                if(World->InstalledPartsCount < 3)
                {
                    TargetY += World->InstalledPartsCount*PART_DIM_Y;
                }
                else
                {
                    TargetY += TILE_SIZE;
                }
                if(Part->P.y <= TargetY)
                {
                    Part->P.y = TargetY;
                    Part->State = PartState_Installed;
                    ++World->InstalledPartsCount;
                }
            } break;

            InvalidDefaultCase;
        }
    }

    //
    // NOTE(slava): Update the assembled rocket
    //
    
    if(World->InstalledPartsCount == ArrayCount(World->Parts))
    {
        if(World->SimState == SimState_Play)
        {
            rectangle2i RocketRect;
            RocketRect.MinX = ROCKET_X;
            RocketRect.MinY = TILE_SIZE;
            RocketRect.MaxX = RocketRect.MinX + PART_DIM_X;
            RocketRect.MaxY = RocketRect.MinY + PART_DIM_Y;
            if(RectsOverlap(GetPlayerRect(Player), RocketRect))
            {
                World->SimState = SimState_TakeOff;
            }
        }

        real32 RocketSpeed = 70.0f;

        if(World->SimState == SimState_TakeOff)
        {
            World->RocketY += RocketSpeed*Input->dt;
            if(World->RocketY >= ROCKET_MAX_Y)
            {
                World->RocketY = (real32)ROCKET_MAX_Y;
                World->SimState = SimState_Landing;
            }
        }

        if(World->SimState == SimState_Landing)
        {
            World->RocketY -= RocketSpeed*Input->dt;
            if(World->RocketY <= ROCKET_MIN_Y)
            {
                World->RocketY = (real32)ROCKET_MIN_Y;
                World->SimState = SimState_Start;
                ResetFuel(World);
                World->InstalledPartsCount = 3;
                World->EnemyType = (enemy_type)((World->EnemyType + 1) % EnemyType_Count);
            }
        }

        real32 RocketBlinkDuration = 0.28f;
        World->RocketBlinkTimer += Input->dt;
        if(World->RocketBlinkTimer >= RocketBlinkDuration)
        {
            World->RocketBlinkTimer -= RocketBlinkDuration;
            World->RocketBlinkColor = World->RocketBlinkColor == Color_White ? Color_BrightMagenta : Color_White;
        }
    }

    //
    // NOTE(slava): Draw rocket and fuel
    //

    bitmap_info *PartInfo = GameState->Atlas->Infos + Bitmap_Part;

    if(World->InstalledPartsCount < 3)
    {
        for(int32 PartIndex = 0;
            PartIndex < 3;
            ++PartIndex)
        {
            part *Part = World->Parts + PartIndex;
            int32 MinX = (int32)Part->P.x;
            int32 MinY = (int32)Part->P.y;
            PushBitmap(RenderGroup, Bitmap_Part, MinX, MinY, 2*PartIndex);
            PushBitmap(RenderGroup, Bitmap_Part, MinX, MinY + PART_DIM_Y/2, 2*PartIndex + 1);
        }
    }
    else if(World->InstalledPartsCount < ArrayCount(World->Parts))
    {
        int32 InstalledFuelCount = World->InstalledPartsCount - 3;

        for(int32 BitmapIndex = 0;
            BitmapIndex < PartInfo->FrameCount;
            ++BitmapIndex)
        {
            color Color = BitmapIndex < InstalledFuelCount ? Color_Magenta : Color_White;
            PushBitmap(RenderGroup, Bitmap_Part, ROCKET_X, (int32)World->RocketY + BitmapIndex*TILE_SIZE, BitmapIndex, false, Color);
        }

        part *Fuel = World->Parts + World->InstalledPartsCount;
        int32 MinX = (int32)Fuel->P.x;
        int32 MinY = (int32)Fuel->P.y;
        PushBitmap(RenderGroup, Bitmap_Fuel, MinX, MinY, 0, false, Color_Magenta);
    }
    else
    {
        for(int32 BitmapIndex = 0;
            BitmapIndex < PartInfo->FrameCount;
            ++BitmapIndex)
        {
            PushBitmap(RenderGroup, Bitmap_Part,
                       ROCKET_X, (int32)World->RocketY + BitmapIndex*TILE_SIZE,
                       BitmapIndex, false, World->RocketBlinkColor);
        }

        if(World->SimState == SimState_TakeOff || World->SimState == SimState_Landing)
        {
            World->FlameFrameTimer += Input->dt;
            real32 FlameFrameDuration = 0.05f;
            if(World->FlameFrameTimer >= FlameFrameDuration)
            {
                World->FlameFrameTimer -= FlameFrameDuration;
                bitmap_info *Info = GameState->Atlas->Infos + Bitmap_Flame;
                World->FlameFrameIndex = (World->FlameFrameIndex + 1) % Info->FrameCount;
            }
            PushBitmap(RenderGroup, Bitmap_Flame,
                       ROCKET_X, (int32)World->RocketY - 2*TILE_SIZE,
                       World->FlameFrameIndex, false, Color_BrightRed);
        }
    }

    //
    // NOTE(slava): Update enemies
    //

    if(World->SimState == SimState_Play ||
       World->SimState == SimState_Hit ||
       World->SimState == SimState_TakeOff)
    {
        for(int32 EnemyIndex = 0;
            EnemyIndex < ArrayCount(World->Enemies);
            ++EnemyIndex)
        {
            enemy *Enemy = World->Enemies + EnemyIndex;
            if(Enemy->IsActive)
            {
                bool32 HitSomething = false;

                try_move_x_result TryMoveXResult =
                    TryMoveX(GameState, Enemy->P, Enemy->V.x, ENEMY_DIM_X, ENEMY_DIM_Y, Input->dt);

                Enemy->P.x = TryMoveXResult.NewX;

                if(TryMoveXResult.Collided)
                {
                    if(World->EnemyType == EnemyType_Asteroid)
                    {
                        HitSomething = true;
                    }
                    else if(World->EnemyType == EnemyType_Face)
                    {
                        Enemy->V.x = -Enemy->V.x;
                    }
                    else
                    {
                        Assert(!"Unknown enemy type");
                    }
                }
                else
                {
                    if(Enemy->EnteredPlayfield)
                    {
                        Enemy->P.x = WrapX(Enemy->P.x);
                    }
                    else
                    {
                        real32 MinX = Enemy->P.x;
                        real32 MaxX = Enemy->P.x + ENEMY_DIM_X;
                        if((0.0f <= MinX) && (MaxX <= PLAYFIELD_DIM_X))
                        {
                            Enemy->EnteredPlayfield = true;
                        }
                    }
                }

                try_move_y_result TryMoveYResult =
                    TryMoveY(GameState, Enemy->P, Enemy->V.y, ENEMY_DIM_X, ENEMY_DIM_Y, Input->dt);

                Enemy->P.y = TryMoveYResult.NewY;

                if(TryMoveYResult.Collided)
                {
                    if(World->EnemyType == EnemyType_Asteroid)
                    {
                        HitSomething = true;
                    }
                    else if(World->EnemyType == EnemyType_Face)
                    {
                        Enemy->V.y = -Enemy->V.y;
                    }
                    else
                    {
                        Assert(!"Unknown enemy type");
                    }
                }

                rectangle2i EnemyCollisionRect;
                EnemyCollisionRect.MinX = (int32)Enemy->P.x + 5;
                EnemyCollisionRect.MinY = (int32)Enemy->P.y + 3;
                EnemyCollisionRect.MaxX = EnemyCollisionRect.MinX + ENEMY_DIM_X - 3;
                EnemyCollisionRect.MaxY = EnemyCollisionRect.MinY + ENEMY_DIM_Y - 3;

                rectangle2i PlayerCollisionRect = GetPlayerRect(Player);
                PlayerCollisionRect.MinX += 5;
                PlayerCollisionRect.MinY += 4;
                PlayerCollisionRect.MaxX -= 5;
                PlayerCollisionRect.MaxY -= 6;

                for(int32 LaserIndex = 0;
                    !HitSomething && (LaserIndex < ArrayCount(World->Lasers));
                    ++LaserIndex)
                {
                    laser *Laser = World->Lasers + LaserIndex;
                    if(Laser->IsActive)
                    {
                        rectangle2i LaserRect = GetLaserRect(Laser);
                        if(RectsOverlapWrapX(EnemyCollisionRect, LaserRect, PLAYFIELD_DIM_X))
                        {
                            HitSomething = true;
                            GameState->Score += 25;
                        }
                    }
                }

#if 0
                PushRect(RenderGroup, EnemyCollisionRect, Color_Cyan, Enemy->EnteredPlayfield);
                PushRect(RenderGroup, PlayerCollisionRect, Color_Red);
#endif

                if(!HitSomething &&
                   World->SimState == SimState_Play &&
                   Enemy->EnteredPlayfield &&
                   RectsOverlapWrapX(EnemyCollisionRect, PlayerCollisionRect, PLAYFIELD_DIM_X))
                {
                    HitSomething = true;
                    World->SimState = SimState_Hit;
                    AddExplosionAt(World, Player->P);
                }

                if(HitSomething)
                {
                    Enemy->IsActive = false;
                    AddExplosionAt(World, Enemy->P);
                }

                bitmap_id BitmapID = Bitmap_None;
                switch(World->EnemyType)
                {
                    case EnemyType_Asteroid:
                    {
                        BitmapID = Bitmap_Asteroid;
                    } break;

                    case EnemyType_Face:
                    {
                        BitmapID = Bitmap_Face;
                    } break;

                    InvalidDefaultCase;
                }

                bitmap_info *BitmapInfo = GameState->Atlas->Infos + BitmapID;
                int32 BitmapCount = BitmapInfo->FrameCount;

                Enemy->FrameTimer += Input->dt;
                real32 FrameDuration = 0.05f;
                if(Enemy->FrameTimer >= FrameDuration)
                {
                    Enemy->FrameTimer -= FrameDuration;
                    Enemy->FrameIndex = (Enemy->FrameIndex + 1) % BitmapCount;
                }

                int32 MinX = (int32)Enemy->P.x;
                int32 MinY = (int32)Enemy->P.y - 3;
                PushBitmap(RenderGroup, BitmapID, MinX, MinY, Enemy->FrameIndex,
                           Enemy->V.x < 0.0f, Enemy->Color, Enemy->EnteredPlayfield);
            }
        }
    }

    for(int32 TileY = 0;
        TileY < TILE_COUNT_Y;
        ++TileY)
    {
        for(int32 TileX = 0;
            TileX < TILE_COUNT_X;
            ++TileX)
        {
            uint8 TileValue = GetTileValue(GameState, TileX, TileY);
            if(TileValue)
            {
                color Color = (color)GameState->TileColors[TileY][TileX];
                PushBitmap(RenderGroup, Bitmap_Ground, TileX*TILE_SIZE, TileY*TILE_SIZE,
                           TileValue - 1, false, Color);
            }
        }
    }

    bitmap_info *ExplosionInfo = GameState->Atlas->Infos + Bitmap_Explosion;

    for(int32 ExplosionIndex = 0;
        ExplosionIndex < ArrayCount(World->Explosions);
        ++ExplosionIndex)
    {
        explosion *Explosion = World->Explosions + ExplosionIndex;
        if(Explosion->IsActive)
        {
            Explosion->FrameTimer += Input->dt;
            real32 FrameDuration = 0.06f;
            if(Explosion->FrameTimer >= FrameDuration)
            {
                Explosion->FrameTimer -= FrameDuration;
                if(Explosion->FrameIndex == ExplosionInfo->FrameCount - 1)
                {
                    Explosion->IsActive = false;
                }
                else
                {
                    ++Explosion->FrameIndex;
                }
            }

            int32 MinX = (int32)Explosion->P.x;
            int32 MinY = (int32)Explosion->P.y;
            PushBitmap(RenderGroup, Bitmap_Explosion, MinX, MinY,
                       Explosion->FrameIndex, false, Explosion->Color);
        }
    }

    if(World->Lives)
    {
        char *LivesBuffer = PushArray(TranArena, 8, char);
        UInt32ToString(World->Lives, LivesBuffer, 1);
        PushString(RenderGroup, LivesBuffer, 8*TILE_SIZE, (TILE_COUNT_Y+1)*TILE_SIZE, Color_White);
        PushBitmap(RenderGroup, Bitmap_Lives, 9*TILE_SIZE, (TILE_COUNT_Y+1)*TILE_SIZE);
    }

    return(GameOver);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    local_persist uint8 Tiles[TILE_COUNT_Y+1][TILE_COUNT_X] =
    {
        // NOTE(slava): Invisible ceiling (for collision)
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},

        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,2,2,2,3,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,2,2,2,2,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,2,3,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3},
    };

    local_persist uint8 TileColors[TILE_COUNT_Y+1][TILE_COUNT_X] =
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},

        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,4,4,4,4,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6},
    };

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!GameState->IsInitialized)
    {
        GameState->MainArena = MakeArena(GameState + 1, Memory->PermanentStorageSize - sizeof(game_state));
        GameState->Atlas = Atlas;

        GameState->WorldArena = SubArena(&GameState->MainArena);
        GameState->World = AllocateWorld(&GameState->WorldArena);

        for(int32 TileY = 0;
            TileY < TILE_COUNT_Y+1;
            ++TileY)
        {
            for(int32 TileX = 0;
                TileX < TILE_COUNT_X;
                ++TileX)
            {
                int32 Y = (TILE_COUNT_Y+1 - 1) - TileY;
                GameState->Tiles[Y][TileX] = Tiles[TileY][TileX];
                GameState->TileColors[Y][TileX] = TileColors[TileY][TileX];
            }
        }

        GameState->IsInitialized = true;
    }

    transient_state *TranState = (transient_state *)Memory->TransientStorage;
    Assert(sizeof(transient_state) <= Memory->TransientStorageSize);
    if(!TranState->IsInitialized)
    {
        TranState->TranArena = MakeArena((TranState + 1), Memory->TransientStorageSize - sizeof(transient_state));

        TranState->IsInitialized = true;
    }

    temp_memory TempRenderMemory = BeginTemporaryMemory(&TranState->TranArena);
    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(8), Backbuffer, GameState->Atlas);
    PushClear(RenderGroup, Color_Black);

    switch(GameState->MetaPhase)
    {
        case MetaPhase_Play:
        {
            if(UpdateAndRenderWorld(GameState, Input, RenderGroup, &TranState->TranArena))
            {
                GameState->MetaPhase = MetaPhase_GameOver;
                ClearArena(&GameState->WorldArena);
            }
        } break;

        case MetaPhase_GameOver:
        {
            PushString(RenderGroup, "GAME OVER", 11*TILE_SIZE, 9*TILE_SIZE, Color_White);

            if(Input->Action.JustWentDown)
            {
                GameState->MetaPhase = MetaPhase_Play;
                GameState->Score = 0;
                GameState->World = AllocateWorld(&GameState->WorldArena);
            }
        } break;

        InvalidDefaultCase;
    }

    PushString(RenderGroup, "1UP", 3*TILE_SIZE, (TILE_COUNT_Y+1)*TILE_SIZE, Color_White);

    char *ScoreBuffer = PushArray(&TranState->TranArena, 8, char);
    UInt32ToString(GameState->Score, ScoreBuffer, 6);
    PushString(RenderGroup, ScoreBuffer, 1*TILE_SIZE, (TILE_COUNT_Y+0)*TILE_SIZE, Color_BrightYellow);

    RenderGroupToOutput(RenderGroup);

    EndTemporaryMemory(TempRenderMemory);
    CheckArena(&TranState->TranArena);
}
