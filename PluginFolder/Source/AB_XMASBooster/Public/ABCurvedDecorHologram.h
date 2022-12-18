// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#pragma once

#include "CoreMinimal.h"
#include "Hologram/FGDecorHologram.h"
#include "Components/SplineMeshComponent.h"

#include "ABCurvedDecorHologram.generated.h"

UENUM()
enum class EBendHoloState : uint8
{
	CDH_Placing =  0b0000,
	CDH_Zooping =  0b0001,
	CDH_Bend_A1 =  0b0010,
	CDH_Bend_B1 =  0b0100,
	CDH_Bend_B2 =  0b1000,

	CDHM_Bending = 0b1110,
	CDHM_BendIn =  0b0111,
	CDHM_BendOut = 0b1011
};

/**
 * Provides a way to create a curved static mesh. Expects the buildable to have a Spline Mesh
 */
UCLASS()
class AB_XMASBOOSTER_API AABCurvedDecorHologram : public AFGDecorHologram
{
	GENERATED_BODY()

public:
	AABCurvedDecorHologram();

	virtual void GetSupportedBuildModes_Implementation(TArray< TSubclassOf<UFGHologramBuildModeDescriptor> >& out_buildmodes) const override;
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;

	virtual void OnBuildModeChanged() override;
	virtual void PreHologramPlacement() override;
	virtual void PostHologramPlacement() override;

	virtual bool DoMultiStepPlacement(bool isInputFromARelease) override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	
protected:
	virtual USceneComponent* SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName) override;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCurved;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCompoundCurve;

	// Custom:
	FVector endPos;
	FVector startTangent;
	FVector endTangent;

	EBendHoloState eState;
	bool isAnyCurvedBeamMode;

	USplineMeshComponent* splineRefComp = NULL;

	void UpdateAndReconstructSpline();
};
