#include "main.h"
#include "branding.h"

#include <thread>
#include <mutex>
#include <chrono>
#include <cmath>
#include <atomic>
#include <unordered_map>
#include <cstring>
#include <vector>
#include <cmath>

FrameTimeController fps_controller = FrameTimeController(120);
std::unordered_map<int, std::string> g_class_name_cache;
uintptr_t lib_base = 0;
pid_t target_pid;
bool is_running = true;
bool branding_displayed = false;

int main(int argc, char *argv[])
{
    // Initial branding integrity check
    if (!verify_branding_integrity())
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
    uintptr_t g_world = 0;
    uintptr_t g_names = 0;
    while (count_try_get_g_objects < 10)
    {
        g_world = Memory::Read<uintptr_t>(lib_base + Offset::g_world, target_pid);
        g_names = Memory::Read<uintptr_t>(Memory::Read<uintptr_t>(lib_base + Offset::g_name, target_pid) + 0x110, target_pid);

        if (g_world && g_names)
            break;

        count_try_get_g_objects++;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    if (!g_world || !g_names)
    {
        Logger::i("Failed to find g_world or g_names\n");
        return -1;
    }

    auto display = android::ANativeWindowCreator::GetDisplayInfo();
    auto window = android::ANativeWindowCreator::Create("CPING", display.width, display.height, false, false);
    Renderer::Init(window, display.width, display.height);

    is_running = true;
    if (display.width < display.height)
        std::swap(display.width, display.height);
    while (is_running)
    {
        Renderer::StartFrame();
        ImDrawList *draw_list = ImGui::GetBackgroundDrawList();

        uintptr_t u_world = Memory::Read<uintptr_t>(Memory::Read<uintptr_t>(g_world + 0x58, target_pid) + 0x78, target_pid);
        uintptr_t u_level = Memory::Read<uintptr_t>(u_world + Offset::persistent_level, target_pid);
        uintptr_t actors_list = Ue4::get_actors_array(u_level, Offset::u_level_to_a_actors, 0x448, target_pid);
        uintptr_t u_level_to_a_actors = Memory::Read<uintptr_t>(actors_list, target_pid);
        int u_level_to_a_actors_count = Memory::Read<int>(actors_list + sizeof(uintptr_t), target_pid);
        uintptr_t player_controller = Memory::Read<uintptr_t>(Memory::Read<uintptr_t>(Memory::Read<uintptr_t>(u_world + 0x38, target_pid) + 0x78, target_pid) + 0x30, target_pid);
        uintptr_t player_camera_manager = Memory::Read<uintptr_t>(player_controller + Offset::player_camera_manager, target_pid);
        if (player_camera_manager <= 0 || !player_controller || !u_level || !u_level_to_a_actors || !actors_list || !u_world || (u_level_to_a_actors_count <= 0 || u_level_to_a_actors_count >= 1000))
        {
            // Anti-tampering check
            if (!verify_branding_integrity())
            {
                exit(13);
            }

            const char *protected_branding = get_protected_branding();
            ImVec2 text_size = ImGui::GetFont()->CalcTextSizeA(50.0f, FLT_MAX, -1.0f, protected_branding);
            draw_list->AddText(ImGui::GetFont(), 50.0f, ImVec2(display.width * 0.5f - text_size.x * 0.5f, 200.0f - text_size.y * 0.5f), IM_COL32(255, 255, 255, 255), protected_branding);
            branding_displayed = true;
        }
        else
        {
            int count_enemies = 0;

            Structs::CameraCacheEntry camera_cache = Memory::Read<Structs::CameraCacheEntry>(player_camera_manager + Offset::camera_cache, target_pid);
            Structs::MinimalViewInfo minimal_view_info = camera_cache.POV;
            uintptr_t gname_buff[100] = {0};
            Memory::ReadArray(g_names, gname_buff, target_pid);

            std::vector<Structs::FVector> off_screen_enemies;
            for (int i = 0; i < u_level_to_a_actors_count; i++)
            {
                uintptr_t actor = Memory::Read<uintptr_t>(u_level_to_a_actors + i * sizeof(uintptr_t), target_pid);
                if (!actor)
                    continue;

                std::string class_name = Ue4::get_cached_class_name(g_names, actor, gname_buff, &g_class_name_cache, target_pid);
                if (class_name == "none" || class_name.empty())
                    continue;

                if (strstr(class_name.c_str(), "VH_PG117_C") || strstr(class_name.c_str(), "BP_CharacterRifle") || strstr(class_name.c_str(), "BP_PlayerRifle") || strstr(class_name.c_str(), "BP_PlayerLobby_Pawn_C") || strstr(class_name.c_str(), "BP_PlayerLobby_Pawn_C") || strstr(class_name.c_str(), "BP_PlayerController") || strstr(class_name.c_str(), "BP_PlayerCameraManager_C"))
                    continue;

                uintptr_t actor_root_component = Memory::Read<uintptr_t>(actor + Offset::root_component, target_pid);
                if (!actor_root_component)
                    continue;

                Structs::FVector grid_step_location = Memory::Read<Structs::FVector>(actor_root_component + Offset::grid_step, target_pid);
                float distance = (Structs::FVector::Distance(minimal_view_info.Location, grid_step_location) / 100.0f);
                Structs::FVector relative_location = Memory::Read<Structs::FVector>(actor_root_component + Offset::relative_location, target_pid);
                Structs::FRotator relative_rotation = Memory::Read<Structs::FRotator>(actor_root_component + Offset::relative_rotation, target_pid);
                Structs::FVector location = Ue4::world_to_screen(relative_location, minimal_view_info, display.width, display.height);

                int margin = 10;
                if (location.X < margin || location.Y < margin || location.X > display.width - margin || location.Y > display.height - margin || location.Z < 0.0f)
                {
                    if (strstr(class_name.c_str(), "BP_Character") || strstr(class_name.c_str(), "BP_Player"))
                    {

                        if (distance < 2.0f || distance > 400.0f)
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

                        int local_team_id = Memory::Read<int>(player_controller + Offset::local_team_id, target_pid);
                        int team_id = Memory::Read<int>(actor + Offset::team_id, target_pid);
                        if (local_team_id == team_id || team_id <= -1 || team_id >= 1000)
                            continue;

                        off_screen_enemies.push_back(relative_location);
                        count_enemies++;
                    }
                    continue;
                }

                if (strstr(class_name.c_str(), "BP_Character") || strstr(class_name.c_str(), "BP_Player"))
                {

                    // draw_list->AddText(ImGui::GetFont(), 100.0f, ImVec2(location.X, location.Y), IM_COL32(255, 255, 255, 255), class_name.c_str());

                    if (distance < 2.0f || distance > 400.0f)
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

                    int local_team_id = Memory::Read<int>(player_controller + Offset::local_team_id, target_pid);
                    int team_id = Memory::Read<int>(actor + Offset::team_id, target_pid);
                    if (local_team_id == team_id || team_id <= -1 || team_id >= 1000)
                        continue;

                    count_enemies++;

                    Structs::FBoxSphereBounds box_sphere_bounds = {};
                    Structs::FTransform transform = {};
                    if (Ue4::process_object_bounds(
                            actor,
                            {0x498},
                            {0},
                            {0x9D4},
                            {0.45f, 0.45f, 0.65f},
                            0.0f,
                            &box_sphere_bounds,
                            &transform,
                            target_pid))
                    {
                        Structs::FVector corners[8] = {
                            {-box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z}};

                        Structs::FVector output_object[8];

                        for (int i = 0; i < 8; ++i)
                        {
                            Structs::FVector local = corners[i] + box_sphere_bounds.Origin;
                            Structs::FVector world = transform.TransformPosition(local);
                            Structs::CameraCacheEntry camera_cache_local = Memory::Read<Structs::CameraCacheEntry>(player_camera_manager + Offset::camera_cache, target_pid);
                            Structs::MinimalViewInfo minimal_view_info_local = camera_cache_local.POV;
                            Structs::FVector screen = Ue4::world_to_screen(world,
                                                                           minimal_view_info_local,
                                                                           display.width,
                                                                           display.height);
                            output_object[i] = screen;
                        }

                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j];
                            Structs::FVector p2 = output_object[(j + 1) % 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(255, 0, 0, 255), 4.0f);
                        }
                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j + 4];
                            Structs::FVector p2 = output_object[((j + 1) % 4) + 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(255, 0, 0, 255), 4.0f);
                        }
                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j];
                            Structs::FVector p2 = output_object[j + 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(255, 0, 0, 255), 4.0f);
                        }
                    }
                }

                if ((strstr(class_name.c_str(), "VH_") || strstr(class_name.c_str(), "Mirado_") || strstr(class_name.c_str(), "CoupeRB_") || strstr(class_name.c_str(), "Rony_") || strstr(class_name.c_str(), "PickUp_") || strstr(class_name.c_str(), "AquaRail_") || strstr(class_name.c_str(), "BP_Bike_") || strstr(class_name.c_str(), "BP_Bike2_") || strstr(class_name.c_str(), "wing_Vehicle_")))
                {
                    Structs::FBoxSphereBounds box_sphere_bounds = {};
                    Structs::FTransform transform = {};

                    if (Ue4::process_object_bounds(actor,
                                                   {0xb18},
                                                   {0x1268},
                                                   {0xac},
                                                   {0.45f, 0.45f, 0.65f},
                                                   0.5f,
                                                   &box_sphere_bounds,
                                                   &transform,
                                                   target_pid))
                    {

                        Structs::FVector corners[8] = {
                            {-box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z}};

                        Structs::FVector output_object[8];

                        for (int i = 0; i < 8; ++i)
                        {
                            Structs::FVector local = corners[i] + box_sphere_bounds.Origin;
                            Structs::FVector world = transform.TransformPosition(local);
                            Structs::CameraCacheEntry camera_cache_local = Memory::Read<Structs::CameraCacheEntry>(player_camera_manager + Offset::camera_cache, target_pid);
                            Structs::MinimalViewInfo minimal_view_info_local = camera_cache_local.POV;
                            Structs::FVector screen = Ue4::world_to_screen(world,
                                                                           minimal_view_info_local,
                                                                           display.width,
                                                                           display.height);
                            output_object[i] = screen;
                        }

                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j];
                            Structs::FVector p2 = output_object[(j + 1) % 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(0, 0, 255, 255), 3.0f);
                        }

                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j + 4];
                            Structs::FVector p2 = output_object[((j + 1) % 4) + 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(0, 0, 255, 255), 3.0f);
                        }

                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j];
                            Structs::FVector p2 = output_object[j + 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(0, 0, 255, 255), 3.0f);
                        }
                    }
                }

                if (strstr(class_name.c_str(), "Pickup_C") || strstr(class_name.c_str(), "PickUp") || strstr(class_name.c_str(), "BP_Ammo") || strstr(class_name.c_str(), "BP_QK") || strstr(class_name.c_str(), "Wrapper"))
                {
                    Structs::FBoxSphereBounds box_sphere_bounds = {};
                    Structs::FTransform transform = {};

                    if (Ue4::process_object_bounds(actor,
                                                   {0x8a0, 0x8a8, 0x8b0, 0x8b8, 0x8c0},
                                                   {0x878, 0x878, 0x878, 0x878, 0x878},
                                                   {0x170, 0x170, 0x170, 0x170, 0x170},
                                                   {1.0f, 1.0f, 1.0f},
                                                   0.0f,
                                                   &box_sphere_bounds,
                                                   &transform,
                                                   target_pid))
                    {
                        Structs::FVector corners[8] = {
                            {-box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z}};

                        Structs::FVector output_object[8];

                        for (int i = 0; i < 8; ++i)
                        {
                            Structs::FVector local = corners[i] + box_sphere_bounds.Origin;
                            Structs::FVector world = transform.TransformPosition(local);
                            Structs::CameraCacheEntry camera_cache_local = Memory::Read<Structs::CameraCacheEntry>(player_camera_manager + Offset::camera_cache, target_pid);
                            Structs::MinimalViewInfo minimal_view_info_local = camera_cache_local.POV;
                            Structs::FVector screen = Ue4::world_to_screen(world,
                                                                           minimal_view_info_local,
                                                                           display.width,
                                                                           display.height);
                            output_object[i] = screen;
                        }

                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j];
                            Structs::FVector p2 = output_object[(j + 1) % 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(0, 255, 0, 255), 1.0f);
                        }

                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j + 4];
                            Structs::FVector p2 = output_object[((j + 1) % 4) + 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(0, 255, 0, 255), 1.0f);
                        }

                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j];
                            Structs::FVector p2 = output_object[j + 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(0, 255, 255, 255), 1.0f);
                        }
                    }
                }
                if (strstr(class_name.c_str(), "BP_AirDropBox"))
                {
                    Structs::FBoxSphereBounds box_sphere_bounds = {};
                    Structs::FTransform transform = {};

                    if (Ue4::process_object_bounds(actor,
                                                   {0x810},
                                                   {0x878},
                                                   {0x170},
                                                   {1.0f, 1.0f, 1.0f},
                                                   0.0f,
                                                   &box_sphere_bounds,
                                                   &transform,
                                                   target_pid))
                    {
                        Structs::FVector corners[8] = {
                            {-box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
                            {-box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z}};

                        Structs::FVector output_object[8];

                        for (int i = 0; i < 8; ++i)
                        {
                            Structs::FVector local = corners[i] + box_sphere_bounds.Origin;
                            Structs::FVector world = transform.TransformPosition(local);
                            Structs::CameraCacheEntry camera_cache_local = Memory::Read<Structs::CameraCacheEntry>(player_camera_manager + Offset::camera_cache, target_pid);
                            Structs::MinimalViewInfo minimal_view_info_local = camera_cache_local.POV;
                            Structs::FVector screen = Ue4::world_to_screen(world,
                                                                           minimal_view_info_local,
                                                                           display.width,
                                                                           display.height);
                            output_object[i] = screen;
                        }

                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j];
                            Structs::FVector p2 = output_object[(j + 1) % 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(255, 150, 0, 255), 3.0f);
                        }
                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j + 4];
                            Structs::FVector p2 = output_object[((j + 1) % 4) + 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(255, 150, 0, 255), 3.0f);
                        }
                        for (int j = 0; j < 4; ++j)
                        {
                            Structs::FVector p1 = output_object[j];
                            Structs::FVector p2 = output_object[j + 4];
                            ImGui::GetForegroundDrawList()->AddLine(ImVec2(p1.X, p1.Y), ImVec2(p2.X, p2.Y),
                                                                    IM_COL32(255, 150, 0, 255), 3.0f);
                        }
                    }
                }
            }

            ImVec2 screen_center(display.width * 0.5f, display.height * 0.5f);
            float edge_padding_x = display.width * 0.20f;
            float edge_padding_y = display.height * 0.10f;
            float arrow_size = fminf(display.width, display.height) * 0.06f;
            float arrow_angle_offset = 0.4f;
            float yaw = minimal_view_info.Rotation.Yaw * (M_PI / 180.0f);
            float pitch = minimal_view_info.Rotation.Pitch * (M_PI / 180.0f);
            Structs::FVector camera_forward = {
                cosf(pitch) * cosf(yaw),
                cosf(pitch) * sinf(yaw),
                sinf(pitch)};
            Structs::FVector camera_right = {
                -sinf(yaw),
                cosf(yaw),
                0.0f};
            Structs::FVector camera_up = {
                -sinf(pitch) * cosf(yaw),
                -sinf(pitch) * sinf(yaw),
                cosf(pitch)};

            for (const auto &enemy_world_pos : off_screen_enemies)
            {
                Structs::FVector to_enemy = enemy_world_pos - minimal_view_info.Location;
                to_enemy.Normalize();

                float forward_dot = Structs::FVector::Dot(camera_forward, to_enemy);
                float right_dot = Structs::FVector::Dot(camera_right, to_enemy);
                float up_dot = Structs::FVector::Dot(camera_up, to_enemy);

                float dir_x, dir_y;

                if (forward_dot > 0)
                {
                    dir_x = right_dot / forward_dot;
                    dir_y = -up_dot / forward_dot;
                }
                else
                {
                    dir_x = right_dot;
                    dir_y = -forward_dot;
                }

                // تطبيع الاتجاه
                float len = sqrtf(dir_x * dir_x + dir_y * dir_y);
                if (len < 1e-4f)
                    continue;

                dir_x /= len;
                dir_y /= len;

                float scale_x = (display.width * 0.5f - edge_padding_x) / fabsf(dir_x);
                float scale_y = (display.height * 0.5f - edge_padding_y) / fabsf(dir_y);
                float scale = fminf(scale_x, scale_y);

                ImVec2 arrow_tip = {
                    screen_center.x + dir_x * scale,
                    screen_center.y + dir_y * scale};

                float angle_screen = atan2f(dir_y, dir_x);

                ImVec2 side1 = {
                    arrow_tip.x - cosf(angle_screen + arrow_angle_offset) * arrow_size,
                    arrow_tip.y - sinf(angle_screen + arrow_angle_offset) * arrow_size};

                ImVec2 side2 = {
                    arrow_tip.x - cosf(angle_screen - arrow_angle_offset) * arrow_size,
                    arrow_tip.y - sinf(angle_screen - arrow_angle_offset) * arrow_size};

                ImVec2 shadow_off = {2.0f, 2.0f};
                draw_list->AddTriangleFilled(
                    ImVec2(arrow_tip.x + shadow_off.x, arrow_tip.y + shadow_off.y),
                    ImVec2(side1.x + shadow_off.x, side1.y + shadow_off.y),
                    ImVec2(side2.x + shadow_off.x, side2.y + shadow_off.y),
                    IM_COL32(0, 0, 0, 100));

                draw_list->AddTriangleFilled(arrow_tip, side1, side2, IM_COL32(255, 60, 60, 255));
                draw_list->AddTriangle(arrow_tip, side1, side2, IM_COL32(255, 120, 120, 255), 2.5f);
            }

            if (count_enemies > 0)
            {

                ImVec2 text_size = ImGui::GetFont()->CalcTextSizeA(60.0f, FLT_MAX, -1.0f, std::string("ENEMIES: " + std::to_string(count_enemies)).c_str());
                draw_list->AddText(ImGui::GetFont(), 60.0f, ImVec2(display.width * 0.5f - text_size.x * 0.5f, 200.0f - text_size.y * 0.5f), IM_COL32(255, 0, 0, 255), std::string("ENEMIES: " + std::to_string(count_enemies)).c_str());
            }

            // Always show branding when enemies are detected
            const char *protected_branding = get_protected_branding();
            ImVec2 brand_text_size = ImGui::GetFont()->CalcTextSizeA(30.0f, FLT_MAX, -1.0f, protected_branding);
            draw_list->AddText(ImGui::GetFont(), 30.0f, ImVec2(20.0f, display.height - 40.0f), IM_COL32(255, 255, 255, 200), protected_branding);
            branding_displayed = true;
        }

        Renderer::EndFrame();

        // Anti-tampering protection - ensure branding was displayed
        if (!branding_displayed)
        {
            exit(13);
        }

        // Reset for next frame
        branding_displayed = false;
    }

    Renderer::Shutdown();
    return 0;
}