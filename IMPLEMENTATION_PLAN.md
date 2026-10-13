# WarbandCraft implementation plan

Updated: 13 October 2026

## Objective and authority

Continue development in `/Users/frankvaldez/Desktop/WarbandCraft` toward functional fortifications, campaign-specific saves, reliable scene collision, Minecraft shader support, Minecraft lights affecting Warband, and correct rendering between the two worlds. The user authorized autonomous work and provided full access for this task. Routine implementation, local builds, and necessary local validation can proceed within that scope; this does not authorize unrelated publishing or destructive changes to other games/worlds.

Target the existing native macOS Warband 1.174 / Minecraft Java 1.21.1 Fabric build. Preserve existing worlds and saves with recoverable backups before migration or runtime validation. Keep each implementation increment independently buildable and reversible.

This document is the continuation plan, not a claim that any milestone has been implemented. Existing validation records are historical evidence. The reported rendering defect has a reproduced framebuffer-reuse cause and a tested 0.2.1 fix. See RELEASE_STATUS.md for its evidence and remaining rendering acceptance.

## Shared-world acceptance standard — 13 October 2026

The user's architectural clarification is part of this plan. Minecraft owns placed blocks, inventories, redstone, crafting and Steve's movement. Warband owns soldiers, horses, army orders, battle AI and campaign consequences. Bridge work is accepted by physical and gameplay consequences in both engines, not by successful publication of more data.

The older GitHub snapshot is background context. Inspect the current Desktop source before treating an older finding as current. Current horizontal terrain probes do not use the fabricated upward normal. The native cast-ray response must report an unknown normal as zero until actual geometric information is available. Its current attribute distinguishes a native prop from terrain; it is not a material or decorative-geometry classification.

Add these explicit checks to the existing milestones:

- Geometry: a decorative flower must not become a full solid column. A route through a staircase, balcony, ceiling and overlapping floor must preserve all traversable spaces. Investigate extraction of the native collision primitives and scene transforms; additional ray sampling remains an approximation, not imported geometry. Include moving gates and siege props.
- Construction: place a three-block-high wall in front of 50 charging troops. Prove physical blocking and AI routing separately. Test an opening, cavalry clearance, arrows, construction removal and rebuilt snapshots. Do not substitute teleportation for navigation.
- Contacts: a real Warband soldier steps on a Minecraft pressure plate, redstone activates a piston door, the mirrored native obstruction opens, and the soldier passes through it. Test stepping off, multiple agents, removal, unload and reconnect. Preserve Warband's movement authority when adding Minecraft contact callbacks to proxies.
- Combat: a ground-level sword swing cannot hit Steve fifteen blocks overhead; native and Minecraft walls prevent transferred hits; Minecraft melee reach, arrows, TNT, armor and directional shields remain consistent. Add actual attacker identity, weapon/attack kind and event geometry before attempting validation; the nearest proxy within 32 blocks is not authoritative evidence of the attacker. Do not reject legitimate long-range projectiles using a melee-distance heuristic.
- Persistence: save and reload the pressure-plate/door/fort scenario in two campaigns and an older checkpoint. Preserve inventory, construction, source identity and collision state together.

0.2.1 first repairs the reported rendering corruption, enables direct Computer Use of both processes, fixes the discovered native collider multiplier error, and records the results of every 0.2.0 feature area. The larger collision import, dynamic navigation, block-contact and combat-contract milestones remain substantive engineering work; a rendering patch cannot establish that they are complete.

## Execution order and checkpoints

| Milestone | Outcome | Dependency |
|---|---|---|
| 1 | Correct depth occlusion between Warband and Minecraft | First priority |
| 2 | Proven native collision/navigation, save identity, and lighting integration routes | Baseline from 1 |
| 3 | Isolated, coordinated campaign/location persistence | Identity proof from 2 |
| 4 | Reliable native-scene collision for Steve | Baseline from 1; persisted identity from 3 |
| 5 | Minecraft structures affect native troops and projectiles | Native capability proof from 2; 3 and 4 |
| 6 | Shader support preserves transparent composition and depth | 1; regression coverage from 4 and 5 |
| 7 | Minecraft light sources illuminate the native game world | Depth from 1; chosen rendering pipeline from 6 |

Add evidence and a continuation checkpoint to this document after each milestone: implemented files, exact build/deployment commands, passed checks, remaining failures, and the next concrete action. Update VALIDATION.md only with checks actually performed; distinguish measured results from untested expectations. Do not mark blocked features complete by substituting a visual approximation without documenting the difference.

## 1. Fix Minecraft blocks drawing through Warband walls

### Diagnosis

Relevant code: `source/native/host_adapter.c`, `source/native/protocol.h`, `FramePassthrough.java`, `CameraSync.java`, and the rendering mixins under `source/minecraft/src/client/java/dev/wbmc/bridge/client/`.

Reproduce with a block behind an opaque native wall, another in front of it, and a block partially crossing a doorway. Record depth images, frame IDs, camera/projection matrices, clip distances, viewport dimensions, framebuffer IDs, and GL errors for the same displayed frame.

Inspect these candidate causes without treating any as a proven root cause:

- Native scene framebuffer selection and the time at which its depth is copied. The current hook chooses candidates from viewport size and attachment presence, which does not prove it captured the final world depth.
- Read/draw framebuffer binding, attachment format, multisample resolve, image orientation, and drawable versus window sizes.
- GL error handling: the current code uses a single `glGetError()` result to enable depth comparison and otherwise draws without occlusion. Isolate errors around the relevant operation and expose an explicit depth-valid state.
- The captured Minecraft near plane is hardcoded to 0.05; verify it against the actual projection before comparing linear depth.
- The relationship between frame, camera, depth, and projection. Reject mismatched frame data, including when the matching pose is absent.
- Which effects are in the world layer versus the always-on-top hand/HUD layer. World effects must obey depth.
- Diagnostic depth flags exist on the Java side; verify their native implementation rather than assuming the flags work.

### Implementation

Establish a versioned frame contract carrying validated depth/projection metadata and a stable native world-depth capture. Compare both worlds in a common camera-space distance convention. Preserve correct alpha blending and the intended HUD/first-person-hand layering. Keep third-person Steve and world particles depth-tested.

Opaque geometry is the first checkpoint. Then define and test transparent-material composition separately: glass, water, particles, and shader bloom may need distinct masks/passes rather than one depth value per pixel. Prevent silent overlay-through-walls behavior when depth acquisition fails; expose the failure and use a recoverable rendering fallback.

### Acceptance

- A block behind a native wall is hidden; one in front is visible.
- Partial overlap, doorways, native troops, terrain, and third-person Steve have correct depth ordering while the camera moves.
- Resize, F8 switching, scene changes, and reloads preserve the result.
- Save deterministic comparison images and pixel/depth measurements. Historical camera-alignment measurements alone do not pass this milestone.

## 2. Prove engine integration routes

Use a dedicated test module/scene and backed-up bridge world. Establish small experiments before constructing large subsystems.

- **Collision:** mirror a solid cube with a native collision prop. Verify runtime spawning/scaling, placement, removal or pooling, visual hiding without losing collision, and projectile blocking. Test realistic object counts and block edit bursts.
- **Navigation:** test infantry and horses routing around the cube and walking onto a newly placed elevated platform. Physical collision does not establish AI traversal. Determine whether native operations suffice or a custom obstacle/path layer is needed; do not assume arbitrary runtime navmesh rebuilding exists.
- **Campaign identity:** verify persistent campaign storage and observable save/load checkpoints on this exact build. The current mission-only patcher needs a campaign integration route beyond mission scene IDs.
- **Lighting:** test whether usable native point lights can be created and updated. If not, prototype a custom lighting pass with native depth and reconstructed positions/normals, clearly labeling its limits.

Record the chosen route, evidence, engine limits, and scaling budgets for each. Continue independent supported work if one experiment fails; record a concrete unresolved capability instead of disguising it with teleport-based troop blocking or global screen brightness.

## 3. Campaign-specific worlds and coordinated saves

Relevant code: `WorldBootstrap.java`, `TerrainManager.java`, `CoordMap.java`, shared-memory protocol, and campaign additions to `source/install_module.py` or a replacement module build pipeline.

- Add a campaign UUID persisted by Warband and publish it with mission/location context.
- Namespace Minecraft worlds, inventory, anchor files, build data, and native mirrors by campaign.
- Use stable settlement/interior IDs for permanent locations. Identify generated terrain by relevant generation inputs/layout identity, plus encounter identity when a battlefield should be temporary. A reused scene ID is insufficient.
- Treat custom battles and other noncampaign play as explicitly separate sessions.
- Add checkpoint IDs and a journal/manifest linking a native save to a recoverable Minecraft checkpoint. Capture construction, inventory, anchor mappings, and relevant persistent world state consistently.
- Support loading older checkpoints without retaining future blocks/items. Detect incomplete checkpoints and recover to the last consistent state rather than silently mixing saves.
- Treat Save As as a branch of campaign history; prevent divergent branches from overwriting each other's checkpoints. New campaigns always receive new identities.
- Back up the current `warband-bridge` world; offer migration as a separate explicit operation into a selected campaign. Never silently associate all old builds with the first campaign opened.

Acceptance: two campaigns do not share builds or inventory; Save As branches can diverge; loading an older save restores matching Minecraft state; repeated permanent-location visits remain stable; reused generated scene IDs do not misplace construction. Exercise interrupted checkpoint creation and restart recovery.

## 4. Native-scene collision and fast movement

Relevant code: `TerrainManager.java`, `TerrainBlock.java`, native `handle_rays`, ray protocol, and mission `cast_ray` operations.

- Fix the current wall-probe mismatch: native responses set every hit normal upward, while Java obstacle detection requires a sufficiently horizontal normal. Supply valid normals or adopt a probe algorithm that does not assume unavailable normals. Likewise, return only collision attributes the native implementation actually supports.
- Separate original native geometry from mirrored Minecraft collision props so sampling does not convert player structures into invisible native terrain.
- Extend single-column approximations to preserve multiple vertical surfaces, ceilings, slopes, stairs, narrow entrances, and bridges above roads.
- Invalidate samples for moving doors/gates and siege props, changed locations, and scene-generation changes.
- Tag ray requests/results with mission and location generation; discard old responses after transitions.
- Prioritize the player's immediate swept movement path and predicted landing area. Make lookahead depend on speed and measured query latency; target fast Elytra flights with rockets.
- Constrain cache growth and retire temporary safety floors when valid geometry arrives. Keep failure states visible and avoid permanent phantom barriers.

Acceptance: repeatable routes through field terrain, town streets, taverns, castles, arenas, village scenes, and siege scenes. Include stairs, low ceilings, bridge-underpass pairs, gates, steep slopes, water boundaries, repeated visits, and high-speed landings. Record misses, stalls, and frame times.

## 5. Functional fortifications and troop navigation

Add versioned construction snapshots/deltas containing campaign/location/generation, actual block collision shapes, sequence IDs, and bounded work queues.

- Track placement/removal and world-driven edits such as TNT, pistons, and doors; do not rely exclusively on player placement callbacks.
- Start with full cubes, then support slabs, stairs, fences, doors, and trapdoors.
- Merge compatible shapes into native colliders, maintain a spatial index, and pool/remove props using the proven engine route.
- Apply changes on the native game thread; acknowledge completed revisions. Rebuild from a snapshot after reconnect and clear colliders belonging to old scenes.
- Preserve native movement/combat and army orders while adding obstacle-aware routing. Handle cavalry clearance, formations, narrow gates, and navigation across constructed stairs/bridges separately.
- Prevent collisions from being removed merely because the player walks away while nearby native agents still need them.
- Mirror projectile blocking and confirm that breaking/exploding structures opens paths and firing lines.

Acceptance: infantry and cavalry stop at solid walls, use openings, traverse supported stairs/bridges, and react to removed blocks. Arrows stop at structures. Repeated edits, piston motion, TNT bursts, reconnection, and scene changes leave no stale colliders. Measure both normal battles and larger battles within the existing agent-table limit.

## 6. Minecraft shader integration

Use Iris and matching Sodium builds for Minecraft 1.21.1. Complementary Reimagined is the first candidate, starting at a low quality preset. Resolve and pin compatible versions at implementation time; do not assume current latest builds target this Minecraft version or the bridge's mixins.

Official project references: [Iris installation](https://www.irisshaders.dev/download/) and [Complementary Reimagined](https://modrinth.com/shader/complementary-reimagined).

- Validate the selected pack on this Mac in the dedicated instance before integrating capture.
- Adapt capture to the final shaded world output, its correct depth source, projection, and any temporal jitter. Preserve the common-frame contract established in milestone 1.
- Keep world coverage independent from final shader alpha when necessary; shader packs may write an opaque background. Preserve native sky/weather and separate GUI/hand rendering deliberately.
- Keep invisible native collision blocks out of visible rendering and inappropriate Minecraft shadow casting while preserving physics.
- Handle glass/water, bloom, temporal effects, particles, third-person Steve, switching shaders, resolution changes, and F8 control switching.
- Prevent glow from hidden Minecraft objects appearing through native walls. Shader effects calculated before composition may need bridge-aware occlusion or a final combined-image pass.
- Provide disabled/low/medium profiles and a known-working fallback. Document that Warband geometry is not automatically present in Minecraft's shader shadow/reflection buffers.

Acceptance: all milestone 1 occlusion checks still pass with shaders; the HUD remains readable; the native world is not covered by a shader sky/background; performance meets the budget established on this machine. If the candidate pack cannot meet the contract, select and test another pack rather than declaring shader support complete with broken depth.

## 7. Minecraft lights affecting native terrain and soldiers

Add a light registry and a versioned bridge containing source identity, location generation, position, color, intensity, range, and revision. Track loaded emitting blocks and supported moving sources rather than rescanning the full world each frame.

- Cover torches, lanterns, glowstone, lava, fire, and powered redstone lamps first. Optional held-item light support follows the same registry.
- Update light states for edits, redstone, fluids, movement, unloading, reconnects, and scene transitions.
- Use the native-light route if proven suitable; otherwise integrate the custom pass demonstrated in milestone 2.
- Reconstruct native surface position from correct depth/projection. Depth-derived normals provide an approximation; accurate materials require access to material/albedo/normal data or native shader integration. Avoid repeatedly adding light to already bright/tone-mapped pixels.
- Limit active lights by relevance and measured cost, with deterministic prioritization and smooth changes. Derive budgets from actual measurements rather than promising unrestricted lighting.
- Add occlusion using available native geometry/rays and the Minecraft shape index. Test walls from both worlds; a depth-only screen-space technique cannot guarantee off-screen occlusion.
- Apply native lighting before the final world/HUD composition and coordinate color/exposure with the selected shader. Keep bloom outside wall silhouettes from reintroducing milestone 1's bug.
- Implement more accurate shadowing where the proven renderer route supports it. Record remaining differences in shadow/material fidelity explicitly.

Acceptance: a torch illuminates a native wall, ground, and moving troop; a redstone lamp toggles native illumination; lights vanish after removal or location change; separating walls suppress direct light. Check daytime/nighttime, interiors, many sources, moving cameras, and shader on/off.

## Combined release check

Build a fort with a gate, stairs, elevated bridge, torches, and a redstone lamp. Confirm that native walls hide blocks behind them. Attack with infantry, cavalry, and archers; verify routing and projectile collision. Toggle the lamp, move a troop through its light, destroy the gate using TNT, and verify the new opening. Fly away and return, change scenes, save, restart both processes, and revisit. Load an older checkpoint and a different campaign to verify isolation and rollback.

Retain screenshots, relevant depth diagnostics, gameplay observations, test commands, and frame-time measurements. Repeat the relevant checks after shader/light integration. A successful compilation or single screenshot is insufficient evidence for collision, campaign persistence, or scene compatibility.

## Next concrete action

Use the correctly placed native collider to test infantry, cavalry and projectiles. A native ray has proven collider presence and removal, but those gameplay consequences remain unproven. Establish navigation separately, then proxy block contacts and the pressure-plate/piston-door sequence. Investigate native collision primitives to replace phantom interior columns, and design actual attacker/weapon/obstruction metadata before claiming shared combat correctness.

## Continuation checkpoint — 13 October 2026

0.2.1 is built and installed in the isolated Next environment. Direct Computer Use works for both processes. Live checks cover town wall occlusion, solid Steve, inventory, real placement/breaking, castle/tavern transitions, shader on/off and native lamp illumination. A native ray hits a one-block collider at the corrected coordinates and misses after removal. Campaign switching now stops and saves the integrated server before opening the other world. Native generation includes campaign/visit identity to prevent fresh mailbox allocations from reusing a mission generation.

All current feature areas and test limits are stated in RELEASE_STATUS.md. The supplied older-snapshot analysis is incorporated in the shared-world acceptance standard above. Full geometry import, navigation, physical block contacts, exact combat and real paired native save I/O are not marked complete. The original normal launcher and earlier backups remain available.
