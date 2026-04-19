# Teleport-Decoy Filter — Design Document

If you are new to memory cheats, here is the short version of what this document is about.

A game like PUBG keeps every player's position in memory. A cheat reads those positions and draws boxes, names, and aim markers on top of the screen. To fight this, the anti-cheat (AC) regularly **writes fake positions** on top of the real ones for a single frame, then lets the real value come back. A naive cheat reads the fake value, so the box suddenly teleports hundreds of meters away and then snaps back. This is what players call a **"decoy"** or **"teleport decoy."**

The goal of this document is to describe a **filter** that:

1. Recognizes a corrupted (decoy) frame when it happens.
2. Throws that frame away.
3. Reuses the last good, verified data, so the visuals stay calm and the aim keeps working.

The approach is simple in spirit: on every good frame, take a full **snapshot** of everything we need about a player. On a bad frame, **replay the last snapshot** instead of using the bad data. The rest of the document explains how to tell a good frame from a bad one, which memory to read, and why.

**Glossary used throughout this text:**

- **actor** — an in-game object (a player pawn, a vehicle, etc.).
- **pawn** — the character object that represents a player in the world.
- **offset** — a fixed number of bytes from the start of an object to one of its fields.
- **XY** — horizontal plane. **Z** — vertical axis (height).
- **CTW** — `ComponentToWorld`, a transform (position + rotation + scale) in world space.
- **CMC** — `CharacterMovementComponent` (Unreal's movement component).
- **REP** — replicated, i.e. sent over the network from the server.

## 1. Problem

The anti-cheat corrupts enemy pawn data in memory to produce false positions for cheats that render from those values. On screen this appears as player boxes, name tags, and aim anchors jumping to wrong positions. The filter must detect corrupt frames and suppress them without altering clean-frame behavior.

*In other words:* the AC does not ban you, it lies to you. It changes a couple of numbers in RAM for one frame so that whatever reads those numbers draws garbage. Our job is to notice the lie, ignore that frame, and keep using the last truthful numbers we saw.

## 2. Attack surfaces

The AC attacks **three independent memory regions** on each pawn:


| Surface                   | Location                         | Trigger                                              | Magnitude                    |
| ------------------------- | -------------------------------- | ---------------------------------------------------- | ---------------------------- |
| **Root ComponentToWorld** | `actor->root + 0x210 + 0x10`     | Always                                               | 150–250 m XY decoy           |
| **Mesh ComponentToWorld** | `actor->mesh + 0x210`            | **On-screen only** (hooked `UpdateComponentToWorld`) | 0–few-m XY perturbation      |
| **Bone array**            | `actor->mesh + 0x990 + idx·0x30` | With mesh spoof                                      | Small bone-translation drift |


**How to read this table:**

- **Root ComponentToWorld** is the pawn's root transform. This is the big, loud decoy: the player gets teleported 150–250 meters away on XY. It happens all the time, for every enemy the cheat knows about.
- **Mesh ComponentToWorld** is the transform of the character's skinned mesh. It is only touched when the pawn is on screen (the AC hooks the renderer's `UpdateComponentToWorld` / UCTW). The shift is small, maybe a few meters, and its job is to desync the box/corners from the root.
- **Bone array** is the list of per-bone transforms inside the mesh. When the mesh itself is spoofed, bones drift a little too. This breaks head/feet anchors that cheats compute from bones.

All three preserve Z. Only X and Y are modified. The AC does not touch `FRepMovement.Location`, `CharacterMovement->LastUpdateLocation`, or the `CurrentVehicle` actor's own root — these fields feed netcode and client prediction, so corrupting them would break the game for legit clients.

*Why does this matter?* It gives us **honest mirrors**. If the AC tampered with the replicated location or the movement component, every legitimate player's game would rubber-band. The AC can't afford that. So we always have at least one clean reference we can compare the suspicious value against.

Pattern is transient: corrupt frames are interleaved with clean frames at roughly 1 Hz per attacked actor. Clean frames dominate.

*Translation:* roughly once per second per visible enemy, you get one bad frame surrounded by many good frames. That is exactly why a "remember the last good frame" strategy works so well.

## 3. Location-like fields in the SDK

Class chain for enemy pawns: `AActor → APawn → ACharacter → AUAECharacter → ASTExtraCharacter`.

*Reminder:* each class in this chain inherits the fields of its parent, so an `ASTExtraCharacter` has all fields of `AActor`, `APawn`, `ACharacter`, and `AUAECharacter` too. The offsets below are relative to the start of the actor object.


| Source                              | Access                               | Offset | Update rate  | Trust                               |
| ----------------------------------- | ------------------------------------ | ------ | ------------ | ----------------------------------- |
| Root translation (attacked)         | `actor+0x208 → +0x210+0x10`          | —      | physics tick | **spoofed**                         |
| FRepMovement.Location (REP)         | `actor+0x128`                        | 0x128  | network tick | clean, lagged                       |
| CMC → LastUpdateLocation (LUL)      | `actor+0x518 → +0x314`               | —      | physics tick | clean, freezes in vehicle           |
| CurrentVehicle → Root → CTW.T (VEH) | `actor+0xEB0 → +0x208 → +0x210+0x10` | —      | physics tick | clean (vehicle is a separate actor) |
| ReplicatedBasedMovement             | `actor+0x56c`                        | 0x56c  | network tick | unreliable, drop                    |


**How to read the "Access" column:** each arrow (`→`) means "follow the pointer". For example, `actor+0x208 → +0x210+0x10` means: read a pointer at `actor+0x208` (root component), then go `0x210` bytes into that object (`ComponentToWorld`), then `0x10` bytes further (`Translation` inside `FTransform`).

**Update rate:**

- **physics tick** — updated every frame by the engine. Current.
- **network tick** — updated only when a packet arrives from the server. A little behind.

**Trust:**

- **spoofed** — the AC writes over it. Never trust directly.
- **clean** — the AC does not touch it. Good reference.
- **unreliable** — we simply don't use it; keep it in the table so readers know we considered it.

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

So when the table says `actor+0x128` for `FRepMovement.Location`, it actually means "`FRepMovement` struct starts at `0x110` and `Location` is at `+0x18` inside it" (`0x110 + 0x18 = 0x128`). We skip directly to the final field.

## 4. Reference comparison

We need a **reference value** — something we trust — to compare the suspicious root translation against. Three candidates are realistic: **LUL**, **VEH**, and **REP**. Here is how they behave.

Measured against `raw` (root translation):


| Reference | Clean-frame gap      | On decoy frame   | In vehicle            | Lag       | Reads             |
| --------- | -------------------- | ---------------- | --------------------- | --------- | ----------------- |
| LUL       | 0 cm (exact)         | stays at real    | **freezes at attach** | 0         | 1 ptr + 1 FVector |
| VEH       | 30–50 cm XY / –180 Z | live vehicle pos | tracks correctly      | 0         | 2 ptr + 1 FVector |
| REP       | 20–70 cm (net-tick)  | untouched        | **freezes at attach** | 50–150 ms | 1 FVector         |


**What the columns mean:**

- **Clean-frame gap** — how far the reference normally is from the real root on a good frame. Smaller is better.
- **On decoy frame** — what happens to the reference when the root is spoofed.
- **In vehicle** — special case: when a player is inside a car, the character's own transforms freeze at the seat position. Only the vehicle actor keeps moving.
- **Lag** — how far behind the reference can be. REP lags because it only updates on network ticks.
- **Reads** — how expensive the reference is. One extra `FVector` read is trivial; chasing multiple pointers costs more.

**Selection:** VEH when in vehicle, else LUL, else REP as last-resort fallback.

*Why this order?* On foot, **LUL** matches the root almost exactly every frame — it is the best possible anchor. In a vehicle, LUL freezes at the seat so it becomes useless for motion; the vehicle actor's own root is clean (the AC does not spoof vehicle roots) and it tracks the driver live. REP is the last fallback because it is always correct but slightly out of date.

## 5. Why direct snap-on-detect is insufficient

*Direct snap-on-detect* means: "if the root looks bad this frame, just overwrite it with the reference value and keep the rest." That sounds simpler than building a snapshot system. It does not work. Here is why.

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

*Concretely:* you would fix the root translation, but the mesh CTW and the bones were also tampered with (or read at a slightly different instant). The bounding box corners come from the mesh, the head anchor comes from a bone, and the root label comes from the root. They would not agree with each other on screen. Any fix therefore has to treat **all** the world reads for one player as a single atomic set — that is what the snapshot does.

## 6. Solution — per-actor snapshot retention with three validation gates

The idea in one sentence: **take a full, consistent copy of every world-space value we need from each player on every clean frame; on a dirty frame, replay the last copy.**

We accept or reject a frame using **three gates** — three cheap sanity checks, each comparing a suspicious value against a reference:

1. **Gate 1: root vs reference.** Is the root translation within 3 m of LUL/VEH/REP?
2. **Gate 2: mesh vs root.** Is the mesh transform within 0.5 m of the root?
3. **Gate 3: bones vs root.** Are the head and feet bones within 1.5 m of the root?

If all three pass, the frame is clean and we save a new snapshot. If any fails, we skip the reads we have not done yet and reuse the previous snapshot.

Keep the full world-space read-set from the most recent clean frame. On any corrupt frame, replay the whole snapshot. Every field rendered stays from the same validated moment.

### 6.1 Per-frame algorithm

Pseudocode below. Read the comments; the structure is: gate 1 → more reads → gate 2 → more reads → gate 3 → store snapshot; then always project the snapshot to screen.

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

*Two small but important details:*

- We **project to screen at the end** using the *current* camera, even when replaying an old snapshot. Screen coordinates depend on where the camera is right now, so storing them would instantly desync when the camera rotates.
- Gate 2 must be done **before** we use `transform_mesh` for bones and corners. Reading it once and reusing it avoids a race where the AC could spoof between two reads of the same field.

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

*Budget check:* about 156 bytes per tracked enemy. A full match has at most ~100 actors in view at once, so ~15 KB total. This is effectively free.

### 6.3 Storage options

Logic is identical across these; only the lookup primitive changes.

1. `std::unordered_map<uintptr_t, ActorSnapshot>` — O(1), one heap alloc per new actor.
2. Flat array + linear scan — O(N), N ≤ 100, cache-friendly.
3. Previous frame's output buffer (add `actor_ptr` to `Structs::Player`) — no new structure, linear scan over live enemy count.

*Recommendation for beginners:* pick option 2 or 3. With only ~100 actors, a flat array is just as fast as a hash map and does not allocate. Option 3 is the cleanest if you already double-buffer your player list.

### 6.4 Edge cases


| Situation                                 | Signal                                 | Response                                       |
| ----------------------------------------- | -------------------------------------- | ---------------------------------------------- |
| Pawn died / disconnected                  | `raw == lul == veh == 0`               | Erase store entry; `continue`.                 |
| First sighting already corrupt            | `!store.has(actor)` and any gate fails | Skip this actor this frame.                    |
| CMC pointer null                          | `cmc_ptr == 0`                         | Reference falls through to REP.                |
| Non-character passing name/health filters | `lul == rep == 0`, `raw != 0`          | No reference → `root_clean = true`; trust raw. |


A few words on the last row: some on-screen objects are not full `ACharacter` instances (pickups with health bars, decorative NPCs, etc.). They have no CMC and no replicated movement. For those, the only readable position is the root, and the AC does not spoof them anyway, so we accept the raw value.

### 6.5 Thresholds


| Gate                     | Threshold  | Clean-frame gap (measured) | Rationale                                                         |
| ------------------------ | ---------- | -------------------------- | ----------------------------------------------------------------- |
| Root vs reference (XY)   | **300 cm** | 0–50 cm                    | > 6× worst clean gap, ≪ smallest observed decoy (15 000 cm).      |
| Mesh vs root (XY)        | **50 cm**  | 0–10 cm                    | Mesh is a child of capsule root; ~0 cm natural offset.            |
| Bone anchor vs root (XY) | **150 cm** | 0–30 cm                    | Head and feet constrained to a vertical cylinder around the pawn. |


*Why these numbers?* Unreal uses centimeters. The worst natural gap between the root and any legitimate reference is around 50 cm (network jitter, tick interpolation). The smallest decoy observed is around 150 m = 15 000 cm. A 300 cm threshold sits comfortably between the two: six times larger than noise, fifty times smaller than the smallest attack. The same logic applies to the other two gates.

## 7. Advantages

1. **Internally consistent data** — every rendered field descends from the same validated moment.
2. **No semantic change for the aimbot** — target comes from the same read chain, just occasionally one clean frame old.
3. **Seat height preserved in vehicles** — `raw.Z` captured at seat height survives into decoy frames; VEH's chassis-Z never enters the pipeline.
4. **Fewer reads on corrupt frames** — skips mesh / bone / velocity / acceleration reads entirely.
5. **Zero overhead on clean frames** — only added cost is LUL/VEH/REP reads (cheap).

*Plain words:* good frames cost almost nothing extra, bad frames actually cost **less** than today because we bail out early, and the picture on screen never jumps again.

## 8. Drawbacks

1. **Staleness during sustained corruption bursts** — snapshot ages if many consecutive frames fail the gates. Clean frames dominate in practice.
2. **First-sighting bootstrap gap** — one dropped frame per actor if first scan is already corrupt.
3. **Unbounded growth if actors leave without hitting all-zeros** — bounded by match size (~100); add TTL if needed.

*Mitigations:*

- If you ever see staleness in the wild, add a **max-age** counter in the snapshot and drop it after N ticks of failed gates.
- The bootstrap gap is cosmetic — one missed render on first enemy appearance, no gameplay impact.
- For memory, a 2-second TTL on untouched entries caps the map even if an actor leaves the world in an unusual way.

## 9. Offsets required

These are the final constants you need. Verify them against your own SDK dump; layouts change between PUBG versions.

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

- Define `struct ActorSnapshot` (top of `main.cpp` or in `structs.h`).
- Declare the store `static` inside the scanning function (container per §6.3).
- After reading `transform`, read LUL / VEH / REP.
- All-zeros erase rule.
- **Gate 1** — root vs reference (300 cm).
- **Gate 2** — `transform_bounds.Translation` vs `raw` (50 cm).
- Reuse `transform_bounds` as `transform_mech` (do not re-read mesh CTW).
- **Gate 3** — computed `head_position` and `root_position` vs `raw` (150 cm each).
- Store the snapshot only when all three gates pass; otherwise fall through to cached.
- Downstream consumes snapshot world values; `world_to_screen` runs every frame with current camera.

## 11. Out of scope

- **Ghost actors** — separate decoy actor instances in the UWorld list with their own valid LUL/VEH. This filter cannot distinguish them; requires a render-suppression or proximity-dedupe detector.
- **Bone index 6 resolving to `UStaticMesh`* on some PUBG Mobile builds** — affects head-anchor accuracy for health bars; independent of decoy filtering.

