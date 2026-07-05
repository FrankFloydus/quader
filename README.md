# Quader Qt bgfx

Minimal C++/CMake Qt Widgets application with a Fusion-styled shell and a bgfx Vulkan viewport.

The UI is arranged as a classic desktop window: toolbar on top, sidebar plus main viewport in the center, and a status bar at the bottom. The viewport initializes bgfx with the Vulkan renderer and draws a rotating red cube with a simple directional diffuse light.

## Build

This repository includes CMake presets for the Qt MinGW installation at `C:/Qt/6.11.1/mingw_64`.

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
ctest --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target check_architecture
cmake --build --preset qt-mingw-debug --target deploy
```

Run:

```powershell
.\build\qt-mingw-debug\quader.exe
```

The first configure downloads `bgfx.cmake`, its bgfx/bx/bimg submodules, and the pinned GoogleTest test dependency. The build compiles `shaderc`, builds the SPIR-V shaders, and copies them beside the executable under `shaders/spirv`.
The test preset runs GoogleTest-discovered unit/render tests plus the architecture boundary checks through CTest. The explicit `check_architecture` target runs the include-boundary guard directly.
The `deploy` target copies the Qt DLLs, Qt platform plugins, and MinGW runtime DLLs beside the executable so it can be launched directly from Explorer.

## Internal dev builds

Public release versioning uses `VERSION`. Local runnable development snapshots use `DEV_VERSION` and display as `<VERSION>-dev.<N>`.

```powershell
python tools/dev_builds.py current
python tools/dev_builds.py list
python tools/dev_builds.py archive --preset qt-mingw-debug
python tools/dev_builds.py launch --version 0.1.0-dev.N
```

Archives are written by default under `C:\Users\Drako\Desktop\quader-dev-builds\<VERSION>-dev.<N>\` as local runtime bundles and are outside the main repository. Set `QUADER_DEV_BUILDS_ROOT` to override that location. `DEV_VERSION` increments only after an archive is created successfully.

## Project board frontend

The project board frontend lives outside this repository at `C:\Users\Drako\Desktop\quader-project-board`. It is a Next.js Kanban view for this repo's `project_board.md`.

```powershell
cd C:\Users\Drako\Desktop\quader-project-board
npm install
npm run dev
```

Open `http://127.0.0.1:3000`. The board polls once per minute and can copy Codex prompts for one or more selected tasks. The control board also shows public/dev version state, can archive or launch builds, and includes Dev Changelog and Changelog routes. The frontend resolves the repo from its sibling `_quader-qt-base` folder by default; set `QUADER_REPO_ROOT` if you run it from a different layout.

## Vulkan ownership

This project intentionally does not use `QVulkanWindow` for the viewport. `QVulkanWindow` is the right Qt helper when the application records raw Vulkan commands through `QVulkanWindowRenderer`, because Qt owns the Vulkan device, queues, command buffers, depth target, and swapchain.

Here bgfx is the renderer. Qt owns the desktop UI and provides a native child surface; bgfx owns the Vulkan backend, swapchain, resize reset path, frame submission, and presentation for that surface. Mixing `QVulkanWindow` and bgfx on the same viewport would create two Vulkan/swapchain owners for one window surface.
