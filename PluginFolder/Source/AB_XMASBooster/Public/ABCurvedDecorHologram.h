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
	CDHM_BendOut = 0b1011,

	CDH_Draw_None = 0b0001 << 4,
	CDH_Draw_Live = 0b0010 << 4,
	CDH_Draw_Done = 0b0011 << 4,

	CDHM_Draw_Any = 0b0010 << 4,
	CDHM_Draw_Some = 0b0010 << 4
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
	virtual void GetSupportedBuildModes_Implementation(TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;
	virtual int32 GetBaseCostMultiplier() const;
	virtual void OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode) override;

	virtual bool DoMultiStepPlacement(bool isInputFromARelease) override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual bool TrySnapToActor(const FHitResult& hitResult) override;
	
protected:
	//virtual USceneComponent* SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName, const FName& attachSocketName) override;
	virtual void ConfigureActor(class AFGBuildable* inBuildable) const;
	virtual void ConfigureComponents(class AFGBuildable* inBuildable) const;

	// Custom:
	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCurved;
		
	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCompoundCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeDrawing;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|Spline")
		float maxLength;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|Spline")
		float maxBend;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|Spline")
		float lengthPerCost;

	UPROPERTY(BlueprintReadOnly, Category = "Hologram|Spline")
		bool bShowMarker;

	UPROPERTY(BlueprintReadOnly, Category = "Hologram|Spline")
		FVector markerPosition;

	float length;

	FVector lastHit;
	FVector endPos;
	FVector startTangent;
	FVector endTangent;

	EBendHoloState eState;
	bool isAnyCurvedBeamMode;

	USplineMeshComponent* splineRefHolo = NULL;
	USplineMeshComponent* splineRefBuild = NULL;

	TArray<FVector> localPointStore;

	void UpdateAndReconstructSpline();
	void ResetLineData();
};
