#include "Horde/SpriteAnimatorComponent.h"

#include "Horde/SpriteBillboardComponent.h"

USpriteAnimatorComponent::USpriteAnimatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USpriteAnimatorComponent::SetBillboard(USpriteBillboardComponent* InBillboard)
{
	Billboard = InBillboard;
	if (Billboard)
	{
		Billboard->PlayClip(IdleClip);
	}
}

void USpriteAnimatorComponent::SetClip(FName ClipId)
{
	if (Billboard)
	{
		Billboard->PlayClip(ClipId);
	}
}
