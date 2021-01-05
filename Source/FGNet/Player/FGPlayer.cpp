#include "FGPlayer.h"
#include "FGPlayerSettings.h"
#include "../FGMovementStatics.h"
#include "../Components/FGMovementComponent.h"
#include "../Debug/FGNetDebugWidget.h"
#include "../FGPickup.h"
#include "../FGRocket.h"

#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerState.h"
#include "Camera/CameraComponent.h"
#include "Engine/NetDriver.h"
#include "Net/UnrealNetwork.h"
#include <Kismet/KismetSystemLibrary.h>

FTimerHandle NetTick;

AFGPlayer::AFGPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->bInheritYaw = false;
	SpringArmComponent->SetupAttachment(CollisionComponent);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);

	MovementComponent = CreateDefaultSubobject<UFGMovementComponent>(TEXT("MovementComponent"));

	
	SetReplicateMovement(false);
}

void AFGPlayer::BeginPlay()
{
	Super::BeginPlay();
	MovementComponent->SetUpdatedComponent(CollisionComponent);

	CreateDebugWidget();
	if (DebugMenuInstance != nullptr)
	{
		DebugMenuInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsLocallyControlled())
	{
		GetWorld()->GetTimerManager().SetTimer(NetTick, this, &AFGPlayer::SendNetUpdate, 1.0F / 60.0F, true);
	}

	SpawnRockets();
}

void AFGPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FireCooldownElapsed -= DeltaTime; 

	if (IsLocallyControlled())
	{
		if (!ensure(PlayerSettings != nullptr))
		{
			return;
		}

		const float MaxVelocity = PlayerSettings->MaxVelocity;
		const float Acceleration = PlayerSettings->Acceleration;
		const float Friction = IsBraking() ? PlayerSettings->BrakingFriction : PlayerSettings->Friction;
		const float Alpha = FMath::Clamp(FMath::Abs(MovementVelocity / (MaxVelocity * 0.75F)), 0.0F, 1.0F);
		const float TurnSpeed = FMath::InterpEaseOut(0.0F, PlayerSettings->TurnSpeedDefault, Alpha, 5.0F);
		const float MovementDirection = MovementVelocity > 0.0F ? Turn : -Turn;

		Yaw += (MovementDirection * TurnSpeed) * DeltaTime;
		const FQuat WantedFacingDirection = FQuat(FVector::UpVector, FMath::DegreesToRadians(Yaw));
		MovementComponent->SetFacingRotation(WantedFacingDirection);

		FFGFrameMovement FrameMovement = MovementComponent->CreateFrameMovement();

		MovementVelocity += Forward * Acceleration * DeltaTime;
		MovementVelocity = FMath::Clamp(MovementVelocity, -MaxVelocity, MaxVelocity);
		MovementVelocity *= FMath::Pow(Friction, DeltaTime);

		MovementComponent->ApplyGravity();
		FrameMovement.AddDelta(GetActorForwardVector() * MovementVelocity * DeltaTime);
		MovementComponent->Move(FrameMovement);

		ReplicatedYaw = MovementComponent->GetFacingRotation().Yaw;
	}
	else
	{
		MovementComponent->SetFacingRotation(FRotator(0.0F, ReplicatedYaw, 0.0F), 7.0F);
		SetActorRotation(MovementComponent->GetFacingRotation());

		const FVector NewLoc = FMath::VInterpTo(GetActorLocation(), ReplicatedLocation, DeltaTime, InterpSpeedLocation);
		SetActorLocation(NewLoc);
	}
}

void AFGPlayer::SendNetUpdate()
{
	Server_SendLocation(GetActorLocation());
	Server_SendYaw(GetActorRotation().Yaw);
}

void AFGPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Accelerate"), this, &AFGPlayer::Handle_Accelerate);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AFGPlayer::Handle_Turn);

	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Pressed, this, &AFGPlayer::Handle_BrakePressed);
	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Released, this, &AFGPlayer::Handle_BrakeReleased);

	PlayerInputComponent->BindAction(TEXT("DebugMenu"), IE_Pressed, this, &AFGPlayer::Handle_DebugMenuPressed);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AFGPlayer::Handle_FirePressed);
}

int32 AFGPlayer::GetPing() const
{
	if (GetPlayerState())
	{
		return static_cast<int32>(GetPlayerState()->GetPing());
	}

	return 0;
}

void AFGPlayer::Server_SendYaw_Implementation(float NewYaw)
{
	ReplicatedYaw = NewYaw;
}

void AFGPlayer::Server_SendLocation_Implementation(const FVector& NewLocation)
{
	ReplicatedLocation = NewLocation;
}

void AFGPlayer::OnPickup(AFGPickup* Pickup)
{
	if(IsLocallyControlled())
	{
		Pickup->HidePickup();
		PredictPickedUp(Pickup);
		Server_OnPickup(Pickup);
	}
}

void AFGPlayer::PredictPickedUp(AFGPickup* Pickup)
{
	switch(Pickup->PickupType)
	{
	case EFGPickupType::Health:
		{
			Health += Pickup->AmountOnPickup;
			break;
		}
	case EFGPickupType::Rocket:
		{
			NumRockets += Pickup->AmountOnPickup;
			BP_OnNumRocketsChanged(NumRockets);
			break;
		}
	default:
		ensure(false); // "Unhandled pickup type"
	}
}

void AFGPlayer::Server_OnPickup_Implementation(AFGPickup* Pickup)
{
	UE_LOG(LogTemp, Warning, TEXT("Serverside, Name: %s"), *Pickup->GetName());

	if(!Pickup->IsPickedUp())
	{
		ServerNumRockets += Pickup->AmountOnPickup;
		Multicast_OnPickup(Pickup);
	}
	if(GetLocalRole() < ROLE_Authority)
	{
		Client_ConfirmPickup(Pickup, !Pickup->IsPickedUp());
	}
}

void AFGPlayer::Client_ConfirmPickup_Implementation(AFGPickup* Pickup, bool bConfirmed)
{
	if(!bConfirmed) // Pickup wasn't confirmed. Rollback.
	{
		switch(Pickup->PickupType)
		{
		case EFGPickupType::Health:
			{
				Health -= Pickup->AmountOnPickup;
				Pickup->ShowPickup();
				break;
			}
		case EFGPickupType::Rocket:
			{
				NumRockets -= Pickup->AmountOnPickup;
				BP_OnNumRocketsChanged(NumRockets);
				Pickup->ShowPickup();
				break;
			}
		default:
			ensure(false); // "Unhandled pickup type"
		}
	}
}

void AFGPlayer::Multicast_OnPickup_Implementation(AFGPickup* Pickup)
{
	Pickup->HandlePickup();
}

void AFGPlayer::ShowDebugMenu()
{
	CreateDebugWidget();

	if (DebugMenuInstance == nullptr)
	{
		return;
	}

	DebugMenuInstance->SetVisibility(ESlateVisibility::Visible);
	DebugMenuInstance->BP_OnShowWidget();
}

void AFGPlayer::HideDebugMenu()
{
	if (DebugMenuInstance == nullptr)
	{
		return;
	}

	DebugMenuInstance->SetVisibility(ESlateVisibility::Collapsed);
	DebugMenuInstance->BP_OnHideWidget();
}

void AFGPlayer::FireRocket()
{
	if (FireCooldownElapsed > 0.0f)
	{
		return;
	}

	if (NumRockets <= 0 && !bUnlimitedRockets)
	{
		UE_LOG(LogTemp, Warning, TEXT("NumRockets: %i"), NumRockets);
		return;
	}

	if (GetNumActiveRockets() >= MaxActiveRockets)
	{
		return;
	}

	AFGRocket* NewRocket = GetFreeRocket();
	
	if (!ensure(NewRocket != nullptr))
	{
		return;
	}

	FireCooldownElapsed = PlayerSettings->FireCooldown;

	if (GetLocalRole() >= ROLE_AutonomousProxy)
	{
		if (HasAuthority())
		{
			Server_FireRocket(NewRocket, GetRocketStartLocation(), GetActorRotation());
			BP_OnNumRocketsChanged(NumRockets);
		}
		else
		{
			NumRockets--;
			NewRocket->StartMoving(GetActorForwardVector(), GetRocketStartLocation());
			Server_FireRocket(NewRocket, GetRocketStartLocation(), GetActorRotation());
			BP_OnNumRocketsChanged(NumRockets);
		}
	}
}

void AFGPlayer::SpawnRockets()
{
	if (HasAuthority() && RocketClass != nullptr)
	{
		const int32 RocketCache = 8;

		for (int32 Index = 0; Index < RocketCache; ++Index)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			SpawnParams.ObjectFlags = RF_Transient;
			SpawnParams.Instigator = this;
			SpawnParams.Owner = this;
			AFGRocket* NewRocketInstance = GetWorld()->SpawnActor<AFGRocket>(RocketClass, GetActorLocation(), GetActorRotation(), SpawnParams);
			RocketInstances.Add(NewRocketInstance);
		}
	}
}

int32 AFGPlayer::GetNumActiveRockets() const
{
	int32 NumActive = 0;
	for (AFGRocket* Rocket : RocketInstances)
	{
		if (!Rocket->IsFree())
		{
			NumActive++;
		}
	}

	return NumActive;
}

FVector AFGPlayer::GetRocketStartLocation() const
{
	const FVector StartLoc = GetActorLocation() + GetActorForwardVector() * 100.0f;
	return StartLoc;
}

AFGRocket* AFGPlayer::GetFreeRocket() const
{
	for (AFGRocket* Rocket : RocketInstances)
	{
		if (Rocket == nullptr)
		{
			continue;
		}

		if (Rocket->IsFree())
		{
			return Rocket;
		}
	}

	return nullptr;
}

void AFGPlayer::Server_FireRocket_Implementation(AFGRocket* Rocket, const FVector& RocketStartLocation, const FRotator& RocketStartRotation)
{
	if ((ServerNumRockets - 1) < 0 && !bUnlimitedRockets)
	{
		Client_RemoveRocket(Rocket);
	}
	else
	{
		const float DeltaYaw = FMath::FindDeltaAngleDegrees(RocketStartRotation.Yaw, GetActorForwardVector().Rotation().Yaw);
		const FRotator NewFacingRotation = RocketStartRotation + FRotator(0.0f, DeltaYaw, 0.0f);
		ServerNumRockets--;
		Multicast_FireRocket(Rocket, RocketStartLocation, NewFacingRotation);
	}
}

void AFGPlayer::Multicast_FireRocket_Implementation(AFGRocket* Rocket, const FVector& RocketStartLocation, const FRotator& RocketStartRotation)
{
	if (!ensure(Rocket != nullptr))
	{
		return;
	}

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		Rocket->ApplyCorrection(RocketStartRotation.Vector());
	}
	else
	{
		NumRockets--;
		Rocket->StartMoving(RocketStartRotation.Vector(), RocketStartLocation);
	}
}

void AFGPlayer::Client_RemoveRocket_Implementation(AFGRocket* ToRemove)
{
	ToRemove->MakeFree();
}

void AFGPlayer::Handle_Accelerate(float Value)
{
	Forward = Value;
}

void AFGPlayer::Handle_Turn(float Value)
{
	Turn = Value;
}

void AFGPlayer::Handle_BrakePressed()
{
	bIsBraking = true;
}

void AFGPlayer::Handle_BrakeReleased()
{
	bIsBraking = false;
}

void AFGPlayer::Handle_FirePressed()
{
	FireRocket();
}

void AFGPlayer::Handle_DebugMenuPressed()
{
	bShowDebugMenu = !bShowDebugMenu;

	if (bShowDebugMenu)
	{
		ShowDebugMenu();
	}
	else
	{
		HideDebugMenu();
	}
}

void AFGPlayer::CreateDebugWidget()
{
	if (DebugMenuClass == nullptr)
	{
		return;
	}

	if (!IsLocallyControlled())
	{
		return;
	}

	if (DebugMenuInstance == nullptr)
	{
		DebugMenuInstance = CreateWidget<UFGNetDebugWidget>(GetWorld(), DebugMenuClass);
		DebugMenuInstance->AddToViewport();
	}

}


void AFGPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFGPlayer, ReplicatedYaw);
	DOREPLIFETIME(AFGPlayer, ReplicatedLocation);
	DOREPLIFETIME(AFGPlayer, RocketInstances);
}