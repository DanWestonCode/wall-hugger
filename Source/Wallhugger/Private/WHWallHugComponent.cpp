// Copyright (c) 2026, Daniel Weston. All rights reserved.

#include "WHWallHugComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "WHWallHugState.h"
#include "WHCharacter.h"

UWHWallHugComponent::UWHWallHugComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UWHWallHugComponent::BeginPlay()
{
	Super::BeginPlay();	
	
	//	Cache the owner (it should be an APCharacter), we will need this for grabbing the last input ect...
	OwnerAPCharacter = Cast<AWHCharacter>(GetOwner());
	if (!OwnerAPCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("UWHWallHugComponent is not of type APCharacter"));
		return;
	}
	
	//	Cache the movement component, we will use this to determine things like if the player is falling and their walk speed ect. 
	CharacterMovementComponent = OwnerAPCharacter->GetCharacterMovement();
	if (!CharacterMovementComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to acquire UCharacterMovementComponent"));
		return;
	}	

	//	Grab the Characters capsule and setup OnHit
	CapsuleComponent = OwnerAPCharacter->GetCapsuleComponent();
	if (!CapsuleComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to acquire UCapsuleComponent"));
		return;
	}
	
	//	Setup callback for walking into things, we can determine if a wall hug begins in there!
	CapsuleComponent->OnComponentHit.AddDynamic(this, &UWHWallHugComponent::OnComponentHit);
	State = EWallHugState::ENone;
	
	//	Cache the standard default mesh relative location (could potentially be prone to issue later on...)
	DefaultMeshRelativeLocation = OwnerAPCharacter->GetMesh()->GetRelativeLocation();
}

void UWHWallHugComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	//	Determine what we need to process based on the current state of the wall hug!
	switch (State) 
	{
	case EWallHugState::EMovingToWall:
		//	Moving to the Wall? Rotates the player into the hug position.
		ProcessMovingToWallState();
		break;
	case EWallHugState::EOnWall:
		//	On the Wall? Determine what our back is currently on. 
		ProcessOnWallState();
		break;
		//	Don't do anything otherwise... maybe one day
	case EWallHugState::ENone:
	default: 
		break;
	}
	
	ProcessMeshRelativeOffset();
}

EWallHugState UWHWallHugComponent::GetState() const
{
	return State;
}

void UWHWallHugComponent::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//	Is this surface even "big" enough for the both of us?
	bool bSuccess = Hit.Location.Z >= MinWallHeight;
	//	We're already hugging a wall, lets not do this again...
	bSuccess &= State == EWallHugState::ENone;
	//	If we're not on the ground we can't hug walls...
	bSuccess &= !CharacterMovementComponent->IsFalling();
	//	Is the player approaching the wall within the correct angle? 
	bSuccess &= FVector::DotProduct(GetOwner()->GetActorForwardVector(), -Hit.Normal) >= 
		FMath::Cos(FMath::DegreesToRadians(ApproachingWallTolerance));
	
	//	So... are we going to do this? 
	if (!bSuccess) return;
	
	//	Sort out all the bits for starting the wall hug
	ProcessBeginWallHug(Hit);
}

bool UWHWallHugComponent::SweepCapsule(const FVector& Direction, const float Distance, FHitResult& HitResult) const
{
	//	Using the Characters Forward Vector, Sweep a capsule forward and determine the actual surface in front of the player!
	const float Radius = CapsuleComponent->GetScaledCapsuleRadius();
	const float HalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = Start + Direction * Distance;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	
	const bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		Start,
		End,
		FQuat::Identity,
		TraceChannelProperty,
		FCollisionShape::MakeCapsule(Radius, HalfHeight),
		Params
	);
	
	/*DrawDebugCapsule(
		 GetWorld(),
		 End,
		 HalfHeight,
		 Radius,
		 FQuat::Identity,
		 bHit ? FColor::Green : FColor::Red,
		 false,
		 0.1f
	 );	*/

	return bHit;
}

void UWHWallHugComponent::ProcessBeginWallHug(const FHitResult& Hit)
{
	//	Cache the starting Stick Input, we'll need this to determine if the 
	//	player is still holding the character against the wall 
	StartingWallHugInput = FVector(OwnerAPCharacter->GetLastInput().GetSafeNormal(), 0);
	//	Calculate the perpendicular axis which we will use to slide the character along the wall!
	SlideAxis = FVector::CrossProduct(StartingWallHugInput, FVector::UpVector);
	
	//	Cache the Rotation we were when starting the Wall Hug
	StartingRotation = GetOwner()->GetActorRotation();	
	//	Create a Rotation for the Character to end up in for wall hugging the surface
	TargetRotation = UKismetMathLibrary::MakeRotFromZX(FVector::UpVector, Hit.Normal);	
	//	Start the process of Moving into the wall hugging pose...
	State = EWallHugState::EMovingToWall;

	//	Prevent the character rotating in the direction of the wall slide
	bPrevOrientRotationToMovement = CharacterMovementComponent->bOrientRotationToMovement;
	CharacterMovementComponent->bOrientRotationToMovement = false;
	
	//	Set the desired walk speed whilst sliding on the wall (probably slower...)
	PrevWalkSpeed = CharacterMovementComponent->MaxWalkSpeed;
	CharacterMovementComponent->MaxWalkSpeed = WallHugWalkSpeed;
}

void UWHWallHugComponent::ProcessMovingToWallState()
{
	//	Rotate the Character to face away from the wall 
	ProcessRotateToWall(InitialToWallRotationSpeed);
	
	//	Are we on the wall? 
	if (GetOwner()->GetActorRotation().Equals(TargetRotation, CharacterAtRotationTolerance))
	{
		State = EWallHugState::EOnWall;
	}
}

void UWHWallHugComponent::ProcessOnWallState()
{
	//	Cast behind the player, is their back still up against a wall? 
	FHitResult Hit;
	if (SweepCapsule(-GetOwner()->GetActorForwardVector(), SweepDistance, Hit))
	{		
		if (Hit.Location.Z <= MinWallHeight)
		{
			ProcessEndWallHug();
			return;
		}
				
		//	Cache the Wall Normal, we use this later to calculate the end of a wall.
		WallNormal = Hit.ImpactNormal;
		//	Ensure the player is always keeping their back in line with the wall
		TargetRotation = UKismetMathLibrary::MakeRotFromZX(FVector::UpVector, WallNormal);	
		//	Determine the perpendicular of the wall normal, this will be the direction for our player to move.
		WallPerpendicular = FVector::CrossProduct(WallNormal, FVector::UpVector);
		//	Keep pushing our character back into the wall!
		OwnerAPCharacter->AddMovementInput(WallNormal, -1);
	}
	else
	{
		ProcessEndWallHug();
	}
	
	ProcessRotateToWall(WallRotationSpeed);
}

void UWHWallHugComponent::ProcessEndWallHug()
{
	State = EWallHugState::ENone;
	CharacterMovementComponent->bOrientRotationToMovement = bPrevOrientRotationToMovement;
	CharacterMovementComponent->MaxWalkSpeed = PrevWalkSpeed;
	//	Ensure that we bounce our character back to the Default relative location
	bIsMeshAtTargetRelativeLocation = true;
}

void UWHWallHugComponent::ProcessRotateToWall(const float Speed) const
{
	GetOwner()->SetActorRotation(FMath::RInterpTo(
		GetOwner()->GetActorRotation(),
		TargetRotation,
		GetWorld()->GetDeltaSeconds(),
		Speed));
}

void UWHWallHugComponent::ProcessMeshRelativeOffset()
{
	USkeletalMeshComponent* Mesh = OwnerAPCharacter->GetMesh();

	//	Determine the Target Mesh relative offset based on the Characters current state...
	switch (State) 
	{
	case EWallHugState::ENone:
		TargetMeshRelativeLocation = DefaultMeshRelativeLocation;
		break;
	case EWallHugState::EMovingToWall:
	case EWallHugState::EOnWall:
		TargetMeshRelativeLocation = DefaultMeshRelativeLocation + (OwnerAPCharacter->bIsCrouched ?
			MeshCrouchingWallHugOffset :
			MeshStandingWallHugOffset);		
		break;
	}
	
	//	Is the Character at the Target?
	bIsMeshAtTargetRelativeLocation = Mesh->GetRelativeLocation().Equals(TargetMeshRelativeLocation);
	if (bIsMeshAtTargetRelativeLocation) return;
	
	//	Lerp the Characters relative offset inside the mesh so it aligns with the wall.
	Mesh->SetRelativeLocation(FMath::VInterpTo(Mesh->GetRelativeLocation(),
		TargetMeshRelativeLocation,
		GetWorld()->GetDeltaSeconds(), 
		MeshOffsetInterpSpeed));
		
}

bool UWHWallHugComponent::CheckForWallBehindOwner(const FVector& Input)
{	
	//	Take the -forward (backwards) of the player and determine if it closely matches the input direction (mapped to the fwd of the game)
	if (FVector::DotProduct(-GetOwner()->GetActorForwardVector(), FVector(Input.Y, Input.X, 0).GetSafeNormal()) < 
		FMath::Cos(FMath::DegreesToRadians(MinimumPushBackAngle))) return false;
	
	//	Is there a wall behind the player?
	FHitResult SweepHit;
	if (!SweepCapsule(-GetOwner()->GetActorForwardVector(), SweepDistance, SweepHit)) return false;
	
	//	Character has is pushing directly back, check if there's a wall
	ProcessBeginWallHug(SweepHit);	
	return true;
}

bool UWHWallHugComponent::DetermineIfAtEndOfWall(const float DirectionScalar) const
{
	const float DirectionScalarSign = FMath::Sign(DirectionScalar);
	//	Oversize the players capsule slightly...
	const float CapsuleSize = CapsuleComponent->GetScaledCapsuleRadius() * CapsuleScale;
	//	Start the line trace to the side of the players capsule based on the direction of movement
	const FVector StartPosition = GetOwner()->GetActorLocation() + WallPerpendicular * (DirectionScalarSign * CapsuleSize);
	//	Trace back into the wall!
	const FVector EndPosition = StartPosition-WallNormal*CapsuleSize;
	
	FHitResult SweepHit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	const bool bHit = GetWorld()->LineTraceSingleByChannel (SweepHit, StartPosition, EndPosition, TraceChannelProperty);
	//DrawDebugLine(GetWorld(), StartPosition, EndPosition, bHit ? FColor::Green : FColor::Red, false, 1, 0, 1.5f);
	return !bHit; // we didn't hit anything, we must be at the end of the wall (I guess...)
}

bool UWHWallHugComponent::ProcessMovementInput(const FVector2D& Input)
{
	const FVector InputVector = FVector(Input, 0);

	if (State == EWallHugState::ENone)
	{
		//	Determine if the Player is pushing back into a wall to start another hug!
		return CheckForWallBehindOwner(InputVector);		
	}
	
	//	Has the player simply let go?
	if (Input.Length() == 0)
	{
		ProcessEndWallHug();
		return false;
	}
	
	//	We're turning to face the wall so digest the movement to prevent any other translation!
	if (State == EWallHugState::EMovingToWall) return true;
	
	//	--- EWallHugState::EOnWall! --- 
	
	//	Is the player still holding their stick/key against the wall?
	if (FVector::DotProduct(StartingWallHugInput, InputVector) < 
		FMath::Cos(FMath::DegreesToRadians(MinimumPushBackAngle)))
	{
		ProcessEndWallHug();
		return false;
	}
	
	//	Determine if we're pushing enough on the Right to slide...
	if (FMath::Abs(FVector::DotProduct(SlideAxis, InputVector)) < 
		FMath::Cos(FMath::DegreesToRadians(StickWallSlideThreshold)))
	{
		return true;
	}
	
	//	How much is the input vector facing the slide axis? 0=nothing, 1=facing, -1=opposite...
	const float DirectionScalar = FVector::DotProduct(InputVector, SlideAxis);
	
	//	If the character has reached the end of the wall we need to do nothing and still eat the input!
	if (DetermineIfAtEndOfWall(DirectionScalar)) return true;
	
	//	Slide the character along the wall!
	OwnerAPCharacter->AddMovementInput(WallPerpendicular, DirectionScalar);
	return true;	
}