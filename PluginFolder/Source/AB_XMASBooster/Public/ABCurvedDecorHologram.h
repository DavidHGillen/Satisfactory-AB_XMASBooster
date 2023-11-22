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
	CDH_BendBoth = 0b0010,
	CDH_Bend_IN =  0b0100,
	CDH_Bend_OUT = 0b1000,

	CDHM_isBendingIn =  0b0111,
	CDHM_isBendingOut = 0b1011,

	CDH_Draw_Live = 0b0010 << 4,
	CDH_Draw_Done = 0b0011 << 4,

	CDHM_DrawingPresent = 0b0010 << 4
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
	
protected:
	virtual USceneComponent* SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName, const FName& attachSocketName) override;
	virtual void ConfigureComponents(class AFGBuildable* inBuildable) const;
	//TODO: use pre-configure to center building around its spline

	// Custom:
	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCurved;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeCompoundCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|BuildMode")
		TSubclassOf< class UFGHologramBuildModeDescriptor > mBuildModeDrawing;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|Spline")
		float minLength;
	UPROPERTY(EditDefaultsOnly, Category = "Hologram|Spline")
		float maxLength;
	UPROPERTY(EditDefaultsOnly, Category = "Hologram|Spline")
		float lengthPerCost;

	UPROPERTY(EditDefaultsOnly, Category = "Hologram|Spline")
		float drawResolution;

	UPROPERTY(BlueprintReadOnly, Category = "Hologram|Spline")
		bool bShowMarker;
	UPROPERTY(BlueprintReadOnly, Category = "Hologram|Spline")
		FVector markerPosition;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hologram|Spline")
		FVector endPos;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hologram|Spline")
		FVector startTangent;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Hologram|Spline")
		FVector endTangent;

	float length;
	FVector lastHit;

	EBendHoloState eState;
	bool isAnyCurvedBeamMode;

	USplineMeshComponent* splineRefHolo = NULL; // the current temp spline mesh in the hologram

	TArray<FVector> localPointStore;

	FVector FindSnappedHitLocation(const FHitResult& hitResult) const;
	void UpdateAndRecalcSpline();
	void ResetLineData();

	static float calculateMeshLength(FVector start, FVector end, FVector startTangent, FVector endTangent);
};
