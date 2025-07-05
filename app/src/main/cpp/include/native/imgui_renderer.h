#ifndef IMGUI_RENDERER_H
#define IMGUI_RENDERER_H

namespace Renderer {
    void Init();

    void SetDisplay(int width, int height);

    void StartFrame();

    void EndFrame();

    void Shutdown();
}

#endif // IMGUI_RENDERER_H
