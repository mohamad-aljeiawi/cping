#include "main.h"
#include <thread>
#include <mutex>
#include <chrono>

struct RenderContext
{
    int width = 0;
    int height = 0;
    int orientation = 0;
    bool isInitialized = false;
};

static RenderContext renderContexts;
SocketServer socket_server("cping_abstract_socket");
FrameTimeController fps_controller = FrameTimeController(60);

std::thread network_thread_obj;
Structs::Response shared_response;
Structs::Request shared_request;
std::mutex mutex_req;
std::mutex mutex_res;

void network_thread(SocketServer *server)
{
    while (renderContexts.isInitialized)
    {
        // Step 1: Send latest request to client
        Structs::Request temp_req{};
        {
            std::lock_guard<std::mutex> lock(mutex_req);
            temp_req = shared_request;
        }
        bool sent = server->send_raw(&temp_req, sizeof(temp_req));
        if (!sent)
        {
            LOGE(TEST_TAG, "server->send_raw failed");
            //            break;
        }

        // Step 2: Receive response from client
        Structs::Response temp_res{};
        bool received = server->receive_raw(&temp_res, sizeof(temp_res));
        if (!received)
        {
            LOGE(TEST_TAG, "server->receive_raw failed");
            //            break;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_res);
            shared_response = temp_res;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeOnSurfaceCreated(JNIEnv *env, jclass)
{
    LOGI(TEST_TAG, "Starting OnSurfaceCreated");
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!renderContexts.isInitialized)
    {
        LOGI(TEST_TAG, "Starting OnSurfaceCreated Init");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(1.0f);
        ImGui_ImplOpenGL3_Init("#version 300 es");

        LOGI(TEST_TAG, "Starting socket server");
        if (!socket_server.start())
        {
            LOGE(TEST_TAG, "Socket start failed");
            return;
        }

        LOGI(TEST_TAG, "Starting network thread");
        renderContexts.isInitialized = true;
        network_thread_obj = std::thread(network_thread, &socket_server);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeOnSurfaceChanged(JNIEnv *env, jclass, jint width,
                                                                jint height)
{
    LOGI(TEST_TAG, "Starting OnSurfaceChanged");

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    renderContexts.width = width;
    renderContexts.height = height;
}

extern "C" JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeOnDrawFrame(JNIEnv *env, jclass)
{
    if (!renderContexts.isInitialized)
        return;

    auto frame_start = std::chrono::high_resolution_clock::now();
    //    fps_controller.start_frame();

    glViewport(0, 0, renderContexts.width, renderContexts.height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)renderContexts.width, (float)renderContexts.height);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    Structs::Response response_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_res);
        response_copy = shared_response;
    }

    Structs::Request request_copy{};
    {
        std::lock_guard<std::mutex> lock(mutex_req);
        request_copy.ScreenHeight = renderContexts.height;
        request_copy.ScreenWidth = renderContexts.width;
        request_copy.ScreenOrientation = renderContexts.orientation;
        shared_request = request_copy;
    }

    Structs::MinimalViewInfo minimal_view_info = response_copy.MinimalViewInfo;
    float width = static_cast<float>(renderContexts.width);
    float height = static_cast<float>(renderContexts.height);

    for (int i = 0; i < response_copy.Count; ++i)
    {
        Structs::WorldObject &obj = response_copy.Objects[i];

        Structs::FVector location = Ue4::world_to_screen(obj.Location,minimal_view_info,(int)width,(int)height);

        float margin = 10.0f;
        if (location.X > margin && location.Y > margin &&
            location.X < width - margin && location.Y < height - margin &&
            location.Z > 0.0f)
        {

            Structs::FTransform transform = obj.Transform;
            Structs::FBoxSphereBounds bounds = obj.BoxSphereBounds;

            Structs::FVector corners[8] = {
                {-bounds.BoxExtent.X, -bounds.BoxExtent.Y, -bounds.BoxExtent.Z},
                {bounds.BoxExtent.X, -bounds.BoxExtent.Y, -bounds.BoxExtent.Z},
                {bounds.BoxExtent.X, bounds.BoxExtent.Y, -bounds.BoxExtent.Z},
                {-bounds.BoxExtent.X, bounds.BoxExtent.Y, -bounds.BoxExtent.Z},
                {-bounds.BoxExtent.X, -bounds.BoxExtent.Y, bounds.BoxExtent.Z},
                {bounds.BoxExtent.X, -bounds.BoxExtent.Y, bounds.BoxExtent.Z},
                {bounds.BoxExtent.X, bounds.BoxExtent.Y, bounds.BoxExtent.Z},
                {-bounds.BoxExtent.X, bounds.BoxExtent.Y, bounds.BoxExtent.Z}};

            Structs::FVector output_object[8];

            for (int i = 0; i < 8; ++i)
            {
                Structs::FVector local = corners[i] + bounds.Origin;
                Structs::FVector world = transform.TransformPosition(local);
                Structs::FVector screen = Ue4::world_to_screen(world,
                                                               minimal_view_info,
                                                               (int)width,
                                                               (int)height);
                output_object[i] = screen;
            }

            ImVec2 textPos(location.X, location.Y);
            ImGui::GetForegroundDrawList()->AddText(textPos, IM_COL32(255, 0, 0, 255), obj.Name);

            for (int j = 0; j < 4; ++j)
            {
                Structs::FVector p1 = output_object[j];
                Structs::FVector p2 = output_object[(j + 1) % 4];
                ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                        IM_COL32(0, 255, 0, 255));
            }
            for (int j = 0; j < 4; ++j)
            {
                Structs::FVector p1 = output_object[j + 4];
                Structs::FVector p2 = output_object[((j + 1) % 4) + 4];
                ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                        IM_COL32(0, 255, 0, 255));
            }
            for (int j = 0; j < 4; ++j)
            {
                Structs::FVector p1 = output_object[j];
                Structs::FVector p2 = output_object[j + 4];
                ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                        IM_COL32(0, 255, 0, 255));
            }
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    auto frame_end = std::chrono::high_resolution_clock::now();
    float frame_duration_ms = std::chrono::duration<float, std::milli>(
                                  frame_end - frame_start)
                                  .count();
    float fps = 1000.0f / frame_duration_ms;
    LOGI(TEST_TAG, "[CPING] FPS: %.2f | FrameTime: %.2f ms | Count: %d\n", fps, frame_duration_ms,
         response_copy.Count);

    //    fps_controller.end_frame();
}

extern "C" JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeOnOrientationChanged(JNIEnv *env, jclass,
                                                                    jint orientation)
{
    LOGI(TEST_TAG, "Starting OnOrientationChanged");
    renderContexts.orientation = orientation;
}

extern "C" JNIEXPORT void JNICALL
Java_com_cping_jo_utils_NativeBridge_onMenuEvent(JNIEnv *env, jobject thiz, jstring key,
                                                 jboolean is_checked, jfloat slider_value)
{
    LOGI(TEST_TAG, "Starting onMenuEvent");

    const char *nativeKey = env->GetStringUTFChars(key, nullptr);
    std::string cppKey(nativeKey);
    env->ReleaseStringUTFChars(key, nativeKey);

    bool checked = static_cast<bool>(is_checked);
    float value = static_cast<float>(slider_value);

    handleMenuEvent(cppKey, checked, value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeSurfaceStop(JNIEnv *env, jclass clazz)
{
    if (!renderContexts.isInitialized)
        return;
    LOGI(TEST_TAG, "Starting nativeSurfaceStop");

    renderContexts.isInitialized = false;
    if (network_thread_obj.joinable())
        network_thread_obj.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    socket_server.stop();
}
