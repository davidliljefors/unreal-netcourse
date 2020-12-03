#include "FGPlayer.h"
#include "../FGMovementStatics.h"
#include "../Components/FGMovementComponent.h"

#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerState.h"
#include "Camera/CameraComponent.h"
#include "Engine/NetDriver.h"
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

	if (IsLocallyControlled())
	{
		GetWorld()->GetTimerManager().SetTimer(NetTick, this, &AFGPlayer::SendNetUpdate,  1.0F / 60.0F, true);
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
		const float Friction = IsBraking() ? BrakingFriction : DefaultFriction;
		const float Alpha = FMath::Clamp(FMath::Abs(MovementVelocity / (MaxVelocity * 0.75F)), 0.0F, 1.0F);
		const float TurnSpeed = FMath::InterpEaseOut(0.0F, TurnSpeedDefault, Alpha, 5.0F);
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
	}
	else
	{
		const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), InterpTargetRotation, DeltaTime, InterpSpeedRotation);
		SetActorRotation(NewRot);

		const FVector NewLoc = FMath::VInterpTo(GetActorLocation(), InterpTargetLocation, DeltaTime, InterpSpeedLocation);
		SetActorLocation(NewLoc);
	}
}

void AFGPlayer::SendNetUpdate()
{
	Server_SendLocationData({ GetActorLocation(), GetActorForwardVector() * MovementVelocity, Timer });
	Server_SendRotation(GetActorRotation());
}

void AFGPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Accelerate"), this, &AFGPlayer::Handle_Accelerate);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AFGPlayer::Handle_Turn);

	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Pressed, this, &AFGPlayer::Handle_BrakePressed);
	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Released, this, &AFGPlayer::Handle_BrakeReleased);
}

int32 AFGPlayer::GetPing() const
{
	if (GetPlayerState())
	{
		return static_cast<int32>(GetPlayerState()->GetPing());
	}

	return 0;
}


void AFGPlayer::Server_SendRotation_Implementation(const FRotator& RotationToSend)
{
	Mulitcast_SendRotation(RotationToSend);
}

void AFGPlayer::Mulitcast_SendRotation_Implementation(const FRotator& RotationToSend)
{
	if (!IsLocallyControlled())
	{
		InterpTargetRotation = RotationToSend;
	}
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

void AFGPlayer::PredictedLocationUpdate(float DeltaTime)
{
	//const FVector MovementDelta = LastVelocity * DeltaTime;
	//AddActorWorldOffset(MovementDelta);
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
