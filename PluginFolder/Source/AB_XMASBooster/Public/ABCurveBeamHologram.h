// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#pragma once

#include "FactoryGame.h"
#include "CoreMinimal.h"

#include "FGBeamHologram.h"

#include "ABCurveBeamHologram.generated.h"

/**
 * Provides a way to create a curvebeam by adding further buildmodes and steps to a regular beam
 * Expects the buildable to have a spline
 */
UCLASS()
class AB_XMASBOOSTER_API AABCurveBeamHologram : public AFGBeamHologram
{
	GENERATED_BODY()

public:
	AABCurveBeamHologram();

	// Begin AFGHologram interface
	//virtual bool IsValidHitResult(const FHitResult& hitResult) const override;
	//virtual bool TrySnapToActor(const FHitResult& hitResult) override;
	//virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual bool DoMultiStepPlacement(bool isInputFromARelease) override;
	//virtual int32 GetRotationStep() const override;
	virtual void GetSupportedBuildModes_Implementation(TArray< TSubclassOf<UFGHologramBuildModeDescriptor> >& out_buildmodes) const override;
	virtual void ConfigureActor(AFGBuildable* inBuildable) const override;
	//virtual int32 GetBaseCostMultiplier() const override;
	//virtual bool CanBeZooped() const override;
	//virtual bool CanIntersectWithDesigner(AFGBuildableBlueprintDesigner* designer) override;
	// End AFGHologram interface

	//virtual void OnPendingConstructionHologramCreated_Implementation(AFGHologram* fromHologram) override;


protected:
	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCurved;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCompoundCurve;

	// Custom:
	bool bTangentInSet = false;
	bool bTangentOutSet = false;
};
