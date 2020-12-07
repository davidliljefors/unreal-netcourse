
#include "FGPowerupComponent.h"
#include "../FGPowerup.h"
#include "../Player/FGPlayer.h"

UFGPowerupComponent::UFGPowerupComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UFGPowerupComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UFGPowerupComponent::UsePowerup()
{
	const FVector SpawnLocation = GetOwner()->GetActorLocation();
	Server_UsePowerup(SpawnLocation);
	GetWorld()->SpawnActor<AActor>(ActorToSpawn, SpawnLocation, FRotator::ZeroRotator);

}

void UFGPowerupComponent::Server_UsePowerup_Implementation(const FVector& LocationUsed)
{
	Multicast_UsePowerup(LocationUsed);
}

void UFGPowerupComponent::Multicast_UsePowerup_Implementation(const FVector& LocationUsed)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	if (OwnerPawn && !OwnerPawn->IsLocallyControlled())
	{
		GetWorld()->SpawnActor<AActor>(ActorToSpawn, LocationUsed, FRotator::ZeroRotator);
	}
}

