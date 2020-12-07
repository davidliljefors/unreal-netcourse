// Fill out your copyright notice in the Description page of Project Settings.

#include "Engine/World.h"
#include "FGPowerup.h"

void UFGPowerup::DoEffect_Implementation(AFGPlayer* UsedBy, UWorld* WorldCtx)
{
	
}

void UFGPowerup::SpawnActorFromClass(UWorld* WorldCtx, const FTransform& T, TSubclassOf<AActor> ActorToSpawn)
{
	WorldCtx->SpawnActor<AActor>(ActorToSpawn, T);
}
