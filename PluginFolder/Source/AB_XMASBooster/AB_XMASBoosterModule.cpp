// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "AB_XMASBoosterModule.h"
#include "FGBlueprintHologram.h"
#include "ABCurvedDecorHologram.h"
#include "ABCurvedDecorBuildable.h"

#include "Patching/NativeHookManager.h"

void FAB_XMASBoosterModule::StartupModule() {

	AFGBlueprintHologram::FCreateBuildableVisualizationDelegate visualizingDelegate;
	visualizingDelegate.BindLambda([](AFGBlueprintHologram* blueprintHologram, AFGBuildable* buildable, USceneComponent* buildableRootComponent) {
		AABCurvedDecorBuildable* curve = Cast<AABCurvedDecorBuildable>(buildable);
		USplineMeshComponent* splineMesh = Cast<USplineMeshComponent>(blueprintHologram->SetupComponent(buildableRootComponent, buildable->GetComponentByClass<USplineMeshComponent>(), buildable->GetFName(), FName()));

		splineMesh->SetStartPosition(curve->StartPosition, false);
		splineMesh->SetEndPosition(curve->EndPosition, false);
		splineMesh->SetStartTangent(curve->StartTangent, false);
		splineMesh->SetEndTangent(curve->EndTangent, false);

		//UE_LOG(LogTemp, Warning, TEXT("[{[ [[BLUEPRINT IT]] %s | %s ||| %s | %s ]]]"), *curve->StartPosition.ToString(), *curve->EndPosition.ToString(), *curve->StartTangent.ToString(), *curve->EndTangent.ToString());

		splineMesh->SetStartRoll(0.001f, false);
		splineMesh->SetEndRoll(0.001f, false);
		splineMesh->UpdateMesh_Concurrent();
		splineMesh->UpdateBounds();
	});

	AFGBlueprintHologram::RegisterCustomBuildableVisualization(AABCurvedDecorBuildable::StaticClass(), visualizingDelegate);

	// Hooking
	#if !WITH_EDITOR

	#endif
	// End Hooking

}

IMPLEMENT_GAME_MODULE(FAB_XMASBoosterModule, AB_XMASBooster);