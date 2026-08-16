#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GobulinPlayerController.generated.h"

class UInputMappingContext;

/** 正式玩家控制器：只管理本地输入上下文和玩家相机管理器。 */
UCLASS()
class GOBULINGAME_API AGobulinPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGobulinPlayerController();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	virtual void SetupInputComponent() override;
};
