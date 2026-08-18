#include "Enemy/GobulinEnemySpawnArea.h"

#include "Combat/CombatEventSubsystem.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Enemy/GobulinEnemyArchetype.h"
#include "Enemy/GobulinEnemySubsystem.h"
#include "Engine/World.h"
#include "NavigationSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGobulinEnemySpawnArea, Log, All);

namespace
{
	constexpr float SpawnGroundClearance = 1.0f;
	constexpr int32 AttemptsPerRequestedCandidate = 32;
	constexpr float CandidatePackingFactor = 0.65f;
}

AGobulinEnemySpawnArea::AGobulinEnemySpawnArea()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SpawnBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBounds"));
	SetRootComponent(SpawnBounds);
	SpawnBounds->SetBoxExtent(FVector(500.0f, 500.0f, 200.0f));
	SpawnBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBounds->SetGenerateOverlapEvents(false);
	SpawnBounds->SetHiddenInGame(true);
}

void AGobulinEnemySpawnArea::BeginPlay()
{
	Super::BeginPlay();
	BindCombatEvents();

	if (bSpawnOnBeginPlay)
	{
		SpawnDefaultBatch();
	}
}

void AGobulinEnemySpawnArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindCombatEvents();
	PendingCommandIds.Reset();
	Super::EndPlay(EndPlayReason);
}

FEnemySpawnAreaSubmission AGobulinEnemySpawnArea::SpawnDefaultBatch()
{
	return SpawnEnemies(SpawnCount);
}

FEnemySpawnAreaSubmission AGobulinEnemySpawnArea::SpawnEnemies(int32 Count)
{
	FEnemySpawnAreaSubmission Submission;
	Submission.RequestedCount = Count;
	Submission.RejectedCount = FMath::Max(0, Count);

	if (Count <= 0 || Count > MaxSpawnCountPerBatch || !EnemyArchetype || !EnemyArchetype->IsDefinitionValid())
	{
		FinalizeSubmission(Submission);
		return Submission;
	}

	UGobulinEnemySubsystem* EnemySubsystem = GetWorld()
		? GetWorld()->GetSubsystem<UGobulinEnemySubsystem>()
		: nullptr;
	if (!EnemySubsystem)
	{
		Submission.Result = EEnemySpawnAreaSubmissionResult::SubsystemUnavailable;
		FinalizeSubmission(Submission);
		return Submission;
	}

	const int32 BatchSequence = RuntimeBatchSequence++;
	const TArray<FTransform> SpawnTransforms = GenerateSpawnTransformsInternal(Count, BatchSequence);
	if (SpawnTransforms.IsEmpty())
	{
		Submission.Result = EEnemySpawnAreaSubmissionResult::NoValidLocation;
		FinalizeSubmission(Submission);
		return Submission;
	}

	Submission.SpawnGroupId = SpawnGroupId != INDEX_NONE
		? SpawnGroupId
		: EnemySubsystem->AllocateSpawnGroupId();

	TArray<FEnemySpawnRequest> Requests;
	Requests.Reserve(SpawnTransforms.Num());
	for (int32 Index = 0; Index < SpawnTransforms.Num(); ++Index)
	{
		FEnemySpawnRequest& Request = Requests.AddDefaulted_GetRef();
		Request.EnemyDefinitionId = EnemyArchetype->GetPrimaryAssetId();
		Request.SpawnTransform = SpawnTransforms[Index];
		Request.Owner = SpawnOwner;
		Request.TeamId = TeamId;
		Request.EnemyLevel = EnemyLevel;
		Request.PowerScale = PowerScale;
		Request.RandomSeed = static_cast<int32>(HashCombineFast(
			HashCombineFast(::GetTypeHash(RandomSeed), ::GetTypeHash(BatchSequence)),
			::GetTypeHash(Index)));
		Request.SpawnGroupId = Submission.SpawnGroupId;
	}

	const TArray<FCombatCommandId> CommandIds = EnemySubsystem->SpawnEnemies(Requests);
	Submission.CommandIds.Reserve(CommandIds.Num());
	for (const FCombatCommandId CommandId : CommandIds)
	{
		if (!CommandId.IsSet())
		{
			continue;
		}
		Submission.CommandIds.Add(CommandId);
		PendingCommandIds.Add(CommandId);
	}

	Submission.SubmittedCount = Submission.CommandIds.Num();
	Submission.RejectedCount = FMath::Max(0, Submission.RequestedCount - Submission.SubmittedCount);
	Submission.Result = Submission.SubmittedCount == Submission.RequestedCount
		? EEnemySpawnAreaSubmissionResult::Submitted
		: EEnemySpawnAreaSubmissionResult::PartiallySubmitted;
	FinalizeSubmission(Submission);
	return Submission;
}

TArray<FTransform> AGobulinEnemySpawnArea::GenerateSpawnTransforms(int32 Count) const
{
	if (Count <= 0 || Count > MaxSpawnCountPerBatch)
	{
		return {};
	}
	return GenerateSpawnTransformsInternal(Count, RuntimeBatchSequence);
}

void AGobulinEnemySpawnArea::RebuildSpawnCandidates()
{
	TArray<FTransform> NewCandidates;
	BuildCandidateCache(NewCandidates);

	Modify();
	CachedLocalCandidates = MoveTemp(NewCandidates);
	MarkPackageDirty();
	PreviewSpawnCandidates();

	UE_LOG(
		LogGobulinEnemySpawnArea,
		Log,
		TEXT("Rebuilt %d cached enemy spawn candidates for %s."),
		CachedLocalCandidates.Num(),
		*GetName());
}

void AGobulinEnemySpawnArea::PreviewSpawnCandidates()
{
	if (!GetWorld() || !SpawnBounds)
	{
		return;
	}

	const FTransform BoundsTransform = SpawnBounds->GetComponentTransform();
	for (const FTransform& LocalCandidate : CachedLocalCandidates)
	{
		FTransform GroundTransform;
		const FVector LocalLocation = LocalCandidate.GetLocation();
		if (!TryResolveGroundCandidate(FVector2D(LocalLocation.X, LocalLocation.Y), GroundTransform, false))
		{
			GroundTransform = FTransform(
				FRotator::ZeroRotator,
				BoundsTransform.TransformPosition(LocalLocation));
		}

		DrawDebugSphere(
			GetWorld(),
			GroundTransform.GetLocation() + FVector(0.0f, 0.0f, 12.0f),
			12.0f,
			8,
			FColor::Green,
			false,
			10.0f,
			0,
			1.5f);
	}
}

void AGobulinEnemySpawnArea::ClearSpawnCandidates()
{
	Modify();
	CachedLocalCandidates.Reset();
	MarkPackageDirty();
}

TArray<FTransform> AGobulinEnemySpawnArea::GenerateSpawnTransformsInternal(
	int32 Count,
	int32 BatchSequence) const
{
	TArray<FTransform> AcceptedTransforms;
	AcceptedTransforms.Reserve(Count);

	const int32 StreamSeed = static_cast<int32>(HashCombineFast(
		::GetTypeHash(RandomSeed),
		::GetTypeHash(BatchSequence)));
	FRandomStream RandomStream(StreamSeed);

	TArray<int32> CandidateIndices;
	CandidateIndices.Reserve(CachedLocalCandidates.Num());
	for (int32 Index = 0; Index < CachedLocalCandidates.Num(); ++Index)
	{
		CandidateIndices.Add(Index);
	}
	for (int32 Index = CandidateIndices.Num() - 1; Index > 0; --Index)
	{
		CandidateIndices.Swap(Index, RandomStream.RandRange(0, Index));
	}

	for (const int32 CandidateIndex : CandidateIndices)
	{
		const FVector LocalLocation = CachedLocalCandidates[CandidateIndex].GetLocation();
		FTransform GroundTransform;
		if (TryResolveGroundCandidate(
			FVector2D(LocalLocation.X, LocalLocation.Y),
			GroundTransform,
			bCheckSpawnClearance)
			&& IsFarEnoughFromAccepted(GroundTransform.GetLocation(), AcceptedTransforms))
		{
			AcceptedTransforms.Add(GroundTransform);
			if (AcceptedTransforms.Num() >= Count)
			{
				return AcceptedTransforms;
			}
		}
	}

	float MinX = 0.0f;
	float MaxX = 0.0f;
	float MinY = 0.0f;
	float MaxY = 0.0f;
	if (!GetLocalSamplingRange(MinX, MaxX, MinY, MaxY))
	{
		return AcceptedTransforms;
	}

	const int32 MaximumAttempts = FMath::Min(
		FMath::Max(Count * AttemptsPerRequestedCandidate, 128),
		MaxSpawnCountPerBatch * AttemptsPerRequestedCandidate);
	for (int32 Attempt = 0; Attempt < MaximumAttempts && AcceptedTransforms.Num() < Count; ++Attempt)
	{
		const FVector2D LocalXY(
			RandomStream.FRandRange(MinX, MaxX),
			RandomStream.FRandRange(MinY, MaxY));
		FTransform GroundTransform;
		if (TryResolveGroundCandidate(LocalXY, GroundTransform, bCheckSpawnClearance)
			&& IsFarEnoughFromAccepted(GroundTransform.GetLocation(), AcceptedTransforms))
		{
			AcceptedTransforms.Add(GroundTransform);
		}
	}

	return AcceptedTransforms;
}

bool AGobulinEnemySpawnArea::TryResolveGroundCandidate(
	const FVector2D& LocalXY,
	FTransform& OutGroundTransform,
	bool bCheckClearance) const
{
	UWorld* World = GetWorld();
	if (!World || !SpawnBounds)
	{
		return false;
	}

	float MinX = 0.0f;
	float MaxX = 0.0f;
	float MinY = 0.0f;
	float MaxY = 0.0f;
	if (!GetLocalSamplingRange(MinX, MaxX, MinY, MaxY)
		|| LocalXY.X < MinX
		|| LocalXY.X > MaxX
		|| LocalXY.Y < MinY
		|| LocalXY.Y > MaxY)
	{
		return false;
	}

	const FVector BoxExtent = SpawnBounds->GetUnscaledBoxExtent();
	const FTransform BoundsTransform = SpawnBounds->GetComponentTransform();
	const FVector TraceStart = BoundsTransform.TransformPosition(FVector(LocalXY.X, LocalXY.Y, BoxExtent.Z));
	const FVector TraceEnd = BoundsTransform.TransformPosition(FVector(LocalXY.X, LocalXY.Y, -BoxExtent.Z));

	FCollisionObjectQueryParams GroundObjects;
	GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	if (bAllowDynamicGround)
	{
		GroundObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
	}
	FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(GobulinEnemySpawnGround), false, this);
	FHitResult GroundHit;
	if (!World->LineTraceSingleByObjectType(GroundHit, TraceStart, TraceEnd, GroundObjects, GroundQuery))
	{
		return false;
	}

	const float MinimumUpNormal = FMath::Cos(FMath::DegreesToRadians(
		FMath::Clamp(MaximumGroundSlope, 0.0f, 89.0f)));
	if (GroundHit.ImpactNormal.GetSafeNormal().Z < MinimumUpNormal)
	{
		return false;
	}

	FVector GroundLocation = GroundHit.ImpactPoint;
	if (bRequireNavigation)
	{
		UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetNavigationSystem(World);
		FNavLocation ProjectedLocation;
		if (!NavigationSystem
			|| !NavigationSystem->ProjectPointToNavigation(
				GroundLocation,
				ProjectedLocation,
				NavigationProjectionExtent.GetAbs()))
		{
			return false;
		}
		GroundLocation = ProjectedLocation.Location;
	}

	const FVector ProjectedLocalLocation = BoundsTransform.InverseTransformPosition(GroundLocation);
	if (ProjectedLocalLocation.X < MinX
		|| ProjectedLocalLocation.X > MaxX
		|| ProjectedLocalLocation.Y < MinY
		|| ProjectedLocalLocation.Y > MaxY
		|| FMath::Abs(ProjectedLocalLocation.Z) > BoxExtent.Z + KINDA_SMALL_NUMBER)
	{
		return false;
	}

	if (bCheckClearance)
	{
		float CapsuleRadius = 0.0f;
		float CapsuleHalfHeight = 0.0f;
		GetPlacementCapsule(CapsuleRadius, CapsuleHalfHeight);

		FCollisionQueryParams ClearanceQuery(SCENE_QUERY_STAT(GobulinEnemySpawnClearance), false, this);
		const FCollisionShape SpawnShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
		const FVector CapsuleCenter = GroundLocation
			+ FVector::UpVector * (CapsuleHalfHeight + SpawnGroundClearance);
		if (World->OverlapBlockingTestByChannel(
			CapsuleCenter,
			FQuat::Identity,
			ECC_Pawn,
			SpawnShape,
			ClearanceQuery))
		{
			return false;
		}
	}

	const float SpawnYaw = SpawnBounds->GetComponentRotation().Yaw;
	OutGroundTransform = FTransform(
		FRotator(0.0f, SpawnYaw, 0.0f),
		GroundLocation,
		FVector::OneVector);
	return true;
}

bool AGobulinEnemySpawnArea::IsFarEnoughFromAccepted(
	const FVector& Location,
	const TArray<FTransform>& AcceptedTransforms) const
{
	const float RequiredSpacingSquared = FMath::Square(GetEffectiveMinimumSpacing());
	for (const FTransform& Accepted : AcceptedTransforms)
	{
		if (FVector::DistSquared2D(Location, Accepted.GetLocation()) < RequiredSpacingSquared)
		{
			return false;
		}
	}
	return true;
}

bool AGobulinEnemySpawnArea::GetLocalSamplingRange(
	float& OutMinX,
	float& OutMaxX,
	float& OutMinY,
	float& OutMaxY) const
{
	if (!SpawnBounds)
	{
		return false;
	}

	const FVector BoxExtent = SpawnBounds->GetUnscaledBoxExtent();
	const FVector ComponentScale = SpawnBounds->GetComponentScale().GetAbs();
	if (ComponentScale.X <= KINDA_SMALL_NUMBER || ComponentScale.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float LocalPaddingX = EdgePadding / ComponentScale.X;
	const float LocalPaddingY = EdgePadding / ComponentScale.Y;
	OutMinX = -BoxExtent.X + LocalPaddingX;
	OutMaxX = BoxExtent.X - LocalPaddingX;
	OutMinY = -BoxExtent.Y + LocalPaddingY;
	OutMaxY = BoxExtent.Y - LocalPaddingY;
	return OutMaxX > OutMinX && OutMaxY > OutMinY;
}

void AGobulinEnemySpawnArea::BuildCandidateCache(TArray<FTransform>& OutLocalCandidates) const
{
	OutLocalCandidates.Reset();
	if (!SpawnBounds)
	{
		return;
	}

	float MinX = 0.0f;
	float MaxX = 0.0f;
	float MinY = 0.0f;
	float MaxY = 0.0f;
	if (!GetLocalSamplingRange(MinX, MaxX, MinY, MaxY))
	{
		return;
	}

	const FVector ComponentScale = SpawnBounds->GetComponentScale().GetAbs();
	const float WorldWidth = (MaxX - MinX) * ComponentScale.X;
	const float WorldDepth = (MaxY - MinY) * ComponentScale.Y;
	const float EffectiveSpacing = GetEffectiveMinimumSpacing();
	const int32 EstimatedCapacity = FMath::FloorToInt(
		(WorldWidth * WorldDepth) / FMath::Square(EffectiveSpacing) * CandidatePackingFactor);
	const int32 TargetCount = FMath::Clamp(
		FMath::Max(SpawnCount, EstimatedCapacity),
		1,
		FMath::Clamp(MaximumCachedCandidateCount, 1, MaxSpawnCountPerBatch));

	TArray<FTransform> WorldCandidates;
	WorldCandidates.Reserve(TargetCount);
	FRandomStream RandomStream(RandomSeed);
	const int32 MaximumAttempts = FMath::Max(TargetCount * AttemptsPerRequestedCandidate, 128);
	for (int32 Attempt = 0; Attempt < MaximumAttempts && WorldCandidates.Num() < TargetCount; ++Attempt)
	{
		const FVector2D LocalXY(
			RandomStream.FRandRange(MinX, MaxX),
			RandomStream.FRandRange(MinY, MaxY));
		FTransform GroundTransform;
		if (TryResolveGroundCandidate(LocalXY, GroundTransform, bCheckSpawnClearance)
			&& IsFarEnoughFromAccepted(GroundTransform.GetLocation(), WorldCandidates))
		{
			WorldCandidates.Add(GroundTransform);
			const FVector LocalLocation = SpawnBounds->GetComponentTransform()
				.InverseTransformPosition(GroundTransform.GetLocation());
			OutLocalCandidates.Add(FTransform(FRotator::ZeroRotator, LocalLocation));
		}
	}
}

void AGobulinEnemySpawnArea::FinalizeSubmission(FEnemySpawnAreaSubmission& Submission)
{
	LastSubmission = Submission;
	OnSpawnBatchSubmitted.Broadcast(Submission);
}

void AGobulinEnemySpawnArea::HandleEnemySpawnResolved(const FEnemySpawnResolvedEvent& Event)
{
	if (!PendingCommandIds.Remove(Event.Result.CommandId))
	{
		return;
	}
	OnEnemySpawnResolved.Broadcast(Event.Result);
}

void AGobulinEnemySpawnArea::BindCombatEvents()
{
	if (UCombatEventSubsystem* Events = GetWorld()
		? GetWorld()->GetSubsystem<UCombatEventSubsystem>()
		: nullptr)
	{
		SpawnResolvedDelegateHandle = Events->OnEnemySpawnResolved().AddUObject(
			this,
			&AGobulinEnemySpawnArea::HandleEnemySpawnResolved);
	}
}

void AGobulinEnemySpawnArea::UnbindCombatEvents()
{
	if (!SpawnResolvedDelegateHandle.IsValid())
	{
		return;
	}

	if (UCombatEventSubsystem* Events = GetWorld()
		? GetWorld()->GetSubsystem<UCombatEventSubsystem>()
		: nullptr)
	{
		Events->OnEnemySpawnResolved().Remove(SpawnResolvedDelegateHandle);
	}
	SpawnResolvedDelegateHandle.Reset();
}

float AGobulinEnemySpawnArea::GetEffectiveMinimumSpacing() const
{
	float CapsuleRadius = 0.0f;
	float CapsuleHalfHeight = 0.0f;
	GetPlacementCapsule(CapsuleRadius, CapsuleHalfHeight);
	return FMath::Max(MinimumSpacing, CapsuleRadius * 2.0f + 2.0f);
}

void AGobulinEnemySpawnArea::GetPlacementCapsule(float& OutRadius, float& OutHalfHeight) const
{
	OutRadius = EnemyArchetype ? EnemyArchetype->Body.CapsuleRadius : 34.0f;
	OutHalfHeight = EnemyArchetype ? EnemyArchetype->Body.CapsuleHalfHeight : 88.0f;
	OutRadius = FMath::Max(1.0f, OutRadius);
	OutHalfHeight = FMath::Max(OutRadius, OutHalfHeight);
}
