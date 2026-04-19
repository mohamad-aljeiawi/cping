#include "utils/ue4.h"
#include "utils/memory.h"
#include "types/offset.h"

#include <cmath>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <cfloat>
#include <cstdint>

namespace Ue4
{

    uint64_t get_actors_array(uint64_t u_level, int actors_Offset, int encrypted_actors_offset, pid_t process_pid)
    {
        if (u_level < 0x10000000)
            return 0;

        if (Memory::Read<uint64_t>(u_level + actors_Offset, process_pid) > 0)
            return u_level + actors_Offset;

        if (Memory::Read<uint64_t>(u_level + encrypted_actors_offset, process_pid) > 0)
            return u_level + encrypted_actors_offset;

        auto a_actors = Memory::Read<Structs::Actors>(u_level + encrypted_actors_offset + 0x10, process_pid);

        if (a_actors.enc_1 > 0)
        {
            auto enc = Memory::Read<Structs::Chunk>(a_actors.enc_1 + 0x80, process_pid);
            return (((Memory::Read<uint8_t>(a_actors.enc_1 + enc.val_1, process_pid) |
                      (Memory::Read<uint8_t>(a_actors.enc_1 + enc.val_2, process_pid) << 8)) |
                     (Memory::Read<uint8_t>(a_actors.enc_1 + enc.val_3, process_pid) << 0x10)) &
                        0xFFFFFF |
                    ((uint64_t)Memory::Read<uint8_t>(a_actors.enc_1 + enc.val_4, process_pid) << 0x18) |
                    ((uint64_t)Memory::Read<uint8_t>(a_actors.enc_1 + enc.val_5, process_pid) << 0x20)) &
                       0xFFFF00FFFFFFFFFF |
                   ((uint64_t)Memory::Read<uint8_t>(a_actors.enc_1 + enc.val_6, process_pid) << 0x28) |
                   ((uint64_t)Memory::Read<uint8_t>(a_actors.enc_1 + enc.val_7, process_pid) << 0x30) |
                   ((uint64_t)Memory::Read<uint8_t>(a_actors.enc_1 + enc.val_8, process_pid) << 0x38);
        }
        else if (a_actors.enc_2 > 0)
        {
            auto lost_actors = Memory::Read<uint64_t>(a_actors.enc_2, process_pid);
            if (lost_actors > 0)
            {
                return (uint16_t)(lost_actors - 0x400) & 0xFF00 |
                       (uint8_t)(lost_actors - 0x04) |
                       (lost_actors + 0xFC0000) & 0xFF0000 |
                       (lost_actors - 0x4000000) & 0xFF000000 |
                       (lost_actors + 0xFC00000000) & 0xFF00000000 |
                       (lost_actors + 0xFC0000000000) & 0xFF0000000000 |
                       (lost_actors + 0xFC000000000000) & 0xFF000000000000 |
                       (lost_actors - 0x400000000000000) & 0xFF00000000000000;
            }
        }
        else if (a_actors.enc_3 > 0)
        {
            auto lost_actors = Memory::Read<uint64_t>(a_actors.enc_3, process_pid);
            if (lost_actors > 0)
            {
                return (lost_actors >> 0x38) | (lost_actors << (64 - 0x38));
            }
        }
        else if (a_actors.enc_4 > 0)
        {
            auto lost_actors = Memory::Read<uint64_t>(a_actors.enc_4, process_pid);
            if (lost_actors > 0)
            {
                return lost_actors ^ 0xCDCD00;
            }
        }
        return 0;
    }

    std::string get_cached_class_name(uintptr_t g_names, uintptr_t actor, uintptr_t gname_buff[100], std::unordered_map<int, std::string> &g_class_name_cache, pid_t target_pid)
    {
        int class_id = Memory::Read<int>(actor + 0x18, target_pid);

        auto it = g_class_name_cache.find(class_id);
        if (it != g_class_name_cache.end())
        {
            return it->second;
        }

        int page = class_id / 0x4000;
        int index = class_id % 0x4000;

        if (gname_buff[page] == 0)
        {
            gname_buff[page] = Memory::Read<uintptr_t>(g_names + page * sizeof(uintptr_t), target_pid);
        }

        uintptr_t entry_ptr = Memory::Read<uintptr_t>(gname_buff[page] + index * sizeof(uintptr_t), target_pid);
        if (!entry_ptr)
        {
            g_class_name_cache.emplace(class_id, "none");
            return "none";
        }

        std::string name = Memory::ReadFName(entry_ptr, target_pid);

        g_class_name_cache.emplace(class_id, name);

        return name;
    }

    std::string get_g_names(uintptr_t g_names, uintptr_t actor, uintptr_t gname_buff[100], pid_t target_pid)
    {
        int class_id = Memory::Read<int>(actor + 0x18, target_pid);
        int page = class_id / 0x4000;
        int index = class_id % 0x4000;

        if (gname_buff[page] == 0)
        {
            gname_buff[page] = Memory::Read<uintptr_t>(g_names + page * sizeof(uintptr_t), target_pid);
        }

        uintptr_t entry_ptr = Memory::Read<uintptr_t>(gname_buff[page] + index * sizeof(uintptr_t), target_pid);
        if (!entry_ptr)
            return "none";

        return Memory::ReadFName(entry_ptr, target_pid);
    }

    Structs::FMatrix rotator_to_matrix(Structs::FRotator rotation)
    {
        float radPitch = rotation.Pitch * ((float)M_PI / 180.0f);
        float radYaw = rotation.Yaw * ((float)M_PI / 180.0f);
        float radRoll = rotation.Roll * ((float)M_PI / 180.0f);

        float SP = sinf(radPitch);
        float CP = cosf(radPitch);
        float SY = sinf(radYaw);
        float CY = cosf(radYaw);
        float SR = sinf(radRoll);
        float CR = cosf(radRoll);

        Structs::FMatrix matrix;

        matrix.M[0][0] = (CP * CY);
        matrix.M[0][1] = (CP * SY);
        matrix.M[0][2] = (SP);
        matrix.M[0][3] = 0;

        matrix.M[1][0] = (SR * SP * CY - CR * SY);
        matrix.M[1][1] = (SR * SP * SY + CR * CY);
        matrix.M[1][2] = (-SR * CP);
        matrix.M[1][3] = 0;

        matrix.M[2][0] = (-(CR * SP * CY + SR * SY));
        matrix.M[2][1] = (CY * SR - CR * SP * SY);
        matrix.M[2][2] = (CR * CP);
        matrix.M[2][3] = 0;

        matrix.M[3][0] = 0;
        matrix.M[3][1] = 0;
        matrix.M[3][2] = 0;
        matrix.M[3][3] = 1;

        return matrix;
    }

    Structs::FVector rotator_to_vector(const Structs::FRotator &r)
    {
        float DEG2RAD = 3.14159265f / 180.f;
        float cp = std::cos(r.Pitch * DEG2RAD);
        float sp = std::sin(r.Pitch * DEG2RAD);
        float cy = std::cos(r.Yaw * DEG2RAD);
        float sy = std::sin(r.Yaw * DEG2RAD);

        return Structs::FVector(cp * cy, // X
                                cp * sy, // Y
                                sp);     // Z
    }

    Structs::FVector cross(const Structs::FVector &a, const Structs::FVector &b)
    {
        return Structs::FVector(
            a.Y * b.Z - a.Z * b.Y,
            a.Z * b.X - a.X * b.Z,
            a.X * b.Y - a.Y * b.X);
    }

    Structs::FVector world_to_screen(Structs::FVector worldLocation, Structs::MinimalViewInfo camViewInfo, int screenWidth, int screenHeight)
    {
        Structs::FMatrix tempMatrix = rotator_to_matrix(camViewInfo.Rotation);

        Structs::FVector vAxisX(tempMatrix.M[0][0], tempMatrix.M[0][1], tempMatrix.M[0][2]);
        Structs::FVector vAxisY(tempMatrix.M[1][0], tempMatrix.M[1][1], tempMatrix.M[1][2]);
        Structs::FVector vAxisZ(tempMatrix.M[2][0], tempMatrix.M[2][1], tempMatrix.M[2][2]);

        Structs::FVector vDelta = worldLocation - camViewInfo.Location;

        Structs::FVector vTransformed(Structs::FVector::Dot(vDelta, vAxisY), Structs::FVector::Dot(vDelta, vAxisZ), Structs::FVector::Dot(vDelta, vAxisX));

        float fov = camViewInfo.FOV;
        float screenCenterX = (screenWidth / 2.0f);
        float screenCenterY = (screenHeight / 2.0f);

        float X = (screenCenterX + vTransformed.X * (screenCenterX / tanf(fov * ((float)M_PI / 360.0f))) / vTransformed.Z);
        float Y = (screenCenterY - vTransformed.Y * (screenCenterX / tanf(fov * ((float)M_PI / 360.0f))) / vTransformed.Z);
        float Z = vTransformed.Z;

        return {X, Y, Z};
    }

    Structs::FTransform get_component_to_world(uintptr_t entity, pid_t process_pid)
    {
        uintptr_t mesh = Memory::Read<uintptr_t>(entity + Offset::mesh, process_pid);
        if (mesh)
        {
            return Memory::Read<Structs::FTransform>(mesh + Offset::component_to_world, process_pid);
        }
        return {};
    }

    Structs::FTransform get_bone_transform(uintptr_t entity, int idx, pid_t process_pid)
    {
        uintptr_t mesh = Memory::Read<uintptr_t>(entity + Offset::mesh, process_pid);
        if (mesh)
        {
            uintptr_t bones = Memory::Read<uintptr_t>(mesh + Offset::static_mesh, process_pid);
            if (bones)
            {
                return Memory::Read<Structs::FTransform>(bones + (idx * 48), process_pid);
            }
        }
        return {};
    }

    // Unified function to process object bounds and create 3D boxes
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
                               pid_t process_pid)

    {
        // Try different component offsets to find a valid one
        uintptr_t component = 0;
        uintptr_t mesh = 0;
        Structs::FBoxSphereBounds box_sphere_bounds = {};
        Structs::FTransform transform = {};

        for (size_t i = 0; i < component_offsets.size(); i++)
        {
            component = Memory::Read<uintptr_t>(actor + component_offsets[i], process_pid);
            if (!component)
                continue;

            if (i < mesh_offsets.size() && mesh_offsets[i] > 0)
            {
                // Special case for vehicles: read mesh directly from actor
                if (mesh_offsets[i] > 0x1000) // Large offset indicates reading from actor
                {
                    mesh = Memory::Read<uintptr_t>(actor + mesh_offsets[i], process_pid);
                }
                else
                {
                    mesh = Memory::Read<uintptr_t>(component + mesh_offsets[i], process_pid);
                }
                if (!mesh)
                    continue;

                if (i < bounds_offsets.size())
                {
                    box_sphere_bounds = Memory::Read<Structs::FBoxSphereBounds>(mesh + bounds_offsets[i], process_pid);
                }
            }
            else
            {
                if (i < bounds_offsets.size())
                {
                    box_sphere_bounds = Memory::Read<Structs::FBoxSphereBounds>(component + bounds_offsets[i], process_pid);
                }
            }

            // Check if bounds are valid
            if (!std::isnan(box_sphere_bounds.Origin.X) && !std::isnan(box_sphere_bounds.BoxExtent.X) &&
                box_sphere_bounds.BoxExtent.X > 0 && box_sphere_bounds.BoxExtent.X < 5000.0f)
            {
                transform = Memory::Read<Structs::FTransform>(component + Offset::component_to_world, process_pid);
                break;
            }
        }

        if (!component)
            return false;

        // Apply scale and offset modifications
        box_sphere_bounds.BoxExtent.X *= scale_factors.X;
        box_sphere_bounds.BoxExtent.Y *= scale_factors.Y;
        box_sphere_bounds.BoxExtent.Z *= scale_factors.Z;
        box_sphere_bounds.Origin.Z -= box_sphere_bounds.BoxExtent.Z * origin_z_offset;

        Structs::FVector corners[8] = {
            {-box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
            {box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
            {box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
            {-box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, -box_sphere_bounds.BoxExtent.Z},
            {-box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
            {box_sphere_bounds.BoxExtent.X, -box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
            {box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z},
            {-box_sphere_bounds.BoxExtent.X, box_sphere_bounds.BoxExtent.Y, box_sphere_bounds.BoxExtent.Z}

        };

        for (int i = 0; i < 8; ++i)
        {
            Structs::FVector local = corners[i] + box_sphere_bounds.Origin;
            Structs::FVector world = transform.TransformPosition(local);
            Structs::FVector screen = Ue4::world_to_screen(world,
                                                           minimal_view_info,
                                                           screen_width,
                                                           screen_height);
            output_object[i] = screen;
        }

        return true;
    }

    Structs::OverlayInfo compute_offscreen_enemy_overlay(const Structs::FVector &enemy_world_pos,
                                                         const Structs::MinimalViewInfo &minimal_view_info,
                                                         int screen_width,
                                                         int screen_height)
    {
        Structs::OverlayInfo result;

        ImVec2 screen_center(screen_width * 0.5f, screen_height * 0.5f);
        float edge_padding_x = screen_width * 0.10f;
        float edge_padding_y = screen_height * 0.20f;
        float arrow_size = fminf(screen_width, screen_height) * 0.06f;
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

        float len = sqrtf(dir_x * dir_x + dir_y * dir_y);
        if (len < 1e-4f)
            return result;

        dir_x /= len;
        dir_y /= len;

        float scale_x = (screen_width * 0.5f - edge_padding_x) / fabsf(dir_x);
        float scale_y = (screen_height * 0.5f - edge_padding_y) / fabsf(dir_y);
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

        result.arrow = {arrow_tip, side1, side2};

        return result;
    }

}
