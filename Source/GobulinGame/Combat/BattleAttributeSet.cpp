#include "Combat/BattleAttributeSet.h"

float UBattleAttributeSet::GetFinalValue(const FGameplayTag& AttributeTag) const
{
	const float* Base = BaseValues.Find(AttributeTag);
	float Value = Base ? *Base : 0.0f;

	float AddSum = 0.0f;
	float MultProduct = 1.0f;
	bool bHasOverride = false;
	float OverrideValue = 0.0f;

	for (const FAttributeModifier& Modifier : Modifiers)
	{
		if (!Modifier.AttributeTag.MatchesTagExact(AttributeTag))
		{
			continue;
		}

		switch (Modifier.Operation)
		{
		case EAttributeModifierOperation::Add:
			AddSum += Modifier.Value * FMath::Max(0, Modifier.CurrentStacks);
			break;
		case EAttributeModifierOperation::Multiply:
			MultProduct *= 1.0f + (Modifier.Value - 1.0f) * FMath::Max(0, Modifier.CurrentStacks);
			break;
		case EAttributeModifierOperation::Override:
			bHasOverride = true;
			OverrideValue = Modifier.Value;
			break;
		}
	}

	if (bHasOverride)
	{
		Value = OverrideValue;
	}
	else
	{
		Value = (Value + AddSum) * MultProduct;
	}

	return Value;
}

void UBattleAttributeSet::SetBaseValue(const FGameplayTag& AttributeTag, float Value, float MinValue, float MaxValue)
{
	BaseValues.Add(AttributeTag, FMath::Clamp(Value, MinValue, MaxValue));
	BroadcastChanged(AttributeTag);
}

void UBattleAttributeSet::AddModifier(const FAttributeModifier& Modifier)
{
	for (FAttributeModifier& Existing : Modifiers)
	{
		if (Existing.ModifierId != Modifier.ModifierId || Existing.SourceId != Modifier.SourceId)
		{
			continue;
		}

		switch (Modifier.StackRule)
		{
		case EAttributeModifierStackRule::Refresh:
			Existing = Modifier;
			BroadcastChanged(Modifier.AttributeTag);
			return;
		case EAttributeModifierStackRule::Stackable:
			Existing.CurrentStacks = FMath::Min(Existing.CurrentStacks + 1, Existing.MaxStacks > 0 ? Existing.MaxStacks : 1);
			BroadcastChanged(Modifier.AttributeTag);
			return;
		case EAttributeModifierStackRule::Single:
		default:
			Existing = Modifier;
			BroadcastChanged(Modifier.AttributeTag);
			return;
		}
	}

	Modifiers.Add(Modifier);
	BroadcastChanged(Modifier.AttributeTag);
}

void UBattleAttributeSet::RemoveModifier(FName ModifierId)
{
	for (int32 Index = Modifiers.Num() - 1; Index >= 0; --Index)
	{
		if (Modifiers[Index].ModifierId != ModifierId)
		{
			continue;
		}

		const FGameplayTag RemovedTag = Modifiers[Index].AttributeTag;
		Modifiers.RemoveAt(Index);
		BroadcastChanged(RemovedTag);
	}
}

void UBattleAttributeSet::RemoveModifiersFromSource(FName SourceId)
{
	for (int32 Index = Modifiers.Num() - 1; Index >= 0; --Index)
	{
		if (Modifiers[Index].SourceId != SourceId)
		{
			continue;
		}

		const FGameplayTag RemovedTag = Modifiers[Index].AttributeTag;
		Modifiers.RemoveAt(Index);
		BroadcastChanged(RemovedTag);
	}
}

bool UBattleAttributeSet::HasAttribute(const FGameplayTag& AttributeTag) const
{
	return BaseValues.Contains(AttributeTag);
}

void UBattleAttributeSet::TickModifiers(float DeltaTime)
{
	for (int32 Index = Modifiers.Num() - 1; Index >= 0; --Index)
	{
		FAttributeModifier& Modifier = Modifiers[Index];
		if (Modifier.Duration != EAttributeModifierDuration::Timed)
		{
			continue;
		}

		Modifier.RemainingTime -= DeltaTime;
		if (Modifier.RemainingTime > 0.0f)
		{
			continue;
		}

		const FGameplayTag ExpiredTag = Modifier.AttributeTag;
		Modifiers.RemoveAt(Index);
		BroadcastChanged(ExpiredTag);
	}
}

void UBattleAttributeSet::BroadcastChanged(const FGameplayTag& AttributeTag)
{
	OnAttributeChanged.Broadcast(AttributeTag, GetFinalValue(AttributeTag));
}
