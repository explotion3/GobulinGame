#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "EnemyAssetSetupCommandlet.generated.h"

/** Creates the minimum EnemyArchetype asset used by the first Actor-backend vertical slice. */
UCLASS()
class UEnemyAssetSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
