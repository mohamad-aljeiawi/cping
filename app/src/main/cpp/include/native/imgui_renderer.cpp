#include "native/imgui_renderer.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "utils/arabic.h"
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

static int g_width = 0, g_height = 0;

void Renderer::Init() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGuiStyle &style = ImGui::GetStyle();
    ImGui_ImplOpenGL3_Init("#version 300 es");
    static const ImWchar ar_ranges[] = {
            0x0020,
            0x00FF, // Basic Latin + Latin Supplement
            0x0400,
            0x052F, // Cyrillic + Cyrillic Supplement
            0x2DE0,
            0x2DFF, // Cyrillic Extended-A
            0xA640,
            0xA69F, // Cyrillic Extended-B
            0xE000,
            0xE226, // icons
            0x2010,
            0x205E, // Punctuations
            0x0600,
            0x06FF, // Arabic
            0xFE00,
            0xFEFF,
            0,
    };
    ImFont *arabic = io.Fonts->AddFontFromMemoryCompressedTTF(font_arabic_data, font_arabic_size,
                                                              24.0f, nullptr, ar_ranges);
    io.FontDefault = arabic;
    style.ScaleAllSizes(3.0f);
}

void Renderer::SetDisplay(int width, int height) {
    g_width = width;
    g_height = height;
}

void Renderer::StartFrame() {
    glViewport(0, 0, g_width, g_height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float) g_width, (float) g_height);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
}

void Renderer::EndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}
