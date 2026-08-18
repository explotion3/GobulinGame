#include "Core/CombatTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_Attack_Melee, "Combat.Attack.Melee", "Attack delivered through a melee contact or sweep.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_Attack_Projectile, "Combat.Attack.Projectile", "Attack delivered by a projectile.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_Damage_Physical, "Combat.Damage.Physical", "Physical damage.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_Damage_Fire, "Combat.Damage.Fire", "Fire damage.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_Damage_Frost, "Combat.Damage.Frost", "Frost damage.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_Reaction_Hit, "Combat.Reaction.Hit", "Standard hit reaction without loss of control.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_Reaction_Stagger, "Combat.Reaction.Stagger", "Hit reaction that temporarily interrupts actions.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_Reaction_Knockback, "Combat.Reaction.Knockback", "Hit reaction that displaces the target.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_EnemyStateReason_SpawnCompleted, "Combat.Enemy.StateReason.SpawnCompleted", "Spawn presentation completed.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_EnemyStateReason_TargetAcquired, "Combat.Enemy.StateReason.TargetAcquired", "A valid target was acquired.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_EnemyStateReason_TargetLost, "Combat.Enemy.StateReason.TargetLost", "The current target became invalid or left the retention range.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_EnemyStateReason_PursuitResumed, "Combat.Enemy.StateReason.PursuitResumed", "The target moved outside the ready-distance hysteresis.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_EnemyStateReason_NavigationFailed, "Combat.Enemy.StateReason.NavigationFailed", "The movement backend could not produce or follow a usable path.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_EnemyStateReason_AttackReady, "Combat.Enemy.StateReason.AttackReady", "Attack requirements and cooldown were satisfied.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_EnemyStateReason_Damaged, "Combat.Enemy.StateReason.Damaged", "Damage caused a gameplay state transition.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_EnemyStateReason_Killed, "Combat.Enemy.StateReason.Killed", "Health reached zero.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_EnemyStateReason_Recovered, "Combat.Enemy.StateReason.Recovered", "A timed action or interruption finished.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(CombatTag_EnemyStateReason_Retired, "Combat.Enemy.StateReason.Retired", "The enemy completed its lifecycle and left simulation.");
