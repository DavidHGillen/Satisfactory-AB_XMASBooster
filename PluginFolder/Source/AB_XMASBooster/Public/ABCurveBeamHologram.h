// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#pragma once

#include "FactoryGame.h"
#include "CoreMinimal.h"

#include "FGBeamHologram.h"
#include "Components/SplineMeshComponent.h"

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
	virtual void GetSupportedBuildModes_Implementation(TArray< TSubclassOf<UFGHologramBuildModeDescriptor> >& out_buildmodes) const override;
	virtual void OnBuildModeChanged() override;

	virtual bool DoMultiStepPlacement(bool isInputFromARelease) override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;

	virtual void PreConfigureActor(AFGBuildable* inBuildable) override;
	virtual void ConfigureActor(AFGBuildable* inBuildable) const override;

protected:
	virtual USceneComponent* SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName) override;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCurved;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCompoundCurve;

	// Custom:
	bool isBeamStarted = false;
	bool isBeamComplete = false;
	bool isAnyCurvedBeamMode = false;

	USplineMeshComponent* splineRef = NULL;
};
