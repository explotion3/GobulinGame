#include "Horde/SpriteBillboardComponent.h"

#include "Core/GameLog.h"
#include "Horde/SpriteFlipbook.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

USpriteBillboardComponent::USpriteBillboardComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		SetStaticMesh(PlaneMesh.Object);
	}

	// 基础形状 Plane 的法线朝 +Z；这里显式构造默认朝向（法线 +X、纸片正立），
	// 让编辑器预览与运行时一致，运行时由 UpdateFacing 接管。
	SetRelativeRotation(FRotationMatrix::MakeFromZX(FVector::ForwardVector, FVector::RightVector).ToQuat());
}

void USpriteBillboardComponent::SetFlipbook(USpriteFlipbook* InFlipbook)
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
	TryAutoPlay();
}

void USpriteBillboardComponent::SetSpriteMaterial(UMaterialInterface* InMaterial)
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

void USpriteBillboardComponent::OnRegister()
{
	Super::OnRegister();
	InitializeMaterial();
}

void USpriteBillboardComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeMaterial();
	TryAutoPlay();
}

void USpriteBillboardComponent::InitializeMaterial()
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

void USpriteBillboardComponent::PlayClip(FName ClipId, int32 DirectionIndex, bool bRestart)
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
		UE_LOG(LogHorde, Warning, TEXT("PlayClip: clip '%s' not found in flipbook '%s'"),
			*ClipId.ToString(), *GetNameSafe(Flipbook));
		bPlaying = false;
		return;
	}

	if (DirectionIndex >= 0)
	{
		SetDirection(DirectionIndex);
	}

	CurrentClipId = ClipId;
	AccumulatedTime = 0.0f;
	CurrentFrame = 0;
	bPlaying = true;
	ApplyFrameUV();
}

void USpriteBillboardComponent::SetDirection(int32 DirectionIndex)
{
	if (!Flipbook)
	{
		return;
	}

	CurrentDirection = FMath::Clamp(DirectionIndex, 0, FMath::Max(0, Flipbook->DirectionCount - 1));
	ApplyFrameUV();
}

void USpriteBillboardComponent::SetFacingDirection(const FVector& InFacingDirection)
{
	const FVector FacingH = FVector(InFacingDirection.X, InFacingDirection.Y, 0.0f).GetSafeNormal();
	if (!FacingH.IsNearlyZero())
	{
		FacingYaw = FMath::RadiansToDegrees(FMath::Atan2(FacingH.Y, FacingH.X));
	}

	// 显式朝向优先于速度方向。
	bFacingFollowsVelocity = false;
}

void USpriteBillboardComponent::SetFacingFollowsVelocity(bool bEnabled)
{
	bFacingFollowsVelocity = bEnabled;
}

void USpriteBillboardComponent::SetDefaultClipId(FName ClipId)
{
	DefaultClipId = ClipId;
	TryAutoPlay();
}

void USpriteBillboardComponent::TryAutoPlay()
{
	if (!bAutoPlayFirstClip || !Flipbook || bPlaying)
	{
		return;
	}

	FName ClipId = DefaultClipId;
	if (ClipId.IsNone() || !Flipbook->GetClip(ClipId))
	{
		if (Flipbook->Clips.Num() == 0)
		{
			UE_LOG(LogHorde, Warning, TEXT("TryAutoPlay: flipbook '%s' has no clips"),
				*GetNameSafe(Flipbook));
			return;
		}
		ClipId = Flipbook->Clips[0].ClipId;
	}

	PlayClip(ClipId);
}

void USpriteBillboardComponent::TickFrame(float DeltaTime)
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

void USpriteBillboardComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateFacing();
	TickFrame(DeltaTime);
}

void USpriteBillboardComponent::UpdateFacing()
{
	if (!bAutoFaceCamera)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. 逻辑朝向：默认跟随移动速度；也可由外部 SetFacingDirection 显式指定。
	if (bFacingFollowsVelocity)
	{
		const AActor* Owner = GetOwner();
		if (Owner)
		{
			const FVector Velocity = Owner->GetVelocity();
			if (!Velocity.IsNearlyZero())
			{
				const FVector VelocityH = FVector(Velocity.X, Velocity.Y, 0.0f).GetSafeNormal();
				FacingYaw = FMath::RadiansToDegrees(FMath::Atan2(VelocityH.Y, VelocityH.X));
			}
		}
	}

	// 2. 纸片正对本地玩家位置（而不是摄像机朝向），避免贴近玩家时出现边缘对向相机。
	const APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	FVector ViewerLocation;
	const AActor* ViewTarget = PlayerController->GetViewTarget();
	if (ViewTarget)
	{
		ViewerLocation = ViewTarget->GetActorLocation();
	}
	else if (PlayerController->PlayerCameraManager)
	{
		ViewerLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	}
	else
	{
		return;
	}

	const FVector ToViewerH = FVector(ViewerLocation - GetComponentLocation()).GetSafeNormal2D();
	if (ToViewerH.IsNearlyZero())
	{
		return;
	}

	// Plane 基础形状：法线 = 局部 +Z，UV 的“上” = 局部 +Y。
	// 显式构造基向量：局部 Z -> 指向玩家；局部 Y -> 世界 Z（纸片正立）；局部 X -> 屏幕右。
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToViewerH);
	SetWorldRotation(FRotationMatrix::MakeFromZX(ToViewerH, Right).ToQuat());

	// 3. 逐观察者选择方向行：正面 = 观察方向与敌人逻辑朝向一致。
	const int32 DirectionIndex = ComputeDirectionIndexForViewer(ToViewerH);
	if (DirectionIndex != CurrentDirection)
	{
		CurrentDirection = DirectionIndex;
		ApplyFrameUV();
	}
}

int32 USpriteBillboardComponent::ComputeDirectionIndexForViewer(const FVector& ToViewerH) const
{
	const int32 SafeDirections = FMath::Max(1, Flipbook ? Flipbook->DirectionCount : 1);
	if (SafeDirections <= 1)
	{
		return 0;
	}

	const float ViewerYaw = FMath::RadiansToDegrees(FMath::Atan2(ToViewerH.Y, ToViewerH.X));
	float DeltaYaw = FRotator::NormalizeAxis(ViewerYaw - FacingYaw);
	if (bReverseDirectionOrder)
	{
		DeltaYaw = -DeltaYaw;
	}

	const float DegreesPerDirection = 360.0f / SafeDirections;
	int32 DirectionIndex = FMath::RoundToInt(DeltaYaw / DegreesPerDirection);
	DirectionIndex %= SafeDirections;
	if (DirectionIndex < 0)
	{
		DirectionIndex += SafeDirections;
	}
	return DirectionIndex;
}

void USpriteBillboardComponent::ApplyFrameUV()
{
	if (!MaterialInstance || !Flipbook)
	{
		return;
	}

	// Clip 的 StartFrame 是图集中的绝对帧号（行优先），当前帧要在其基础上累加。
	int32 AbsoluteFrame = CurrentFrame;
	if (const FSpriteClip* Clip = Flipbook->GetClip(CurrentClipId))
	{
		AbsoluteFrame = Clip->StartFrame + CurrentFrame;
	}

	const FVector2D Offset = Flipbook->GetFrameOffset(CurrentDirection, AbsoluteFrame);
	const FVector2D Size = Flipbook->GetFrameSize();

	// Plane 基础形状的 UV 方向与图集约定相反：U/V 都翻转。
	// scale 取负 + offset 补一帧尺寸，让图集第 0 帧正立且不镜像。
	MaterialInstance->SetVectorParameterValue(FName("UVOffset"), FLinearColor(Offset.X + Size.X, Offset.Y + Size.Y, 0.0f, 0.0f));
	MaterialInstance->SetVectorParameterValue(FName("UVScale"), FLinearColor(-Size.X, -Size.Y, 0.0f, 0.0f));
}
