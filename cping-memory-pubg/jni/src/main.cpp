#include "main.h"

// Double-buffer system
Structs::GameData game_buffers[2];
std::atomic<int> write_buffer_index{0};
std::atomic<int> ready_buffer_index{-1};

std::thread aimbot_thread_handle;
std::thread drawing_thread_handle;

std::atomic<uintptr_t> lib_base{0};
std::atomic<pid_t> target_pid{0};
std::atomic<bool> is_running{true};
std::atomic<float> frame_rate{60.0f};
std::atomic<bool> aim_is_weapon_firing{false};

std::atomic<bool> aim_is_aim{false};
std::atomic<float> aim_fov{300.0f};
std::atomic<float> aim_touch_x{1665.1f};
std::atomic<float> aim_touch_y{475.0f};
std::atomic<float> aim_touch_radius{100.0f};
std::atomic<float> aim_zone_fire_x{380.0f};
std::atomic<float> aim_zone_fire_y{180.0f};
std::atomic<float> aim_zone_fire_radius{150.0f};
std::atomic<float> aim_sensitivity_factor{0.04f};
std::atomic<float> aim_latency_drag{0.13f};
std::atomic<float> aim_swipe_duration{10.0f};

std::atomic<bool> visual_box{false};
std::atomic<bool> visual_health{false};
std::atomic<bool> visual_name{false};
std::atomic<bool> visual_marks{false};
std::atomic<bool> visual_counter{false};

void save_settings()
{
    std::ofstream file("/data/local/tmp/settings.cfg", std::ios::binary);
    if (file.is_open())
    {
        bool aim_enabled = aim_is_aim.load();
        file.write(reinterpret_cast<char *>(&aim_enabled), sizeof(aim_enabled));
        float fov = aim_fov.load();
        file.write(reinterpret_cast<char *>(&fov), sizeof(fov));
        float sensitivity = aim_sensitivity_factor.load();
        file.write(reinterpret_cast<char *>(&sensitivity), sizeof(sensitivity));
        float swipe_duration = aim_swipe_duration.load();
        file.write(reinterpret_cast<char *>(&swipe_duration), sizeof(swipe_duration));
        float latency_drag = aim_latency_drag.load();
        file.write(reinterpret_cast<char *>(&latency_drag), sizeof(latency_drag));
        float touch_x = aim_touch_x.load();
        file.write(reinterpret_cast<char *>(&touch_x), sizeof(touch_x));
        float touch_y = aim_touch_y.load();
        file.write(reinterpret_cast<char *>(&touch_y), sizeof(touch_y));
        float touch_radius = aim_touch_radius.load();
        file.write(reinterpret_cast<char *>(&touch_radius), sizeof(touch_radius));
        float fire_x = aim_zone_fire_x.load();
        file.write(reinterpret_cast<char *>(&fire_x), sizeof(fire_x));
        float fire_y = aim_zone_fire_y.load();
        file.write(reinterpret_cast<char *>(&fire_y), sizeof(fire_y));
        float fire_radius = aim_zone_fire_radius.load();
        file.write(reinterpret_cast<char *>(&fire_radius), sizeof(fire_radius));
        float frame_rate_value = frame_rate.load();
        file.write(reinterpret_cast<char *>(&frame_rate_value), sizeof(frame_rate_value));

        bool box_enabled = visual_box.load();
        file.write(reinterpret_cast<char *>(&box_enabled), sizeof(box_enabled));
        bool health_enabled = visual_health.load();
        file.write(reinterpret_cast<char *>(&health_enabled), sizeof(health_enabled));
        bool name_enabled = visual_name.load();
        file.write(reinterpret_cast<char *>(&name_enabled), sizeof(name_enabled));
        bool marks_enabled = visual_marks.load();
        file.write(reinterpret_cast<char *>(&marks_enabled), sizeof(marks_enabled));
        bool counter_enabled = visual_counter.load();
        file.write(reinterpret_cast<char *>(&counter_enabled), sizeof(counter_enabled));
        file.close();
    }
}

void load_settings()
{
    std::ifstream file("/data/local/tmp/settings.cfg", std::ios::binary);
    if (file.is_open())
    {
        bool aim_enabled;
        if (file.read(reinterpret_cast<char *>(&aim_enabled), sizeof(aim_enabled)))
            aim_is_aim.store(aim_enabled);
        float fov;
        if (file.read(reinterpret_cast<char *>(&fov), sizeof(fov)))
            aim_fov.store(fov);
        float sensitivity;
        if (file.read(reinterpret_cast<char *>(&sensitivity), sizeof(sensitivity)))
            aim_sensitivity_factor.store(sensitivity);
        float swipe_duration;
        if (file.read(reinterpret_cast<char *>(&swipe_duration), sizeof(swipe_duration)))
            aim_swipe_duration.store(swipe_duration);
        float latency_drag;
        if (file.read(reinterpret_cast<char *>(&latency_drag), sizeof(latency_drag)))
            aim_latency_drag.store(latency_drag);
        float touch_x;
        if (file.read(reinterpret_cast<char *>(&touch_x), sizeof(touch_x)))
            aim_touch_x.store(touch_x);
        float touch_y;
        if (file.read(reinterpret_cast<char *>(&touch_y), sizeof(touch_y)))
            aim_touch_y.store(touch_y);
        float touch_radius;
        if (file.read(reinterpret_cast<char *>(&touch_radius), sizeof(touch_radius)))
            aim_touch_radius.store(touch_radius);
        float fire_x;
        if (file.read(reinterpret_cast<char *>(&fire_x), sizeof(fire_x)))
            aim_zone_fire_x.store(fire_x);
        float fire_y;
        if (file.read(reinterpret_cast<char *>(&fire_y), sizeof(fire_y)))
            aim_zone_fire_y.store(fire_y);
        float fire_radius;
        if (file.read(reinterpret_cast<char *>(&fire_radius), sizeof(fire_radius)))
            aim_zone_fire_radius.store(fire_radius);
        float frame_rate_value;
        if (file.read(reinterpret_cast<char *>(&frame_rate_value), sizeof(frame_rate_value)))
            frame_rate.store(frame_rate_value);

        bool box_enabled;
        if (file.read(reinterpret_cast<char *>(&box_enabled), sizeof(box_enabled)))
            visual_box.store(box_enabled);
        bool health_enabled;
        if (file.read(reinterpret_cast<char *>(&health_enabled), sizeof(health_enabled)))
            visual_health.store(health_enabled);
        bool name_enabled;
        if (file.read(reinterpret_cast<char *>(&name_enabled), sizeof(name_enabled)))
            visual_name.store(name_enabled);
        bool marks_enabled;
        if (file.read(reinterpret_cast<char *>(&marks_enabled), sizeof(marks_enabled)))
            visual_marks.store(marks_enabled);
        bool counter_enabled;
        if (file.read(reinterpret_cast<char *>(&counter_enabled), sizeof(counter_enabled)))
            visual_counter.store(counter_enabled);
        file.close();
    }
}

void aimbot_thread()
{

    const float min_sensitivity = aim_sensitivity_factor.load();
    const float max_sensitivity = aim_sensitivity_factor.load() * 1.5f;
    const float precision_radius = 20.0f;
    const float speed_radius = 150.0f;
    const float distance_to_target = aim_fov.load();

    bool is_touching = false;
    float tx = aim_touch_x.load(std::memory_order_relaxed);
    float ty = aim_touch_y.load(std::memory_order_relaxed);

    while (is_running.load(std::memory_order_relaxed))
    {
        Utils::control_frame_rate(frame_rate.load(std::memory_order_relaxed));
        int read_idx = ready_buffer_index.load(std::memory_order_acquire);
        const auto &game_data = game_buffers[read_idx];

        android::ANativeWindowCreator::DisplayInfo display = android::ANativeWindowCreator::GetDisplayInfo();
        if (display.width < display.height)
            std::swap(display.width, display.height);
        TouchInput::setDisplayInfo(display.width, display.height, display.orientation);

        TouchInput::TouchRect aim_zone_fire = TouchInput::createZoneFromCenter(
            aim_zone_fire_x.load(), aim_zone_fire_y.load(), aim_zone_fire_radius.load());
        aim_is_weapon_firing.store(TouchInput::isTouchInZone(aim_zone_fire));

        if (!aim_is_aim.load() || !aim_is_weapon_firing.load())
        {
            if (is_touching)
            {
                TouchInput::sendTouchUp();
                is_touching = false;
                tx = aim_touch_x.load();
                ty = aim_touch_y.load();
            }
            continue;
        }

        // Aim at the selected target
        Structs::FVector selected_target = {0.0f, 0.0f, 0.0f};
        float best_world_distance = FLT_MAX;
        float best_screen_distance = FLT_MAX;
        bool close_enemy_found = false;

        int cx = display.width / 2;
        int cy = display.height / 2;

        for (int i = 0; i < game_data.count_enemies; i++)
        {
            const Structs::Player &player = game_data.players[i];
            if (!player.is_on_screen)
                continue;

            float world_distance = player.distance;
            float dx = player.target.X - cx;
            float dy = player.target.Y - cy;
            float screen_distance = sqrtf(dx * dx + dy * dy);

            if (world_distance <= 50.0f && screen_distance <= aim_fov.load(std::memory_order_relaxed))
            {
                close_enemy_found = true;
                if (world_distance < best_world_distance)
                {
                    best_world_distance = world_distance;
                    selected_target = player.target;
                }
            }
            else if (!close_enemy_found && screen_distance <= aim_fov.load(std::memory_order_relaxed))
            {
                if (screen_distance < best_screen_distance)
                {
                    best_screen_distance = screen_distance;
                    selected_target = player.target;
                }
            }
        }

        if (selected_target.X <= 0 && selected_target.Y <= 0)
        {
            if (is_touching)
            {
                TouchInput::sendTouchUp();
                is_touching = false;
                tx = aim_touch_x.load();
                ty = aim_touch_y.load();
            }
            continue;
        }

        // Calculate the swipe sensitivity based on distance to target
        float sensitivity = 0.0f;
        if (distance_to_target <= precision_radius)
        {
            sensitivity = min_sensitivity;
        }
        else if (distance_to_target >= speed_radius)
        {
            sensitivity = max_sensitivity;
        }
        else
        {
            float t = (distance_to_target - precision_radius) / (speed_radius - precision_radius);
            sensitivity = min_sensitivity + t * (max_sensitivity - min_sensitivity);
        }

        // Calculate the swipe distance and perform the swipe
        float swipe_dx = (selected_target.X - cx) * sensitivity;
        float swipe_dy = (selected_target.Y - cy) * sensitivity;
        float start_x = tx;
        float start_y = ty;
        float end_x = start_x + swipe_dx;
        float end_y = start_y + swipe_dy;

        // Ensure the swipe distance is significant enough to be considered a swipe
        float distance_to_swipe = sqrtf(swipe_dx * swipe_dx + swipe_dy * swipe_dy);
        if (distance_to_swipe < 0.9f) // If the swipe distance is too small, skip it, 0.9f is the threshold
            continue;

        // Calculate the number of iterations based on the swipe distance
        int max_iterations = std::clamp((int)(distance_to_swipe / 10.0f), 5, 10);

        // If the touch is not already active, send the initial touch down event
        if (!is_touching)
        {
            TouchInput::sendTouchMove(start_x, start_y);
            is_touching = true;
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
        }

        // Perform the swipe in a smooth manner
        for (int i = 0; i < max_iterations; ++i)
        {
            float t = powf((float)(i + 1) / max_iterations, 0.7f); // Apply easing to the swipe
            float ix = start_x + t * (end_x - start_x);            // Interpolated x position
            float iy = start_y + t * (end_y - start_y);            // Interpolated y position

            TouchInput::sendTouchMove(ix, iy);
            std::this_thread::sleep_for(std::chrono::microseconds((int)(aim_swipe_duration.load() * 1000 / max_iterations))); // Sleep for a short duration to simulate a smooth swipe
        }

        // Finalize the touch position saving the last position
        tx = end_x;
        ty = end_y;
    }
}

void draw_slider_with_buttons(const char *label, std::atomic<float> &value, float min, float max, float step = 1.0f)
{
    float tmp = value.load();
    bool changed = false;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::Text("%s", label);
    ImGui::PopStyleColor();

    std::string slider_id = std::string("##slider_") + label;
    float button_width = 80.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float padding_right = 10.0f;
    float available_width = ImGui::GetContentRegionAvail().x - (2 * button_width) - (2 * spacing) - padding_right;

    ImGui::PushItemWidth(available_width);
    if (ImGui::SliderFloat(slider_id.c_str(), &tmp, min, max))
    {
        value.store(tmp);
        changed = true;
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button((std::string("-##") + label).c_str(), ImVec2(button_width, 0)))
    {
        tmp = value.load() - step;
        if (tmp < min)
            tmp = min;
        value.store(tmp);
        changed = true;
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding_right); // إبعاد عن الحافة اليمنى
    if (ImGui::Button((std::string("+##") + label).c_str(), ImVec2(button_width, 0)))
    {
        tmp = value.load() + step;
        if (tmp > max)
            tmp = max;
        value.store(tmp);
        changed = true;
    }

    if (changed)
    {
        save_settings();
    }
}

void drawing_thread()
{
    ImColor player_color = IM_COL32(255, 0, 0, 255);
    ImColor bot_color = IM_COL32(220, 220, 220, 255);
    ImColor not_alive_color = IM_COL32(119, 0, 179, 255);
    ImColor text_color_player = IM_COL32(179, 173, 0, 255);

    android::ANativeWindowCreator::DisplayInfo display = android::ANativeWindowCreator::GetDisplayInfo();
    if (display.width < display.height)
        std::swap(display.width, display.height);
    ANativeWindow *window = android::ANativeWindowCreator::Create("CPING", display.width, display.height, false);
    Renderer::Init(window, display.width, display.height);

    load_settings();

    while (is_running.load(std::memory_order_relaxed))
    {
        Utils::control_frame_rate(frame_rate.load(std::memory_order_relaxed));
        int read_idx = ready_buffer_index.load(std::memory_order_acquire);
        const auto &game_data = game_buffers[read_idx];

        Renderer::StartFrame();
        ImDrawList *draw_list = ImGui::GetBackgroundDrawList();

        /// Control Panel////////////////////////////////////////////////
        auto &io = ImGui::GetIO();
        auto activeTouches = TouchInput::getActiveTouchPoints();
        if (!activeTouches.empty())
        {
            io.MousePos = ImVec2(float(activeTouches[0].first), float(activeTouches[0].second));
            io.MouseDown[0] = true;
        }
        else
        {
            io.MouseDown[0] = false;
        }
        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(1.3f);
        io.FontGlobalScale = 1.5f;
        ImGui::SetNextWindowPos(ImVec2(display.width * 0.3f, display.height * 0.3f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(display.width * 0.35f, display.height * 0.55f), ImGuiCond_FirstUseEver);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysVerticalScrollbar;
        if (ImGui::Begin("Control Panel", nullptr, window_flags))
        {
            ImGuiTabBarFlags tab_bar_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysVerticalScrollbar;
            if (ImGui::BeginTabBar("SettingsTabs", tab_bar_flags))
            {
                if (ImGui::BeginTabItem("Aimbot"))
                {
                    ImGui::Spacing();
                    ImGui::SeparatorText("Activation");
                    bool tmp_aim = aim_is_aim.load();
                    if (ImGui::Checkbox("Enable Aimbot", &tmp_aim))
                    {
                        aim_is_aim.store(tmp_aim);
                        save_settings();
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::SeparatorText("Targeting Settings");
                    draw_slider_with_buttons("FOV", aim_fov, 100.0f, 300.0f, 10.0f);
                    draw_slider_with_buttons("Sensitivity", aim_sensitivity_factor, 0.001f, 1.0f, 0.001f);
                    draw_slider_with_buttons("Swipe Duration", aim_swipe_duration, 5.0f, 50.0f, 1.0f);
                    draw_slider_with_buttons("Latency Drag", aim_latency_drag, 0.0f, 1.0f, 0.01f);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::SeparatorText("Touch Zone Settings");
                    draw_slider_with_buttons("Touch Position X", aim_touch_x, 0.0f, display.width, 10.0f);
                    draw_slider_with_buttons("Touch Position Y", aim_touch_y, 0.0f, display.height, 10.0f);
                    draw_slider_with_buttons("Touch Radius", aim_touch_radius, 10.0f, 300.0f, 10.0f);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::SeparatorText("Fire Zone Settings");
                    draw_slider_with_buttons("Fire Position X", aim_zone_fire_x, 0.0f, display.width, 10.0f);
                    draw_slider_with_buttons("Fire Position Y", aim_zone_fire_y, 0.0f, display.height, 10.0f);
                    draw_slider_with_buttons("Fire Radius", aim_zone_fire_radius, 10.0f, 300.0f, 10.0f);

                    ImGui::Spacing();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Visuals"))
                {
                    ImGui::Spacing();

                    ImGui::SeparatorText("Display Options");
                    ImGui::Columns(2, "VisualColumns", false);

                    bool tmp_box = visual_box.load();
                    if (ImGui::Checkbox("Show Box", &tmp_box))
                    {
                        visual_box.store(tmp_box);
                        save_settings();
                    }

                    bool tmp_health = visual_health.load();
                    if (ImGui::Checkbox("Show Health", &tmp_health))
                    {
                        visual_health.store(tmp_health);
                        save_settings();
                    }

                    bool tmp_counter = visual_counter.load();
                    if (ImGui::Checkbox("Show Counter", &tmp_counter))
                    {
                        visual_counter.store(tmp_counter);
                        save_settings();
                    }

                    ImGui::NextColumn();

                    bool tmp_name = visual_name.load();
                    if (ImGui::Checkbox("Show Name", &tmp_name))
                    {
                        visual_name.store(tmp_name);
                        save_settings();
                    }

                    bool tmp_marks = visual_marks.load();
                    if (ImGui::Checkbox("Show Marks", &tmp_marks))
                    {
                        visual_marks.store(tmp_marks);
                        save_settings();
                    }

                    ImGui::Columns(1);
                    ImGui::Spacing();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Settings"))
                {
                    ImGui::Spacing();
                    ImGui::SeparatorText("General Settings");
                    ImGui::Text("Settings are saved automatically!");
                    ImGui::Spacing();

                    float button_height = 40.0f;
                    if (ImGui::Button("Load Settings", ImVec2(ImGui::CalcTextSize("Load Settings").x + 20.0f, button_height)))
                    {
                        load_settings();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset Default", ImVec2(ImGui::CalcTextSize("Reset Default").x + 20.0f, button_height)))
                    {
                        frame_rate.store(60.0f);
                        aim_is_aim.store(false);
                        aim_fov.store(300.0f);
                        aim_touch_x.store(1665.1f);
                        aim_touch_y.store(475.0f);
                        aim_touch_radius.store(100.0f);
                        aim_zone_fire_x.store(380.0f);
                        aim_zone_fire_y.store(180.0f);
                        aim_zone_fire_radius.store(150.0f);
                        aim_sensitivity_factor.store(0.04f);
                        aim_latency_drag.store(0.13f);
                        aim_swipe_duration.store(10.0f);

                        visual_box.store(false);
                        visual_health.store(false);
                        visual_name.store(false);
                        visual_marks.store(false);
                        visual_counter.store(false);
                        save_settings();
                    }

                    ImGui::Spacing();
                    ImGui::SeparatorText("FPS Settings");
                    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + display.width * 0.35f);
                    ImGui::Text("The frame rate (FPS) should be set to match your game's support. Do not lower or increase it, as this will affect performance. Please understand.");
                    ImGui::PopTextWrapPos();
                    draw_slider_with_buttons("", frame_rate, 20.0f, 120.0f, 10.0f);
                    ImGui::Spacing();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
        style.ScaleAllSizes(1.0f / 1.3f);
        io.FontGlobalScale = 1.0f;
        /// Control Panel////////////////////////////////////////////////

        for (int i = 0; i < game_data.count_enemies; i++)
        {
            const Structs::Player &player = game_data.players[i];
            if (player.health < 0)
                continue;

            ImU32 color = (player.is_bot) ? bot_color : (player.is_alive) ? player_color
                                                                          : not_alive_color;
            ImU32 text_color = (player.is_bot) ? bot_color : (player.is_alive) ? text_color_player
                                                                               : not_alive_color;

            if (player.is_on_screen)
            {
                float text_scale_size = Utils::calculateTextSize(player.distance, 10.0f, 460.0f, 10.0f, 30.0f, 0.2f);
                if (visual_box.load())
                {
                    Structs::FVector screen_pos = player.location;
                    Structs::FVector bounds[8] = {player.bounds[0], player.bounds[1], player.bounds[2],
                                                  player.bounds[3], player.bounds[4], player.bounds[5],
                                                  player.bounds[6], player.bounds[7]};
                    for (int j = 0; j < 4; ++j)
                    {
                        Structs::FVector p1 = bounds[j];
                        Structs::FVector p2 = bounds[(j + 1) % 4];
                        draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), color, 2.0f);
                    }
                    for (int j = 0; j < 4; ++j)
                    {
                        Structs::FVector p1 = bounds[j + 4];
                        Structs::FVector p2 = bounds[((j + 1) % 4) + 4];
                        draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), color, 2.0f);
                    }
                    for (int j = 0; j < 4; ++j)
                    {
                        Structs::FVector p1 = bounds[j];
                        Structs::FVector p2 = bounds[j + 4];
                        draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), color, 2.0f);
                    }
                }

                if (visual_name.load())
                {
                    std::string display_text = std::to_string((int)player.distance) + "m";
                    Utils::add_text_center(draw_list, display_text, text_scale_size,
                                           ImVec2(player.root.X, player.root.Y), text_color, true,
                                           0.95f);
                }
                if (visual_health.load())
                {

                    Utils::advanced_health_bar(draw_list, player.head.X, player.head.Y, display.width,
                                               display.height, player.health, 100.0f, color,
                                               IM_COL32(0, 0, 0, 100), player.distance, player.team_id);
                }
            }
            else
            {
                if (visual_marks.load())
                {
                    Structs::OverlayInfo overlay = Ue4::compute_offscreen_enemy_overlay(player.position, game_data.minimal_view_info, display.width, display.height);

                    ImVec2 shadow_off = {3.5f, 2.5f};

                    float dx = overlay.arrow.tip.x - (overlay.arrow.side1.x + overlay.arrow.side2.x) / 2.0f;
                    float dy = overlay.arrow.tip.y - (overlay.arrow.side1.y + overlay.arrow.side2.y) / 2.0f;
                    float length = sqrt(dx * dx + dy * dy);

                    dx /= length;
                    dy /= length;

                    float scale_arrow = 3.0f;
                    float arrow_head_size = 14.0f * scale_arrow;
                    float arrow_body_width = 8.0f * scale_arrow;
                    float arrow_body_length = 15.0f * scale_arrow;

                    ImVec2 tip = overlay.arrow.tip;
                    ImVec2 head_base = ImVec2(tip.x - dx * arrow_head_size, tip.y - dy * arrow_head_size);
                    ImVec2 head_left = ImVec2(head_base.x - dy * arrow_head_size * 0.6f, head_base.y + dx * arrow_head_size * 0.6f);
                    ImVec2 head_right = ImVec2(head_base.x + dy * arrow_head_size * 0.6f, head_base.y - dx * arrow_head_size * 0.6f);

                    ImVec2 body_top_left = ImVec2(head_base.x - dy * arrow_body_width * 0.5f, head_base.y + dx * arrow_body_width * 0.5f);
                    ImVec2 body_top_right = ImVec2(head_base.x + dy * arrow_body_width * 0.5f, head_base.y - dx * arrow_body_width * 0.5f);
                    ImVec2 body_bottom_left = ImVec2(body_top_left.x - dx * arrow_body_length, body_top_left.y - dy * arrow_body_length);
                    ImVec2 body_bottom_right = ImVec2(body_top_right.x - dx * arrow_body_length, body_top_right.y - dy * arrow_body_length);

                    ImGui::GetForegroundDrawList()->AddTriangleFilled(
                        ImVec2(tip.x + shadow_off.x, tip.y + shadow_off.y),
                        ImVec2(head_left.x + shadow_off.x, head_left.y + shadow_off.y),
                        ImVec2(head_right.x + shadow_off.x, head_right.y + shadow_off.y),
                        IM_COL32(0, 0, 0, 200));

                    ImGui::GetForegroundDrawList()->AddQuadFilled(
                        ImVec2(body_top_left.x + shadow_off.x, body_top_left.y + shadow_off.y),
                        ImVec2(body_top_right.x + shadow_off.x, body_top_right.y + shadow_off.y),
                        ImVec2(body_bottom_right.x + shadow_off.x, body_bottom_right.y + shadow_off.y),
                        ImVec2(body_bottom_left.x + shadow_off.x, body_bottom_left.y + shadow_off.y),
                        IM_COL32(0, 0, 0, 200));

                    ImGui::GetForegroundDrawList()->AddQuadFilled(body_top_left, body_top_right, body_bottom_right, body_bottom_left, color);
                    ImGui::GetForegroundDrawList()->AddTriangleFilled(tip, head_left, head_right, color);
                }
            }
        }

        if (visual_counter.load())
        {
            if (game_data.count_enemies > 0)
            {
                std::string text = std::string("Enemies: " + std::to_string(game_data.count_enemies));
                Utils::add_text_center(draw_list, text, 60.0f,
                                       ImVec2(display.width * 0.5f, display.height * 0.12f),
                                       text_color_player, true, 1.1f);
            }

            draw_list->AddCircle(ImVec2(display.width * 0.5f, display.height * 0.5f), aim_fov,
                                 IM_COL32(255, 255, 255, 100), 100, 3.0f);
            draw_list->AddCircleFilled(ImVec2(aim_zone_fire_x, aim_zone_fire_y), aim_zone_fire_radius,
                                       IM_COL32(0, 180, 180, 50));
            draw_list->AddCircleFilled(ImVec2(aim_touch_x, aim_touch_y), aim_touch_radius,
                                       IM_COL32(180, 90, 0, 50), 100);
        }

        Utils::add_text_center(draw_list, EncryptedBranding::get_protected_branding(), 30.0f,
                               ImVec2(display.width * 0.1f, display.height * 0.9f),
                               text_color_player, true, 1.1f);
        Renderer::EndFrame();

        if (!EncryptedBranding::verify_branding_integrity())
        {
            exit(13);
        }
    }

    Renderer::Shutdown();
    android::ANativeWindowCreator::Destroy(window);
    window = nullptr;
}

int main(int argc, char *argv[])
{
    Utils::decrypt_and_run(cipher, key);

    if (!EncryptedBranding::verify_branding_integrity())
    {
        exit(13);
    }

    if (Utils::is_package_running("com.tencent.ig"))
        target_pid.store(Utils::find_pid_by_package_name("com.tencent.ig"));
    else if (Utils::is_package_running("com.vng.pubgmobile"))
        target_pid.store(Utils::find_pid_by_package_name("com.vng.pubgmobile"));
    else if (Utils::is_package_running("com.pubg.krmobile"))
        target_pid.store(Utils::find_pid_by_package_name("com.pubg.krmobile"));
    else if (Utils::is_package_running("com.rekoo.pubgm"))
        target_pid.store(Utils::find_pid_by_package_name("com.rekoo.pubgm"));
    else if (Utils::is_package_running("com.pubg.imobile"))
        target_pid.store(Utils::find_pid_by_package_name("com.pubg.imobile"));
    else
    {
        Logger::i("not open game or not support\n");
        exit(1);
        return 0;
    }

    lib_base.store(Utils::find_ue4_base(target_pid.load(std::memory_order_relaxed)));
    if (!lib_base.load(std::memory_order_relaxed))
    {
        Logger::i("Failed to find libUE4.so base address!\n");
        return -1;
    }

    android::ANativeWindowCreator::DisplayInfo display = android::ANativeWindowCreator::GetDisplayInfo();
    if (display.width < display.height)
        std::swap(display.width, display.height);

    is_running.store(true);
    aim_is_weapon_firing.store(false);

    TouchInput::touchInputStart();
    TouchInput::setDisplayInfo(display.width, display.height, display.orientation);

    aimbot_thread_handle = std::thread(aimbot_thread);
    drawing_thread_handle = std::thread(drawing_thread);

    float margin = 20.0f; // Margin around the screen area
    while (is_running.load(std::memory_order_relaxed))
    {
        Utils::control_frame_rate(frame_rate.load(std::memory_order_relaxed));
        int write_idx = write_buffer_index.load(std::memory_order_relaxed);
        auto &current_buffer = game_buffers[write_idx];
        current_buffer.clear();

        uintptr_t u_g_world = Memory::Read<uintptr_t>(lib_base.load() + Offset::g_world, target_pid);
        uintptr_t u_world = Memory::Read<uintptr_t>(Memory::Read<uintptr_t>(u_g_world + 0x58, target_pid) + 0x78, target_pid);
        uintptr_t u_level = Memory::Read<uintptr_t>(u_world + Offset::persistent_level, target_pid);
        uintptr_t actors_list = Ue4::get_actors_array(u_level, Offset::u_level_to_a_actors, 0x448, target_pid);
        uintptr_t u_level_to_a_actors = Memory::Read<uintptr_t>(actors_list, target_pid);
        int u_level_to_a_actors_count = Memory::Read<int>(actors_list + sizeof(uintptr_t), target_pid);

        // local player
        uintptr_t player_controller = Memory::Read<uintptr_t>(Memory::Read<uintptr_t>(Memory::Read<uintptr_t>(u_world + 0x38, target_pid) + 0x78, target_pid) + 0x30, target_pid);
        uintptr_t acknowledged_pawn = Memory::Read<uintptr_t>(player_controller + Offset::acknowledged_pawn, target_pid);
        uintptr_t character_movement_local = Memory::Read<uintptr_t>(acknowledged_pawn + Offset::character_movement, target_pid);
        uintptr_t weapon_manager = Memory::Read<uintptr_t>(acknowledged_pawn + Offset::weapon_manager, target_pid);
        uintptr_t current_weapon = Memory::Read<uintptr_t>(weapon_manager + Offset::current_weapon, target_pid);
        uintptr_t weapon_entity = Memory::Read<uintptr_t>(current_weapon + Offset::weapon_entity, target_pid);

        int weapon_id = Memory::Read<int>(weapon_entity + Offset::weapon_id, target_pid);
        float bullet_speed_local = Memory::Read<float>(weapon_entity + Offset::bullet_speed, target_pid);
        uint8_t b_has_auto_fire_mode_local = Memory::Read<bool>(weapon_entity + Offset::b_has_auto_fire_mode, target_pid);
        bool b_is_gun_ads_local = Memory::Read<bool>(acknowledged_pawn + Offset::b_is_gun_ads, target_pid);
        float recoil_kick_local = Memory::Read<float>(weapon_entity + Offset::recoil_kick, target_pid);
        float recoil_kick_ads_local = Memory::Read<float>(weapon_entity + Offset::recoil_kick_ads, target_pid);
        float accessories_v_recoil_factor_local = Memory::Read<float>(weapon_entity + Offset::accessories_v_recoil_factor, target_pid);
        float accessories_h_recoil_factor_local = Memory::Read<float>(weapon_entity + Offset::accessories_h_recoil_factor, target_pid);
        int team_id_local = Memory::Read<int>(player_controller + Offset::team_id_local, target_pid);
        float gravity_scale_local = Memory::Read<float>(character_movement_local + Offset::gravity_scale, target_pid);
        Structs::FVector shooter_velocity_local = Memory::Read<Structs::FVector>(character_movement_local + Offset::velocity, target_pid);

        uintptr_t player_camera_manager = Memory::Read<uintptr_t>(player_controller + Offset::player_camera_manager, target_pid);
        Structs::CameraCacheEntry camera_cache = Memory::Read<Structs::CameraCacheEntry>(player_camera_manager + Offset::camera_cache, target_pid);
        Structs::MinimalViewInfo minimal_view_info = camera_cache.POV;
        current_buffer.minimal_view_info = minimal_view_info;

        for (size_t i = 0; i < u_level_to_a_actors_count; i++)
        {
            uintptr_t actor = Memory::Read<uintptr_t>(u_level_to_a_actors + i * sizeof(uintptr_t), target_pid);
            if (!actor)
                continue;

            if (actor == acknowledged_pawn)
                continue;

            if (i + 1 < u_level_to_a_actors_count)
            {
                __builtin_prefetch((void *)(u_level_to_a_actors + (i + 1) * sizeof(uintptr_t)), 0, 1);
            }

            int team_id = Memory::Read<int>(actor + Offset::team_id, target_pid);
            if (team_id_local == team_id || team_id <= -1 || team_id >= 1000)
                continue;

            int player_death = Memory::Read<int>(actor + Offset::bis_dead, target_pid);
            if (player_death)
                continue;

            int current_states = Memory::Read<int>(actor + Offset::current_states, target_pid);
            if (current_states == 262144 || current_states == 6 || current_states == 1700229408 || current_states == 0)
                continue;

            float health = Memory::Read<float>(actor + Offset::health, target_pid);
            bool is_alive = health > 0;
            if (health < 0)
                continue;

            std::string player_name = Memory::ReadFString(actor + Offset::player_name, target_pid);
            if (player_name.empty() || player_name == "Unknown")
                continue;

            uintptr_t actor_root_component = Memory::Read<uintptr_t>(actor + Offset::root_component, target_pid);
            if (!actor_root_component)
                continue;

            Structs::FTransform transform = Memory::Read<Structs::FTransform>(actor_root_component + Offset::component_to_world, target_pid);
            float distance = (Structs::FVector::Distance(minimal_view_info.Location, transform.Translation) / 100.0f);
            if (distance > 400.0f || distance < 1.0f)
                continue;

            bool is_bot = Memory::Read<bool>(actor + Offset::bis_ai, target_pid);

            Structs::FVector screen_pos = Ue4::world_to_screen(transform.Translation, minimal_view_info, display.width, display.height);
            bool is_on_screen = !(screen_pos.X < margin || screen_pos.Y < margin || screen_pos.X > display.width - margin || screen_pos.Y > display.height - margin || screen_pos.Z < 0.0f);

            uintptr_t mesh = Memory::Read<uintptr_t>(actor + Offset::mesh, target_pid);
            Structs::FBoxSphereBounds cached_local_bounds = Memory::Read<Structs::FBoxSphereBounds>(mesh + Offset::cached_local_bounds, target_pid);
            Structs::FTransform transform_bounds = Memory::Read<Structs::FTransform>(mesh + Offset::component_to_world, target_pid);

            uintptr_t character_movement = Memory::Read<uintptr_t>(actor + Offset::character_movement, target_pid);
            Structs::FVector target_velocity = Memory::Read<Structs::FVector>(character_movement + Offset::velocity, target_pid);
            Structs::FVector target_acceleration = Memory::Read<Structs::FVector>(character_movement + Offset::acceleration, target_pid);

            // Fill player object 3D box corners
            cached_local_bounds.BoxExtent.X *= 0.45f;
            cached_local_bounds.BoxExtent.Y *= 0.45f;
            cached_local_bounds.BoxExtent.Z *= 0.65f;
            cached_local_bounds.Origin.Z -= cached_local_bounds.BoxExtent.Z * 0.0f;

            Structs::FVector corners[8] = {
                {-cached_local_bounds.BoxExtent.X, -cached_local_bounds.BoxExtent.Y, -cached_local_bounds.BoxExtent.Z},
                {cached_local_bounds.BoxExtent.X, -cached_local_bounds.BoxExtent.Y, -cached_local_bounds.BoxExtent.Z},
                {cached_local_bounds.BoxExtent.X, cached_local_bounds.BoxExtent.Y, -cached_local_bounds.BoxExtent.Z},
                {-cached_local_bounds.BoxExtent.X, cached_local_bounds.BoxExtent.Y, -cached_local_bounds.BoxExtent.Z},
                {-cached_local_bounds.BoxExtent.X, -cached_local_bounds.BoxExtent.Y, cached_local_bounds.BoxExtent.Z},
                {cached_local_bounds.BoxExtent.X, -cached_local_bounds.BoxExtent.Y, cached_local_bounds.BoxExtent.Z},
                {cached_local_bounds.BoxExtent.X, cached_local_bounds.BoxExtent.Y, cached_local_bounds.BoxExtent.Z},
                {-cached_local_bounds.BoxExtent.X, cached_local_bounds.BoxExtent.Y, cached_local_bounds.BoxExtent.Z}};

            Structs::FTransform transform_mech = Ue4::get_component_to_world(actor, target_pid);
            Structs::FTransform bone_transform_head = Ue4::get_bone_transform(actor, 6, target_pid);
            Structs::FTransform bone_transform_root = Ue4::get_bone_transform(actor, 0, target_pid);
            Structs::FVector head_position = transform_mech.TransformPosition(bone_transform_head.Translation);
            Structs::FVector root_position = transform_mech.TransformPosition(bone_transform_root.Translation);
            Structs::FVector head_location = Ue4::world_to_screen(head_position, minimal_view_info, display.width, display.height);
            Structs::FVector root_location = Ue4::world_to_screen(root_position, minimal_view_info, display.width, display.height);

            Structs::FVector aim_point = {0.0f, 0.0f, 0.0f};
            float cx = display.width / 2.0f;
            float cy = display.height / 2.0f;
            float screen_distance = sqrt(powf(head_location.X - cx, 2) + powf(head_location.Y - cy, 2));
            if (aim_is_aim.load() && aim_is_weapon_firing.load() && is_on_screen && is_alive && screen_distance <= aim_fov.load())
            {

                constexpr int MAX_ITERS = 10;                  // number of convergence iterations
                constexpr float TO_DEGREES = 57.2957795f;      // conversion factor from radians to degrees
                constexpr float DEG2RAD = 3.14159265f / 180.f; // conversion factor from degrees to radians
                constexpr float G_CM = 980.f;                  // acceleration due to gravity in cm/s² in PUBG

                Structs::FVector target_position = head_position;
                Structs::FVector shooter_position = minimal_view_info.Location;
                float gravity_scale = gravity_scale_local;
                float bullet_speed = bullet_speed_local;
                Structs::FVector acceleration = target_acceleration;
                Structs::FVector velocity = target_velocity;
                float latency_drag = aim_latency_drag.load();
                float g_cm = G_CM * gravity_scale;

                bool b_is_gun_ads = b_is_gun_ads_local;
                bool b_has_auto_fire_mode = b_has_auto_fire_mode_local;
                float recoil_kick = recoil_kick_local;
                float recoil_kick_ads = recoil_kick_ads_local;
                float accessories_v_recoil_factor = accessories_v_recoil_factor_local;
                float accessories_h_recoil_factor = accessories_h_recoil_factor_local;

                // calculating the flight time of a bullet from a shooter (1)
                float distance_cm = (target_position - shooter_position).Length(); // unit cm.
                if (bullet_speed < 1.f)
                    bullet_speed = 1.f;
                float t_flight = distance_cm / bullet_speed; // time of flight in seconds

                // If the target is moving, we need to predict its future position, calculate iteratively loop (2)
                for (int iter = 0; iter < MAX_ITERS; ++iter)
                {
                    float t_squared = t_flight * t_flight; // square of the flight time
                    // calculate the future position of the target
                    Structs::FVector target_future = target_position + velocity * t_flight + acceleration * 0.5f * t_squared;

                    // calculate the drop of the bullet , g_cm is positive, so drop is positive
                    float drop = 0.5f * g_cm * t_squared;
                    aim_point = target_future;
                    aim_point.Z += drop;

                    // calculate the new flight time, aim_point is the point where the bullet should hit
                    float new_distance = (aim_point - shooter_position).Length();
                    t_flight = new_distance / bullet_speed;
                }

                // Adjust aim_point for latency (3)
                aim_point = aim_point + (target_velocity * latency_drag);

                if (b_has_auto_fire_mode || !b_is_gun_ads)
                {
                    // Initial gun recoil with total recoil compensation per shot for scoped and unscoped rifles. (4)
                    float pitch_off_deg = (b_is_gun_ads ? recoil_kick + (recoil_kick + recoil_kick_ads) : recoil_kick) * accessories_v_recoil_factor;
                    float yaw_off_deg = 0.0f; // No yaw compensation for now

                    float dz = distance_cm * tanf(pitch_off_deg * DEG2RAD); // vertical offset due to pitch
                    float dx = distance_cm * tanf(yaw_off_deg * DEG2RAD);   // horizontal offset due to yaw

                    // calculate the camera forward, right, and up vectors
                    Structs::FVector cam_fwd = Ue4::rotator_to_vector(minimal_view_info.Rotation); // Camera forward vector
                    Structs::FVector world_up(0.f, 0.f, 1.f);                                      // World up vector in PUBG is Z axis
                    Structs::FVector cam_right = Ue4::cross(world_up, cam_fwd).GetSafeNormal();    // Right vector is perpendicular to the camera forward and world up
                    Structs::FVector cam_up = Ue4::cross(cam_fwd, cam_right);                      // Up vector is perpendicular to the camera forward and right

                    // Adjust aim_point based on camera orientation and recoil
                    dz = distance_cm * std::tan(pitch_off_deg * DEG2RAD);
                    dx = distance_cm * std::tan(yaw_off_deg * DEG2RAD);

                    aim_point = aim_point - cam_up * dz;    // Adjust aim_point for vertical recoil
                    aim_point = aim_point - cam_right * dx; // Adjust aim_point for horizontal recoil
                }
            }

            Structs::Player player_obj;
            for (int i = 0; i < 8; ++i)
            {
                Structs::FVector local = corners[i] + cached_local_bounds.Origin;
                Structs::FVector world = transform_bounds.TransformPosition(local);
                Structs::FVector screen = Ue4::world_to_screen(world, minimal_view_info, display.width, display.height);
                player_obj.bounds[i] = screen;
            }
            player_obj.position = transform.Translation;
            player_obj.target = Ue4::world_to_screen(aim_point, minimal_view_info, display.width, display.height);
            player_obj.location = screen_pos;
            player_obj.head = head_location;
            player_obj.root = root_location;
            player_obj.distance = distance;
            player_obj.health = health;
            player_obj.is_alive = is_alive;
            player_obj.is_bot = is_bot;
            player_obj.team_id = team_id;
            player_obj.is_on_screen = is_on_screen;
            if (current_buffer.count_enemies < 200)
            {
                current_buffer.players[current_buffer.count_enemies] = player_obj;
                current_buffer.count_enemies++;
            }
        }

        ready_buffer_index.store(write_idx, std::memory_order_release);
        write_buffer_index.store(1 - write_idx, std::memory_order_relaxed);

        if (!EncryptedBranding::verify_branding_integrity())
        {
            exit(13);
        }
    }

    is_running.store(false, std::memory_order_release);
    if (aimbot_thread_handle.joinable())
        aimbot_thread_handle.join();
    if (drawing_thread_handle.joinable())
        drawing_thread_handle.join();
    TouchInput::touchInputStop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return 0;
}