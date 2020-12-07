#pragma once

#include "Engine/DataAsset.h"
#include "FGPlayerSettings.generated.h"

UCLASS()
class FGNET_API UFGPlayerSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()
	

public:
	UPROPERTY(EditAnywhere, Category = Movmement)
	float Acceleration = 500.0F;

	UPROPERTY(EditAnywhere, Category = Movmement, meta = (DisplayName = "TurnSpeed"))
	float TurnSpeedDefault = 100.0F;

	UPROPERTY(EditAnywhere, Category = Movmement)
	float MaxVelocity = 2000.0F;

	UPROPERTY(EditAnywhere, Category = Movmement, meta = (ClampMin = 0.0, ClampMax = 1.0))
	float Friction = 0.75F;

	UPROPERTY(EditAnywhere, Category = Movmement, meta = (ClampMin = 0.0, ClampMax = 1.0))
	float BrakingFriction = 0.001F;
};
