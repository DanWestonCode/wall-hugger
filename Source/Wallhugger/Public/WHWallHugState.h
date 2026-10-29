// Copyright (c) 2026, Daniel Weston. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "WHWallHugState.generated.h"

UENUM(BlueprintType)
enum class EWallHugState : uint8
{
	ENone UMETA(DisplayName="None"),
	EMovingToWall UMETA(DisplayName="Moving to Wall"),
	EOnWall UMETA(DisplayName="On Wall"),
};
