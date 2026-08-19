#include "Enemy/GobulinEnemySwarmBoundary.h"

#include "Components/BoxComponent.h"
#include "Core/GobulinCollisionChannels.h"

AGobulinEnemySwarmBoundary::AGobulinEnemySwarmBoundary()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	BoundaryBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundaryBox"));
	SetRootComponent(BoundaryBox);
	BoundaryBox->SetBoxExtent(FVector(100.0f, 1000.0f, 5000.0f));
	BoundaryBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoundaryBox->SetCollisionObjectType(GobulinCollision::EnemySwarmBoundary);
	BoundaryBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoundaryBox->SetCollisionResponseToChannel(GobulinCollision::EnemyBody, ECR_Block);
	BoundaryBox->SetCollisionResponseToChannel(GobulinCollision::EnemyCorpse, ECR_Block);
	BoundaryBox->SetGenerateOverlapEvents(false);
	BoundaryBox->SetHiddenInGame(true);
	BoundaryBox->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
}
