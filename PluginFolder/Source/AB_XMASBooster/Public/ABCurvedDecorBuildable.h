// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineMeshComponent.h"
#include "Buildables/FGBuildable.h"
#include "ABCurvedDecorBuildable.generated.h"

/**
 * 
 */
UCLASS()
class AB_XMASBOOSTER_API AABCurvedDecorBuildable : public AFGBuildable
{
	GENERATED_BODY()

public:
	AABCurvedDecorBuildable();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostSerializedFromBlueprint(bool isBlueprintWorld = false) override;
	virtual int32 GetDismantleRefundReturnsMultiplier() const override;

	FTimerHandle Timir;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, SaveGame, Category = "CurvedDecor")
	int32 CostMultiplier;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, SaveGame, Category = "CurvedDecor")
	float SplineLength;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, SaveGame, Category = "CurvedDecor")
	FVector StartPosition;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, SaveGame, Category = "CurvedDecor")
	FVector EndPosition;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, SaveGame, Category = "CurvedDecor")
	FVector StartTangent;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, SaveGame, Category = "CurvedDecor")
	FVector EndTangent;

	UFUNCTION(BlueprintCallable, Category = "CurvedDecor")
	virtual void UpdateSplineMesh();
};
