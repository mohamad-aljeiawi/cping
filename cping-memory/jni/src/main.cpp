#include "main.h"

std::unordered_map<int, std::string> g_class_name_cache;
uintptr_t lib_base = 0;
pid_t target_pid;
std::atomic<bool> is_running{true};
std::atomic<int> write_buffer_index{0};
std::atomic<bool> data_ready{false};
std::atomic<bool> buffer_swap_pending{false};
std::atomic<uintptr_t> g_world{0};
std::atomic<uintptr_t> g_names{0};
GameObjects::FrameData frame_buffers[2];
std::thread scanner_thread_handle;
float app_fps = 120.0f;

ImColor vehicle_color = IM_COL32(180, 180, 170, 255);
ImColor player_color = IM_COL32(255, 0, 0, 255);
ImColor loot_color = IM_COL32(0, 179, 27, 255);
ImColor airdrop_color = IM_COL32(0, 179, 116, 255);
ImColor bot_color = IM_COL32(220, 220, 220, 255);
ImColor not_alive_color = IM_COL32(119, 0, 179, 255);
ImColor text_color_player = IM_COL32(179, 173, 0, 255);

std::thread aimbot_thread_handle;
std::atomic<bool> control_is_aiming{true};
std::atomic<bool> aim_bis_weapon_firing{false};
static float aim_slide_x = 1665.1f;
static float aim_slide_y = 475.0f;

static float aim_fov = 400.0f;
static float aim_interval = 1.0f;
static float aim_speed = 50.0f;
static float aim_rang_touch = 100.0f;

void aimbot_thread()
{
    bool isDown = false;
    float tx = aim_slide_x, ty = aim_slide_y;
    float breakpoints[] = {1, 2, 3, 5, 10, 15, 20, 25, 30, 40, 50, 60, 70, 80, 90, 100, 150, 200};
    float divisors[] = {0.09, 0.11, 0.12, 0.15, 0.25, 0.4, 0.5, 0.6, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0, 1.05, 1.25, 1.5};
    float aimSpace = 0.f;

    while (is_running.load())
    {
        Utils::control_frame_rate(app_fps);

        // التحقق من حالة التصويب
        if (!control_is_aiming.load())
        {
            // إذا توقف التصويب، ارفع اللمس
            if (isDown)
            {
                TouchInput::sendTouchUp();
                isDown = false;
                // printf("AIM STOPPED - TOUCH UP\n");
            }
            buffer_swap_pending.store(false, std::memory_order_release);
            continue;
        }

        buffer_swap_pending.store(true, std::memory_order_release);
        int read_idx = 1 - write_buffer_index.load(std::memory_order_acquire);
        const auto &frame_data = frame_buffers[read_idx];

        Structs::FVector selected_target = {0.0f, 0.0f, 0.0f};
        float best_world_distance = FLT_MAX;
        float best_screen_distance = FLT_MAX;
        bool close_enemy_found = false;

        int cx = frame_data.display_width / 2;
        int cy = frame_data.display_height / 2;

        // البحث عن الهدف
        for (const auto &player : frame_data.players)
        {
            if (!player.is_on_screen || !player.is_alive)
                continue;

            float world_distance = player.distance;
            float screen_distance = sqrt(
                powf(player.target.X - cx, 2) +
                powf(player.target.Y - cy, 2));

            if (world_distance <= 50.0f && screen_distance <= aim_fov)
            {
                close_enemy_found = true;
                if (world_distance < best_world_distance)
                {
                    best_world_distance = world_distance;
                    selected_target = player.target;
                }
            }
            else if (!close_enemy_found && screen_distance <= aim_fov)
            {
                if (screen_distance < best_screen_distance)
                {
                    best_screen_distance = screen_distance;
                    selected_target = player.target;
                }
            }
        }

        // إذا لم يتم العثور على هدف
        if (selected_target.X <= 0 && selected_target.Y <= 0)
        {
            if (isDown)
            {
                TouchInput::sendTouchUp();
                isDown = false;
                // printf("NO TARGET - TOUCH UP\n");
            }
            buffer_swap_pending.store(false, std::memory_order_release);
            continue;
        }

        // printf("selected_target: %f, %f\n", selected_target.X, selected_target.Y);

        // التحقق من حالة إطلاق النار
        // if (!aim_bis_weapon_firing.load())
        // {
        //     if (isDown)
        //     {
        //         TouchInput::sendTouchUp();
        //         isDown = false;
        //         printf("NOT FIRING - TOUCH UP\n");
        //     }
        //     buffer_swap_pending.store(false, std::memory_order_release);
        //     continue;
        // }

        float dx = selected_target.X - cx;
        float dy = selected_target.Y - cy;
        float distance = sqrtf(dx * dx + dy * dy);
        // printf("distance: %f\n", distance);

        // بدء اللمس إذا لم يكن نشطاً
        if (!isDown)
        {
            tx = aim_slide_x;
            ty = aim_slide_y;
            TouchInput::sendTouchMove(tx, ty);
            isDown = true;
            // printf("TOUCH DOWN at tx: %f, ty: %f\n", tx, ty);
        }

        // حساب سرعة التصويب
        for (int i = 0; i < sizeof(breakpoints) / sizeof(float); i++)
        {
            if (distance < breakpoints[i])
            {
                aimSpace = aim_speed / divisors[i];
                // printf("aimSpace: %f\n", aimSpace);
                break;
            }
        }

        if (distance >= breakpoints[sizeof(breakpoints) / sizeof(float) - 1])
        {
            aimSpace = aim_speed / 1.55;
        }

        float targetX = (dx > 0 ? 1 : -1) * fabs(dx) / aimSpace;
        float targetY = (dy > 0 ? 1 : -1) * fabs(dy) / aimSpace;
        // printf("targetX: %f, targetY: %f\n", targetX, targetY);

        // التحقق من القيم الحدية
        if (fabs(targetX) >= 35 || fabs(targetY) >= 35)
        {
            if (isDown)
            {
                TouchInput::sendTouchUp();
                isDown = false;
                // printf("LARGE MOVEMENT - TOUCH UP\n");
            }
            usleep(aim_interval * 1000);
            buffer_swap_pending.store(false, std::memory_order_release);
            continue;
        }

        // تحديث الموقع
        tx += targetX;
        ty += targetY;

        // التحقق من الحدود
        if (tx >= aim_slide_x + aim_rang_touch || tx <= aim_slide_x - aim_rang_touch ||
            ty >= aim_slide_y + aim_rang_touch || ty <= aim_slide_y - aim_rang_touch)
        {
            TouchInput::sendTouchUp();
            // printf("OUT OF BOUNDS - TOUCH UP\n");
            usleep(aim_interval / 3 * 1000);

            tx = aim_slide_x;
            ty = aim_slide_y;
            TouchInput::sendTouchMove(tx, ty);
            // printf("RESET POSITION - TOUCH DOWN\n");
            // لا تغير isDown لأننا أعدنا تعيين الموقع
        }
        else if (isDown)
        {
            // استمرار الحركة
            TouchInput::sendTouchMove(tx, ty);
            // printf("CONTINUOUS MOVE to tx: %f, ty: %f\n", tx, ty);
        }

        usleep(aim_interval * 1000);
        buffer_swap_pending.store(false, std::memory_order_release);
    }

    // تنظيف عند انتهاء الخيط
    if (isDown)
    {
        TouchInput::sendTouchUp();
        // printf("THREAD EXIT - TOUCH UP\n");
    }
}

void scanner_thread()
{
    const int margin_screen = 20;
    frame_buffers[0].reserve_capacity();
    frame_buffers[1].reserve_capacity();
    uintptr_t gname_buff[100] = {0};
    uintptr_t local_g_world = g_world.load(std::memory_order_acquire);
    uintptr_t local_g_names = g_names.load(std::memory_order_acquire);
    if (!local_g_world || !local_g_names)
    {
        return;
    }

    while (is_running.load(std::memory_order_relaxed))
    {
        Utils::control_frame_rate(app_fps * 1.5f);

        int write_idx = write_buffer_index.load(std::memory_order_relaxed);
        auto &current_buffer = frame_buffers[write_idx];
        current_buffer.clear();

        uintptr_t u_world = Memory::Read<uintptr_t>(Memory::Read<uintptr_t>(local_g_world + 0x58, target_pid) + 0x78, target_pid);
        uintptr_t u_level = Memory::Read<uintptr_t>(u_world + Offset::persistent_level, target_pid);
        uintptr_t actors_list = Ue4::get_actors_array(u_level, Offset::u_level_to_a_actors, 0x448, target_pid);
        uintptr_t u_level_to_a_actors = Memory::Read<uintptr_t>(actors_list, target_pid);
        int u_level_to_a_actors_count = Memory::Read<int>(actors_list + sizeof(uintptr_t), target_pid);
        uintptr_t player_controller = Memory::Read<uintptr_t>(Memory::Read<uintptr_t>(Memory::Read<uintptr_t>(u_world + 0x38, target_pid) + 0x78, target_pid) + 0x30, target_pid);
        uintptr_t player_camera_manager = Memory::Read<uintptr_t>(player_controller + Offset::player_camera_manager, target_pid);

        auto display = android::ANativeWindowCreator::GetDisplayInfo();
        if (display.width < display.height)
            std::swap(display.width, display.height);
        current_buffer.display_width = display.width;
        current_buffer.display_height = display.height;
        current_buffer.display_orientation = display.orientation;

        Structs::CameraCacheEntry camera_cache = Memory::Read<Structs::CameraCacheEntry>(player_camera_manager + Offset::camera_cache, target_pid);
        current_buffer.camera_view = camera_cache.POV;

        Memory::ReadArray(local_g_names, gname_buff, target_pid);

        int local_team_id = Memory::Read<int>(player_controller + Offset::local_team_id, target_pid);
        float bullet_speed = 0.0f;
        for (int i = 0; i < u_level_to_a_actors_count && is_running.load(std::memory_order_relaxed); i++)
        {
            uintptr_t actor = Memory::Read<uintptr_t>(u_level_to_a_actors + i * sizeof(uintptr_t), target_pid);
            if (!actor)
                continue;

            if (i + 1 < u_level_to_a_actors_count)
            {
                __builtin_prefetch((void *)(u_level_to_a_actors + (i + 1) * sizeof(uintptr_t)), 0, 1);
            }

            std::string class_name = Ue4::get_cached_class_name(local_g_names, actor, gname_buff, g_class_name_cache, target_pid);
            if (class_name == "none" || class_name.empty())
                continue;

            if (strstr(class_name.c_str(), "VH_PG117_C") || strstr(class_name.c_str(), "BP_CharacterRifle") ||
                strstr(class_name.c_str(), "BP_PlayerRifle") || strstr(class_name.c_str(), "BP_PlayerLobby_Pawn_C") ||
                strstr(class_name.c_str(), "BP_PlayerController") || strstr(class_name.c_str(), "BP_PlayerCameraManager_C"))
                continue;

            uintptr_t actor_root_component = Memory::Read<uintptr_t>(actor + Offset::root_component, target_pid);
            if (!actor_root_component)
                continue;

            Structs::FTransform transform = Memory::Read<Structs::FTransform>(actor_root_component + Offset::component_to_world, target_pid);
            float distance = (Structs::FVector::Distance(current_buffer.camera_view.Location, transform.Translation) / 100.0f);
            Structs::FVector location = Ue4::world_to_screen(transform.Translation, current_buffer.camera_view, display.width, display.height);

            // Process Players
            if (strstr(class_name.c_str(), "BP_Character") || strstr(class_name.c_str(), "BP_Player"))
            {
                if (distance < 1.0f || distance > 400.0f)
                    continue;

                int player_death = Memory::Read<int>(actor + Offset::bis_dead, target_pid);
                if (player_death)
                    continue;

                int current_states = Memory::Read<int>(actor + Offset::current_states, target_pid);
                if (current_states == 262144 || current_states == 6)
                    continue;

                float health = Memory::Read<float>(actor + Offset::health, target_pid);
                if (health < 0)
                    continue;

                // Class: STExtraBaseCharacter.STExtraCharacter.UAECharacter.Character.Pawn.Actor.Object
                // // CharacterWeaponManagerComponent* WeaponManagerComponent;//[Offset: 0x2488, Size: 0x8]
                uintptr_t weapon_manager = Memory::Read<uintptr_t>(actor + 0x2488, target_pid);
                // Class: WeaponManagerComponent.ActorComponent.Object
                // // STExtraWeapon* CurrentWeaponReplicated;//[Offset: 0x558, Size: 0x8]
                uintptr_t current_weapon = Memory::Read<uintptr_t>(weapon_manager + 0x558, target_pid);
                // Class: STExtraWeapon.LuaActor.Actor.Object
                // // WeaponEntity* WeaponEntityComp;//[Offset: 0x770, Size: 0x8]
                uintptr_t weapon_entity = Memory::Read<uintptr_t>(current_weapon + 0x770, target_pid);
                // Class: WeaponEntity.WeaponLogicBaseComponent.ActorComponent.Object
                // // int WeaponId;//[Offset: 0x178, Size: 0x4]
                int weapon_id = Memory::Read<int>(weapon_entity + 0x178, target_pid);

                int team_id = Memory::Read<int>(actor + Offset::team_id, target_pid);
                if (local_team_id == team_id || team_id <= -1 || team_id >= 1000)
                {
                    printf("current_weapon: %p\n", current_weapon);
                    if (weapon_id <= 0)
                    {
                        bullet_speed = 0.0f;
                        continue;
                    }
                    // Class: ShootWeaponEntity.WeaponEntity.WeaponLogicBaseComponent.ActorComponent.Object
                    // // float BulletFireSpeed;//[Offset: 0x4c0, Size: 0x4]
                    bullet_speed = Memory::Read<float>(weapon_entity + 0x4c0, target_pid);
                    // printf("bullet_speed: %f\n", bullet_speed);
                    continue;
                }

                GameObjects::Player player_obj;
                bool has_valid_bounds = Ue4::process_object_bounds(actor, {0x498}, {0}, {0x9D4}, {0.45f, 0.45f, 0.65f}, 0.0f, current_buffer.camera_view, display.width, display.height, &player_obj.bounds[0], target_pid);
                if (!has_valid_bounds)
                    continue;

                Structs::FTransform transform_mech = Ue4::get_component_to_world(actor, target_pid);
                Structs::FTransform bone_transform = Ue4::get_bone_transform(actor, 6, target_pid);
                Structs::FVector bone_head = transform_mech.TransformPosition(bone_transform.Translation);
                Structs::FVector head_location = Ue4::world_to_screen(bone_head, current_buffer.camera_view, display.width, display.height);
                bone_transform = Ue4::get_bone_transform(actor, 0, target_pid);
                Structs::FVector bone_root = transform_mech.TransformPosition(bone_transform.Translation);
                Structs::FVector root_location = Ue4::world_to_screen(bone_root, current_buffer.camera_view, display.width, display.height);

                if (distance <= 65 && distance >= 1.0f)
                {
                    std::vector<int> skeleton_bones;
                    if (strstr(class_name.c_str(), "BP_Character"))
                    {
                        skeleton_bones = {
                            1, // pelvis index[0]
                            4, // spine_2 index[1]
                            5, // neck_base index[2]

                            14, // right_upper_arm index[3]
                            15, // right_lower_arm index[4]
                            16, // right_hand index[5]

                            35, // left_upper_arm index[6]
                            36, // left_lower_arm index[7]
                            37, // left_hand index[8]

                            56, // right_thigh index[9]
                            57, // right_foot index[10]

                            60, // left_thigh index[11]
                            61  // left_foot index[12]
                        };
                    }
                    else
                    {
                        skeleton_bones = {
                            1, // pelvis index[0]
                            4, // spine_2 index[1]
                            5, // neck_base index[2]

                            12, // right_upper_arm index[3]
                            13, // right_lower_arm index[4]
                            14, // right_hand index[5]

                            33, // left_upper_arm index[6]
                            34, // left_lower_arm index[7]
                            35, // left_hand index[8]

                            54, // right_thigh index[9]
                            55, // right_foot index[10]

                            58, // left_thigh index[11]
                            59  // left_foot index[12]
                        };
                    }
                    for (int i = 0; i < skeleton_bones.size(); i++)
                    {
                        bone_transform = Ue4::get_bone_transform(actor, skeleton_bones[i], target_pid);
                        Structs::FVector bone_world = transform_mech.TransformPosition(bone_transform.Translation);
                        Structs::FVector bone_screen = Ue4::world_to_screen(bone_world, current_buffer.camera_view, display.width, display.height);
                        player_obj.bones[i] = bone_screen;
                    }
                }

                player_obj.position = transform.Translation;
                player_obj.location = location;
                player_obj.head = head_location;
                player_obj.root = root_location;
                player_obj.name = Memory::ReadFString(actor + Offset::player_name, target_pid);
                player_obj.state = player_states[current_states];
                player_obj.health = health;
                player_obj.distance = distance;
                player_obj.team_id = team_id;
                player_obj.weapon_id = weapon_id;
                player_obj.is_bot = Memory::Read<bool>(actor + Offset::bis_ai, target_pid);
                player_obj.is_alive = health > 0.0f;
                player_obj.is_on_screen = !(location.X < margin_screen || location.Y < margin_screen || location.X > display.width - margin_screen || location.Y > display.height - margin_screen || location.Z < 0.0f);

                // Class: Character.Pawn.Actor.Object
                // // CharacterMovementComponent* CharacterMovement;//[Offset: 0x4a0, Size: 0x8]
                uintptr_t character_movement_component = Memory::Read<uintptr_t>(actor + 0x4a0, target_pid);
                // Class: MovementComponent.ActorComponent.Object
                // // Vector Velocity;//[Offset: 0x12c, Size: 0xc]
                Structs::FVector velocity = Memory::Read<Structs::FVector>(character_movement_component + 0x12c, target_pid);
                // Class: CharacterMovementComponent.PawnMovementComponent.NavMovementComponent.MovementComponent.ActorComponent.Object
                // // float GravityScale;//[Offset: 0x1b4, Size: 0x4]
                float gravity_scale = Memory::Read<float>(character_movement_component + 0x1b4, target_pid);
                float gravity_acceleration = 9.81f * gravity_scale;
                float distance_head_enemy = (Structs::FVector::Distance(current_buffer.camera_view.Location, bone_head));
                float travel_time = distance_head_enemy / bullet_speed;
                Structs::FVector drop = {0.0f, 0.0f, -0.5f * gravity_acceleration * travel_time * travel_time};
                Structs::FVector lead_target_position = bone_head + (velocity * travel_time) + drop;
                Structs::FVector lead_target_location = Ue4::world_to_screen(lead_target_position, current_buffer.camera_view, display.width, display.height);
                player_obj.target = lead_target_location;

                // printf("distance_head_enemy: %f, travel_time: %f, bullet_speed: %f, direction: %f, %f, %f, lead_target_location: %f, %f\n", distance_head_enemy, travel_time, bullet_speed, direction.X, direction.Y, direction.Z, lead_target_location.X, lead_target_location.Y);
                current_buffer.players.push_back(std::move(player_obj));
                current_buffer.count_enemies++;
            }

            // Process Vehicles
            else if (strstr(class_name.c_str(), "VH_") || strstr(class_name.c_str(), "Mirado_") ||
                     strstr(class_name.c_str(), "CoupeRB_") || strstr(class_name.c_str(), "Rony_") ||
                     strstr(class_name.c_str(), "PickUp_") || strstr(class_name.c_str(), "AquaRail_") ||
                     strstr(class_name.c_str(), "BP_Bike_") || strstr(class_name.c_str(), "BP_Bike2_") ||
                     strstr(class_name.c_str(), "wing_Vehicle_"))
            {
                GameObjects::Vehicle vehicle_obj;
                bool has_valid_bounds = Ue4::process_object_bounds(actor, {0xb18}, {0x1268}, {0xac}, {0.45f, 0.45f, 0.65f}, 0.5f, current_buffer.camera_view, display.width, display.height, &vehicle_obj.bounds[0], target_pid);
                if (!has_valid_bounds)
                    continue;

                vehicle_obj.position = transform.Translation;
                vehicle_obj.location = location;
                vehicle_obj.distance = distance;
                vehicle_obj.name = class_name;
                vehicle_obj.is_on_screen = !(location.X < margin_screen || location.Y < margin_screen || location.X > display.width - margin_screen || location.Y > display.height - margin_screen || location.Z < 0.0f);
                current_buffer.vehicles.push_back(std::move(vehicle_obj));
            }
            // Process Loot
            else if (strstr(class_name.c_str(), "Pickup_C") || strstr(class_name.c_str(), "PickUp") ||
                     strstr(class_name.c_str(), "BP_Ammo") || strstr(class_name.c_str(), "BP_QK") ||
                     strstr(class_name.c_str(), "Wrapper"))
            {
                GameObjects::LootItem loot_obj;
                bool has_valid_bounds = Ue4::process_object_bounds(actor, {0x8a0, 0x8a8, 0x8b0, 0x8b8, 0x8c0}, {0x878, 0x878, 0x878, 0x878, 0x878}, {0x170, 0x170, 0x170, 0x170, 0x170}, {1.0f, 1.0f, 1.0f}, 0.0f, current_buffer.camera_view, display.width, display.height, &loot_obj.bounds[0], target_pid);
                if (!has_valid_bounds)
                    continue;

                loot_obj.position = transform.Translation;
                loot_obj.location = location;
                loot_obj.distance = distance;
                loot_obj.name = class_name;
                loot_obj.type = GameObjects::LOOT;
                loot_obj.is_on_screen = !(location.X < margin_screen || location.Y < margin_screen || location.X > display.width - margin_screen || location.Y > display.height - margin_screen || location.Z < 0.0f);
                current_buffer.loot_items.push_back(std::move(loot_obj));
            }
            // Process Airdrop
            else if (strstr(class_name.c_str(), "BP_AirDropBox") || strstr(class_name.c_str(), "AirDropPlane"))
            {
                GameObjects::LootItem airdrop_obj;
                bool has_valid_bounds = Ue4::process_object_bounds(actor, {0x810}, {0x878}, {0x170}, {1.0f, 1.0f, 1.0f}, 0.0f, current_buffer.camera_view, display.width, display.height, &airdrop_obj.bounds[0], target_pid);
                if (!has_valid_bounds)
                    continue;

                airdrop_obj.position = transform.Translation;
                airdrop_obj.location = location;
                airdrop_obj.distance = distance;
                airdrop_obj.name = class_name;
                airdrop_obj.type = GameObjects::AIRDROP;
                airdrop_obj.is_on_screen = !(location.X < margin_screen || location.Y < margin_screen || location.X > display.width - margin_screen || location.Y > display.height - margin_screen || location.Z < 0.0f);
                current_buffer.loot_items.push_back(std::move(airdrop_obj));
            }
        }

        // Swap buffers atomically - wait for render thread to finish reading
        while (buffer_swap_pending.load(std::memory_order_acquire))
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        write_buffer_index.store(1 - write_idx, std::memory_order_release);
        data_ready.store(true, std::memory_order_release);
    }
}

// Rendering thread
void rendering_thread()
{
    Utils::control_frame_rate(app_fps);

    auto display = android::ANativeWindowCreator::GetDisplayInfo();
    if (display.width < display.height)
        std::swap(display.width, display.height);
    ImDrawList *draw_list = ImGui::GetBackgroundDrawList();

    if (!data_ready.load(std::memory_order_acquire))
    {
        if (!EncryptedBranding::verify_branding_integrity())
        {
            exit(13);
        }
        Utils::add_text_center(draw_list, EncryptedBranding::get_protected_branding(), 50.0f, ImVec2(display.width * 0.5f, display.height * 0.20f), IM_COL32(255, 0, 0, 255), true, 1.1f);
    }
    else
    {
        buffer_swap_pending.store(true, std::memory_order_release);
        int read_idx = 1 - write_buffer_index.load(std::memory_order_acquire);
        const auto &frame_data = frame_buffers[read_idx];

        // Render Aim Target
        draw_list->AddCircleFilled(ImVec2(aim_slide_x, aim_slide_y), aim_rang_touch, IM_COL32(255, 0, 0, 100));
        draw_list->AddCircle(ImVec2(display.width * 0.5f, display.height * 0.5f), aim_fov, IM_COL32(255, 0, 0, 100), 16, 4.0f);

        // Render Loot Items
        for (const auto &loot : frame_data.loot_items)
        {
            if (loot.is_on_screen)
            {
                ImU32 color = (loot.type == GameObjects::AIRDROP) ? airdrop_color : loot_color;
                float thickness = (loot.type == GameObjects::AIRDROP) ? 3.0f : 1.0f;

                for (int j = 0; j < 4; ++j)
                {
                    Structs::FVector p1 = loot.bounds[j];
                    Structs::FVector p2 = loot.bounds[(j + 1) % 4];
                    draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), color, thickness);
                }
                for (int j = 0; j < 4; ++j)
                {
                    Structs::FVector p1 = loot.bounds[j + 4];
                    Structs::FVector p2 = loot.bounds[((j + 1) % 4) + 4];
                    draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), color, thickness);
                }
                for (int j = 0; j < 4; ++j)
                {
                    Structs::FVector p1 = loot.bounds[j];
                    Structs::FVector p2 = loot.bounds[j + 4];
                    draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), color, thickness);
                }
            }
        }

        // Render Vehicles
        for (const auto &vehicle : frame_data.vehicles)
        {
            if (vehicle.is_on_screen)
            {
                for (int j = 0; j < 4; ++j)
                {
                    Structs::FVector p1 = vehicle.bounds[j];
                    Structs::FVector p2 = vehicle.bounds[(j + 1) % 4];
                    draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), vehicle_color, 3.0f);
                }
                for (int j = 0; j < 4; ++j)
                {
                    Structs::FVector p1 = vehicle.bounds[j + 4];
                    Structs::FVector p2 = vehicle.bounds[((j + 1) % 4) + 4];
                    draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), vehicle_color, 3.0f);
                }
                for (int j = 0; j < 4; ++j)
                {
                    Structs::FVector p1 = vehicle.bounds[j];
                    Structs::FVector p2 = vehicle.bounds[j + 4];
                    draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), vehicle_color, 3.0f);
                }
            }
        }

        // Render Players
        for (const auto &player : frame_data.players)
        {
            ImU32 color = (player.is_bot) ? bot_color : (player.is_alive) ? player_color
                                                                          : not_alive_color;
            ImU32 text_color = (player.is_bot) ? bot_color : (player.is_alive) ? text_color_player
                                                                               : not_alive_color;

            if (player.is_on_screen)
            {
                float text_scale_size = Utils::calculateTextSize(player.distance, 10.0f, 460.0f, 10.0f, 30.0f, 0.2f);

                if (player.distance <= 65 && player.distance >= 1.0f)
                {
                    const std::vector<std::pair<int, int>> skeleton_connections = {
                        {0, 1}, // pelvis [0] → spine_2 [1]
                        {1, 2}, // spine_2 [1] → neck_base [2]

                        {2, 3}, // neck_base [2] → right_upper_arm [3]
                        {3, 4}, // right_upper_arm [3] → right_lower_arm [4]
                        {4, 5}, // right_lower_arm [4] → right_hand [5]

                        {2, 6}, // neck_base [2] → left_upper_arm [6]
                        {6, 7}, // left_upper_arm [6] → left_lower_arm [7]
                        {7, 8}, // left_lower_arm [7] → left_hand [8]

                        {0, 9},  // pelvis [0] → right_thigh [9]
                        {9, 10}, // right_thigh [9] → right_foot [10]

                        {0, 11}, // pelvis [0] → left_thigh [11]
                        {11, 12} // left_thigh [11] → left_foot [12]
                    };
                    for (const auto &[from, to] : skeleton_connections)
                    {
                        const auto &pt1 = player.bones[from];
                        const auto &pt2 = player.bones[to];
                        draw_list->AddLine(ImVec2(pt1.X, pt1.Y), ImVec2(pt2.X, pt2.Y), color, 2.0f);
                    }
                }

                for (int j = 0; j < 4; ++j)
                {
                    Structs::FVector p1 = player.bounds[j];
                    Structs::FVector p2 = player.bounds[(j + 1) % 4];
                    draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), color, 4.0f);
                }
                for (int j = 0; j < 4; ++j)
                {
                    Structs::FVector p1 = player.bounds[j + 4];
                    Structs::FVector p2 = player.bounds[((j + 1) % 4) + 4];
                    draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), color, 4.0f);
                }
                for (int j = 0; j < 4; ++j)
                {
                    Structs::FVector p1 = player.bounds[j];
                    Structs::FVector p2 = player.bounds[j + 4];
                    draw_list->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y), color, 4.0f);
                }

                std::string display_text = "";
                if (!player.name.empty())
                    display_text += Utils::get_farsi_text(player.name) + " \n";
                if (player.is_bot)
                    display_text += "Bot \n";
                display_text += std::to_string((int)player.distance) + "m\n";
                if (!player.state.empty())
                    display_text += player.state + " \n";
                if (player.weapon_id > 0)
                    display_text += player_weapon[player.weapon_id] + " \n";

                Utils::add_text_center(draw_list, display_text, text_scale_size,
                                       ImVec2(player.root.X, player.root.Y),
                                       text_color, true, 0.95f);

                Utils::advanced_health_bar(draw_list, player.head.X, player.head.Y, display.width, display.height, player.health, 100.0f, color, IM_COL32(0, 0, 0, 100), player.distance, player.team_id);
            }
            else
            {
                Structs::OverlayInfo overlay = Ue4::compute_offscreen_enemy_overlay(player.position, frame_data.camera_view, display.width, display.height);

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

                // Draw shadows
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

                // Draw arrows
                ImGui::GetForegroundDrawList()->AddQuadFilled(body_top_left, body_top_right, body_bottom_right, body_bottom_left, color);
                ImGui::GetForegroundDrawList()->AddTriangleFilled(tip, head_left, head_right, color);
            }
        }

        // Show enemy count
        if (frame_data.count_enemies > 0)
        {
            std::string text = std::string("Enemies: " + std::to_string(frame_data.players.size()));
            Utils::add_text_center(draw_list, text, 60.0f, ImVec2(display.width * 0.5f, display.height * 0.12f), text_color_player, true, 1.1f);
        }
        else
        {
            Utils::add_text_center(draw_list, EncryptedBranding::get_protected_branding(), 60.0f, ImVec2(display.width * 0.5f, display.height * 0.12f), IM_COL32(255, 255, 255, 255), true, 1.1f);
        }

        buffer_swap_pending.store(false, std::memory_order_release);
    }
}

int main(int argc, char *argv[])
{
    if (!EncryptedBranding::verify_branding_integrity())
    {
        exit(13);
    }

    if (Utils::is_package_running("com.tencent.ig"))
        target_pid = Utils::find_pid_by_package_name("com.tencent.ig");
    else if (Utils::is_package_running("com.vng.pubgmobile"))
        target_pid = Utils::find_pid_by_package_name("com.vng.pubgmobile");
    else if (Utils::is_package_running("com.pubg.krmobile"))
        target_pid = Utils::find_pid_by_package_name("com.pubg.krmobile");
    else if (Utils::is_package_running("com.rekoo.pubgm"))
        target_pid = Utils::find_pid_by_package_name("com.rekoo.pubgm");
    else if (Utils::is_package_running("com.pubg.imobile"))
        target_pid = Utils::find_pid_by_package_name("com.pubg.imobile");
    else
    {
        printf("not open game or not support\n");
        exit(1);
        return 0;
    }

    lib_base = Utils::find_ue4_base(target_pid);
    if (!lib_base)
    {
        Logger::i("Failed to find libUE4.so base address!\n");
        return -1;
    }

    int count_try_get_g_objects = 0;
    while (count_try_get_g_objects < 10)
    {
        uintptr_t temp_g_world = Memory::Read<uintptr_t>(lib_base + Offset::g_world, target_pid);
        uintptr_t temp_g_names = Memory::Read<uintptr_t>(Memory::Read<uintptr_t>(lib_base + Offset::g_name, target_pid) + 0x110, target_pid);

        if (temp_g_world && temp_g_names)
        {
            g_world.store(temp_g_world, std::memory_order_release);
            g_names.store(temp_g_names, std::memory_order_release);
            break;
        }

        count_try_get_g_objects++;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    if (!g_world.load() || !g_names.load())
    {
        Logger::i("Failed to find g_world or g_names\n");
        return -1;
    }

    auto display = android::ANativeWindowCreator::GetDisplayInfo();
    if (display.width < display.height)
        std::swap(display.width, display.height);
    auto window = android::ANativeWindowCreator::Create("CPING", display.width, display.height);
    Renderer::Init(window, display.width, display.height);

    is_running.store(true);
    TouchInput::setDisplayInfo(display.width, display.height, display.orientation);
    TouchInput::touchInputStart();
    scanner_thread_handle = std::thread(scanner_thread);
    aimbot_thread_handle = std::thread(aimbot_thread);

    while (is_running.load(std::memory_order_relaxed))
    {
        Renderer::StartFrame();
        rendering_thread();
        Renderer::EndFrame();
    }

    is_running.store(false, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (scanner_thread_handle.joinable())
    {
        scanner_thread_handle.join();
    }
    if (aimbot_thread_handle.joinable())
    {
        aimbot_thread_handle.join();
    }
    TouchInput::touchInputStop();
    Renderer::Shutdown();

    return 0;
}