#ifndef OFFSETS_H
#define OFFSETS_H

#include <cstdint>

namespace Offset
{
    //-- Base Offset G-Objects
    constexpr uintptr_t g_world = 0xEA59FA8;

    //-- Base Offset UWorld
    constexpr uintptr_t persistent_level = 0x30;
    constexpr uintptr_t u_level_to_a_actors = 0xA0;
    constexpr uintptr_t component_to_world = 0x1B0;

    //-- Class: Actor.Object
    constexpr uintptr_t root_component = 0x1B0; // SceneComponent* RootComponent;

    // Class: PlayerController.Controller.Actor.Object
    constexpr uintptr_t player_camera_manager = 0x4f0; // PlayerCameraManager* PlayerCameraManager;
    constexpr uintptr_t acknowledged_pawn = 0x4d0;     // Pawn* AcknowledgedPawn;

    // Class: PlayerCameraManager.Actor.Object
    constexpr uintptr_t camera_cache = 0x4d0; // CameraCacheEntry CameraCache;

    // Class: Character.Pawn.Actor.Object
    constexpr uintptr_t mesh = 0x4b8;               // SkeletalMeshComponent* Mesh;
    constexpr uintptr_t character_movement = 0x4c0; // CharacterMovementComponent* CharacterMovement
    constexpr uintptr_t capsule_component = 0x4c8;  // CapsuleComponent* CapsuleComponent;

    // Class: SkinnedMeshComponent.MeshComponent.PrimitiveComponent.SceneComponent.ActorComponent.Object
    constexpr uintptr_t cached_local_bounds = 0xa24; // BoxSphereBounds CachedLocalBounds;

    //-- Class: StaticMeshComponent.MeshComponent.PrimitiveComponent.SceneComponent.ActorComponent.Object
    constexpr uintptr_t static_mesh = 0x8c8; // StaticMesh* StaticMesh;

    // Class: CapsuleComponent.ShapeComponent.PrimitiveComponent.SceneComponent.ActorComponent.Object
    constexpr uintptr_t capsule_half_height = 0x890; // float CapsuleHalfHeight;
    constexpr uintptr_t capsule_radius = 0x894;      // float CapsuleRadius;

    // Class: MovementComponent.ActorComponent.Object
    constexpr uintptr_t velocity = 0x12c; // Vector Velocity;

    // Class: CharacterMovementComponent.PawnMovementComponent.NavMovementComponent.MovementComponent.ActorComponent.Object
    constexpr uintptr_t gravity_scale = 0x1b4; // float GravityScale;
    constexpr uintptr_t acceleration = 0x2a8;  // Vector Acceleration;

    //-- Class: UAECharacter.Character.Pawn.Actor.Object
    constexpr uintptr_t player_name = 0x910; // FString PlayerName;
    constexpr uintptr_t team_id = 0x948;     // int TeamID;
    constexpr uintptr_t bis_ai = 0x9f9;      // bool bEnsure;

    //-- Class: UAEPlayerController
    constexpr uintptr_t team_id_local = 0x8e8; // int TeamID;

    //-- Class: STExtraCharacter.UAECharacter.Character.Pawn.Actor.Object
    constexpr uintptr_t health = 0xdc8;         // float Health;
    constexpr uintptr_t bis_dead = 0xde4;       // bool bDead;
    constexpr uintptr_t current_states = 0xfb8; // unsigned long long CurrentStates; or uint64 CurrentStates;
    constexpr uintptr_t b_is_gun_ads = 0x1081;  // bool bIsGunADS;

    // Class: STExtraBaseCharacter.STExtraCharacter.UAECharacter.Character.Pawn.Actor.Object
    constexpr uintptr_t weapon_manager = 0x2508; // CharacterWeaponManagerComponent* WeaponManagerComponent;

    // Class: WeaponManagerComponent.ActorComponent.Object
    constexpr uintptr_t current_weapon = 0x558; // STExtraWeapon* CurrentWeaponReplicated;

    // Class: STExtraWeapon.LuaActor.Actor.Object
    constexpr uintptr_t weapon_entity = 0x788; // WeaponEntity* WeaponEntityComp;

    // Class: WeaponEntity.WeaponLogicBaseComponent.ActorComponent.Object
    constexpr uintptr_t weapon_id = 0x178; // int WeaponId;

    // Class: ShootWeaponEntity.WeaponEntity.WeaponLogicBaseComponent.ActorComponent.Object
    constexpr uintptr_t bullet_speed = 0x4e0;                // float BulletFireSpeed;
    constexpr uintptr_t b_has_auto_fire_mode = 0x581;        // bool bHasAutoFireMode;
    constexpr uintptr_t recoil_kick = 0xc6c;                 // float RecoilKick;
    constexpr uintptr_t recoil_kick_ads = 0xc70;             // float RecoilKickADS;
    constexpr uintptr_t accessories_v_recoil_factor = 0x128; // float AccessoriesVRecoilFactor;
    constexpr uintptr_t accessories_h_recoil_factor = 0x12c; // float AccessoriesHRecoilFactor;

}
#endif // OFFSETS_H
