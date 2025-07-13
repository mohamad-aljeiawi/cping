#ifndef OFFSETS_H
#define OFFSETS_H

#include <cstdint>

namespace Offset
{
    //-- Base Offset G-Objects
    constexpr uintptr_t g_world = 0xE793470;

    //-- Base Offset UWorld
    constexpr uintptr_t persistent_level = 0x30;
    constexpr uintptr_t u_level_to_a_actors = 0xA0;
    constexpr uintptr_t component_to_world = 0x1B0;

    //-- Class: Actor.Object
    constexpr uintptr_t root_component = 0x1B0; // SceneComponent* RootComponent;

    // Class: PlayerController.Controller.Actor.Object
    constexpr uintptr_t player_camera_manager = 0x4D0; // PlayerCameraManager* PlayerCameraManager;
    constexpr uintptr_t acknowledged_pawn = 0x4B0;     // Pawn* AcknowledgedPawn;

    // Class: PlayerCameraManager.Actor.Object
    constexpr uintptr_t camera_cache = 0x4B0; // CameraCacheEntry CameraCache;

    // Class: Character.Pawn.Actor.Object
    constexpr uintptr_t mesh = 0x498;               // SkeletalMeshComponent* Mesh;
    constexpr uintptr_t character_movement = 0x4A0; // CharacterMovementComponent* CharacterMovement
    constexpr uintptr_t capsule_component = 0x4A8;  // CapsuleComponent* CapsuleComponent;

    // Class: SkinnedMeshComponent.MeshComponent.PrimitiveComponent.SceneComponent.ActorComponent.Object
    constexpr uintptr_t cached_local_bounds = 0xA04; // BoxSphereBounds CachedLocalBounds;

    //-- Class: StaticMeshComponent.MeshComponent.PrimitiveComponent.SceneComponent.ActorComponent.Object
    constexpr uintptr_t static_mesh = 0x8A8; // StaticMesh* StaticMesh;

    // Class: CapsuleComponent.ShapeComponent.PrimitiveComponent.SceneComponent.ActorComponent.Object
    constexpr uintptr_t capsule_half_height = 0x878; // float CapsuleHalfHeight;
    constexpr uintptr_t capsule_radius = 0x87C;      // float CapsuleRadius;

    // Class: MovementComponent.ActorComponent.Object
    constexpr uintptr_t velocity = 0x12C; // Vector Velocity;

    // Class: CharacterMovementComponent.PawnMovementComponent.NavMovementComponent.MovementComponent.ActorComponent.Object
    constexpr uintptr_t gravity_scale = 0x1B4; // float GravityScale;
    constexpr uintptr_t acceleration = 0x2A8;  // Vector Acceleration;

    //-- Class: UAECharacter.Character.Pawn.Actor.Object
    constexpr uintptr_t player_name = 0x8F0; // FString PlayerName;
    constexpr uintptr_t team_id = 0x928;     // int TeamID;
    constexpr uintptr_t bis_ai = 0x9D9;      // bool bEnsure;

    //-- Class: STExtraCharacter.UAECharacter.Character.Pawn.Actor.Object
    constexpr uintptr_t health = 0xDB0;         // float Health;
    constexpr uintptr_t bis_dead = 0xDCC;       // bool bDead;
    constexpr uintptr_t current_states = 0xFA0; // unsigned long long CurrentStates;
    constexpr uintptr_t b_is_gun_ads = 0x1069;  // bool bIsGunADS;

    // Class: STExtraBaseCharacter.STExtraCharacter.UAECharacter.Character.Pawn.Actor.Object
    constexpr uintptr_t weapon_manager = 0x24D8; // CharacterWeaponManagerComponent* WeaponManagerComponent;

    // Class: WeaponManagerComponent.ActorComponent.Object
    constexpr uintptr_t current_weapon = 0x558; // STExtraWeapon* CurrentWeaponReplicated;

    // Class: STExtraWeapon.LuaActor.Actor.Object
    constexpr uintptr_t weapon_entity = 0x770; // WeaponEntity* WeaponEntityComp;

    // Class: WeaponEntity.WeaponLogicBaseComponent.ActorComponent.Object
    constexpr uintptr_t weapon_id = 0x178; // int WeaponId;

    // Class: ShootWeaponEntity.WeaponEntity.WeaponLogicBaseComponent.ActorComponent.Object
    constexpr uintptr_t bullet_speed = 0x4C0;                // float BulletFireSpeed;
    constexpr uintptr_t b_has_auto_fire_mode = 0x559;        // bool bHasAutoFireMode;
    constexpr uintptr_t recoil_kick = 0xBF4;                 // float RecoilKick;
    constexpr uintptr_t recoil_kick_ads = 0xBF8;             // float RecoilKickADS;
    constexpr uintptr_t accessories_v_recoil_factor = 0xAD0; // float AccessoriesVRecoilFactor;
    constexpr uintptr_t accessories_h_recoil_factor = 0xAD8; // float AccessoriesHRecoilFactor;

}
#endif // OFFSETS_H
