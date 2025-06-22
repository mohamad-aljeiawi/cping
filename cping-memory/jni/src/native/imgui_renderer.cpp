#include "native/imgui_renderer.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_android.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

static EGLDisplay egl_display;
static EGLSurface egl_surface;
static EGLContext egl_context;

#if defined(_WIN32) && !defined(__ANDROID__)
typedef struct ANativeWindow *EGLNativeWindowType;
#endif

void Renderer::Init(ANativeWindow *window, int width, int height)
{
    const EGLint config_attr[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE};

    const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE};

    EGLConfig egl_config;
    EGLint num_configs;
    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(egl_display, nullptr, nullptr);
    eglChooseConfig(egl_display, config_attr, &egl_config, 1, &num_configs);
    egl_surface = eglCreateWindowSurface(egl_display, egl_config, window, nullptr);
    egl_context = eglCreateContext(egl_display, egl_config, nullptr, context_attr);
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplAndroid_Init(window);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    io.IniFilename = nullptr; // إلغاء التخزين التلقائي
}

void Renderer::StartFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
}

void Renderer::EndFrame()
{
    ImGui::Render();
    glViewport(0, 0, 1920, 1080); // عدّل لاحقًا لو بدك ديناميكي
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(egl_display, egl_surface);
}

void Renderer::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
    eglDestroyContext(egl_display, egl_context);
    eglDestroySurface(egl_display, egl_surface);
    eglTerminate(egl_display);
}

void Renderer::DrawWorldObjects(const Structs::Response &response)
{
    ImGui::Begin("ESP Data");

    ImGui::Text("Object Count: %d", response.Count);
    for (int i = 0; i < response.Count; ++i)
    {
        const auto &obj = response.Objects[i];
        ImGui::Text("%s - Pos: (%.1f, %.1f, %.1f)", obj.Name, obj.Location.X, obj.Location.Y, obj.Location.Z);
    }

    ImGui::End();
}
