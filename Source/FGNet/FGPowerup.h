// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "FGPowerup.generated.h"

class AFGPlayer;


UCLASS(Blueprintable)
class FGNET_API UFGPowerup : public UObject
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintNativeEvent, Category = Powerup)
	void DoEffect(AFGPlayer* UsedBy, UWorld* WorldCtx);

	UFUNCTION(BlueprintCallable, Category = Utility)
	void SpawnActorFromClass(UWorld* WorldCtx, const FTransform& T, TSubclassOf<AActor> ActorToSpawnconst);

	UPROPERTY(EditDefaultsOnly, Category = Powerup)
	int32 UsesLeft = 1;
};
