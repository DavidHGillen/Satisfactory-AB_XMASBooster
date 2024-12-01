// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "AB_XMASBoosterModule.h"
#include "FGBlueprintHologram.h"
#include "ABCurvedDecorHologram.h"
#include "ABCurvedDecorBuildable.h"

#include "Patching/NativeHookManager.h"

void FAB_XMASBoosterModule::StartupModule() {

	AFGBlueprintHologram::FCreateBuildableVisualizationDelegate visualizingDelegate;
	visualizingDelegate.BindLambda([](AFGBlueprintHologram* blueprintHologram, AFGBuildable* buildable, USceneComponent* buildableRootComponent) {
		UE_LOG(LogTemp, Warning, TEXT("[{[LOOK MA!]]]"));

		USplineMeshComponent* meshref = buildable->GetComponentByClass<USplineMeshComponent>();
		USplineMeshComponent* splineMesh = DuplicateObject <USplineMeshComponent>(meshref, blueprintHologram);
		AABCurvedDecorBuildable* curve = Cast<AABCurvedDecorBuildable>(buildable);

		splineMesh->SetMobility(EComponentMobility::Movable);
		// colouration
		// hologram FX
		// solidity

		splineMesh->SetStartPosition(curve->StartPosition, false);
		splineMesh->SetEndPosition(curve->EndPosition, false);
		splineMesh->SetStartTangent(curve->StartTangent, false);
		splineMesh->SetEndTangent(curve->EndTangent, false);

		UE_LOG(LogTemp, Warning, TEXT("[{[ [[BLUEPRINT IT]] %s | %s ||| %s | %s ]]]"), *curve->StartPosition.ToString(), *curve->EndPosition.ToString(), *curve->StartTangent.ToString(), *curve->EndTangent.ToString());

		splineMesh->SetStartRoll(0.001f, false);
		splineMesh->SetEndRoll(0.001f, false);
		splineMesh->UpdateMesh_Concurrent();
		splineMesh->UpdateBounds();

		splineMesh->SetupAttachment(buildableRootComponent);
		splineMesh->RegisterComponent();

		splineMesh->SetRelativeLocationAndRotation(buildable->GetActorLocation(), buildable->GetActorRotation());

		FVector vTemp;

		vTemp = blueprintHologram->GetActorLocation();
		UE_LOG(LogTemp, Warning, TEXT("[{[ [[Vec]] %f, %f, %f ]]]"), vTemp.X, vTemp.Y, vTemp.Z);

		vTemp = buildable->GetActorLocation();
		UE_LOG(LogTemp, Warning, TEXT("[{[ [[Vec]] %f, %f, %f ]]]"), vTemp.X, vTemp.Y, vTemp.Z);

		vTemp = splineMesh->GetComponentLocation();
		UE_LOG(LogTemp, Warning, TEXT("[{[ [[Vec]] %f, %f, %f ]]]"), vTemp.X, vTemp.Y, vTemp.Z);

	});

	AFGBlueprintHologram::RegisterCustomBuildableVisualization(AABCurvedDecorBuildable::StaticClass(), visualizingDelegate);

	// Hooking
	#if !WITH_EDITOR

	#endif
	// End Hooking

}

IMPLEMENT_GAME_MODULE(FAB_XMASBoosterModule, AB_XMASBooster);