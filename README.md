# ImGuiManager

## Overview

ImGuiManager is a Virtools Manager that helps you utilize Dear ImGui within Virtools Dev. It implements a ImGui backend based on Virtools Render APIs. By using ImGuiManager, you can use Dear ImGui without concerning low-level rasterizer implementation.


## Getting Started

This project provides three libraries: ImGui, ImGuiManager, and CKImGui.

- ImGui: A dynamic library version of Dear ImGui included with Virtools backend.
- ImGuiManager: A Virtools manager to make it easy to harness Dear ImGui.
- CKImGui: A Wrapper library for ImGuiManager. This is useful when your program is not able to link against Virtools SDK.

With ImGuiManager, you don't have to handle the tedious procedures such as initialization and its inverse processes. Just pass the content between `NewFrame` and `EndFrame` as a callback into the function `AddToFrame`, then the expected window will be drew every frame automatically.

```cpp
ImGuiManager *man = (ImGuiManager *) GetCKContext(0)->GetManagerByGuid(IMGUI_MANAGER_GUID);
man->AddToFrame([]{
    ImGui::ShowDemoWindow(nullptr);
});
```

## Building

1. Clone the repository RECURSIVELY with `git clone https://github.com/doyaGu/ImGuiManager.git --recursive`.

2. Get into the cloned repository. Please run the command `cmake -B build -G "Visual Studio 16 2019" -A Win32` to generate Visual Studio solution. 

3. Open `ImGuiManager.sln` under the `build` directory and compile it.
