# Teleport-Decoy Filter — Design Document

## 1. Problem

The anti-cheat corrupts enemy pawn data in memory to produce false positions for cheats that render from those values. On screen this appears as player boxes, name tags, and aim anchors jumping to wrong positions. The filter must detect corrupt frames and suppress them without altering clean-frame behavior.

## 2. Attack surfaces

The AC attacks **three independent memory regions** on each pawn:

| Surface | Location | Trigger | Magnitude |
|---|---|---|---|
| **Root ComponentToWorld** | `actor->root + 0x210 + 0x10` | Always | 150–250 m XY decoy |
| **Mesh ComponentToWorld** | `actor->mesh + 0x210` | **On-screen only** (hooked `UpdateComponentToWorld`) | 0–few-m XY perturbation |
| **Bone array** | `actor->mesh + 0x990 + idx·0x30` | With mesh spoof | Small bone-translation drift |

All three preserve Z. Only X and Y are modified. The AC does not touch `FRepMovement.Location`, `CharacterMovement->LastUpdateLocation`, or the `CurrentVehicle` actor's own root — these fields feed netcode and client prediction, so corrupting them would break the game for legit clients.

Pattern is transient: corrupt frames are interleaved with clean frames at roughly 1 Hz per attacked actor. Clean frames dominate.

## 3. Location-like fields in the SDK

Class chain for enemy pawns: `AActor → APawn → ACharacter → AUAECharacter → ASTExtraCharacter`.

| Source | Access | Offset | Update rate | Trust |
|---|---|---|---|---|
| Root translation (attacked) | `actor+0x208 → +0x210+0x10` | — | physics tick | **spoofed** |
| FRepMovement.Location (REP) | `actor+0x128` | 0x128 | network tick | clean, lagged |
| CMC → LastUpdateLocation (LUL) | `actor+0x518 → +0x314` | — | physics tick | clean, freezes in vehicle |
| CurrentVehicle → Root → CTW.T (VEH) | `actor+0xEB0 → +0x208 → +0x210+0x10` | — | physics tick | clean (vehicle is a separate actor) |
| ReplicatedBasedMovement | `actor+0x56c` | 0x56c | network tick | unreliable, drop |

Struct layouts (from `Structs.hpp`):

```
FRepMovement (0x48)
    0x00  FVector    LinearVelocity
    0x0C  FVector    AngularVelocity
    0x18  FVector    Location        ← used
    0x24  FRotator   Rotation

FBasedMovementInfo (0x30)
    0x00  UPrimitiveComponent*   MovementBase    (null = world-space)
    0x14  FVector_NetQuantize100 Location
```

## 4. Reference comparison

Measured against `raw` (root translation):

| Reference | Clean-frame gap | On decoy frame | In vehicle | Lag | Reads |
|---|---|---|---|---|---|
| LUL | 0 cm (exact) | stays at real | **freezes at attach** | 0 | 1 ptr + 1 FVector |
| VEH | 30–50 cm XY / –180 Z | live vehicle pos | tracks correctly | 0 | 2 ptr + 1 FVector |
| REP | 20–70 cm (net-tick) | untouched | **freezes at attach** | 50–150 ms | 1 FVector |

**Selection:** VEH when in vehicle, else LUL, else REP as last-resort fallback.

## 5. Why direct snap-on-detect is insufficient

`player_obj` is built from multiple independent world reads:

```
transform         ← root CTW
transform_bounds  ← mesh CTW
transform_mech    ← mesh CTW
head_position     = transform_mech × head bone
root_position     = transform_mech × root bone
corners_world[8]  = transform_bounds × local corners
```

Patching only `transform.Translation` leaves mesh-derived fields (box corners, head, root) coming from an independent surface that the AC also attacks. Result: mixed-source visual artifacts — box in one place, name tag in another.

## 6. Solution — per-actor snapshot retention with three validation gates

Keep the full world-space read-set from the most recent clean frame. On any corrupt frame, replay the whole snapshot. Every field rendered stays from the same validated moment.

### 6.1 Per-frame algorithm

```
for each actor:
    # existing early rejects (dead, team, name) …

    raw = read(root CTW translation)
    lul = CMC ? read(CMC + LastUpdateLocation)           : 0
    veh = vehicle ? read(vehicle_root CTW translation)   : 0

    # Death / exit
    if raw == 0 && lul == 0 && veh == 0:
        store.remove(actor); continue

    # Select reference
    reference = in_vehicle ? veh : lul ?: rep ?: none

    # ─── Gate 1: root vs reference (XY ≤ 300 cm) ───
    root_clean = (reference is none) ||
                 xy_dist(raw, reference) ≤ 300

    snapshot_valid = false
    if root_clean:
        mesh           = read(actor + mesh_ptr)
        bounds_local   = read(mesh + cached_local_bounds)
        transform_mesh = read(mesh + CTW)

        # ─── Gate 2: mesh vs root (XY ≤ 50 cm) ───
        if xy_dist(transform_mesh.Translation, raw) ≤ 50:

            # Reuse transform_mesh for both corners and bones
            # (one read, no inter-read race window)
            head = transform_mesh × read_bone(6).Translation
            root = transform_mesh × read_bone(0).Translation

            # ─── Gate 3: bone anchors vs root (XY ≤ 150 cm) ───
            if xy_dist(head, raw) ≤ 150 && xy_dist(root, raw) ≤ 150:
                snapshot = {
                    translation:      raw,
                    head_position:    head,
                    root_position:    root,
                    world_corners[8]: transform_mesh × (local_corners + origin),
                    target_velocity:  read(CMC + velocity),
                    target_acceleration: read(CMC + acceleration)
                }
                store.set(actor, snapshot)
                snapshot_valid = true

    if !snapshot_valid:
        if store.has(actor):
            snapshot = store.get(actor)
        else:
            continue   # first sighting already corrupt

    # Project to screen with current camera (never store screen-space)
    player_obj.location        = world_to_screen(snapshot.translation)
    player_obj.head            = world_to_screen(snapshot.head_position)
    player_obj.root            = world_to_screen(snapshot.root_position)
    for i in 0..7:
        player_obj.bounds[i]   = world_to_screen(snapshot.world_corners[i])

    # Aimbot consumes snapshot.head_position / velocity / acceleration
```

### 6.2 Snapshot contents

Only world-space. **Never store screen-space** — it depends on the current camera.

```cpp
struct ActorSnapshot {
    FVector translation;            // 12 B — root world translation
    FVector world_corners[8];       // 96 B — pre-transformed box corners
    FVector head_position;          // 12 B — head bone, world
    FVector root_position;          // 12 B — feet bone, world
    FVector target_velocity;        // 12 B — ballistics
    FVector target_acceleration;    // 12 B
};                                  // ~156 B per actor ≈ 15 KB for 100 actors
```

### 6.3 Storage options

Logic is identical across these; only the lookup primitive changes.

1. `std::unordered_map<uintptr_t, ActorSnapshot>` — O(1), one heap alloc per new actor.
2. Flat array + linear scan — O(N), N ≤ 100, cache-friendly.
3. Previous frame's output buffer (add `actor_ptr` to `Structs::Player`) — no new structure, linear scan over live enemy count.

### 6.4 Edge cases

| Situation | Signal | Response |
|---|---|---|
| Pawn died / disconnected | `raw == lul == veh == 0` | Erase store entry; `continue`. |
| First sighting already corrupt | `!store.has(actor)` and any gate fails | Skip this actor this frame. |
| CMC pointer null | `cmc_ptr == 0` | Reference falls through to REP. |
| Non-character passing name/health filters | `lul == rep == 0`, `raw != 0` | No reference → `root_clean = true`; trust raw. |

### 6.5 Thresholds

| Gate | Threshold | Clean-frame gap (measured) | Rationale |
|---|---|---|---|
| Root vs reference (XY) | **300 cm** | 0–50 cm | > 6× worst clean gap, ≪ smallest observed decoy (15 000 cm). |
| Mesh vs root (XY) | **50 cm** | 0–10 cm | Mesh is a child of capsule root; ~0 cm natural offset. |
| Bone anchor vs root (XY) | **150 cm** | 0–30 cm | Head and feet constrained to a vertical cylinder around the pawn. |

## 7. Advantages

1. **Internally consistent data** — every rendered field descends from the same validated moment.
2. **No semantic change for the aimbot** — target comes from the same read chain, just occasionally one clean frame old.
3. **Seat height preserved in vehicles** — `raw.Z` captured at seat height survives into decoy frames; VEH's chassis-Z never enters the pipeline.
4. **Fewer reads on corrupt frames** — skips mesh / bone / velocity / acceleration reads entirely.
5. **Zero overhead on clean frames** — only added cost is LUL/VEH/REP reads (cheap).

## 8. Drawbacks

1. **Staleness during sustained corruption bursts** — snapshot ages if many consecutive frames fail the gates. Clean frames dominate in practice.
2. **First-sighting bootstrap gap** — one dropped frame per actor if first scan is already corrupt.
3. **Unbounded growth if actors leave without hitting all-zeros** — bounded by match size (~100); add TTL if needed.

## 9. Offsets required

```cpp
namespace Offset {
    //-- AActor
    constexpr uintptr_t root_component               = 0x208;
    constexpr uintptr_t replicated_movement_location = 0x128;   // FRepMovement.Location

    //-- USceneComponent
    constexpr uintptr_t component_to_world           = 0x210;
    // FTransform.Translation = +0x10 inside the FTransform (FQuaternion precedes it)

    //-- ACharacter
    constexpr uintptr_t mesh                         = 0x510;
    constexpr uintptr_t character_movement           = 0x518;

    //-- UCharacterMovementComponent
    constexpr uintptr_t last_update_location         = 0x314;
    constexpr uintptr_t velocity                     = 0x18c;
    constexpr uintptr_t acceleration                 = 0x308;

    //-- USkinnedMeshComponent
    constexpr uintptr_t cached_local_bounds          = 0xaec;
    constexpr uintptr_t static_mesh                  = 0x990;   // bone array base

    //-- ASTExtraCharacter
    constexpr uintptr_t current_vehicle              = 0xeb0;
}
```

## 10. Implementation checklist

- [ ] Define `struct ActorSnapshot` (top of `main.cpp` or in `structs.h`).
- [ ] Declare the store `static` inside the scanning function (container per §6.3).
- [ ] After reading `transform`, read LUL / VEH / REP.
- [ ] All-zeros erase rule.
- [ ] **Gate 1** — root vs reference (300 cm).
- [ ] **Gate 2** — `transform_bounds.Translation` vs `raw` (50 cm).
- [ ] Reuse `transform_bounds` as `transform_mech` (do not re-read mesh CTW).
- [ ] **Gate 3** — computed `head_position` and `root_position` vs `raw` (150 cm each).
- [ ] Store the snapshot only when all three gates pass; otherwise fall through to cached.
- [ ] Downstream consumes snapshot world values; `world_to_screen` runs every frame with current camera.

## 11. Out of scope

- **Ghost actors** — separate decoy actor instances in the UWorld list with their own valid LUL/VEH. This filter cannot distinguish them; requires a render-suppression or proximity-dedupe detector.
- **Bone index 6 resolving to `UStaticMesh*` on some PUBG Mobile builds** — affects head-anchor accuracy for health bars; independent of decoy filtering.
