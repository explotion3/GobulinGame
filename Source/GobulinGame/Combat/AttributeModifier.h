#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AttributeModifier.generated.h"

UENUM(BlueprintType)
enum class EAttributeModifierOperation : uint8
{
	Add			UMETA(DisplayName = "Add"),
	Multiply	UMETA(DisplayName = "Multiply"),
	Override	UMETA(DisplayName = "Override")
};

UENUM(BlueprintType)
enum class EAttributeModifierDuration : uint8
{
	Permanent	UMETA(DisplayName = "Permanent"),
	Timed		UMETA(DisplayName = "Timed"),
	UntilDeath	UMETA(DisplayName = "UntilDeath")
};

UENUM(BlueprintType)
enum class EAttributeModifierStackRule : uint8
{
	Single		UMETA(DisplayName = "Single"),
	Stackable	UMETA(DisplayName = "Stackable"),
	Refresh		UMETA(DisplayName = "Refresh")
};

/** 属性修改器：Perk / 武器 Mod / 技能 / Buff / 设施 / 悬赏 统一通过它改属性 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FAttributeModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName ModifierId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FGameplayTag AttributeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EAttributeModifierOperation Operation = EAttributeModifierOperation::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EAttributeModifierDuration Duration = EAttributeModifierDuration::Permanent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	float RemainingTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EAttributeModifierStackRule StackRule = EAttributeModifierStackRule::Single;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	int32 MaxStacks = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	int32 CurrentStacks = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName SourceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FGameplayTagContainer SourceTags;
};
