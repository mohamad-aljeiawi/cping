#ifndef OFFSETS_H
#define OFFSETS_H

#include <cstdint>

namespace Offset
{
    //-- Base Offset G-Engine
    constexpr uintptr_t g_engine = 0xEA10468;

    //-- Base Offset UWorld

    constexpr uintptr_t u_level_to_a_actors = 0xa0;
    constexpr uintptr_t component_to_world = 0x210;

    //-- class UEngine : public UObject
    constexpr uintptr_t game_viewport = 0x810; // UGameViewportClient* GameViewport;

    //-- class UGameViewportClient : public UScriptViewportClient
    constexpr uintptr_t world = 0x78; // UWorld* World;

    //-- class UWorld : public UObject
    constexpr uintptr_t persistent_level = 0x30; // ULevel* PersistentLevel;
    constexpr uintptr_t net_driver = 0x38; // UNetDriver* NetDriver;

    //-- class UNetDriver : public UObject
    constexpr uintptr_t server_connection = 0x78; // UNetConnection* ServerConnection;

    //-- class UPlayer : public UObject
    constexpr uintptr_t player_controller = 0x30; // APlayerController* PlayerController;

    //-- class AActor : public UObject
    constexpr uintptr_t root_component = 0x208; // USceneComponent* RootComponent;
    constexpr uintptr_t replicated_movement = 0x110; // FRepMovement ReplicatedMovement; (Location at +0x18)
    constexpr uintptr_t replicated_movement_location = replicated_movement + 0x18; // FVector (0x128)

    //-- class USceneComponent (mesh inherits) – render-suppression discriminators
    constexpr uintptr_t scene_visibility_byte = 0x2dc; // bHiddenInGame (bit0) | bVisible (bit1)
    constexpr uintptr_t relative_location = 0x1e4;     // FVector USceneComponent::RelativeLocation
    constexpr uintptr_t relative_rotation = 0x1f0;     // FRotator USceneComponent::RelativeRotation
    constexpr uintptr_t relative_scale3d  = 0x1fc;     // FVector USceneComponent::RelativeScale3D

    //-- class APlayerController : public AController
    constexpr uintptr_t acknowledged_pawn = 0x528; // APawn* AcknowledgedPawn;
    constexpr uintptr_t player_camera_manager = 0x548; // APlayerCameraManager* PlayerCameraManager;

    //-- class APlayerCameraManager : public AActor
    constexpr uintptr_t camera_cache = 0x520; // FCameraCacheEntry CameraCache;

    //-- class ACharacter : public APawn
    constexpr uintptr_t mesh = 0x510; // USkeletalMeshComponent* Mesh;
    constexpr uintptr_t character_movement = 0x518; // UCharacterMovementComponent* CharacterMovement;
    constexpr uintptr_t capsule_component = 0x520; // UCapsuleComponent* CapsuleComponent;

    //-- class USkinnedMeshComponent : public UMeshComponent
    constexpr uintptr_t cached_local_bounds = 0xaec; // FBoxSphereBounds CachedLocalBounds;

    //-- class UStaticMeshComponent : public UMeshComponent
    constexpr uintptr_t static_mesh = 0x990; // UStaticMesh* StaticMesh;

    //-- class UCapsuleComponent : public UShapeComponent
    constexpr uintptr_t capsule_half_height = 0x958; // float CapsuleHalfHeight;
    constexpr uintptr_t capsule_radius = 0x95c; // float CapsuleRadius;

    //-- class UMovementComponent : public UActorComponent
    constexpr uintptr_t velocity = 0x18c; // Vector Velocity;

    //-- class UCharacterMovementComponent : public UPawnMovementComponent
    constexpr uintptr_t gravity_scale = 0x214; // float GravityScale;
    constexpr uintptr_t acceleration = 0x308; // FVector Acceleration;

    //-- class AUAECharacter : public ACharacter
    constexpr uintptr_t player_name = 0x960; // FString PlayerName;
    constexpr uintptr_t team_id = 0x998; // int32 TeamID;
    constexpr uintptr_t bis_ai = 0xa59; // bool bEnsure;

    //-- class AUAEPlayerController : public ALuaPlayerController
    constexpr uintptr_t team_id_local = 0x940; // int32 TeamID;

    //-- class ASTExtraCharacter : public AUAECharacter
    constexpr uintptr_t health = 0xe60; // float Health;
    constexpr uintptr_t bis_dead = 0xe7c; // uint8 bDead;
    constexpr uintptr_t current_vehicle = 0xeb0; // ASTExtraVehicleBase* CurrentVehicle;
    constexpr uintptr_t current_states = 0x1058; // uint64 CurrentStates;
    constexpr uintptr_t b_is_gun_ads = 0x1134;  // bool bIsGunADS;

    //-- class ASTExtraBaseCharacter : public ASTExtraCharacter
    constexpr uintptr_t weapon_manager = 0x25b8; // UCharacterWeaponManagerComponent* WeaponManagerComponent;

    //-- class UWeaponManagerComponent : public UActorComponent
    constexpr uintptr_t current_weapon = 0x5c8; // ASTExtraWeapon* CurrentWeaponReplicated;

    //-- class ASTExtraWeapon : public ALuaActor
    constexpr uintptr_t weapon_entity = 0x7e0; // UWeaponEntity* WeaponEntityComp;

    //-- class UWeaponEntity : public UWeaponLogicBaseComponent
    constexpr uintptr_t weapon_id = 0x1e0; // int32 WeaponId;

    //-- class UShootWeaponEntity : public UWeaponEntity
    constexpr uintptr_t bullet_speed = 0x560; // float BulletFireSpeed;
    constexpr uintptr_t b_has_auto_fire_mode = 0x601; // bool bHasAutoFireMode;
    constexpr uintptr_t recoil_kick = 0xcec; // float RecoilKick;
    constexpr uintptr_t recoil_kick_ads = 0xcf0; // float RecoilKickADS;
    constexpr uintptr_t accessories_v_recoil_factor = 0xbc8; // float AccessoriesVRecoilFactor;
    constexpr uintptr_t accessories_h_recoil_factor = 0xbd0; // float AccessoriesHRecoilFactor;

}
#endif // OFFSETS_H
