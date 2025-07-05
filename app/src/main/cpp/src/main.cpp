#include "main.h"

Structs::GameData game_buffers[2];
std::atomic<int> write_buffer_index{0};
std::atomic<int> ready_buffer_index{-1};

std::thread socket_thread_handle;
SocketServer socket_server("cping_socket_server");

std::atomic<bool> is_running{false};
std::atomic<float> frame_rate{60.0f};
float display_width = 0.0f;
float display_height = 0.0f;

ImColor player_color = IM_COL32(255, 0, 0, 255);
ImColor text_color_player = IM_COL32(179, 173, 0, 255);

static float aim_slide_x = 1665.1f;
static float aim_slide_y = 475.0f;
static float aim_zone_fire_x = 380.0f;
static float aim_zone_fire_y = 180.0f;
static float aim_zone_fire_radius = 150.0f;
static float aim_fov = 300.0f;
static float aim_interval = 7.0f;
static float aim_speed = 30.0f;
static float aim_rang_touch = 100.0f;

void socket_thread() {
    if (!socket_server.start()) return;

    while (is_running.load(std::memory_order_relaxed)) {
        int client_socket = accept(socket_server.server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            if (is_running.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            continue;
        }
        socket_server.client_socket = client_socket;

        while (is_running.load(std::memory_order_relaxed)) {
            Utils::control_frame_rate(frame_rate.load(std::memory_order_relaxed));

            int write_idx = write_buffer_index.load(std::memory_order_relaxed);
            auto &current_buffer = game_buffers[write_idx];
            current_buffer.clear();
            if (!socket_server.receive_raw(&current_buffer, sizeof(Structs::GameData))) break;

            ready_buffer_index.store(write_idx, std::memory_order_release);
            write_buffer_index.store(1 - write_idx, std::memory_order_relaxed);
        }

        if (socket_server.client_socket != -1) {
            close(socket_server.client_socket);
            socket_server.client_socket = -1;
        }
    }

    socket_server.stop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeOnSurfaceCreated(JNIEnv *env, jclass) {
    Renderer::Init();
    is_running.store(true);
    socket_thread_handle = std::thread(socket_thread);
}

extern "C" JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeOnSurfaceChanged(JNIEnv *env, jclass, jint width,
                                                                jint height) {
    display_width = (float) width;
    display_height = (float) height;
    if (display_width < display_height)
        std::swap(display_width, display_height);
    Renderer::SetDisplay((int) display_width, (int) display_height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeOnDrawFrame(JNIEnv *env, jclass) {
    if (!is_running.load(std::memory_order_relaxed)) return;
    if (display_width <= 0.0f || display_height <= 0.0f) return;

    Utils::control_frame_rate(frame_rate.load());

    int read_idx = ready_buffer_index.load(std::memory_order_acquire);
    const auto &game_data = game_buffers[read_idx];

    Renderer::StartFrame();
    ImDrawList *draw_list = ImGui::GetBackgroundDrawList();

    for (int i = 0; i < game_data.count_enemies; i++) {
        const Structs::Player &player = game_data.players[i];
        if (!player.is_on_screen)
            continue;

        if (player.health <= 0)
            continue;

        Structs::FVector screen_pos = player.location;
        Structs::FVector bounds[8] = {player.bounds[0], player.bounds[1], player.bounds[2],
                                      player.bounds[3], player.bounds[4], player.bounds[5],
                                      player.bounds[6], player.bounds[7]};
        for (int j = 0; j < 4; ++j) {
            Structs::FVector p1 = bounds[j];
            Structs::FVector p2 = bounds[(j + 1) % 4];
            draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), player_color, 2.0f);
        }
        for (int j = 0; j < 4; ++j) {
            Structs::FVector p1 = bounds[j + 4];
            Structs::FVector p2 = bounds[((j + 1) % 4) + 4];
            draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), player_color, 2.0f);
        }
        for (int j = 0; j < 4; ++j) {
            Structs::FVector p1 = bounds[j];
            Structs::FVector p2 = bounds[j + 4];
            draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), player_color, 2.0f);
        }

        std::string display_text = std::to_string((int) player.distance) + "m";

        float text_scale_size = Utils::calculateTextSize(player.distance, 10.0f, 460.0f, 10.0f,
                                                         30.0f, 0.2f);
        Utils::add_text_center(draw_list, display_text, text_scale_size,
                               ImVec2(player.root.X, player.root.Y), text_color_player, true,
                               0.95f);
        Utils::advanced_health_bar(draw_list, player.head.X, player.head.Y, display_width,
                                   display_height, player.health, 100.0f, player_color,
                                   IM_COL32(0, 0, 0, 100), player.distance, player.team_id);
    }

    if (game_data.count_enemies > 0) {
        std::string text = std::string("Enemies: " + std::to_string(game_data.count_enemies));
        Utils::add_text_center(draw_list, text, 60.0f,
                               ImVec2(display_width * 0.5f, display_height * 0.12f),
                               text_color_player, true, 1.1f);
    }

    draw_list->AddCircle(ImVec2(display_width * 0.5f, display_height * 0.5f), aim_fov,
                         IM_COL32(255, 255, 255, 100), 100, 3.0f);
    draw_list->AddCircleFilled(ImVec2(aim_zone_fire_x, aim_zone_fire_y), aim_zone_fire_radius,
                               IM_COL32(0, 180, 180, 50));
    draw_list->AddCircleFilled(ImVec2(aim_slide_x, aim_slide_y), aim_rang_touch,
                               IM_COL32(180, 90, 0, 50), 100);


    std::string connection_status = socket_server.client_socket != -1 ? "Client Connected" : "Waiting for Client";
    Utils::add_text_center(draw_list, connection_status, 40.0f,
                           ImVec2(display_width * 0.5f, display_height * 0.9f),
                           IM_COL32(150, 255, 150, 255), true, 1.0f);

    Renderer::EndFrame();
}

extern "C" JNIEXPORT void JNICALL
Java_com_cping_jo_service_NativeRenderer_nativeSurfaceStop(JNIEnv *env, jclass clazz) {
    if (!is_running.load(std::memory_order_relaxed)) return;

    is_running.store(false);
    if (socket_thread_handle.joinable()) socket_thread_handle.join();
    Renderer::Shutdown();
}

extern "C" JNIEXPORT void JNICALL
Java_com_cping_jo_utils_NativeBridge_onMenuEvent(JNIEnv *env, jobject thiz, jstring key, jboolean is_checked, jfloat slider_value) {
    const char *nativeKey = env->GetStringUTFChars(key, nullptr);
    std::string cppKey(nativeKey);
    env->ReleaseStringUTFChars(key, nativeKey);

    bool checked = static_cast<bool>(is_checked);
    float value = static_cast<float>(slider_value);

    handleMenuEvent(cppKey, checked, value);
}
