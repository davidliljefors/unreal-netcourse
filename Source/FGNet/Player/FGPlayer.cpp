#include "FGPlayer.h"
#include "FGPlayerSettings.h"
#include "../FGMovementStatics.h"
#include "../Components/FGMovementComponent.h"
#include "../Debug/FGNetDebugWidget.h"

#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerState.h"
#include "Camera/CameraComponent.h"
#include "Engine/NetDriver.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
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
}

void AFGPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Timer += DeltaTime;
	if (Timer > 300.0F)
	{
		Timer -= 300.0F;
	}

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
		FQuat WantedFacingDirection = FQuat(FVector::UpVector, FMath::DegreesToRadians(Yaw));
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
	//Server_SendLocationData({ GetActorLocation(), GetActorForwardVector() * MovementVelocity, Timer });
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

void AFGPlayer::Server_SendLocationData_Implementation(const FNetLocationData& DataToSend)
{
	Mulitcast_SendLocationData(DataToSend);
}

void AFGPlayer::Mulitcast_SendLocationData_Implementation(const FNetLocationData& DataToReceive)
{
	if (!IsLocallyControlled())
	{
		float Delta = DataToReceive.Timestamp - LastTimer;
		LastTimer = DataToReceive.Timestamp;
		LastVelocity = DataToReceive.Velocity;
		InterpTargetLocation = DataToReceive.Location + LastVelocity * Delta;
	}
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
}