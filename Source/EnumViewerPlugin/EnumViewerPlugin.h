// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnumViewerPlugin.generated.h"

UENUM(BlueprintType)
enum class ETestEnum1 : uint8
{
	TypeA,
	TypeB,
	TypeC
};

UENUM(BlueprintType, meta = (DisplayName = "TestEnum1_Continuation"))
enum class ETestEnum2 : uint8
{
	TypeD,
	TypeE,
	TypeF
};
