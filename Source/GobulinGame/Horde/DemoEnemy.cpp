#include "Horde/DemoEnemy.h"

#include "Combat/BattleAttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/BattleTags.h"
#include "Data/EnemyDefinition.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Horde/SpriteBillboardComponent.h"
#include "Horde/SpriteFlipbook.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

ADemoEnemy::ADemoEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	SpriteBillboard = CreateDefaultSubobject<USpriteBillboardComponent>(TEXT("SpriteBillboard"));
	SpriteBillboard->SetupAttachment(RootComponent);
	SpriteBillboard->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
	SpriteBillboard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpriteBillboard->SetCastShadow(false);

	Attributes = CreateDefaultSubobject<UBattleAttributeComponent>(TEXT("Attributes"));

	// 纸片敌人不需要骨骼网格与胶囊转向
	GetMesh()->SetHiddenInGame(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
}

void ADemoEnemy::BeginPlay()
{
	Super::BeginPlay();
	ApplyDefinition();
}

void ADemoEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDead)
	{
		return;
	}

	// 简单行为：朝第一个玩家水平移动（后续由 Mass/StateTree 接管）
	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			if (const APawn* PlayerPawn = PlayerController->GetPawn())
			{
				const FVector ToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
				const FVector Direction = FVector(ToPlayer.X, ToPlayer.Y, 0.0f).GetSafeNormal();
				if (!Direction.IsNearlyZero())
				{
					AddMovementInput(Direction);
				}
			}
		}
	}

	UpdateSpriteState();
}

void ADemoEnemy::ApplyDefinition()
{
	if (!EnemyDefinition)
	{
		return;
	}

	if (Attributes)
	{
		Attributes->SetBaseAttribute(BattleTag_Health_Max, EnemyDefinition->MaxHealth, 1.0f, 99999.0f);
		Attributes->SetBaseAttribute(BattleTag_Health_Current, EnemyDefinition->MaxHealth, 0.0f, 99999.0f);
	}

	GetCharacterMovement()->MaxWalkSpeed = EnemyDefinition->MoveSpeed;

	if (SpriteBillboard)
	{
		// 兜底：构造函数里的 FObjectFinder 在部分命令行环境下取不到 DreamShader 材质，运行时再试一次。
		if (!SpriteBillboard->GetSpriteMaterial())
		{
			if (UMaterialInterface* DefaultMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/_Game/Material/M_Flipbook.M_Flipbook")))
			{
				SpriteBillboard->SetSpriteMaterial(DefaultMaterial);
			}
		}

		ApplyNormalSprite();
	}
}

void ADemoEnemy::UpdateSpriteState()
{
	if (bDead || bPlayingHurt)
	{
		return;
	}

	ApplyNormalSprite();
}

void ADemoEnemy::ApplyNormalSprite()
{
	if (!SpriteBillboard || !EnemyDefinition)
	{
		return;
	}

	USpriteFlipbook* NormalFlipbook = EnemyDefinition->IdleFlipbook ? EnemyDefinition->IdleFlipbook : EnemyDefinition->Flipbook;
	if (SpriteBillboard->GetFlipbook() != NormalFlipbook)
	{
		SpriteBillboard->SetFlipbook(NormalFlipbook);
		ActiveClip = NAME_None;
	}

	const bool bMoving = GetVelocity().SizeSquared2D() > 25.0f;
	const FName DesiredClip = bMoving && EnemyDefinition->MoveClip != NAME_None
		? EnemyDefinition->MoveClip
		: EnemyDefinition->IdleClip;

	if (DesiredClip != NAME_None && DesiredClip != ActiveClip)
	{
		ActiveClip = DesiredClip;
		SpriteBillboard->PlayClip(DesiredClip);
	}
}

void ADemoEnemy::TakeDamage_Implementation(const FDamageInfo& DamageInfo)
{
	if (bDead || !Attributes)
	{
		return;
	}

	const float CurrentHealth = Attributes->GetAttributeValue(BattleTag_Health_Current);
	if (CurrentHealth <= 0.0f)
	{
		return;
	}

	const float NewHealth = FMath::Max(0.0f, CurrentHealth - DamageInfo.Amount);
	Attributes->SetBaseAttribute(BattleTag_Health_Current, NewHealth, 0.0f, 99999.0f);

	if (NewHealth <= 0.0f)
	{
		Die();
	}
	else
	{
		PlayHurtFeedback();
	}
}

void ADemoEnemy::PlayHurtFeedback()
{
	if (bDead || !SpriteBillboard || !EnemyDefinition)
	{
		return;
	}

	USpriteFlipbook* HurtFlipbook = EnemyDefinition->HurtFlipbook ? EnemyDefinition->HurtFlipbook : EnemyDefinition->Flipbook;
	if (!HurtFlipbook)
	{
		return;
	}

	bPlayingHurt = true;
	SpriteBillboard->SetFlipbook(HurtFlipbook);

	FName ClipToPlay = EnemyDefinition->HurtClip;
	if (ClipToPlay.IsNone() || !HurtFlipbook->GetClip(ClipToPlay))
	{
		ClipToPlay = HurtFlipbook->Clips.Num() > 0 ? HurtFlipbook->Clips[0].ClipId : NAME_None;
	}
	if (!ClipToPlay.IsNone())
	{
		SpriteBillboard->PlayClip(ClipToPlay);
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(HurtTimer, this, &ADemoEnemy::OnHurtTimer,
			FMath::Max(0.05f, EnemyDefinition->HurtFeedbackDuration), false);
	}
}

void ADemoEnemy::OnHurtTimer()
{
	bPlayingHurt = false;
	ApplyNormalSprite();
}

float ADemoEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	FDamageInfo DamageInfo;
	DamageInfo.Amount = DamageAmount;
	DamageInfo.Instigator = DamageCauser ? DamageCauser : (EventInstigator ? EventInstigator->GetPawn() : nullptr);
	DamageInfo.DamageSourceId = TEXT("ClassicDamage");
	IDamageable::Execute_TakeDamage(this, DamageInfo);
	return DamageAmount;
}

void ADemoEnemy::Die()
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorEnableCollision(false);

	if (SpriteBillboard && EnemyDefinition)
	{
		USpriteFlipbook* DeathFlipbook = EnemyDefinition->DeathFlipbook ? EnemyDefinition->DeathFlipbook : EnemyDefinition->Flipbook;
		if (DeathFlipbook)
		{
			SpriteBillboard->SetFlipbook(DeathFlipbook);
			FName ClipToPlay = EnemyDefinition->DeathClip;
			if (ClipToPlay.IsNone() || !DeathFlipbook->GetClip(ClipToPlay))
			{
				ClipToPlay = DeathFlipbook->Clips.Num() > 0 ? DeathFlipbook->Clips[0].ClipId : NAME_None;
			}
			if (!ClipToPlay.IsNone())
			{
				SpriteBillboard->PlayClip(ClipToPlay);
			}
		}
	}

	OnEnemyDied.Broadcast(this);

	if (DeathDestroyDelay > 0.0f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(DeathDestroyTimer, this, &ADemoEnemy::OnDeathDestroyTimer, DeathDestroyDelay, false);
	}
	else
	{
		Destroy();
	}
}

void ADemoEnemy::OnDeathDestroyTimer()
{
	Destroy();
}

void ADemoEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DeathDestroyTimer);
		GetWorld()->GetTimerManager().ClearTimer(HurtTimer);
	}
	Super::EndPlay(EndPlayReason);
}
