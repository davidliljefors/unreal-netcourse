#pragma once

#include "GameFramework/Actor.h"
#include "FGRocket.generated.h"

class UStaticMeshComponent;
class UParticleSystem;

UCLASS()
class FGNET_API AFGRocket : public AActor
{
	GENERATED_BODY()
private:
	UPROPERTY(EditAnywhere)
	float MovementVelocity = 1300.0f;

	UPROPERTY(EditAnywhere)
	float LifeTime = 2.0f;

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystem* Explosion = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* MeshComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = Debug)
	bool bDebugDrawCorrection = true;

	FVector OriginalFacingDirection = FVector::ZeroVector;
	FVector FacingRotationStart = FVector::ZeroVector;
	FVector RocketStartLocation = FVector::ZeroVector;
	FQuat FacingRotationCorrection = FQuat::Identity;

	FCollisionQueryParams CachedCollisionQueryParams;

	bool bIsFree = true;
	float LifeTimeElapsed = 0.0f;
	float DistanceMoved = 0.0f;

public:
	AFGRocket();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void StartMoving(const FVector& Forward, const FVector& InStartLocation);
	void ApplyCorrection(const FVector& Forward);

	bool IsFree() const { return bIsFree; }

	void Explode(AActor* HitActor);
	void MakeFree();

private:
	void SetRocketVisibility(bool bVisible);

};