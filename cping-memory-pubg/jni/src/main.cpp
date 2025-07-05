#include "main.h"

// Double-buffer system
Structs::GameData game_buffers[2];
std::atomic<int> write_buffer_index{0};
std::atomic<int> ready_buffer_index{-1};

std::thread socket_thread_handle;
std::thread aimbot_thread_handle;

std::atomic<uintptr_t> lib_base{0};
std::atomic<pid_t> target_pid{0};
std::atomic<bool> is_running{true};
std::atomic<float> frame_rate{60.0f};

std::atomic<bool> aim_is_weapon_firing{false};
std::atomic<bool> aim_is_aiming{true};
std::atomic<float> aim_fov{300.0f};
std::atomic<float> aim_shooting{7.0f};
std::atomic<float> aim_touch_x{1665.1f};
std::atomic<float> aim_touch_y{475.0f};
std::atomic<float> aim_touch_radius{100.0f};
std::atomic<float> aim_zone_fire_x{380.0f};
std::atomic<float> aim_zone_fire_y{180.0f};
std::atomic<float> aim_zone_fire_radius{150.0f};

void socket_thread()
{
    SocketClient socket_client;
    bool socket_connected = false;
    const char *socket_name = "cping_socket_server";
    while (is_running.load(std::memory_order_relaxed))
    {
        Utils::control_frame_rate(frame_rate.load(std::memory_order_relaxed));
        if (!socket_connected)
        {
            if (socket_client.connect_to_server(socket_name))
            {
                socket_connected = true;
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
        }

        if (socket_connected)
        {
            int read_idx = ready_buffer_index.load(std::memory_order_acquire);
            const auto &game_data = game_buffers[read_idx];

            if (!socket_client.send_raw(&game_data, sizeof(Structs::GameData)))
            {
                socket_connected = false;
                socket_client.close_connection();
                continue;
            }
        }

        if (!EncryptedBranding::verify_branding_integrity())
        {
            exit(13);
        }
    }

    if (socket_connected)
    {
        socket_client.close_connection();
        socket_connected = false;
    }
}

/******************************************************************
 *  Aimbot Thread – الإصدار النهائي مع:
 *    • نموذج السحب التدريجي (Incremental Pull)
 *    • حماية حد دائرة اللمس
 *    • تعويض تسارع اللمس التلقائي (Auto-Tuning Velocity Factor)
 ******************************************************************/
void aimbot_thread()
{
    /* حالة اللمس الحالي */
    bool isDown = false;
    float tx = aim_touch_x.load(std::memory_order_relaxed);
    float ty = aim_touch_y.load(std::memory_order_relaxed);

    /* ثوابت الإحساس الأساسية */
    constexpr float kBaseSpeed = 0.25f;
    constexpr float kMinSpeed = 0.05f;
    constexpr float kSpeedFalloffRange = 100.f;

    constexpr float kMaxSingleMove = 40.f;
    constexpr float kDeadzone = 1.5f;
    constexpr float kPredictionFactor = 0.12f;

    constexpr float kFovEdgeThreshold = 0.8f;
    constexpr float kEdgeDampingFactor = 0.4f;

    constexpr float kMinWorldDist = 100.f;
    constexpr float kMaxWorldDist = 400.f;
    constexpr float kMaxDistanceDamping = 2.f;

    /* تعويض تسارع اللمس (Auto-Tuning) */
    float vel_factor = 1.0f; // يبدأ بدون تخميد
    constexpr float kVelLearnRate = 0.02f;
    constexpr int kErrorWindow = 30;
    float err_accum = 0.f;
    int err_cnt = 0;

    /* متغيرات التنبّؤ */
    Structs::FVector last_target{0, 0, 0};
    float last_dx = 0.f, last_dy = 0.f;

    /* مسافة الهدف قبل الحركة في الإطار السابق */
    float dist_before_move = -1.f;

    while (is_running.load(std::memory_order_relaxed))
    {
        /* ـــــ التحكم في معدل الإطارات ـــــ */
        Utils::control_frame_rate(frame_rate.load(std::memory_order_relaxed));

        int read_idx = ready_buffer_index.load(std::memory_order_acquire);
        const auto &game_data = game_buffers[read_idx];

        /* معلومات الشاشة */
        auto display = android::ANativeWindowCreator::GetDisplayInfo();
        if (display.width < display.height)
            std::swap(display.width, display.height);
        TouchInput::setDisplayInfo(display.width, display.height, display.orientation);

        /* فحص زر الإطلاق */
        auto fire_zone = TouchInput::createZoneFromCenter(
            aim_zone_fire_x.load(std::memory_order_relaxed),
            aim_zone_fire_y.load(std::memory_order_relaxed),
            aim_zone_fire_radius.load(std::memory_order_relaxed));
        aim_is_weapon_firing.store(TouchInput::isTouchInZone(fire_zone));

        /* إيقاف Aim */
        if (!aim_is_aiming.load(std::memory_order_relaxed))
        {
            if (isDown)
            {
                TouchInput::sendTouchUp();
                isDown = false;
            }
            last_dx = last_dy = 0.f;
            last_target = {0, 0, 0};
            dist_before_move = -1.f;
            continue;
        }

        /* ---------------- اختيار الهدف ---------------- */
        Structs::FVector target{0, 0, 0};
        float best_screen = FLT_MAX, best_world = FLT_MAX;
        bool close_found = false;
        int cx = display.width / 2, cy = display.height / 2;
        float fov = aim_fov.load(std::memory_order_relaxed);
        float sel_world_dist = -1.f;

        for (int i = 0; i < game_data.count_enemies; ++i)
        {
            const auto &p = game_data.players[i];
            if (!p.is_on_screen)
                continue;

            float dxp = p.target.X - cx;
            float dyp = p.target.Y - cy;
            float screen_d = std::sqrt(dxp * dxp + dyp * dyp);
            if (screen_d > fov)
                continue;

            if (p.distance <= 50.f) // عدو قريب جداً
            {
                close_found = true;
                if (p.distance < best_world)
                {
                    best_world = p.distance;
                    target = p.target;
                    sel_world_dist = p.distance;
                }
            }
            else if (!close_found && screen_d < best_screen) // أبعد – اختر الأقرب للشاشة
            {
                best_screen = screen_d;
                target = p.target;
                sel_world_dist = p.distance;
            }
        }

        /* لا يوجد هدف أو لا إطلاق */
        if ((target.X <= 0 && target.Y <= 0) ||
            !aim_is_weapon_firing.load(std::memory_order_relaxed))
        {
            if (isDown)
            {
                TouchInput::sendTouchUp();
                isDown = false;
            }
            last_dx = last_dy = 0.f;
            last_target = {0, 0, 0};
            dist_before_move = -1.f;
            continue;
        }

        /* ------------- حساب الإزاحة -------------- */
        float dx = target.X - cx;
        float dy = target.Y - cy;
        float dist = std::sqrt(dx * dx + dy * dy);

        /* Dead-zone */
        if (dist < kDeadzone)
        {
            if (isDown)
                TouchInput::sendTouchMove(tx, ty);
            last_dx = dx;
            last_dy = dy;
            last_target = target;
            dist_before_move = dist;
            continue;
        }

        /* لمس أوّل مرة */
        if (!isDown)
        {
            tx = aim_touch_x.load(std::memory_order_relaxed);
            ty = aim_touch_y.load(std::memory_order_relaxed);
            TouchInput::sendTouchMove(tx, ty); // Down
            isDown = true;
            last_dx = last_dy = 0.f;
        }

        /* ------------- السرعة الديناميكية ------------- */
        float ratio = std::min(1.f, dist / kSpeedFalloffRange);
        float dyn_speed = kMinSpeed + (kBaseSpeed - kMinSpeed) * ratio;

        /* تخميد حافة الـFOV */
        if (dist > fov * kFovEdgeThreshold)
        {
            float edge = (dist - fov * kFovEdgeThreshold) / (fov * (1.f - kFovEdgeThreshold));
            dyn_speed *= 1.f - edge * (1.f - kEdgeDampingFactor);
        }

        /* تخميد حسب المسافة الحقيقية */
        if (sel_world_dist > kMinWorldDist)
        {
            float r = (sel_world_dist - kMinWorldDist) / (kMaxWorldDist - kMinWorldDist);
            r = std::clamp(r, 0.f, 1.f);
            dyn_speed /= 1.f + r * (kMaxDistanceDamping - 1.f);
        }

        /* تنبؤ بالهدف */
        float pdx = dx + (dx - last_dx) * kPredictionFactor;
        float pdy = dy + (dy - last_dy) * kPredictionFactor;

        float move_x = pdx * dyn_speed;
        float move_y = pdy * dyn_speed;

        /* -------- تعويض سرعة السحب (بالعامل المتعلم) -------- */
        move_x *= vel_factor;
        move_y *= vel_factor;

        /* حد أقصى للحركة */
        float move_mag = std::sqrt(move_x * move_x + move_y * move_y);
        if (move_mag > kMaxSingleMove)
        {
            move_x = (move_x / move_mag) * kMaxSingleMove;
            move_y = (move_y / move_mag) * kMaxSingleMove;
        }

        /* ------------ حماية حد منطقة اللمس ------------ */
        float cx_touch = aim_touch_x.load(std::memory_order_relaxed);
        float cy_touch = aim_touch_y.load(std::memory_order_relaxed);
        float r_touch = aim_touch_radius.load(std::memory_order_relaxed);

        float new_tx = tx + move_x;
        float new_ty = ty + move_y;

        float dx_t = new_tx - cx_touch;
        float dy_t = new_ty - cy_touch;
        float dist2_t = dx_t * dx_t + dy_t * dy_t;
        constexpr float kGuard = 2.f;

        if (dist2_t >= (r_touch - kGuard) * (r_touch - kGuard))
        {
            if (isDown)
            {
                TouchInput::sendTouchUp();
                isDown = false;
            }
            tx = cx_touch;
            ty = cy_touch;
            TouchInput::sendTouchMove(tx, ty); // Down جديد
            isDown = true;

            new_tx = tx + move_x * 0.4f;
            new_ty = ty + move_y * 0.4f;
        }

        /* ------------- إرسال الحركة ------------- */
        if (std::fabs(new_tx - tx) > 0.1f || std::fabs(new_ty - ty) > 0.1f)
        {
            tx = new_tx;
            ty = new_ty;
            TouchInput::sendTouchMove(tx, ty);
        }

        /* ------------- Auto-Tuning للـ vel_factor ------------- */
        if (dist_before_move >= 0.f)
        {
            float reduction = dist_before_move - dist;                  // كم انخفضت المسافة فعليًّا
            float expected = dist_before_move * dyn_speed * vel_factor; // كم يفترض
            if (expected > 1.f)                                         // تجنّب القيم الصغيرة
            {
                float ratio_err = (reduction / expected) - 1.f; // >0 = مبالغة
                err_accum += ratio_err;
                if (++err_cnt >= kErrorWindow)
                {
                    float avg_err = err_accum / err_cnt;
                    /* اضبط العامل بعكس متوسط الخطأ */
                    vel_factor *= 1.f / (1.f + avg_err * kVelLearnRate);
                    vel_factor = std::clamp(vel_factor, 0.25f, 1.f);
                    err_accum = 0.f;
                    err_cnt = 0;
                }
            }
        }
        dist_before_move = dist; // خزّن للمقارنة في الإطار التالي

        /* تحديث متغيّرات التنبّؤ */
        last_dx = dx;
        last_dy = dy;
        last_target = target;

        if (!EncryptedBranding::verify_branding_integrity())
            exit(13);
    }

    /* تنظيف */
    if (isDown)
        TouchInput::sendTouchUp();
}

// void aimbot_thread()
// {
//     bool isDown = false;
//     float tx = aim_touch_x.load(std::memory_order_relaxed), ty = aim_touch_y.load(std::memory_order_relaxed);
//     float breakpoints[] = {1, 2, 3, 5, 10, 15, 20, 25, 30, 40, 50, 60, 70, 80, 90, 100, 150, 200};
//     float divisors[] = {0.09, 0.11, 0.12, 0.15, 0.25, 0.4, 0.5, 0.6, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0, 1.05, 1.25, 1.5};
//     float aimSpace = 0.f;

//     while (is_running.load( std::memory_order_relaxed))
//     {
//         Utils::control_frame_rate(frame_rate.load(std::memory_order_relaxed));

//         int read_idx = ready_buffer_index.load(std::memory_order_acquire);
//         const auto &game_data = game_buffers[read_idx];

//         android::ANativeWindowCreator::DisplayInfo display = android::ANativeWindowCreator::GetDisplayInfo();
//         if (display.width < display.height)
//             std::swap(display.width, display.height);
//         TouchInput::setDisplayInfo(display.width, display.height, display.orientation);
//         TouchInput::TouchRect aim_zone_fire = TouchInput::createZoneFromCenter(aim_zone_fire_x.load(std::memory_order_relaxed), aim_zone_fire_y.load(std::memory_order_relaxed), aim_zone_fire_radius.load(std::memory_order_relaxed));
//         aim_is_weapon_firing.store(TouchInput::isTouchInZone(aim_zone_fire));

//         if (!aim_is_aiming.load(std::memory_order_relaxed))
//         {
//             if (isDown)
//             {
//                 TouchInput::sendTouchUp();
//                 isDown = false;
//             }
//             continue;
//         }

//         Structs::FVector selected_target = {0.0f, 0.0f, 0.0f};
//         float best_world_distance = FLT_MAX;
//         float best_screen_distance = FLT_MAX;
//         bool close_enemy_found = false;

//         int cx = display.width / 2;
//         int cy = display.height / 2;

//         for (int i = 0; i < game_data.count_enemies; i++)
//         {
//             const Structs::Player &player = game_data.players[i];
//             if (!player.is_on_screen)
//                 continue;

//             float world_distance = player.distance;
//             float dx = player.target.X - cx;
//             float dy = player.target.Y - cy;
//             float screen_distance = sqrtf(dx * dx + dy * dy);

//             if (world_distance <= 50.0f && screen_distance <= aim_fov.load(std::memory_order_relaxed))
//             {
//                 close_enemy_found = true;
//                 if (world_distance < best_world_distance)
//                 {
//                     best_world_distance = world_distance;
//                     selected_target = player.target;
//                 }
//             }
//             else if (!close_enemy_found && screen_distance <= aim_fov.load(std::memory_order_relaxed))
//             {
//                 if (screen_distance < best_screen_distance)
//                 {
//                     best_screen_distance = screen_distance;
//                     selected_target = player.target;
//                 }
//             }
//         }

//         if (selected_target.X <= 0 && selected_target.Y <= 0)
//         {
//             if (isDown)
//             {
//                 TouchInput::sendTouchUp();
//                 isDown = false;
//             }
//             continue;
//         }

//         if (!aim_is_weapon_firing.load(std::memory_order_relaxed))
//         {
//             if (isDown)
//             {
//                 TouchInput::sendTouchUp();
//                 isDown = false;
//             }
//             continue;
//         }

//         float dx = selected_target.X - cx;
//         float dy = selected_target.Y - cy;
//         float distance = sqrtf(dx * dx + dy * dy);

//         if (!isDown)
//         {
//             tx = aim_touch_x.load(std::memory_order_relaxed);
//             ty = aim_touch_y.load(std::memory_order_relaxed);
//             TouchInput::sendTouchMove(tx, ty);
//             isDown = true;
//         }

//         for (int i = 0; i < sizeof(breakpoints) / sizeof(float); i++)
//         {
//             if (distance < breakpoints[i])
//             {
//                 aimSpace = (divisors[i] != 0.0f) ? aim_touch_radius.load(std::memory_order_relaxed) / divisors[i] : aim_touch_radius.load(std::memory_order_relaxed);
//                 break;
//             }
//         }

//         if (distance >= breakpoints[sizeof(breakpoints) / sizeof(float) - 1])
//         {
//             aimSpace = aim_touch_radius.load(std::memory_order_relaxed) / 1.55;
//         }

//         float targetX = (dx > 0 ? 1 : -1) * fabs(dx) / aimSpace;
//         float targetY = (dy > 0 ? 1 : -1) * fabs(dy) / aimSpace;

//         if (fabs(targetX) >= 35 || fabs(targetY) >= 35)
//         {
//             if (isDown)
//             {
//                 TouchInput::sendTouchUp();
//                 isDown = false;
//             }
//             usleep(aim_shooting.load(std::memory_order_relaxed) * 1000);
//             continue;
//         }

//         tx += targetX;
//         ty += targetY;

//         if (tx >= aim_touch_x.load(std::memory_order_relaxed) + aim_touch_radius.load(std::memory_order_relaxed) || tx <= aim_touch_x.load(std::memory_order_relaxed) - aim_touch_radius.load(std::memory_order_relaxed) ||
//             ty >= aim_touch_y.load(std::memory_order_relaxed) + aim_touch_radius.load(std::memory_order_relaxed) || ty <= aim_touch_y.load(std::memory_order_relaxed) - aim_touch_radius.load(std::memory_order_relaxed))
//         {
//             TouchInput::sendTouchUp();
//             usleep(aim_shooting.load(std::memory_order_relaxed) / 3 * 1000);

//             tx = aim_touch_x.load(std::memory_order_relaxed);
//             ty = aim_touch_y.load(std::memory_order_relaxed);
//             TouchInput::sendTouchMove(tx, ty);
//         }
//         else if (isDown)
//         {
//             TouchInput::sendTouchMove(tx, ty);
//         }

//         usleep(aim_shooting.load(std::memory_order_relaxed) * 1000);

//         if (!EncryptedBranding::verify_branding_integrity())
//         {
//             exit(13);
//         }
//     }

//     if (isDown)
//     {
//         TouchInput::sendTouchUp();
//     }
// }

int main(int argc, char *argv[])
{

    if (!EncryptedBranding::verify_branding_integrity())
    {
        exit(13);
    }

    if (Utils::is_package_running("com.garena.game.df"))
        target_pid.store(Utils::find_pid_by_package_name("com.garena.game.df"));
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
    aim_is_aiming.store(true);
    aim_is_weapon_firing.store(false);

    TouchInput::touchInputStart();
    TouchInput::setDisplayInfo(display.width, display.height, display.orientation);

    aimbot_thread_handle = std::thread(aimbot_thread);
    socket_thread_handle = std::thread(socket_thread);

    float margin = 20.0f;

    float current_projectile_initial_velocity_rate = 0.0f;
    float current_fire_interval = 0.0f;
    float current_burst_interval = 0.0f;
    int current_burst_count = 0;
    while (is_running.load(std::memory_order_relaxed))
    {
        Utils::control_frame_rate(frame_rate.load(std::memory_order_relaxed));

        int write_idx = write_buffer_index.load(std::memory_order_relaxed);
        auto &current_buffer = game_buffers[write_idx];
        current_buffer.clear();

        uintptr_t u_world = Memory::Read<uintptr_t>(lib_base + Offset::g_world, target_pid);
        uintptr_t game_state = Memory::Read<uintptr_t>(u_world + Offset::game_state, target_pid);
        Structs::TArray player_array = Memory::Read<Structs::TArray>(game_state + Offset::player_array, target_pid);

        uintptr_t owning_game_instance = Memory::Read<uintptr_t>(u_world + Offset::owning_game_instance, target_pid);
        Structs::TArray local_players = Memory::Read<Structs::TArray>(owning_game_instance + Offset::local_players, target_pid);
        uintptr_t local_player = Memory::Read<uintptr_t>(local_players.data, target_pid);
        uintptr_t player_controller = Memory::Read<uintptr_t>(local_player + Offset::player_controller, target_pid);
        uintptr_t acknowledged_pawn = Memory::Read<uintptr_t>(player_controller + Offset::acknowledged_pawn, target_pid);
        uintptr_t health_comp_local = Memory::Read<uintptr_t>(acknowledged_pawn + Offset::health_comp, target_pid);
        uintptr_t team_comp_local = Memory::Read<uintptr_t>(acknowledged_pawn + Offset::team_comp, target_pid);
        int32_t team_id_local = Memory::Read<int32_t>(team_comp_local + Offset::team_id, target_pid);
        int32_t camp_id_local = Memory::Read<int32_t>(team_comp_local + Offset::camp_id, target_pid);

        uintptr_t player_camera_manager = Memory::Read<uintptr_t>(player_controller + Offset::player_camera_manager, target_pid);
        Structs::CameraCacheEntry camera_cahce = Memory::Read<Structs::CameraCacheEntry>(player_camera_manager + Offset::camera_cache, target_pid);
        Structs::MinimalViewInfo minimal_view_info = camera_cahce.POV;

        for (size_t i = 0; i < player_array.count; i++)
        {
            uintptr_t player_state = Memory::Read<uintptr_t>(player_array.data + i * sizeof(uintptr_t), target_pid);
            if (!player_state)
                continue;
            if (i + 1 < player_array.count)
            {
                __builtin_prefetch((void *)(player_array.data + (i + 1) * sizeof(uintptr_t)), 0, 1);
            }
            uintptr_t pawn = Memory::Read<uintptr_t>(player_state + Offset::pawn, target_pid);
            if (!pawn)
                continue;

            if (pawn == acknowledged_pawn)
            {
                uintptr_t cache_cur_weapon = Memory::Read<uintptr_t>(pawn + Offset::cache_cur_weapon, target_pid);
                uint32_t weapon_equip_position = Memory::Read<uint32_t>(cache_cur_weapon + Offset::weapon_equip_position, target_pid);
                uintptr_t cached_attribute_set_fire_mode = Memory::Read<uintptr_t>(cache_cur_weapon + Offset::cached_attribute_set_fire_mode, target_pid);

                Structs::FGameplayAttributeData fire_interval = Memory::Read<Structs::FGameplayAttributeData>(cached_attribute_set_fire_mode + Offset::fire_interval, target_pid);
                current_fire_interval = fire_interval.CurrentValue;

                Structs::FGameplayAttributeData projectile_initial_velocity_rate = Memory::Read<Structs::FGameplayAttributeData>(cached_attribute_set_fire_mode + Offset::projectile_initial_velocity_rate, target_pid);
                current_projectile_initial_velocity_rate = projectile_initial_velocity_rate.CurrentValue;

                Structs::FGameplayAttributeData burst_interval = Memory::Read<Structs::FGameplayAttributeData>(cached_attribute_set_fire_mode + Offset::burst_interval, target_pid);
                current_burst_interval = burst_interval.CurrentValue;

                Structs::FGameplayAttributeData burst_fire_bullet_count = Memory::Read<Structs::FGameplayAttributeData>(cached_attribute_set_fire_mode + Offset::burst_fire_bullet_count, target_pid);
                current_burst_count = (int)burst_fire_bullet_count.CurrentValue;

                continue;
            }

            uintptr_t root_component = Memory::Read<uintptr_t>(pawn + Offset::root_component, target_pid);
            if (!root_component)
                continue;

            Structs::FTransform transform = Memory::Read<Structs::FTransform>(root_component + Offset::component_to_world, target_pid);
            float distance = (Structs::FVector::Distance(minimal_view_info.Location, transform.Translation) / 100.0f);

            Structs::FVector screen_pos = Ue4::world_to_screen(transform.Translation, minimal_view_info, display.width, display.height);
            bool is_on_screen = !(screen_pos.X < margin || screen_pos.Y < margin || screen_pos.X > display.width - margin || screen_pos.Y > display.height - margin || screen_pos.Z < 0.0f);

            uintptr_t health_comp = Memory::Read<uintptr_t>(pawn + Offset::health_comp, target_pid);
            uintptr_t health_set = Memory::Read<uintptr_t>(health_comp + Offset::health_set, target_pid);
            Structs::FGPGameplayAttributeData health = Memory::Read<Structs::FGPGameplayAttributeData>(health_set + Offset::health, target_pid);
            float current_health = health.CurrentValue;
            if (current_health <= 0)
                continue;

            uintptr_t team_comp = Memory::Read<uintptr_t>(pawn + Offset::team_comp, target_pid);
            int32_t team_id = Memory::Read<int32_t>(team_comp + Offset::team_id, target_pid);
            int32_t camp_id = Memory::Read<int32_t>(team_comp + Offset::camp_id, target_pid);
            if (team_id == team_id_local || camp_id == camp_id_local)
                continue;

            uintptr_t capsule_component = Memory::Read<uintptr_t>(pawn + Offset::capsule_component, target_pid);
            if (!capsule_component)
                continue;
            float half_height = Memory::Read<float>(capsule_component + Offset::capsule_half_height, target_pid);
            float radius = Memory::Read<float>(capsule_component + Offset::capsule_radius, target_pid);

            Structs::Player player_obj;

            Structs::FVector center = transform.Translation;
            center.Z -= half_height;

            Structs::FVector corners[8] = {
                {-radius, -radius, 0},
                {radius, -radius, 0},
                {radius, radius, 0},
                {-radius, radius, 0},
                {-radius, -radius, half_height * 2},
                {radius, -radius, half_height * 2},
                {radius, radius, half_height * 2},
                {-radius, radius, half_height * 2},
            };

            float avg_top_x = (corners[4].X + corners[5].X + corners[6].X + corners[7].X) / 4.0f;
            float avg_top_y = (corners[4].Y + corners[5].Y + corners[6].Y + corners[7].Y) / 4.0f;
            float avg_top_z = (corners[4].Z + corners[5].Z + corners[6].Z + corners[7].Z) / 4.0f;
            Structs::FVector head_position = center + Structs::FVector(avg_top_x, avg_top_y, avg_top_z);

            float avg_bottom_x = (corners[0].X + corners[1].X + corners[2].X + corners[3].X) / 4.0f;
            float avg_bottom_y = (corners[0].Y + corners[1].Y + corners[2].Y + corners[3].Y) / 4.0f;
            float avg_bottom_z = (corners[0].Z + corners[1].Z + corners[2].Z + corners[3].Z) / 4.0f;
            Structs::FVector root_position = center + Structs::FVector(avg_bottom_x, avg_bottom_y, avg_bottom_z);

            uintptr_t character_movement = Memory::Read<uintptr_t>(pawn + Offset::character_movement, target_pid);
            Structs::FVector last_update_velocity = Memory::Read<Structs::FVector>(character_movement + Offset::last_update_velocity, target_pid);
            float current_last_update_velocity = last_update_velocity.Length();
            float gravity_scale = Memory::Read<float>(character_movement + Offset::gravity_scale, target_pid);
            const float world_gravity = 980.f;
            float gravity = world_gravity * gravity_scale;
            float bullet_speed = current_projectile_initial_velocity_rate;
            if (bullet_speed < 1.0f)
            {
                bullet_speed = 7000.0f;
            }

            Structs::FVector fire_start = minimal_view_info.Location;
            Structs::FVector center_mass = root_position + (head_position - root_position) * 0.7f;
            Structs::FVector aim_dir = (center_mass - fire_start).GetSafeNormal();
            float distance_to_target = Structs::FVector::Distance(fire_start, center_mass);
            float travel_time = distance_to_target / bullet_speed;
            Structs::FVector predicted_location = center_mass;

            if (current_last_update_velocity > 100.0f && travel_time < 2.0f)
            {
                // عامل الحركة: كل ما زادت سرعة العدو أو الوقت، زادت الإزاحة
                float movement_factor = std::clamp(travel_time / std::max(0.05f, current_fire_interval), 0.f, 0.5f);

                // نحرك موقع العدو بناءً على اتجاه حركته وسرعته
                predicted_location = predicted_location + (last_update_velocity * travel_time * movement_factor);

                // نحسب السقوط بسبب الجاذبية
                // float gravity_drop = std::clamp(0.5f * gravity * travel_time * travel_time, 0.f, 500.f);

                // نطبق السقوط على محور Z
                // predicted_location.Z -= gravity_drop;
            }

            if (predicted_location.Z < fire_start.Z - 10000.f ||
                predicted_location.Z > fire_start.Z + 10000.f)
            {
                predicted_location = center_mass; // نرجع للتصويب على الرأس
            }

            // float correction_factor = std::clamp(distance_to_target * 0.02f, 0.0f, 40.0f);
            // predicted_location.Z -= correction_factor;

            for (int i = 0; i < 8; ++i)
            {
                Structs::FVector local = corners[i];
                local.Z -= half_height;
                Structs::FVector world = transform.TransformPositionNoScale(local);
                Structs::FVector screen = Ue4::world_to_screen(world, minimal_view_info, display.width, display.height);
                player_obj.bounds[i] = screen;
            }

            player_obj.position = transform.Translation;
            player_obj.target = Ue4::world_to_screen(predicted_location, minimal_view_info, display.width, display.height);
            player_obj.location = screen_pos;
            player_obj.head = Ue4::world_to_screen(head_position, minimal_view_info, display.width, display.height);
            player_obj.root = Ue4::world_to_screen(root_position, minimal_view_info, display.width, display.height);
            player_obj.distance = distance;
            player_obj.health = current_health;
            player_obj.team_id = team_id;
            player_obj.camp_id = camp_id;
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
    if (socket_thread_handle.joinable())
        socket_thread_handle.join();
    TouchInput::touchInputStop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return 0;
}