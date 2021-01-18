#pragma once

#include "GameFramework/Pawn.h"
#include "Containers/Array.h"
#include "TimerManager.h"
#include "Engine/EngineTypes.h"
#include "FGNet/FGPickup.h"


#include "FGPlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFGMovementComponent;
class UStaticMeshComponent;
class USphereComponent;
class UFGPlayerSettings;
class UFGNetDebugWidget;
class AFGPickup;
class AFGRocket;


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

	void OnPickup(AFGPickup* Pickup);
	void PredictPickedUp(AFGPickup* Pickup);

	UFUNCTION(Server, Reliable)
	void Server_OnPickup(AFGPickup* Pickup);

	UFUNCTION(Server, Reliable)
	void Client_ConfirmPickup(AFGPickup* Pickup, bool bConfirmed);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnPickup(AFGPickup* Pickup);

	void ShowDebugMenu();
	void HideDebugMenu();
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_GetHit(AFGRocket* HitBy);

	UFUNCTION(NetMulticast, Reliable)
    void Multicast_GetHit(AFGRocket* HitBy, int32 NewHealth);

	UFUNCTION(NetMulticast, Reliable)
    void Multicast_UpdateItemCount(EFGPickupType Type, int32 ServerAmount);
	
	UFUNCTION(BlueprintImplementableEvent, Category = Player, meta = (DisplayName = "On Num Rockets Changed"))
	void BP_OnNumRocketsChanged(int32 NewAmount);

	UFUNCTION(BlueprintImplementableEvent, Category = Player, meta = (DisplayName = "On Health Changed"))
    void BP_OnHealthChanged(int32 NewAmount);
	
	void FireRocket();

	void SpawnRockets();

private:

	int32 GetNumActiveRockets() const;

	FVector GetRocketStartLocation() const;

	AFGRocket* GetFreeRocket() const;

	UFUNCTION(Server, Reliable)
	void Server_FireRocket(AFGRocket* Rocket, const FVector& RocketStartLocation, const FRotator& RocketStartRotation);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_FireRocket(AFGRocket* Rocket, const FVector& RocketStartLocation, const FRotator& RocketStartRotation);

	UFUNCTION(Client, Reliable)
	void Client_RemoveRocket(AFGRocket* ToRemove);


	void Handle_Accelerate(float Value);
	void Handle_Turn(float Value);
	void Handle_BrakePressed();
	void Handle_BrakeReleased();
	void Handle_FirePressed();

	void Handle_DebugMenuPressed();

	void CreateDebugWidget();

private:

	UPROPERTY(BlueprintReadOnly, Category = Player, meta = (AllowPrivateAccess = true))
	int32 Health = 100;

	int32 ServerHealth = 100;

	UPROPERTY(Replicated, Transient)
	TArray<AFGRocket*> RocketInstances;

	int32 ServerNumRockets = 0;

	UPROPERTY(BlueprintReadOnly, Category = Player, meta = (AllowPrivateAccess = true))
	int32 NumRockets = 0;

	UPROPERTY(EditAnywhere, Category = Weapon)
	TSubclassOf<AFGRocket> RocketClass;

	UPROPERTY(EditAnywhere, Category = Weapon)
	bool bUnlimitedRockets = false;

	int32 MaxActiveRockets = 3;

	float FireCooldownElapsed = 0.0f;

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
