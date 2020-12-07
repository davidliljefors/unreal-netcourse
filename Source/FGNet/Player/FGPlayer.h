#pragma once

#include "GameFramework/Pawn.h"
#include "Containers/Array.h"
#include "TimerManager.h"
#include "Engine/EngineTypes.h"

#include "FGPlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFGMovementComponent;
class UStaticMeshComponent;
class USphereComponent;
class UFGPlayerSettings;
class UFGNetDebugWidget;


USTRUCT()
struct FNetLocationData
{
	GENERATED_BODY()

	FNetLocationData() = default;

	FNetLocationData(const FVector& InLocation, const FVector& InVelocity, float InTimestamp)
		:Location(InLocation), Velocity(InVelocity), Timestamp(InTimestamp)
	{}

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY()
	float Timestamp = 0.0F;
};

UCLASS()
class FGNET_API AFGPlayer : public APawn
{
	GENERATED_BODY()

public:
	AFGPlayer();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void SendNetUpdate();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = Settings)
	UFGPlayerSettings* PlayerSettings = nullptr;

	UFUNCTION(BlueprintPure)
	bool IsBraking() const { return bIsBraking; }

	UFUNCTION(BlueprintPure)
	int32 GetPing() const;

	UPROPERTY(EditAnywhere, Category = Debug)
	TSubclassOf<UFGNetDebugWidget> DebugMenuClass;

	UFUNCTION(Server, Unreliable)
	void Server_SendYaw(float NewYaw);

	UFUNCTION(Server, Unreliable)
	void Server_SendLocation(const FVector& NewLocation);

	UFUNCTION(Server, Unreliable)
	void Server_SendLocationData(const FNetLocationData& DataToSend);

	UFUNCTION(NetMulticast, Unreliable)
	void Mulitcast_SendLocationData(const FNetLocationData& DataToSend);
	
	void ShowDebugMenu();
	void HideDebugMenu();

private:
	float Timer = 0.0F;
	float LastTimer = 0.0F;
	FVector LastVelocity = FVector::ZeroVector;

	UFUNCTION(BlueprintPure)
	float GetTimer() const { return Timer; }

	UFUNCTION(BlueprintPure)
	float GetLastTimer() const { return LastTimer; }

	void Handle_Accelerate(float Value);
	void Handle_Turn(float Value);
	void Handle_BrakePressed();
	void Handle_BrakeReleased();

	void Handle_DebugMenuPressed();

	void CreateDebugWidget();

	UPROPERTY(Transient)
	UFGNetDebugWidget* DebugMenuInstance = nullptr;

	bool bShowDebugMenu = false;

	UPROPERTY(Replicated)
	float ReplicatedYaw = 0.0F;

	UPROPERTY(Replicated)
	FVector ReplicatedLocation;

	float Forward = 0.0F;
	float Turn = 0.0F;

	float MovementVelocity = 0.0F;
	float Yaw = 0.0F;

	UPROPERTY(EditAnywhere, Category = Movement)
	float InterpSpeedLocation = 5.0F; 

	FVector InterpTargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = Movement)
	float InterpSpeedRotation = 5.0F;

	FRotator InterpTargetRotation = FRotator::ZeroRotator;


	bool bIsBraking = false;

	UPROPERTY(VisibleDefaultsOnly, Category = Collision)
	USphereComponent* CollisionComponent;

	UPROPERTY(VisibleDefaultsOnly, Category =  Mesh)
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = Camera)
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = Camera)
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = Movement)
	UFGMovementComponent* MovementComponent;
};
