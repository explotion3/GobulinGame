#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "AttributeModifier.h"
#include "BattleAttributeSet.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChangedSignature, FGameplayTag, AttributeTag, float, NewValue);

/**
 * 自研轻量属性集（ADR-006）：
 * 最终值 = Clamp((Base + ΣAdd) × ΣMult)，Override 存在时直接覆盖。
 * 所有 Perk/Mod/技能/Buff 的数值效果统一以 FAttributeModifier 进入。
 */
UCLASS(BlueprintType)
class GOBULINGAME_API UBattleAttributeSet : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Attribute")
	float GetFinalValue(const FGameplayTag& AttributeTag) const;

	UFUNCTION(BlueprintCallable, Category = "Attribute")
	void SetBaseValue(const FGameplayTag& AttributeTag, float Value, float MinValue = -1.0e9f, float MaxValue = 1.0e9f);

	UFUNCTION(BlueprintCallable, Category = "Attribute")
	void AddModifier(const FAttributeModifier& Modifier);

	UFUNCTION(BlueprintCallable, Category = "Attribute")
	void RemoveModifier(FName ModifierId);

	UFUNCTION(BlueprintCallable, Category = "Attribute")
	void RemoveModifiersFromSource(FName SourceId);

	UFUNCTION(BlueprintCallable, Category = "Attribute")
	bool HasAttribute(const FGameplayTag& AttributeTag) const;

	const TArray<FAttributeModifier>& GetModifiers() const { return Modifiers; }

	void TickModifiers(float DeltaTime);

	UPROPERTY(BlueprintAssignable, Category = "Attribute")
	FOnAttributeChangedSignature OnAttributeChanged;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Attribute")
	TMap<FGameplayTag, float> BaseValues;

	UPROPERTY(VisibleAnywhere, Category = "Attribute")
	TArray<FAttributeModifier> Modifiers;

	void BroadcastChanged(const FGameplayTag& AttributeTag);
};
