#ifndef UE4_H
#define UE4_H
#include <math.h>
#include "memory.h"
#include "utils/structs.h"

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

    static size_t detect_gnames_pages(uintptr_t g_names, pid_t target_pid)
    {
        static bool already_executed = false;
        static size_t cached_pages = 0;

        if (already_executed)
        {
            return cached_pages;
        }

        already_executed = true;

        const size_t MAX_POSSIBLE_PAGES = 512;
        size_t detected_pages = 0;

        FILE *output_file = fopen("/data/local/tmp/names_tree.txt", "w");
        if (!output_file)
        {
            printf("Error: Could not create output file\n");
            return 0;
        }

        uintptr_t first_page = Memory::Read<uintptr_t>(g_names, target_pid);
        if (!first_page)
        {
            fprintf(output_file, "GNames detection: First page NULL\n");
            printf("GNames detection: First page NULL\n");
            fclose(output_file);
            return 0;
        }

        fprintf(output_file, "GNames Tree Structure - Base Address: 0x%lx\n", g_names);
        fprintf(output_file, "┌── Root\n");
        printf("GNames detection scanning - address: 0x%lx\n", g_names);

        for (size_t page = 1; page < MAX_POSSIBLE_PAGES; page++)
        {
            uintptr_t page_ptr = Memory::Read<uintptr_t>(g_names + page * sizeof(uintptr_t), target_pid);

            if (page_ptr)
            {
                fprintf(output_file, "│   ├── Page %zu (0x%lx)\n", page, page_ptr);
            }
            else
            {
                fprintf(output_file, "│   └── Page %zu (NULL) [End of Pages]\n", page);
                printf("GNames detection: End detected at page %zu\n", page);
                detected_pages = page;
                break;
            }

            printf("Page %zu address: 0x%lx\n", page, page_ptr);

            const int ENTRIES_PER_PAGE = 0x4000;
            const int ENTRIES_TO_DISPLAY = ENTRIES_PER_PAGE;

            int valid_entries = 0;
            int icon_entries = 0;

            fprintf(output_file, "│   │   ├── Entries:\n");

            for (int i = 0; i < ENTRIES_TO_DISPLAY; i++)
            {
                uintptr_t entry = Memory::Read<uintptr_t>(page_ptr + i * sizeof(uintptr_t), target_pid);

                if (entry)
                {
                    valid_entries++;

                    char name_buffer[256] = {0};
                    uintptr_t string_ptr = entry + 4 + sizeof(uintptr_t);

                    size_t read_size = 0;
                    while (read_size < sizeof(name_buffer) - 1)
                    {
                        char c = Memory::Read<char>(string_ptr + read_size, target_pid);
                        if (c == 0)
                            break;
                        name_buffer[read_size++] = c;
                    }

                    int32_t name_length = Memory::Read<int32_t>(entry, target_pid);
                    int32_t name_flags = Memory::Read<int32_t>(entry + 4, target_pid);

                    bool is_icon = strstr(name_buffer, "Icon_") != NULL;
                    bool is_weapon = strstr(name_buffer, "WEP_") != NULL;

                    if (is_icon)
                    {
                        icon_entries++;

                        if (is_weapon)
                        {
                            fprintf(output_file, "│   │   │   ├── 🔫 [%d] \"%s\" (0x%lx, Flags: 0x%x)\n",
                                    i, name_buffer, entry, name_flags);
                        }
                        else
                        {
                            fprintf(output_file, "│   │   │   ├── 🏷️ [%d] \"%s\" (0x%lx, Flags: 0x%x)\n",
                                    i, name_buffer, entry, name_flags);
                        }
                    }
                    else if (i < 20 || i % 1000 == 0 || read_size > 20)
                    {
                        fprintf(output_file, "│   │   │   ├── [%d] \"%s\" (0x%lx, Flags: 0x%x)\n",
                                i, name_buffer, entry, name_flags);
                    }

                    if ((i < 10 || i % 1000 == 0 || is_icon) && read_size > 0)
                    {
                        printf("  Entry %d [0x%x]: 0x%lx -> \"%s\"\n", i, i, entry, name_buffer);
                    }
                }
                else if (i < 10 || i % 1000 == 0)
                {
                    fprintf(output_file, "│   │   │   ├── [%d] NULL\n", i);
                }

                if (i > 0 && i % 1000 == 0)
                {
                    printf("  ... Scanned %d entries, found %d valid entries, %d icon entries...\n",
                           i, valid_entries, icon_entries);
                }
            }

            fprintf(output_file, "│   │   └── Summary: %d valid entries, %d icon entries\n",
                    valid_entries, icon_entries);
            printf("  Total valid entries in page %zu: %d/%d\n", page, valid_entries, ENTRIES_TO_DISPLAY);

            if (valid_entries == 0)
            {
                fprintf(output_file, "│   └── Invalid page (all entries NULL)\n");
                printf("GNames detection: Invalid page %zu (all entries NULL)\n", page);
                detected_pages = page;
                break;
            }
        }

        if (detected_pages == 0)
        {
            fprintf(output_file, "└── No end found, using default MAX_POSSIBLE_PAGES\n");
            printf("GNames detection: No end found, using default MAX_POSSIBLE_PAGES\n");
            detected_pages = MAX_POSSIBLE_PAGES;
        }
        else
        {
            fprintf(output_file, "└── Detection completed: %zu pages\n", detected_pages);
        }

        printf("GNames detection completed: %zu pages and saved to gnames_tree.txt\n", detected_pages);
        fclose(output_file);
        cached_pages = detected_pages;
        return detected_pages;
    }

    std::string get_g_names(uintptr_t g_names, uintptr_t actor, pid_t target_pid)
    {
        std::vector<uintptr_t> gname_buff(30, 0);
        gname_buff[0] = Memory::Read<uintptr_t>(g_names, target_pid);
        if (!gname_buff[0])
            return "none";

        int class_id = Memory::Read<int>(actor + sizeof(uintptr_t) + 2 * sizeof(uintptr_t), target_pid);
        int page = class_id / 0x4000;
        int index = class_id % 0x4000;

        if (page < 1 || page >= 30)
            return "none";

        if (!gname_buff[page])
        {
            gname_buff[page] = Memory::Read<uintptr_t>(g_names + page * sizeof(uintptr_t), target_pid);
        }
        uintptr_t name_entry = Memory::Read<uintptr_t>(gname_buff[page] + index * sizeof(uintptr_t), target_pid);
        if (!name_entry)
            return "none";

        char name_buffer[256] = {0};
        uintptr_t string_ptr = name_entry + 4 + sizeof(uintptr_t);
        size_t read_size = 0;

        while (read_size < sizeof(name_buffer) - 1)
        {
            char c = Memory::Read<char>(string_ptr + read_size, target_pid);
            if (c == 0)
                break;
            name_buffer[read_size++] = c;
        }

        if (read_size > 0 && Utils::is_printable_ascii(name_buffer))
        {
            return std::string(name_buffer);
        }
        return "none";
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

    Structs::FTransform get_component_to_world(uintptr_t entity)
    {
        uintptr_t mesh = Memory::Read<uintptr_t>(entity + Offset::mesh, target_pid);
        if (mesh)
        {
            return Memory::Read<Structs::FTransform>(mesh + Offset::component_to_world, target_pid);
        }
        return {};
    }

    Structs::FTransform get_bone_transform(uintptr_t entity, int idx)
    {
        uintptr_t mesh = Memory::Read<uintptr_t>(entity + Offset::mesh, target_pid);
        if (mesh)
        {
            uintptr_t bones = Memory::Read<uintptr_t>(mesh + Offset::min_lob, target_pid);
            if (bones)
            {
                return Memory::Read<Structs::FTransform>(bones + (idx * 48), target_pid);
            }
        }
        return {};
    }

    Structs::FVector transform_to_location(Structs::FTransform c2w, Structs::FTransform transform)
    {
        return matrix_to_vector(matrix_multiplication(transform_to_matrix(transform), transform_to_matrix(c2w)));
    }

    Structs::FVector get_bone_location(uintptr_t player_base, int bone_index)
    {
        if (player_base)
        {
            return transform_to_location(get_component_to_world(player_base), get_bone_transform(player_base, bone_index));
        }
        return {};
    }

} // namespace Ue4

#endif // UE4_H