#ifndef OFFSETS_H
#define OFFSETS_H

#include <cstdint>

namespace Offset
{
    //-- Base Offset G-Objects
    constexpr uintptr_t g_world = 0x1ad79b68;
    //-- Base Offset UWorld
    constexpr uintptr_t persistent_level = 0xF8;
    constexpr uintptr_t u_level_to_a_actors = 0x98;
    constexpr uintptr_t component_to_world = 0x210;

    // struct UWorld : UObject
    constexpr uintptr_t game_state = 0x140;           // struct AGameStateBase* GameState;
    constexpr uintptr_t owning_game_instance = 0x190; // struct UGameInstance* OwningGameInstance;

    // struct AGameStateBase : AInfo
    constexpr uintptr_t player_array = 0x388; // struct TArray<struct APlayerState*> PlayerArray;

    // struct UGameInstance : UObject
    constexpr uintptr_t local_players = 0x38; // struct TArray<struct ULocalPlayer*> LocalPlayers;

    // struct UPlayer : UObject
    constexpr uintptr_t player_controller = 0x30; // struct APlayerController* PlayerController;

    // struct APlayerController : AController
    constexpr uintptr_t player_camera_manager = 0x408; // struct APlayerCameraManager* PlayerCameraManager;
    constexpr uintptr_t acknowledged_pawn = 0x3F0;     // struct APawn* AcknowledgedPawn;

    // struct APlayerCameraManager : AActor
    constexpr uintptr_t camera_cache = 0x2B60; // struct FCameraCacheEntry CameraCachePrivate;

    // struct AController : AActor
    constexpr uintptr_t control_rotation = 0x3D8; // struct FRotator ControlRotation;

    // struct APlayerState : AInfo
    constexpr uintptr_t pawn = 0x3F8;        // struct APawn* PawnPrivate;
    constexpr uintptr_t player_name = 0x378; // struct FString PlayerNamePrivate;

    // struct AGPCharacterBase : ACharacterBase
    constexpr uintptr_t health_comp = 0xEE0; //  struct UGPHealthDataComponent* HealthComp;
    constexpr uintptr_t team_comp = 0xEE8;   // struct UGPTeamComponent* TeamComp;

    // struct UGPHealthDataComponent : UGPAttributeBaseComponent
    constexpr uintptr_t health_set = 0x248; // struct UGPAttributeSetHealth* HealthSet;

    // struct UGPAttributeSetHealth : UGPAttributeSet
    constexpr uintptr_t health = 0x38; // struct FGPGameplayAttributeData Health;

    // struct UGPTeamComponent : UActorComponent
    constexpr uintptr_t team_id = 0x108; // int32_t TeamId;
    constexpr uintptr_t camp_id = 0x10C; // int32_t CampId;

    // struct ACHARACTER : APawn
    constexpr uintptr_t mesh = 0x3D0;               // struct USkeletalMeshComponent* Mesh;
    constexpr uintptr_t capsule_component = 0x3E0;  // struct UCapsuleComponent* CapsuleComponent;
    constexpr uintptr_t character_movement = 0x3D8; // struct UCharacterMovementComponent* CharacterMovement;

    // struct USkeletalMeshComponent : USkinnedMeshComponent
    constexpr uintptr_t cached_bone_space_transforms = 0x9D8; // struct TArray<struct FTransform> CachedComponentSpaceTransforms;

    // struct AActor : UObject
    constexpr uintptr_t root_component = 0x180; // struct USceneComponent* RootComponent;

    // struct UCapsuleComponent : UShapeComponent
    constexpr uintptr_t capsule_half_height = 0x590; // float CapsuleHalfHeight;
    constexpr uintptr_t capsule_radius = 0x594;      // float CapsuleRadius;

    // struct UMovementComponent : UActorComponent
    constexpr uintptr_t velocity = 0x10C; // struct FVector Velocity;

    // struct UCharacterMovementComponent : UPawnMovementComponent
    constexpr uintptr_t gravity_scale = 0x198; // float GravityScale;
    constexpr uintptr_t acceleration = 0x27C;  // struct FVector Acceleration;

    // struct AGPCharacterBase : ACharacterBase
    constexpr uintptr_t cache_cur_weapon = 0x1540; // struct AWeaponBase* CacheCurWeapon;

    // struct AWeaponBase : AGPWeaponBase
    constexpr uintptr_t weapon_data_component_list = 0xA98;      // struct TArray<struct TWeakObjectPtr<struct UGPWeaponDataComponentBase>> GPWeaponDataComponentList; // 0xA98(0x10)
    constexpr uintptr_t current_modular_weapon_desc = 0xA68;     // struct UGPModularWeaponDesc* CurrentModularWeaponDesc;
    constexpr uintptr_t cached_attribute_set_fire_mode = 0x1108; // struct UGPWeaponAttributeSetFireMode* CachedAttributeSetFireMode;
    constexpr uintptr_t weapon_id = 0x810;                       // uint64_t WeaponID;
    constexpr uintptr_t weapon_equip_position = 0x820;           // uint32_t WeaponEquipPosition;

    // // struct UGPWeaponAttributeSetFireMode : UGPAttributeSetBase
    // constexpr uintptr_t fire_interval = 0x48;                     // struct FGameplayAttributeData FireInterval;
    // constexpr uintptr_t burst_interval = 0xD8;                    // struct struct FGameplayAttributeData BurstInterval;
    // constexpr uintptr_t burst_fire_bullet_count = 0x128;          // struct FGameplayAttributeData BurstFireBulletCount;
    // constexpr uintptr_t projectile_initial_velocity_rate = 0x1D8; // struct FGameplayAttributeData ProjectileInitialVelocityRate;

}
#endif // OFFSETS_H
