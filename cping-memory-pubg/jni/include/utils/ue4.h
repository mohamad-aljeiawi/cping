#ifndef UE4_H
#define UE4_H

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "types/structs.h"
#include "imgui/imgui.h"

namespace Ue4
{
    uint64_t get_actors_array(uint64_t u_level, int actors_Offset, int encrypted_actors_offset, pid_t process_pid);
    std::string get_cached_class_name(uintptr_t g_names, uintptr_t actor, uintptr_t gname_buff[100], std::unordered_map<int, std::string> &g_class_name_cache, pid_t target_pid);
    std::string get_g_names(uintptr_t g_names, uintptr_t actor, uintptr_t gname_buff[100], pid_t target_pid);
    Structs::FMatrix rotator_to_matrix(Structs::FRotator rotation);
    Structs::FVector rotator_to_vector(const Structs::FRotator &r);
    Structs::FVector cross(const Structs::FVector &a, const Structs::FVector &b);
    Structs::FVector world_to_screen(Structs::FVector worldLocation, Structs::MinimalViewInfo camViewInfo, int screenWidth, int screenHeight);
    Structs::FTransform get_component_to_world(uintptr_t entity, pid_t process_pid);
    Structs::FTransform get_bone_transform(uintptr_t entity, int idx, pid_t process_pid);
    bool process_object_bounds(uintptr_t actor,
                               const std::vector<uintptr_t> &component_offsets,
                               const std::vector<uintptr_t> &mesh_offsets,
                               const std::vector<uintptr_t> &bounds_offsets,
                               Structs::FVector scale_factors,
                               float origin_z_offset,
                               Structs::MinimalViewInfo &minimal_view_info,
                               int screen_width,
                               int screen_height,
                               Structs::FVector *output_object,
                               pid_t process_pid);
    Structs::OverlayInfo compute_offscreen_enemy_overlay(const Structs::FVector &enemy_world_pos,
                                                         const Structs::MinimalViewInfo &minimal_view_info,
                                                         int screen_width,
                                                         int screen_height);
}

#endif // UE4_H