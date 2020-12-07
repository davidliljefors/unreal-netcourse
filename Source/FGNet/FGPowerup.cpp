
#include "FGPowerup.h"
#include "Engine/World.h"

void UFGPowerup::DoEffect_Implementation(AFGPlayer* UsedBy, UWorld* WorldCtx)
{
	
}

void UFGPowerup::SpawnActorFromClass(UWorld* WorldCtx, const FTransform& T, TSubclassOf<AActor> ActorToSpawn)
{
	WorldCtx->SpawnActor<AActor>(ActorToSpawn, T);
}
