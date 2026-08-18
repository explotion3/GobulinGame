#include "Combat/CombatSubsystem.h"

#include "Combat/CombatantEndpoint.h"
#include "Combat/CombatantRegistrySubsystem.h"
#include "Combat/CombatEventSubsystem.h"
#include "GameFramework/Actor.h"

FCombatCommandId UCombatSubsystem::AllocateCommandId()
{
	int64 Value = NextCommandValue.IncrementExchange();
	if (Value <= 0)
	{
		NextCommandValue.Store(2);
		Value = 1;
	}
	return FCombatCommandId(Value);
}

FCombatCommandId UCombatSubsystem::SubmitDamage(FCombatDamageRequest Request)
{
	check(IsInGameThread());

	if (!Request.CommandId.IsSet())
	{
		Request.CommandId = AllocateCommandId();
	}

	FCombatDamageResult Result;
	Result.CommandId = Request.CommandId;
	Result.Source = Request.Source;
	Result.Target = Request.Target;
	Result.RequestedAmount = FMath::IsFinite(Request.BaseAmount)
		? FMath::Max(0.0f, Request.BaseAmount)
		: 0.0f;

	if (!RememberCommand(Request.CommandId))
	{
		Result.Result = ECombatDamageResult::Duplicate;
		PublishDamageResult(Request, Result);
		return Request.CommandId;
	}

	if (!Request.IsValid())
	{
		Result.Result = ECombatDamageResult::InvalidRequest;
		PublishDamageResult(Request, Result);
		return Request.CommandId;
	}

	UCombatantRegistrySubsystem* Registry = GetWorld()
		? GetWorld()->GetSubsystem<UCombatantRegistrySubsystem>()
		: nullptr;
	AActor* TargetActor = Registry ? Registry->ResolveActor(Request.Target) : nullptr;
	if (!TargetActor || !TargetActor->GetClass()->ImplementsInterface(UCombatantEndpoint::StaticClass()))
	{
		Result.Result = ECombatDamageResult::InvalidTarget;
		PublishDamageResult(Request, Result);
		return Request.CommandId;
	}

	Result = ICombatantEndpoint::Execute_ResolveCombatDamage(TargetActor, Request);
	Result.CommandId = Request.CommandId;
	Result.Source = Request.Source;
	Result.Target = Request.Target;
	Result.RequestedAmount = FMath::Max(0.0f, Request.BaseAmount);
	PublishDamageResult(Request, Result);
	return Request.CommandId;
}

void UCombatSubsystem::Deinitialize()
{
	RememberedCommands.Reset();
	FCombatCommandId Ignored;
	while (CommandEvictionQueue.Dequeue(Ignored))
	{
	}
	Super::Deinitialize();
}

bool UCombatSubsystem::RememberCommand(FCombatCommandId CommandId)
{
	if (RememberedCommands.Contains(CommandId))
	{
		return false;
	}

	RememberedCommands.Add(CommandId);
	CommandEvictionQueue.Enqueue(CommandId);

	while (RememberedCommands.Num() > MaxRememberedCommandCount)
	{
		FCombatCommandId ExpiredCommandId;
		if (!CommandEvictionQueue.Dequeue(ExpiredCommandId))
		{
			break;
		}
		RememberedCommands.Remove(ExpiredCommandId);
	}

	return true;
}

void UCombatSubsystem::PublishDamageResult(
	const FCombatDamageRequest& Request,
	FCombatDamageResult Result) const
{
	if (!GetWorld())
	{
		return;
	}

	if (UCombatEventSubsystem* EventSubsystem = GetWorld()->GetSubsystem<UCombatEventSubsystem>())
	{
		FCombatDamageResolvedEvent Event;
		Event.Request = Request;
		Event.Result = MoveTemp(Result);
		EventSubsystem->EnqueueDamageResolved(Event);
	}
}
