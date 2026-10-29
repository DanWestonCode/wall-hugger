// Copyright (c) 2026, Daniel Weston. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WHWallHugComponent.generated.h"

enum class EWallHugState : uint8;
class AWHCharacter;
class UCharacterMovementComponent;
class UCapsuleComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WALLHUGGER_API UWHWallHugComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWHWallHugComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	bool ProcessMovementInput(const FVector2D& Input);
protected:
	//	Cache of the Owner as APCharacter (Which it should always be, it's my game...s)
	UPROPERTY(Transient)
	TObjectPtr<AWHCharacter> OwnerAPCharacter;
	
	//	Cache of the owners Movement Component
	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> CharacterMovementComponent;
	
	//	Cache of the owners Capsule component
	UPROPERTY(Transient)
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	
	//	Trace Channel used by SweepSingleByChannel
	UPROPERTY(EditAnywhere, Category="Collision")
	TEnumAsByte<ECollisionChannel> TraceChannelProperty = ECC_Pawn;

	//	How far forward the SweepSingleByChannel will go
	UPROPERTY(EditAnywhere, Category="Collision")
	float SweepDistance = 1.0f;
	
	//	Minimum Height of a surface which we would consider huggable...
	UPROPERTY(EditAnywhere, Category="Collision")
	float MinWallHeight = 60.0f;
	
	//	Allowance for how "direct" the player needs to approach the wall (degrees)
	UPROPERTY(EditAnywhere, Category="Collision")
	float ApproachingWallTolerance = 10.0f;
	
	//	How fast the Character will rotate to face away from the wall when they START wall hugging
	UPROPERTY(EditAnywhere, Category="Moving to Wall")
	float InitialToWallRotationSpeed = 10.0f;
	
	//	How fast the Character will rotate to keep their back on the wall
	UPROPERTY(EditAnywhere, Category="Moving to Wall")
	float WallRotationSpeed = 10.0f;
	
	//	How fast the Character will rotate to keep their back on the wall
	UPROPERTY(EditAnywhere, Category="Moving to Wall")
	float WallHugWalkSpeed = 100.0f;
	
	//	Tolerance for determining equality of characters current rotation to TargetRotation
	UPROPERTY(EditAnywhere, Category="Moving to Wall")
	float CharacterAtRotationTolerance = 1.0f;
	
	//	How accurate a push back into a wall needs to be...
	UPROPERTY(EditAnywhere, Category="On Wall")
	float CapsuleScale = 1.25f;
	
	//	Threshold for considering the stick as "Sliding" across the wall
	UPROPERTY(EditAnywhere, Category="Input")
	float StickWallSlideThreshold = 70.0f;
	
	//	How accurate a push back into a wall needs to be...
	UPROPERTY(EditAnywhere, Category="Input")
	float MinimumPushBackAngle = 120.0f;
	
	//	Mesh Relative offset during wall hug whilst standing
	UPROPERTY(EditAnywhere, Category="Wall")
	FVector MeshStandingWallHugOffset = FVector(-12.f,0,0); 
	
	//	Mesh Relative offset during wall hug whilst crouching
	UPROPERTY(EditAnywhere, Category="Wall")
	FVector MeshCrouchingWallHugOffset = FVector(-8.f,0,0); 

	//	SetRelativeLocation VInterpTo speed
	UPROPERTY(EditAnywhere, Category="Wall")
	float MeshOffsetInterpSpeed = 10.f;
	
	//	Getter for State
	UFUNCTION(BlueprintPure)
	EWallHugState GetState() const;

	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

private:
	//	The current state
	EWallHugState State;
	//	The Rotation of the Character starting a Wall Hug.
	FRotator StartingRotation;
	//	The Rotation of the Character to be in the correct orientation for wall hugging  
	FRotator TargetRotation;
	//	Cached value of Owner bOrientRotationToMovement before starting Wall hug 
	bool bPrevOrientRotationToMovement;
	//	Cached value of Owner MaxWalkSpeed before starting Wall hug 
	float PrevWalkSpeed;
	//	Flag to determine if we're using the Right direction for sliding along the wall.
	bool bRightDirIntoWall;
	//	Flag to determine if our Character is at TargetMeshRelativeLocation (otherwise we're at DefaultMeshRelativeLocation)
	bool bIsMeshAtTargetRelativeLocation = false;
	//	The perpendicular of the current hugged wall normal
	UE::Math::TVector<double> WallPerpendicular;
	//	The Players Input used to begin wall hugging
	FVector StartingWallHugInput;
	//	The perpendicular of the StartingWallHugInput, the axis we will use to slide along the wall.
	FVector SlideAxis;
	//	The current Normal of the Wall we're huggin
	FVector WallNormal;
	//	Cache position of the mesh inside the capsule
	FVector DefaultMeshRelativeLocation;
	//	The target position of the mesh inside the capsule whilst wall hugging
	FVector TargetMeshRelativeLocation;

	//	Capsule Trace from the Actors forward location to determine obstruction.
	bool SweepCapsule(const FVector& Direction, float Distance, FHitResult& HitResult) const;
	//bool GetNearestPointToWall(FHitResult& HitResult);
	void ProcessBeginWallHug(const FHitResult& HitResult);
	//	Move the Character into the Wall Hugging Pose
	void ProcessMovingToWallState();
	//	Process Character movement whilst hugging 
	void ProcessOnWallState();
	//	Process logic to stop the Character wall hugging 
	void ProcessEndWallHug();
	//	Wrapper to RInterpTo Character to face away from the current wall Normal.
	void ProcessRotateToWall(float Speed) const;
	//	Translates the Characters Mesh's relative offset to align with the Surface
	void ProcessMeshRelativeOffset();
	//	Determine if the current input would restart a wall hug behind the player
	bool CheckForWallBehindOwner(const FVector& Input);
	//	Determine if the player has reached the end of the wall.
	bool DetermineIfAtEndOfWall(const float DirectionScalar) const;
};