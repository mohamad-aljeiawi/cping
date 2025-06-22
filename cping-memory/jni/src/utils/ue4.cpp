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

    std::string get_cached_class_name(uintptr_t g_names, uintptr_t actor, uintptr_t gname_buff[100], std::unordered_map<int, std::string> *g_class_name_cache, pid_t target_pid)
    {
        int class_id = Memory::Read<int>(actor + 0x18, target_pid);

        auto it = g_class_name_cache->find(class_id);
        if (it != g_class_name_cache->end())
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
            g_class_name_cache->insert({class_id, "none"});
            return "none";
        }

        std::string name = Memory::ReadFName(entry_ptr, target_pid);

        g_class_name_cache->insert({class_id, name});

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
        float radPitch = rotation.Pitch * ((float)3.14159265358979323846 / 180.0f);
        float radYaw = rotation.Yaw * ((float)3.14159265358979323846 / 180.0f);
        float radRoll = rotation.Roll * ((float)3.14159265358979323846 / 180.0f);

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

        float X = (screenCenterX + vTransformed.X * (screenCenterX / tanf(fov * ((float)3.14159265358979323846 / 360.0f))) / vTransformed.Z);
        float Y = (screenCenterY - vTransformed.Y * (screenCenterX / tanf(fov * ((float)3.14159265358979323846 / 360.0f))) / vTransformed.Z);
        float Z = vTransformed.Z;

        return {X, Y, Z};
    }

    Structs::FMatrix matrix_multiplication(Structs::FMatrix m1, Structs::FMatrix m2)
    {
        Structs::FMatrix matrix = Structs::FMatrix();
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                for (int k = 0; k < 4; k++)
                {
                    matrix.M[i][j] += m1.M[i][k] * m2.M[k][j];
                }
            }
        }
        return matrix;
    }

    Structs::FMatrix transform_to_matrix(Structs::FTransform transform)
    {
        Structs::FMatrix matrix;

        matrix.M[3][0] = transform.Translation.X;
        matrix.M[3][1] = transform.Translation.Y;
        matrix.M[3][2] = transform.Translation.Z;

        float x2 = transform.Rotation.X + transform.Rotation.X;
        float y2 = transform.Rotation.Y + transform.Rotation.Y;
        float z2 = transform.Rotation.Z + transform.Rotation.Z;

        float xx2 = transform.Rotation.X * x2;
        float yy2 = transform.Rotation.Y * y2;
        float zz2 = transform.Rotation.Z * z2;

        matrix.M[0][0] = (1.0f - (yy2 + zz2)) * transform.Scale3D.X;
        matrix.M[1][1] = (1.0f - (xx2 + zz2)) * transform.Scale3D.Y;
        matrix.M[2][2] = (1.0f - (xx2 + yy2)) * transform.Scale3D.Z;

        float yz2 = transform.Rotation.Y * z2;
        float wx2 = transform.Rotation.W * x2;
        matrix.M[2][1] = (yz2 - wx2) * transform.Scale3D.Z;
        matrix.M[1][2] = (yz2 + wx2) * transform.Scale3D.Y;

        float xy2 = transform.Rotation.X * y2;
        float wz2 = transform.Rotation.W * z2;
        matrix.M[1][0] = (xy2 - wz2) * transform.Scale3D.Y;
        matrix.M[0][1] = (xy2 + wz2) * transform.Scale3D.X;

        float xz2 = transform.Rotation.X * z2;
        float wy2 = transform.Rotation.W * y2;
        matrix.M[2][0] = (xz2 + wy2) * transform.Scale3D.Z;
        matrix.M[0][2] = (xz2 - wy2) * transform.Scale3D.X;

        matrix.M[0][3] = 0;
        matrix.M[1][3] = 0;
        matrix.M[2][3] = 0;
        matrix.M[3][3] = 1;

        return matrix;
    }

    Structs::FVector matrix_to_vector(Structs::FMatrix matrix)
    {
        return {matrix.M[3][0], matrix.M[3][1], matrix.M[3][2]};
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
            uintptr_t bones = Memory::Read<uintptr_t>(mesh + Offset::min_lob, process_pid);
            if (bones)
            {
                return Memory::Read<Structs::FTransform>(bones + (idx * 48), process_pid);
            }
        }
        return {};
    }

    Structs::FVector transform_to_location(Structs::FTransform c2w, Structs::FTransform transform)
    {
        return matrix_to_vector(matrix_multiplication(transform_to_matrix(transform), transform_to_matrix(c2w)));
    }

    Structs::FVector get_bone_location(uintptr_t player_base, int bone_index, pid_t process_pid)
    {
        if (player_base)
        {
            return transform_to_location(get_component_to_world(player_base, process_pid), get_bone_transform(player_base, bone_index, process_pid));
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
                               Structs::FBoxSphereBounds *box_sphere_bounds,
                               Structs::FTransform *transform,
                               pid_t process_pid)
    {
        // Try different component offsets to find a valid one
        uintptr_t component = 0;
        uintptr_t mesh = 0;
        Structs::FBoxSphereBounds _box_sphere_bounds = {};
        Structs::FTransform _transform = {};

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
                    _box_sphere_bounds = Memory::Read<Structs::FBoxSphereBounds>(mesh + bounds_offsets[i], process_pid);
                }
            }
            else
            {
                if (i < bounds_offsets.size())
                {
                    _box_sphere_bounds = Memory::Read<Structs::FBoxSphereBounds>(component + bounds_offsets[i], process_pid);
                }
            }

            // Check if bounds are valid
            if (!std::isnan(_box_sphere_bounds.Origin.X) && !std::isnan(_box_sphere_bounds.BoxExtent.X) &&
                _box_sphere_bounds.BoxExtent.X > 0 && _box_sphere_bounds.BoxExtent.X < 5000.0f)
            {
                _transform = Memory::Read<Structs::FTransform>(component + Offset::component_to_world, process_pid);
                break;
            }
        }

        if (!component)
            return false;

        *transform = _transform;
        *box_sphere_bounds = _box_sphere_bounds;

        // Apply scale and offset modifications
        box_sphere_bounds->BoxExtent.X *= scale_factors.X;
        box_sphere_bounds->BoxExtent.Y *= scale_factors.Y;
        box_sphere_bounds->BoxExtent.Z *= scale_factors.Z;
        box_sphere_bounds->Origin.Z -= box_sphere_bounds->BoxExtent.Z * origin_z_offset;

        return true;
    }

}
