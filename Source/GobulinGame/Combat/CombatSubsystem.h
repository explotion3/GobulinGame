#pragma once

#include "CoreMinimal.h"
#include "Combat/DamageProtocol.h"
#include "Containers/Queue.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatSubsystem.generated.h"

/** Routes backend-neutral combat commands and publishes their confirmed outcomes. */
UCLASS()
class GOBULINGAME_API UCombatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FCombatCommandId AllocateCommandId();

	/**
	 * Submits a damage request. Actor targets currently resolve immediately, but callers must
	 * consume FCombatDamageResolvedEvent rather than relying on synchronous completion.
	 */
	FCombatCommandId SubmitDamage(FCombatDamageRequest Request);

	virtual void Deinitialize() override;

private:
	bool RememberCommand(FCombatCommandId CommandId);
	void PublishDamageResult(const FCombatDamageRequest& Request, FCombatDamageResult Result) const;

	TAtomic<int64> NextCommandValue { 1 };
	TSet<FCombatCommandId> RememberedCommands;
	TQueue<FCombatCommandId, EQueueMode::Spsc> CommandEvictionQueue;

	static constexpr int32 MaxRememberedCommandCount = 32768;
};
