#ifndef IMGUI_RENDERER_H
#define IMGUI_RENDERER_H

#include <android/native_window.h>
#include "types/structs.h"

namespace Renderer
{
    void Init(ANativeWindow *window, int width, int height);
    void StartFrame();
    void EndFrame();
    void Shutdown();

    void DrawWorldObjects(const Structs::Response &response);
}

#endif // IMGUI_RENDERER_H
