#ifndef OFFSETS_H
#define OFFSETS_H

#include <cstdint>

namespace Offset
{
    //-- Base Offset G-Objects
    constexpr uintptr_t g_world = 0xEA54A70;

    //-- Base Offset UWorld
    constexpr uintptr_t persistent_level = 0x30;
    constexpr uintptr_t u_level_to_a_actors = 0xA0;
    constexpr uintptr_t component_to_world = 0x1B0;

    //-- Class: Actor.Object
    constexpr uintptr_t root_component = 0x1B0; // SceneComponent* RootComponent;

    // Class: PlayerController.Controller.Actor.Object
    constexpr uintptr_t player_camera_manager = 0x4e0; // PlayerCameraManager* PlayerCameraManager;
    constexpr uintptr_t acknowledged_pawn = 0x4c0;     // Pawn* AcknowledgedPawn;

    // Class: PlayerCameraManager.Actor.Object
    constexpr uintptr_t camera_cache = 0x4c0; // CameraCacheEntry CameraCache;

    // Class: Character.Pawn.Actor.Object
    constexpr uintptr_t mesh = 0x4a8;               // SkeletalMeshComponent* Mesh;
    constexpr uintptr_t character_movement = 0x4b0; // CharacterMovementComponent* CharacterMovement
    constexpr uintptr_t capsule_component = 0x4b8;  // CapsuleComponent* CapsuleComponent;

    // Class: SkinnedMeshComponent.MeshComponent.PrimitiveComponent.SceneComponent.ActorComponent.Object
    constexpr uintptr_t cached_local_bounds = 0xa04; // BoxSphereBounds CachedLocalBounds;

    //-- Class: StaticMeshComponent.MeshComponent.PrimitiveComponent.SceneComponent.ActorComponent.Object
    constexpr uintptr_t static_mesh = 0x8a8; // StaticMesh* StaticMesh;

    // Class: CapsuleComponent.ShapeComponent.PrimitiveComponent.SceneComponent.ActorComponent.Object
    constexpr uintptr_t capsule_half_height = 0x878; // float CapsuleHalfHeight;
    constexpr uintptr_t capsule_radius = 0x87c;      // float CapsuleRadius;

    // Class: MovementComponent.ActorComponent.Object
    constexpr uintptr_t velocity = 0x12c; // Vector Velocity;

    // Class: CharacterMovementComponent.PawnMovementComponent.NavMovementComponent.MovementComponent.ActorComponent.Object
    constexpr uintptr_t gravity_scale = 0x1b4; // float GravityScale;
    constexpr uintptr_t acceleration = 0x2a8;  // Vector Acceleration;

    //-- Class: UAECharacter.Character.Pawn.Actor.Object
    constexpr uintptr_t player_name = 0x900; // FString PlayerName;
    constexpr uintptr_t team_id = 0x938;     // int TeamID;
    constexpr uintptr_t bis_ai = 0x9e9;      // bool bEnsure;

    // //-- Class: UAEPlayerController
    constexpr uintptr_t team_id_local = 0x8d8; // int TeamID;

    //-- Class: STExtraCharacter.UAECharacter.Character.Pawn.Actor.Object
    constexpr uintptr_t health = 0xdb8;         // float Health;
    constexpr uintptr_t bis_dead = 0xdd4;       // bool bDead;
    constexpr uintptr_t current_states = 0xfa8; // unsigned long long CurrentStates; or uint64 CurrentStates;
    constexpr uintptr_t b_is_gun_ads = 0x1071;  // bool bIsGunADS;

    // Class: STExtraBaseCharacter.STExtraCharacter.UAECharacter.Character.Pawn.Actor.Object
    constexpr uintptr_t weapon_manager = 0x2528; // CharacterWeaponManagerComponent* WeaponManagerComponent;

    // Class: WeaponManagerComponent.ActorComponent.Object
    constexpr uintptr_t current_weapon = 0x558; // STExtraWeapon* CurrentWeaponReplicated;

    // Class: STExtraWeapon.LuaActor.Actor.Object
    constexpr uintptr_t weapon_entity = 0x780; // WeaponEntity* WeaponEntityComp;

    // Class: WeaponEntity.WeaponLogicBaseComponent.ActorComponent.Object
    constexpr uintptr_t weapon_id = 0x178; // int WeaponId;

    // Class: ShootWeaponEntity.WeaponEntity.WeaponLogicBaseComponent.ActorComponent.Object
    constexpr uintptr_t bullet_speed = 0x4c0;                // float BulletFireSpeed;
    constexpr uintptr_t b_has_auto_fire_mode = 0x561;        // bool bHasAutoFireMode;
    constexpr uintptr_t recoil_kick = 0xc4c;                 // float RecoilKick;
    constexpr uintptr_t recoil_kick_ads = 0xc50;             // float RecoilKickADS;
    constexpr uintptr_t accessories_v_recoil_factor = 0x128; // float AccessoriesVRecoilFactor;
    constexpr uintptr_t accessories_h_recoil_factor = 0x12c; // float AccessoriesHRecoilFactor;

}
#endif // OFFSETS_H
