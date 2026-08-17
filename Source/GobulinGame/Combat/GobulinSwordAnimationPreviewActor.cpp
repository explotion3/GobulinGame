#include "Combat/GobulinSwordAnimationPreviewActor.h"

#include "Channels/MovieSceneDoubleChannel.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveVector.h"
#include "Data/GobulinSwordDefinition.h"
#include "Engine/StaticMesh.h"
#include "LevelSequence.h"
#include "Math/UnrealMathUtility.h"
#include "MovieScene.h"
#include "MovieSceneBinding.h"
#include "MovieSceneSection.h"
#include "MovieSceneTrack.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "UObject/ConstructorHelpers.h"

#include "Combat/GobulinWeaponViewComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSwordAnimationPreview, Log, All);

namespace
{
	float UnwrapAngle(float PreviousAngle, float CurrentAngle)
	{
		return PreviousAngle + FMath::FindDeltaAngleDegrees(PreviousAngle, CurrentAngle);
	}

	bool EvaluateChannel(const FMovieSceneDoubleChannel* Channel, FFrameTime Time, double& OutValue)
	{
		if (!Channel)
		{
			OutValue = 0.0;
			return false;
		}

		if (Channel->Evaluate(Time, OutValue))
		{
			return true;
		}

		if (const TOptional<double> DefaultValue = Channel->GetDefault())
		{
			OutValue = DefaultValue.GetValue();
			return true;
		}

		OutValue = 0.0;
		return false;
	}

	void WriteVectorCurve(UCurveVector* Curve, const TArray<TPair<float, FVector>>& Keys)
	{
		if (!Curve)
		{
			return;
		}

		Curve->Modify();
		for (FRichCurve& FloatCurve : Curve->FloatCurves)
		{
			FloatCurve.Reset();
			FloatCurve.SetDefaultValue(0.0f);
		}

		for (const TPair<float, FVector>& Key : Keys)
		{
			const float Components[3] = { Key.Value.X, Key.Value.Y, Key.Value.Z };
			for (int32 ComponentIndex = 0; ComponentIndex < 3; ++ComponentIndex)
			{
				const FKeyHandle KeyHandle = Curve->FloatCurves[ComponentIndex].AddKey(Key.Key, Components[ComponentIndex]);
				FRichCurveKey& RichKey = Curve->FloatCurves[ComponentIndex].GetKey(KeyHandle);
				RichKey.InterpMode = RCIM_Cubic;
				RichKey.TangentMode = RCTM_Auto;
			}
		}

		Curve->MarkPackageDirty();
	}

	UMovieScene3DTransformSection* FindOnlyTransformSection(const UMovieScene* MovieScene, int32& OutTrackCount, int32& OutSectionCount)
	{
		OutTrackCount = 0;
		OutSectionCount = 0;
		UMovieScene3DTransformSection* Result = nullptr;

		auto InspectTracks = [&Result, &OutTrackCount, &OutSectionCount](const TArray<UMovieSceneTrack*>& Tracks)
		{
			for (UMovieSceneTrack* Track : Tracks)
			{
				UMovieScene3DTransformTrack* TransformTrack = Cast<UMovieScene3DTransformTrack>(Track);
				if (!TransformTrack)
				{
					continue;
				}

				++OutTrackCount;
				for (UMovieSceneSection* Section : TransformTrack->GetAllSections())
				{
					if (UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(Section))
					{
						++OutSectionCount;
						Result = Result ? Result : TransformSection;
					}
				}
			}
		};

		InspectTracks(MovieScene->GetTracks());
		for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
		{
			InspectTracks(Binding.GetTracks());
		}

		return Result;
	}
}

AGobulinSwordAnimationPreviewActor::AGobulinSwordAnimationPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SwordPivot = CreateDefaultSubobject<UGobulinWeaponViewComponent>(TEXT("SwordPivot"));
	RootComponent = SwordPivot;

	SwordVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordVisual"));
	SwordVisual->SetupAttachment(SwordPivot);
	SwordVisual->SetOnlyOwnerSee(false);
	SwordVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordVisual->SetCastShadow(false);
	SwordVisual->SetReceivesDecals(false);
	SwordVisual->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		SwordVisual->SetStaticMesh(PlaneMesh.Object);
	}

	SwordPivot->SetVisualMesh(SwordVisual);
	SetActorHiddenInGame(true);
}

void AGobulinSwordAnimationPreviewActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SwordPivot)
	{
		SwordPivot->SetVisualMesh(SwordVisual);
		SwordPivot->SetSwordDefinition(SwordDefinition);
	}
}

void AGobulinSwordAnimationPreviewActor::BakeAttackSequence()
{
#if WITH_EDITOR
	if (!SwordDefinition)
	{
		UE_LOG(LogSwordAnimationPreview, Error, TEXT("Bake failed: assign SwordDefinition first."));
		return;
	}

	if (!SwordDefinition->AttackLocationCurve || !SwordDefinition->AttackRotationCurve)
	{
		UE_LOG(LogSwordAnimationPreview, Error, TEXT("Bake failed: SwordDefinition needs both AttackLocationCurve and AttackRotationCurve."));
		return;
	}

	if (!AttackSequence)
	{
		UE_LOG(LogSwordAnimationPreview, Error, TEXT("Bake failed: assign AttackSequence first."));
		return;
	}

	UMovieScene* MovieScene = AttackSequence->GetMovieScene();
	if (!MovieScene)
	{
		UE_LOG(LogSwordAnimationPreview, Error, TEXT("Bake failed: AttackSequence has no MovieScene."));
		return;
	}

	int32 TransformTrackCount = 0;
	int32 TransformSectionCount = 0;
	UMovieScene3DTransformSection* TransformSection = FindOnlyTransformSection(MovieScene, TransformTrackCount, TransformSectionCount);
	if (!TransformSection || TransformTrackCount != 1 || TransformSectionCount != 1)
	{
		UE_LOG(LogSwordAnimationPreview, Error, TEXT("Bake failed: use a dedicated sequence with exactly one actor Transform Track and one section. Found %d tracks / %d sections."), TransformTrackCount, TransformSectionCount);
		return;
	}

	FMovieSceneChannelProxy& ChannelProxy = TransformSection->GetChannelProxy();
	FMovieSceneDoubleChannel* LocationChannels[3] = {
		ChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>(TEXT("Location.X")).Get(),
		ChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>(TEXT("Location.Y")).Get(),
		ChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>(TEXT("Location.Z")).Get()
	};
	FMovieSceneDoubleChannel* RotationChannels[3] = {
		ChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>(TEXT("Rotation.X")).Get(),
		ChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>(TEXT("Rotation.Y")).Get(),
		ChannelProxy.GetChannelByName<FMovieSceneDoubleChannel>(TEXT("Rotation.Z")).Get()
	};

	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (!LocationChannels[Index] || !RotationChannels[Index])
		{
			UE_LOG(LogSwordAnimationPreview, Error, TEXT("Bake failed: the Transform Track is missing a location or rotation channel."));
			return;
		}
	}

	TRange<FFrameNumber> BakeRange = TransformSection->GetRange();
	if (!BakeRange.HasLowerBound() || !BakeRange.HasUpperBound())
	{
		BakeRange = MovieScene->GetPlaybackRange();
	}
	if (!BakeRange.HasLowerBound() || !BakeRange.HasUpperBound())
	{
		UE_LOG(LogSwordAnimationPreview, Error, TEXT("Bake failed: set a finite Sequencer section or playback range."));
		return;
	}

	const FFrameNumber StartFrame = BakeRange.GetLowerBoundValue();
	const FFrameNumber EndFrame = BakeRange.GetUpperBoundValue();
	if (EndFrame <= StartFrame)
	{
		UE_LOG(LogSwordAnimationPreview, Error, TEXT("Bake failed: the Sequencer range has no duration."));
		return;
	}

	const FFrameRate TickResolution = MovieScene->GetTickResolution();
	const double DurationSeconds = TickResolution.AsSeconds(FFrameTime(EndFrame - StartFrame));
	if (DurationSeconds <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogSwordAnimationPreview, Error, TEXT("Bake failed: the Sequencer range duration is too small."));
		return;
	}

	const int32 SampleCount = FMath::Max(1, FMath::CeilToInt(DurationSeconds * static_cast<double>(FMath::Max(1, BakeSampleRate))));

	FVector RestLocation = FVector::ZeroVector;
	FRotator RestRotation = FRotator::ZeroRotator;
	// Sequencer Rotation.X/Y/Z are Roll/Pitch/Yaw, while the runtime curve
	// vector is consumed as FRotator(Pitch, Yaw, Roll).
	double* RestRotationComponents[3] = { &RestRotation.Roll, &RestRotation.Pitch, &RestRotation.Yaw };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		double Value = 0.0;
		EvaluateChannel(LocationChannels[Index], FFrameTime(StartFrame), Value);
		RestLocation[Index] = static_cast<float>(Value);
		EvaluateChannel(RotationChannels[Index], FFrameTime(StartFrame), Value);
		*RestRotationComponents[Index] = static_cast<float>(Value);
	}

	const FQuat RestQuaternion = RestRotation.Quaternion();
	float PreviousPitch = 0.0f;
	float PreviousYaw = 0.0f;
	float PreviousRoll = 0.0f;
	bool bHasPreviousRotation = false;
	TArray<TPair<float, FVector>> LocationKeys;
	TArray<TPair<float, FVector>> RotationKeys;
	LocationKeys.Reserve(SampleCount + 1);
	RotationKeys.Reserve(SampleCount + 1);

	for (int32 SampleIndex = 0; SampleIndex <= SampleCount; ++SampleIndex)
	{
		const float NormalizedTime = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
		const FFrameTime SampleTime = FFrameTime(StartFrame) + TickResolution.AsFrameTime(DurationSeconds * NormalizedTime);

		FVector SampleLocation = FVector::ZeroVector;
		FRotator SampleRotation = FRotator::ZeroRotator;
		double* SampleRotationComponents[3] = { &SampleRotation.Roll, &SampleRotation.Pitch, &SampleRotation.Yaw };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			double Value = 0.0;
			EvaluateChannel(LocationChannels[Index], SampleTime, Value);
			SampleLocation[Index] = static_cast<float>(Value);
			EvaluateChannel(RotationChannels[Index], SampleTime, Value);
			*SampleRotationComponents[Index] = static_cast<float>(Value);
		}

		const FVector LocationOffset = SampleLocation - RestLocation;
		const FRotator RelativeRotation = (RestQuaternion.Inverse() * SampleRotation.Quaternion()).Rotator();
		FRotator RotationOffset = RelativeRotation;
		if (bHasPreviousRotation)
		{
			RotationOffset.Pitch = UnwrapAngle(PreviousPitch, RotationOffset.Pitch);
			RotationOffset.Yaw = UnwrapAngle(PreviousYaw, RotationOffset.Yaw);
			RotationOffset.Roll = UnwrapAngle(PreviousRoll, RotationOffset.Roll);
		}
		PreviousPitch = RotationOffset.Pitch;
		PreviousYaw = RotationOffset.Yaw;
		PreviousRoll = RotationOffset.Roll;
		bHasPreviousRotation = true;

		LocationKeys.Emplace(NormalizedTime, LocationOffset);
		RotationKeys.Emplace(NormalizedTime, FVector(RotationOffset.Pitch, RotationOffset.Yaw, RotationOffset.Roll));
	}

	WriteVectorCurve(SwordDefinition->AttackLocationCurve, LocationKeys);
	WriteVectorCurve(SwordDefinition->AttackRotationCurve, RotationKeys);
	SwordDefinition->Modify();
	SwordDefinition->AttackDuration = static_cast<float>(DurationSeconds);
	SwordDefinition->MarkPackageDirty();
	SwordDefinition->PostEditChange();

	UE_LOG(LogSwordAnimationPreview, Display, TEXT("Baked %d samples from %s into %s."), SampleCount + 1, *GetNameSafe(AttackSequence), *GetNameSafe(SwordDefinition));
#else
	UE_LOG(LogSwordAnimationPreview, Warning, TEXT("BakeAttackSequence is editor-only."));
#endif
}
