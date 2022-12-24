// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#pragma once

#include "CoreMinimal.h"
#include "FGGenericBuildableHologram.h"
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
class AB_XMASBOOSTER_API AABCurvedDecorHologram : public AFGGenericBuildableHologram
{
	GENERATED_BODY()

public:
	AABCurvedDecorHologram();

	virtual void GetSupportedBuildModes_Implementation(TArray< TSubclassOf<UFGHologramBuildModeDescriptor> >& out_buildmodes) const override;
	virtual int32 GetBaseCostMultiplier() const;
	virtual void OnBuildModeChanged() override;

	virtual bool DoMultiStepPlacement(bool isInputFromARelease) override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	
protected:
	virtual USceneComponent* SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName) override;

	// Custom:
	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCurved;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCompoundCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram")
		float maxLength;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram")
		float maxBend;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram")
		float lengthPerCost;

	float length;

	FVector lastHit;
	FVector endPos;
	FVector startTangent;
	FVector endTangent;

	EBendHoloState eState;
	bool isAnyCurvedBeamMode;

	UMeshComponent* markerBall = NULL;
	USplineMeshComponent* splineRefHolo = NULL;
	USplineMeshComponent* splineRefBuild = NULL;

	void UpdateAndReconstructSpline();
	void ResetLineData();
};
