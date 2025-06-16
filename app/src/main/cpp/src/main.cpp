#include "main.h"

struct RenderContext {
    int width = 0;
    int height = 0;
    int orientation = 0;
    bool isInitialized = false;
};

static RenderContext renderContexts;


extern "C"
JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeOnSurfaceCreated(JNIEnv *env, jclass) {

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!renderContexts.isInitialized) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(1.0f);
        ImGui_ImplOpenGL3_Init("#version 300 es");
        renderContexts.isInitialized = true;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeOnSurfaceChanged(JNIEnv *env, jclass, jint width,
                                                                jint height) {
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    renderContexts.width = width;
    renderContexts.height = height;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeOnDrawFrame(JNIEnv *env, jclass) {
    if (!renderContexts.isInitialized) return;

    glViewport(0, 0, renderContexts.width, renderContexts.height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float) renderContexts.width, (float) renderContexts.height);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Overlay");
    ImGui::Text("🎉 ImGui on Android!");
    ImGui::Button("Click Me");
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeOnOrientationChanged(JNIEnv *env, jclass,
                                                                    jint orientation) {
    renderContexts.orientation = orientation;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeSurfaceStop(JNIEnv *env, jclass clazz) {
    if (!renderContexts.isInitialized) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    renderContexts.isInitialized = false;
}