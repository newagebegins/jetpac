#include <windows.h>
#include <stdio.h>

#include "platform.h"
#include "win32_game.h"

global_variable bool32 GlobalRunning;
global_variable win32_backbuffer GlobalBackbuffer;

struct win32_client_dim
{
    int32 X;
    int32 Y;
};

inline win32_client_dim
Win32GetClientDim(HWND Window)
{
    win32_client_dim Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.X = ClientRect.right - ClientRect.left;
    Result.Y = ClientRect.bottom - ClientRect.top;
    return(Result);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    HANDLE File = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, 0);
    Assert(File != INVALID_HANDLE_VALUE);

    LARGE_INTEGER FileSizeLI;
    BOOL GetGetFileSizeExSuccess = GetFileSizeEx(File, &FileSizeLI);
    Assert(GetGetFileSizeExSuccess);
    DWORD FileSize = FileSizeLI.LowPart;
    Assert(FileSizeLI.HighPart == 0);

    void *FileContents = VirtualAlloc(0, FileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(FileContents);

    DWORD BytesRead;
    BOOL ReadFileSuccess = ReadFile(File, FileContents, FileSize, &BytesRead, 0);
    Assert(ReadFileSuccess);
    Assert(BytesRead == FileSize);

    BOOL CloseHandleSuccess = CloseHandle(File);
    Assert(CloseHandleSuccess);

    return(FileContents);
}

inline LARGE_INTEGER
Win32GetCounter(void)
{
    LARGE_INTEGER Result;
    BOOL Success = QueryPerformanceCounter(&Result);
    Assert(Success);
    return(Result);
}

internal void
Win32DisplayBackbuffer(win32_backbuffer *Backbuffer, HDC DeviceContext,
                       int32 ClientWidth, int32 ClientHeight)
{
    int32 OutputWidth = Backbuffer->Scale*Backbuffer->Width;
    int32 OutputHeight = Backbuffer->Scale*Backbuffer->Height;

    DWORD BorderOp = BLACKNESS;
    // NOTE(slava): Bottom border
    PatBlt(DeviceContext, 0, OutputHeight, ClientWidth, ClientHeight - OutputHeight, BorderOp);
    // NOTE(slava): Right border
    PatBlt(DeviceContext, OutputWidth, 0, ClientWidth - OutputWidth, ClientHeight, BorderOp);

    StretchDIBits(DeviceContext,
                  0, 0, OutputWidth, OutputHeight,
                  0, 0, Backbuffer->Width, Backbuffer->Height,
                  Backbuffer->Memory, &Backbuffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT
Win32WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            Assert(DeviceContext);
            win32_client_dim ClientDim = Win32GetClientDim(Window);
            Win32DisplayBackbuffer(&GlobalBackbuffer, DeviceContext, ClientDim.X, ClientDim.Y);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

internal FILETIME
Win32GetLastWriteTime(char *FilePath)
{
    FILETIME Result = {};
    WIN32_FILE_ATTRIBUTE_DATA FileAttributes;
    if(GetFileAttributesExA(FilePath, GetFileExInfoStandard, &FileAttributes))
    {
        Result = FileAttributes.ftLastWriteTime;
    }
    return(Result);
}

internal bool32
Win32FileExists(char *FilePath)
{
    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    bool32 Result = GetFileAttributesExA(FilePath, GetFileExInfoStandard, &Ignored);
    return(Result);
}

internal void
ConcatenateStrings(char *Source0, uint32 Count0,
                   char *Source1, uint32 Count1,
                   char *Dest, uint32 DestMaxCount)
{
    uint32 DestIndex = 0;
    for(uint32 SourceIndex = 0;
        SourceIndex < Count0;
        ++SourceIndex, ++DestIndex)
    {
        Dest[DestIndex] = Source0[SourceIndex];
    }
    for(uint32 SourceIndex = 0;
        SourceIndex < Count1;
        ++SourceIndex, ++DestIndex)
    {
        Dest[DestIndex] = Source1[SourceIndex];
    }
    Assert(DestIndex <= DestMaxCount);
}

internal win32_game
Win32LoadGame(char *GameDLLFullPath, char *GameTempDLLFullPath)
{
    win32_game Result = {};

    BOOL CopyFileSuccess = CopyFile(GameDLLFullPath, GameTempDLLFullPath, false);
    Assert(CopyFileSuccess);

    Result.DLL = LoadLibraryA(GameTempDLLFullPath);
    Assert(Result.DLL);

    Result.LastWriteTime = Win32GetLastWriteTime(GameDLLFullPath);
    
    Result.UpdateAndRender = (game_update_and_render *)GetProcAddress(Result.DLL, "GameUpdateAndRender");
    Assert(Result.UpdateAndRender);

    return(Result);
}

inline void
Win32RewindFile(HANDLE File)
{
    DWORD SetFilePointerResult = SetFilePointer(File, 0, 0, FILE_BEGIN);
    Assert(SetFilePointerResult != INVALID_SET_FILE_POINTER);
}

#if GAME_INTERNAL
inline void
Win32SaveMemory(HANDLE InputRecorderFile, game_memory *Memory)
{
    DWORD NumberOfBytesWritten;
    BOOL WriteFileSuccess = WriteFile(InputRecorderFile,
                                      Memory->PermanentStorage,
                                      Memory->PermanentStorageSize,
                                      &NumberOfBytesWritten,
                                      0);
    Assert(WriteFileSuccess);
    Assert(NumberOfBytesWritten == Memory->PermanentStorageSize);
}

inline void
Win32LoadMemory(HANDLE InputRecorderFile, game_memory *Memory)
{
    DWORD NumberOfBytesRead;
    BOOL ReadFileSuccess = ReadFile(InputRecorderFile,
                                    Memory->PermanentStorage,
                                    Memory->PermanentStorageSize,
                                    &NumberOfBytesRead,
                                    0);
    Assert(ReadFileSuccess);
    Assert(NumberOfBytesRead == Memory->PermanentStorageSize);
}

inline void
Win32SaveInput(HANDLE InputRecorderFile, game_input *Input)
{
    DWORD NumberOfBytesWritten;
    BOOL WriteFileSuccess = WriteFile(InputRecorderFile,
                                      Input,
                                      sizeof(*Input),
                                      &NumberOfBytesWritten,
                                      0);
    Assert(WriteFileSuccess);
    Assert(NumberOfBytesWritten == sizeof(*Input));
}

inline bool32
Win32LoadInput(HANDLE InputRecorderFile, game_input *Input)
{
    DWORD NumberOfBytesRead;
    BOOL ReadFileSuccess = ReadFile(InputRecorderFile,
                                    Input,
                                    sizeof(*Input),
                                    &NumberOfBytesRead,
                                    0);
    Assert(ReadFileSuccess);
    bool32 ReachedEndOfFile = (NumberOfBytesRead == 0);
    if(!ReachedEndOfFile)
    {
        Assert(NumberOfBytesRead == sizeof(*Input));
    }
    return(ReachedEndOfFile);
}
#endif

int APIENTRY
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, char *CommandLine, int ShowCmd)
{
    char EXEFullPath[WIN32_MAX_FILE_PATH_LENGTH];
    DWORD CharsCopied = GetModuleFileNameA(0, EXEFullPath, sizeof(EXEFullPath));
    Assert(CharsCopied > 0);
    Assert(CharsCopied < sizeof(EXEFullPath));

    uint32 LastSlashIndex = 0;
    for(uint32 CharIndex = 0;
        CharIndex < CharsCopied;
        ++CharIndex)
    {
        if(EXEFullPath[CharIndex] == '\\')
        {
            LastSlashIndex = CharIndex;
        }
    }
    Assert(LastSlashIndex > 0);
    Assert(LastSlashIndex < CharsCopied);
    uint32 OnePastLastSlashIndex = LastSlashIndex + 1;

    char GameDLLFullPath[WIN32_MAX_FILE_PATH_LENGTH];
    char GameDLLName[] = "game.dll";
    ConcatenateStrings(EXEFullPath, OnePastLastSlashIndex,
                       GameDLLName, sizeof(GameDLLName),
                       GameDLLFullPath, sizeof(GameDLLFullPath));

    char GameTempDLLFullPath[WIN32_MAX_FILE_PATH_LENGTH];
    char GameTempDLLName[] = "game_temp.dll";
    ConcatenateStrings(EXEFullPath, OnePastLastSlashIndex,
                       GameTempDLLName, sizeof(GameTempDLLName),
                       GameTempDLLFullPath, sizeof(GameTempDLLFullPath));

    char LockFullPath[WIN32_MAX_FILE_PATH_LENGTH];
    char LockName[] = "lock.tmp";
    ConcatenateStrings(EXEFullPath, OnePastLastSlashIndex,
                       LockName, sizeof(LockName),
                       LockFullPath, sizeof(LockFullPath));

    win32_game Game = Win32LoadGame(GameDLLFullPath, GameTempDLLFullPath);

    LARGE_INTEGER CounterFrequency;
    BOOL QueryPerformanceFrequencySuccess = QueryPerformanceFrequency(&CounterFrequency);
    Assert(QueryPerformanceFrequencySuccess);
    int64 CountsPerSecond = CounterFrequency.QuadPart;

    MMRESULT TimeBeginPeriodResult = timeBeginPeriod(1);
    bool32 SleepIsGranular = (TimeBeginPeriodResult == TIMERR_NOERROR);

    win32_backbuffer *Backbuffer = &GlobalBackbuffer;

    Backbuffer->Width = 256;
    Backbuffer->Height = 192;
    Backbuffer->Scale = 4;
    Backbuffer->Pitch = Backbuffer->Width*BITMAP_BYTES_PER_PIXEL;

    Backbuffer->Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    Backbuffer->Info.bmiHeader.biWidth = Backbuffer->Width;
    Backbuffer->Info.bmiHeader.biHeight = Backbuffer->Height;
    Backbuffer->Info.bmiHeader.biPlanes = 1;
    Backbuffer->Info.bmiHeader.biBitCount = 8*BITMAP_BYTES_PER_PIXEL;
    Backbuffer->Info.bmiHeader.biCompression = BI_RGB;
    Backbuffer->Info.bmiHeader.biSizeImage = 0;
    Backbuffer->Info.bmiHeader.biXPelsPerMeter = 0;
    Backbuffer->Info.bmiHeader.biYPelsPerMeter = 0;
    Backbuffer->Info.bmiHeader.biClrUsed = 0;
    Backbuffer->Info.bmiHeader.biClrImportant = 0;

    int32 BackbufferSize = Backbuffer->Width*Backbuffer->Height*BITMAP_BYTES_PER_PIXEL;
    Backbuffer->Memory = VirtualAlloc(0, BackbufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(Backbuffer->Memory);

    WNDCLASSA WindowClass;
    WindowClass.style = 0;
    WindowClass.lpfnWndProc = Win32WindowProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = Instance;
    WindowClass.hIcon = 0;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.hbrBackground = 0;
    WindowClass.lpszMenuName = 0;
    WindowClass.lpszClassName = "MyWindowClass";

    ATOM WindowClassAtom = RegisterClassA(&WindowClass);
    Assert(WindowClassAtom);

    DWORD WindowStyle = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
    RECT WindowRect = {0, 0, Backbuffer->Scale*Backbuffer->Width, Backbuffer->Scale*Backbuffer->Height};
    AdjustWindowRect(&WindowRect, WindowStyle, false);

    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "My Game",
                                  WindowStyle,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  WindowRect.right - WindowRect.left,
                                  WindowRect.bottom - WindowRect.top,
                                  0, 0, Instance, 0);
    Assert(Window);

    int32 TargetFramesPerSecond = 60;
    int64 CountsPerFrame = CountsPerSecond / TargetFramesPerSecond;
    real32 TargetSecondsPerFrame = 1.0f / (real32)TargetFramesPerSecond;
    real32 dt = TargetSecondsPerFrame;

#if GAME_INTERNAL
    LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
    LPVOID BaseAddress = 0;
#endif
    
    game_memory Memory = {};

    Memory.PermanentStorageSize = Megabytes(8);
    Memory.TransientStorageSize = Megabytes(16);
    memory_index TotalMemorySize = Memory.PermanentStorageSize + Memory.TransientStorageSize;

    Memory.PermanentStorage = VirtualAlloc(BaseAddress, TotalMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(Memory.PermanentStorage);
    Memory.TransientStorage = (uint8 *)Memory.PermanentStorage + Memory.PermanentStorageSize;

    Memory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;

    game_input Inputs[2] = {};
    game_input *OldInput = &Inputs[0];
    game_input *NewInput = &Inputs[1];

#if GAME_INTERNAL
    input_recorder_state InputRecorderState = InputRecorderState_Off;

    char InputRecorderFilePath[WIN32_MAX_FILE_PATH_LENGTH];
    char InputRecorderFileName[] = "recorded_input.bin";
    ConcatenateStrings(EXEFullPath, OnePastLastSlashIndex,
                       InputRecorderFileName, sizeof(InputRecorderFileName),
                       InputRecorderFilePath, sizeof(InputRecorderFilePath));

    HANDLE InputRecorderFile = CreateFileA(InputRecorderFilePath,
                                                 GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                                                 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    Assert(InputRecorderFile != INVALID_HANDLE_VALUE);
#endif

    GlobalRunning = true;

    LARGE_INTEGER FrameStartCounter = Win32GetCounter();

    while(GlobalRunning)
    {
        FILETIME NewLastWriteTime = Win32GetLastWriteTime(GameDLLFullPath);
        if((CompareFileTime(&Game.LastWriteTime, &NewLastWriteTime) != 0) &&
           !Win32FileExists(LockFullPath))
        {
            BOOL FreeLibrarySuccess = FreeLibrary(Game.DLL);
            Assert(FreeLibrarySuccess);

            Game = Win32LoadGame(GameDLLFullPath, GameTempDLLFullPath);
        }

        MSG Message;
        while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
        {
            switch(Message.message)
            {
                case WM_QUIT:
                {
                    GlobalRunning = false;
                } break;

                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                {
                    uint32 VKCode  = (uint32)Message.wParam;
                    bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                    bool32 IsDown  = ((Message.lParam & (1 << 31)) == 0);

                    if(IsDown != WasDown)
                    {
                        if(VKCode == 'A')
                        {
                            NewInput->Left.IsDown = IsDown;
                        }
                        if(VKCode == 'D')
                        {
                            NewInput->Right.IsDown = IsDown;
                        }
                        if(VKCode == 'W')
                        {
                            NewInput->Up.IsDown = IsDown;
                        }
                        if(VKCode == 'S')
                        {
                            NewInput->Down.IsDown = IsDown;
                        }
                        if(VKCode == VK_SPACE)
                        {
                            NewInput->Action.IsDown = IsDown;
                        }
#if GAME_INTERNAL
                        if(VKCode == 'L' && IsDown)
                        {
                            switch(InputRecorderState)
                            {
                                case InputRecorderState_Off:
                                {
                                    Win32RewindFile(InputRecorderFile);
                                    Win32SaveMemory(InputRecorderFile, &Memory);
                                    InputRecorderState = InputRecorderState_Recording;
                                } break;

                                case InputRecorderState_Playing:
                                {
                                    InputRecorderState = InputRecorderState_Off;
                                    // NOTE(slava): To avoid buttons becoming stuck
                                    *NewInput = {};
                                } break;

                                case InputRecorderState_Recording:
                                {
                                    Win32RewindFile(InputRecorderFile);
                                    Win32LoadMemory(InputRecorderFile, &Memory);
                                    InputRecorderState = InputRecorderState_Playing;
                                } break;

                                InvalidDefaultCase;
                            }
                        }
#endif
                    }
                } break;

                default:
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                } break;
            }
        }

#if GAME_INTERNAL
        switch(InputRecorderState)
        {
            case InputRecorderState_Off:
            {
                // NOTE(slava): Do nothing
            } break;

            case InputRecorderState_Playing:
            {
                if(Win32LoadInput(InputRecorderFile, NewInput))
                {
                    Win32RewindFile(InputRecorderFile);
                    Win32LoadMemory(InputRecorderFile, &Memory);
                    bool32 ReachedEndOfFile = Win32LoadInput(InputRecorderFile, NewInput);
                    Assert(!ReachedEndOfFile);
                }
            } break;

            case InputRecorderState_Recording:
            {
                Win32SaveInput(InputRecorderFile, NewInput);
            } break;

            InvalidDefaultCase;
        }
#endif

        for(int32 ButtonIndex = 0;
            ButtonIndex < ArrayCount(NewInput->Buttons);
            ++ButtonIndex)
        {
            game_button *NewButton = NewInput->Buttons + ButtonIndex;
            game_button *OldButton = OldInput->Buttons + ButtonIndex;
            NewButton->JustWentDown = NewButton->IsDown && !OldButton->IsDown;
        }

        NewInput->dt = dt;
        Game.UpdateAndRender(&Memory, NewInput, (game_bitmap *)Backbuffer);

        LARGE_INTEGER FrameEndCounter = Win32GetCounter();
        int64 CountsElapsed = FrameEndCounter.QuadPart - FrameStartCounter.QuadPart;
        while(CountsElapsed < CountsPerFrame)
        {
            int64 SleepCounts = CountsPerFrame - CountsElapsed;
            int32 SleepMS = (int32)((1000*SleepCounts) / CountsPerSecond);
            if((SleepMS > 0) && SleepIsGranular)
            {
                Sleep(SleepMS);
            }

            FrameEndCounter = Win32GetCounter();
            CountsElapsed = FrameEndCounter.QuadPart - FrameStartCounter.QuadPart;
        }

        FrameStartCounter = FrameEndCounter;

        // NOTE(slava): Frame flip

        HDC DeviceContext = GetDC(Window);
        win32_client_dim ClientDim = Win32GetClientDim(Window);
        Win32DisplayBackbuffer(Backbuffer, DeviceContext, ClientDim.X, ClientDim.Y);
        ReleaseDC(Window, DeviceContext);

        game_input *TempInput = NewInput;
        NewInput = OldInput;
        OldInput = TempInput;
        *NewInput = *OldInput;

#if 0
        real32 SecondsElapsed = (real32)CountsElapsed / (real32)CountsPerSecond;
        real32 MSElapsed = 1000.0f*SecondsElapsed;
        real32 FramesPerSecond = 1.0f / SecondsElapsed;

        char Buffer[256];
        sprintf_s(Buffer, "%.2fms %.2ffps\n", MSElapsed, FramesPerSecond);
        OutputDebugStringA(Buffer);
#endif
    }

    return(0);
}
