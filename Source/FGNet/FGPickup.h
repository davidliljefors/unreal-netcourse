#pragma once

#include "GameFramework/Actor.h"
#include "FGPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EFGPickupType : uint8
{
	Rocket,
	Health
};

UCLASS()
class FGNET_API AFGPickup : public AActor
{
	GENERATED_BODY()
public:
	AFGPickup();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaTime) override;

	bool IsPickedUp() const { return bPickedUp; }
	void HandlePickup();
	void HidePickup();
	void ShowPickup();

	UPROPERTY(VisibleDefaultsOnly, Category = Collision)
	USphereComponent* SphereComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	UStaticMeshComponent*  MeshComponent;

	UPROPERTY(EditAnywhere)
	EFGPickupType PickupType = EFGPickupType::Rocket;

	UPROPERTY(EditAnywhere)
	int32 AmountOnPickup = 5;

	UPROPERTY(EditAnywhere)
	float ReActivateTime = 5.0F;

private:
	FVector CachedMeshRelativeLocation = FVector::ZeroVector;
	FTimerHandle ReActivateHandle;

	UFUNCTION()
	void ReActivatePickup();

	UFUNCTION()
	void OverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bPickedUp = false;
};