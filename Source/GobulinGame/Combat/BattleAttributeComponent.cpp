#include "Combat/BattleAttributeComponent.h"

#include "Combat/AttributeModifier.h"
#include "Combat/BattleAttributeSet.h"

UBattleAttributeComponent::UBattleAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	AttributeSet = CreateDefaultSubobject<UBattleAttributeSet>(TEXT("AttributeSet"));
}

float UBattleAttributeComponent::GetAttributeValue(const FGameplayTag& AttributeTag) const
{
	return AttributeSet ? AttributeSet->GetFinalValue(AttributeTag) : 0.0f;
}

void UBattleAttributeComponent::SetBaseAttribute(const FGameplayTag& AttributeTag, float Value, float MinValue, float MaxValue)
{
	if (AttributeSet)
	{
		AttributeSet->SetBaseValue(AttributeTag, Value, MinValue, MaxValue);
	}
}

void UBattleAttributeComponent::ApplyModifier(const FAttributeModifier& Modifier)
{
	if (AttributeSet)
	{
		AttributeSet->AddModifier(Modifier);
	}
}

void UBattleAttributeComponent::RemoveModifiersFromSource(FName SourceId)
{
	if (AttributeSet)
	{
		AttributeSet->RemoveModifiersFromSource(SourceId);
	}
}

void UBattleAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (AttributeSet)
	{
		AttributeSet->TickModifiers(DeltaTime);
	}
}
