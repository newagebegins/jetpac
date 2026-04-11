#if !defined(INTRINSICS_H)
#define INTRINSICS_H

#include <math.h>

inline real32
SquareRoot(real32 Value)
{
    real32 Result = sqrtf(Value);
    return(Result);
}

inline int32
RoundReal32ToInt32(real32 X)
{
    int32 Result = (int32)roundf(X);
    return(Result);
}

inline real32
Floor(real32 X)
{
    real32 Result = floorf(X);
    return(Result);
}

inline int32
FloorReal32ToInt32(real32 X)
{
    int32 Result = (int32)Floor(X);
    return(Result);
}

inline real32
AbsoluteValue(real32 Value)
{
    real32 Result = fabsf(Value);
    return(Result);
}

inline int32
ModuloN(int32 Value, int32 N)
{
    int32 Result = Value % N;
    if(Result < 0)
    {
        Result += N;
    }
    return(Result);
}

inline real32
Cos(real32 Angle)
{
    real32 Result = cosf(Angle);
    return(Result);
}

inline real32
Sin(real32 Angle)
{
    real32 Result = sinf(Angle);
    return(Result);
}

#endif
