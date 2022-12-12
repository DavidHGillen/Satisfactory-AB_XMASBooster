// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ABICurveBeamHologram.generated.h"

UINTERFACE(MinimalAPI)
class UABICurveBeamHologram : public UInterface
{
	GENERATED_BODY()
};

/**
 * Provide a standardized way to pass the important data back to the buildable from the hologram that the construction script can catch
 */
class AB_XMASBOOSTER_API IABICurveBeamHologram
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
		void UpdateSplineData(bool bUseSpline, FVector EndPosition, FVector StartTangent, FVector EndTangent);
};
