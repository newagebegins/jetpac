# Handmade Jetpac

A recreation of the ZX Spectrum game Jetpac (1983) written in Handmade Hero style C++. It features independent native Win32 and WebAssembly/WebGL platform layers.

You can play the WebAssembly/WebGL version directly in your browser:  
https://newagebegins.github.io/jetpac/

**Controls:** W, A, D, Space

![Gameplay Recording](gameplay.gif)

---

## Technical Specifications & Architecture

A zero-dependency 2D game and engine built from scratch to practice low-level memory management and cross-platform architecture (inspired by the Handmade Hero philosophy).

### 1. Isolated Engine Core (Pure C++)
* **Zero Global State:** The core contains no global variables and communicates with platform layers strictly via a single frame entry point:
  `void GameUpdateAndRender(game_memory *Memory, game_input *Input, bitmap_info *BitmapInfos)`
* **Pointer-Driven Data Flow:** The platform layer manages OS resources and passes state via pointers. The core processes raw inputs and outputs commands back by writing data directly into a pre-allocated contiguous memory buffer, tracking byte utilization via `RenderListUsed`.
* **Fixed Memory Footprint:** Banned runtime dynamic allocations (`malloc`/`new`) during the frame execution loop. Memory is managed deterministically via a sub-allocated `memory_arena` layout.

### 2. Native Win32 Platform Layer
* **Dynamic Code Iteration:** Implemented real-time game code hot-reloading (`LoadLibraryA`) by separating the core logic into a dynamic library (`game.dll`).
* **State Replication:** Implemented an in-engine input recording and playback system that writes raw game memory snapshots and input streams directly to disk (`recorded_input.bin`) for deterministic debugging.
* **Software Rendering Pipeline:** Processes window messages via an event-driven loop (`WM_KEYDOWN`/`WM_KEYUP`) and presents a raw system memory backbuffer to the screen using `StretchDIBits`.

### 3. WebAssembly & WebGL Presentation Layer
* **Zero-Dependency WASM:** Compiles directly via clang into bytecode, bypassing heavy frameworks like Emscripten and completely avoiding the C Standard Library (CRT).
* **Command-Based Instanced Rendering:** The core populates a `RenderList` command buffer with discrete primitives (`RenderEntry_Clear`, `RenderEntry_Bitmap`). 
* **Single Draw-Call Execution:** The JavaScript layer streams the processed instance data into GPU buffers via `bufferSubData`, executing the entire frame in a single draw call (`drawArraysInstancedANGLE`).

### 4. Data-Driven Asset Pipeline
* **Texture Atlas Automation:** Features a standalone asset packer tool (`atlas_builder.cpp`) that bakes individual BMP sprites into a single binary asset file (`jetpac.atls`). This ensures the entire frame renders using a **single texture bind**, completely eliminating expensive texture-switching overhead in WebGL.
* **I/O & Structural Isolation:** The game core contains no file-loading logic or asset path hardcoding. It references resources via `enum bitmap_id`, resolving texture UV coordinates and frame offsets dynamically at runtime via direct indexing into a data-driven `bitmap_info` array layout.
