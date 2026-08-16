#include "Combat/FPSWeaponOverlayComponent.h"

#include "Horde/SpriteFlipbook.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UFPSWeaponOverlayComponent::UFPSWeaponOverlayComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		SetStaticMesh(PlaneMesh.Object);
	}
}

void UFPSWeaponOverlayComponent::SetFlipbook(USpriteFlipbook* InFlipbook)
{
	Flipbook = InFlipbook;
	CurrentClipId = NAME_None;
	CurrentFrame = 0;
	AccumulatedTime = 0.0f;
	bPlaying = false;

	InitializeMaterial();
	if (!Flipbook)
	{
		return;
	}

	if (MaterialInstance && Flipbook->Atlas)
	{
		MaterialInstance->SetTextureParameterValue(FName("Texture"), Flipbook->Atlas);
	}

	ApplyFrameUV();
}

void UFPSWeaponOverlayComponent::SetSpriteMaterial(UMaterialInterface* InMaterial)
{
	if (SpriteMaterial == InMaterial)
	{
		return;
	}

	SpriteMaterial = InMaterial;
	MaterialInstance = nullptr;

	if (IsRegistered())
	{
		InitializeMaterial();
	}
}

void UFPSWeaponOverlayComponent::OnRegister()
{
	Super::OnRegister();
	InitializeMaterial();
}

void UFPSWeaponOverlayComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeMaterial();
}

void UFPSWeaponOverlayComponent::InitializeMaterial()
{
	if (!SpriteMaterial || MaterialInstance)
	{
		return;
	}

	MaterialInstance = CreateDynamicMaterialInstance(0, SpriteMaterial);
	SetMaterial(0, MaterialInstance);

	if (Flipbook && Flipbook->Atlas)
	{
		MaterialInstance->SetTextureParameterValue(FName("Texture"), Flipbook->Atlas);
	}

	ApplyFrameUV();
}

void UFPSWeaponOverlayComponent::PlayClip(FName ClipId, bool bRestart)
{
	if (!Flipbook)
	{
		return;
	}

	if (!bRestart && ClipId == CurrentClipId)
	{
		return;
	}

	const FSpriteClip* Clip = Flipbook->GetClip(ClipId);
	if (!Clip)
	{
		bPlaying = false;
		return;
	}

	CurrentClipId = ClipId;
	AccumulatedTime = 0.0f;
	CurrentFrame = 0;
	bPlaying = true;
	ApplyFrameUV();
}

void UFPSWeaponOverlayComponent::SetAiming(bool bInAiming)
{
	bAiming = bInAiming;
}

void UFPSWeaponOverlayComponent::AddRecoil(float Strength)
{
	CurrentRecoil = FMath::Max(CurrentRecoil, Strength < 0.0f ? RecoilKick : Strength);
}

void UFPSWeaponOverlayComponent::UpdateProceduralMotion(float DeltaTime, float MovementSpeed, const FVector2D& LookDelta, bool bIsGrounded)
{
	BobPhase += DeltaTime * BobFrequency * FMath::Clamp(MovementSpeed / 500.0f, 0.0f, 1.0f);

	const float SwayAlpha = 1.0f - FMath::Exp(-8.0f * DeltaTime);
	CurrentSway.X = FMath::Lerp(CurrentSway.X, LookDelta.X * SwayScale, SwayAlpha);
	CurrentSway.Y = FMath::Lerp(CurrentSway.Y, LookDelta.Y * SwayScale, SwayAlpha);

	CurrentRecoil = FMath::FInterpTo(CurrentRecoil, 0.0f, DeltaTime, RecoilRecoverySpeed);
}

void UFPSWeaponOverlayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickFrame(DeltaTime);
	ApplyProceduralTransform(DeltaTime);
}

void UFPSWeaponOverlayComponent::TickFrame(float DeltaTime)
{
	if (!bPlaying || !Flipbook)
	{
		return;
	}

	const FSpriteClip* Clip = Flipbook->GetClip(CurrentClipId);
	if (!Clip || Clip->FrameCount <= 0)
	{
		bPlaying = false;
		return;
	}

	AccumulatedTime += DeltaTime;
	const int32 NewFrame = FMath::FloorToInt(AccumulatedTime * Flipbook->FrameRate);
	if (Clip->bLoop)
	{
		CurrentFrame = NewFrame % Clip->FrameCount;
	}
	else
	{
		CurrentFrame = FMath::Min(NewFrame, Clip->FrameCount - 1);
		if (NewFrame >= Clip->FrameCount)
		{
			bPlaying = false;
		}
	}

	ApplyFrameUV();
}

void UFPSWeaponOverlayComponent::ApplyProceduralTransform(float DeltaTime)
{
	const float BobX = FMath::Sin(BobPhase) * BobAmplitude;
	const float BobY = FMath::Abs(FMath::Cos(BobPhase)) * BobAmplitude * 0.5f;

	FVector TargetLocation = RestLocation + FVector(0.0f, BobY + CurrentSway.Y, BobX + CurrentSway.X + CurrentRecoil);
	FRotator TargetRotation = RestRotation + FRotator(CurrentSway.Y * 0.5f, CurrentSway.X * 0.5f, 0.0f);

	if (bAiming)
	{
		TargetLocation = FMath::VInterpTo(GetRelativeLocation(), RestLocation + AimLocationOffset, DeltaTime, 12.0f);
		TargetRotation = FMath::RInterpTo(GetRelativeRotation(), RestRotation, DeltaTime, 12.0f);
	}

	SetRelativeLocation(TargetLocation);
	SetRelativeRotation(TargetRotation);
}

void UFPSWeaponOverlayComponent::ApplyFrameUV()
{
	if (!MaterialInstance || !Flipbook)
	{
		return;
	}

	const FVector2D Offset = Flipbook->GetFrameOffset(0, CurrentFrame);
	const FVector2D Size = Flipbook->GetFrameSize();
	MaterialInstance->SetVectorParameterValue(FName("UVOffset"), FLinearColor(Offset.X, Offset.Y, 0.0f, 0.0f));
	MaterialInstance->SetVectorParameterValue(FName("UVScale"), FLinearColor(Size.X, Size.Y, 0.0f, 0.0f));
}
