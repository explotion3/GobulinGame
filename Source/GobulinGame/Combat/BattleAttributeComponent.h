#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "BattleAttributeComponent.generated.h"

class UBattleAttributeSet;
struct FAttributeModifier;

/** 挂在玩家/单位上的运行时属性容器，驱动修改器计时并广播变化 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API UBattleAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBattleAttributeComponent();

	UFUNCTION(BlueprintCallable, Category = "Attribute")
	UBattleAttributeSet* GetAttributeSet() const { return AttributeSet; }

	UFUNCTION(BlueprintCallable, Category = "Attribute")
	float GetAttributeValue(const FGameplayTag& AttributeTag) const;

	UFUNCTION(BlueprintCallable, Category = "Attribute")
	void SetBaseAttribute(const FGameplayTag& AttributeTag, float Value, float MinValue = -1.0e9f, float MaxValue = 1.0e9f);

	UFUNCTION(BlueprintCallable, Category = "Attribute")
	void ApplyModifier(const FAttributeModifier& Modifier);

	UFUNCTION(BlueprintCallable, Category = "Attribute")
	void RemoveModifiersFromSource(FName SourceId);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(VisibleAnywhere, Transient, Category = "Attribute")
	TObjectPtr<UBattleAttributeSet> AttributeSet;
};
