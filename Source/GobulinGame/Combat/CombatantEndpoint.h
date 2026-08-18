#pragma once

#include "CoreMinimal.h"
#include "Combat/DamageProtocol.h"
#include "UObject/Interface.h"
#include "CombatantEndpoint.generated.h"

/** Actor-backend endpoint used by the combat router after resolving a logical handle. */
UINTERFACE(MinimalAPI, BlueprintType)
class UCombatantEndpoint : public UInterface
{
	GENERATED_BODY()
};

class GOBULINGAME_API ICombatantEndpoint
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Combat")
	FCombatDamageResult ResolveCombatDamage(const FCombatDamageRequest& Request);
};
