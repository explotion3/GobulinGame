# 敌人运行时地基

The enemy runtime is backend-neutral at its public boundary:

- `FEnemySpawnRequest` addresses an archetype by `FPrimaryAssetId`.
- `FCombatantHandle` is the only identity shared with combat callers.
- `FGobulinEnemyRuntimeData` contains no Actor, component, UObject, or Mass entity pointer.
- `UGobulinEnemySubsystem` owns lifecycle rules and currently stores an Actor adapter record.
- `AGobulinEnemyActor` is the current ACharacter adapter. It owns the capsule, CharacterMovement,
  feet anchor and Paper2D presentation while gameplay decisions remain in the subsystem.
- `AGobulinEnemyAIController` only executes backend-neutral move intents through NavMesh; it owns
  no targeting, Behavior Tree, Blackboard or attack policy.
- `AGobulinEnemySpawnArea` is the placeable level-authoring entry. It caches local ground
  candidates, revalidates placement at runtime, and submits grouped requests without owning enemies.
- Confirmed spawn, state, damage, death, and retire facts are published through `UCombatEventSubsystem`.

`FEnemySpawnRequest::SpawnTransform` is a logical ground-contact anchor. The Actor adapter offsets
the capsule origin by the archetype half-height and reports the accepted ground anchor in spawn events.

Enemy archetype assets must be created under `/Game/_Game/Enemy` so the Asset Manager scans them
as the `EnemyArchetype` primary asset type. Native Paper2D Flipbooks and optional materials are soft
references loaded through the `Presentation` bundle. The editor-only `AGobulinEnemyPreviewActor`
can bake capsule and whitelisted presentation parameters back to an archetype.

The minimum basic-enemy presentation contains viewer-relative `Idle` and `Run` sets. Each set has
`TowardViewer`, `AwayFromViewer`, `ViewerLeft`, and `ViewerRight`, plus one non-looping `Death` slot.
Direction is selected locally from actual horizontal velocity relative to the local camera. Below
the movement threshold, the presentation loops the last valid direction's Idle. These choices are
presentation-only and are not replicated or added to authoritative enemy state.

The minimum behavior lifecycle implemented here is:

`Inactive -> Spawning -> SeekingTarget -> Moving <-> ReadyToAttack -> Dying -> Inactive`

Target selection uses team/active/location snapshots from the combatant registry. Actor navigation
uses CharacterMovement and RVO. Attacks and Mass processors remain outside this slice. A future
Mass adapter should map the same combatant handle to an `FMassEntityHandle`, split runtime values
into per-entity and const-shared fragments, and preserve the same event order.
