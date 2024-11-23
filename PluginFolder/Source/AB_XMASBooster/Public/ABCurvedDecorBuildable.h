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

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

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
