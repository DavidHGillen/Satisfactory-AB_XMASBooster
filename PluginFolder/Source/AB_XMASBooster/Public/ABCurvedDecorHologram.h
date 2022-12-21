// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#pragma once

#include "CoreMinimal.h"
#include "Hologram/FGBuildableHologram.h"
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
class AB_XMASBOOSTER_API AABCurvedDecorHologram : public AFGBuildableHologram
{
	GENERATED_BODY()

public:
	AABCurvedDecorHologram();

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
	virtual void GetSupportedBuildModes_Implementation(TArray< TSubclassOf<UFGHologramBuildModeDescriptor> >& out_buildmodes) const override;
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;
	virtual int32 GetBaseCostMultiplier() const;

	virtual void ConfigureActor(class AFGBuildable* inBuildable) const override;
	virtual void BeginPlay() override;
	virtual void OnBuildModeChanged() override;
	virtual void PreHologramPlacement() override;
	virtual void PostHologramPlacement() override;

	virtual bool DoMultiStepPlacement(bool isInputFromARelease) override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	
protected:
	virtual void CheckValidPlacement() override;
	//virtual void ConfigureActor(class AFGBuildable* inBuildable) const override;
	virtual USceneComponent* SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName) override;

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

	// Custom:
	float length;

	FVector lastHit;
	FVector endPos;
	FVector startTangent;
	FVector endTangent;

	EBendHoloState eState;
	bool isAnyCurvedBeamMode;

	USplineMeshComponent* splineRefComp = NULL;

	void UpdateAndReconstructSpline();
	void ResetLine();
};
