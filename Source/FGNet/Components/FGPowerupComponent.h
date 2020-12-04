#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FGPowerupComponent.generated.h"

class UFGPowerup;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FGNET_API UFGPowerupComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFGPowerupComponent();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = Powerup)
	void UsePowerup();

	UFUNCTION(Server, Reliable)
	void Server_UsePowerup(const FVector& LocationUsed);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_UsePowerup(const FVector& LocationUsed);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Powerup)
	TSubclassOf<AActor> ActorToSpawn;

};
