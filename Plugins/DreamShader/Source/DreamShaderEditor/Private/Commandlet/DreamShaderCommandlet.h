#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "DreamShaderCommandlet.generated.h"

UCLASS()
class UDreamShaderCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UDreamShaderCommandlet();

	virtual int32 Main(const FString& Params) override;
};
