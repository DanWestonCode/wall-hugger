// Copyright (c) 2026, Daniel Weston. All rights reserved.

#include "WHCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "WHWallHugComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AWHCharacter::AWHCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(FName("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(FName("UCameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComp);
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
	
	WallHugComp = CreateDefaultSubobject<UWHWallHugComponent>("WallHugComp");
}

void AWHCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

// Called every frame
void AWHCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AWHCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (bIsInputBound) return;
	
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWHCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Ongoing, this, &AWHCharacter::MoveUpdate);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AWHCharacter::MoveFinished);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AWHCharacter::MoveFinished);

		bIsInputBound = true;
	}
}

void AWHCharacter::Move(const FInputActionValue& Value)
{
	if (Controller == nullptr) return;
	
	// Input is a Vector2D
	LastInputVector = Value.Get<FVector2D>();	
	
	//	Determine if the Wall Hug component will move the character...
	if (WallHugComp->ProcessMovementInput(LastInputVector))
	{
		return;
	}

	// Find out which way is forward
	FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	// Get forward vector
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	// Get right vector 
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	// Add movement 
	AddMovementInput(ForwardDirection, LastInputVector.Y);
	AddMovementInput(RightDirection, LastInputVector.X);	
}

void AWHCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// add yaw and pitch input to controller
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AWHCharacter::MoveUpdate(const FInputActionValue& Value)
{
	if (Controller == nullptr) return;
	
	// Input is a Vector2D
	LastInputVector = Value.Get<FVector2D>();	
	
	//	Determine if the Wall Hug component will move the character...
	if (WallHugComp->ProcessMovementInput(LastInputVector)) return;
}

void AWHCharacter::MoveFinished(const FInputActionValue& Value)
{
	if (Controller == nullptr) return;
	
	LastInputVector = Value.Get<FVector2D>();	
	WallHugComp->ProcessMovementInput(LastInputVector);
}
