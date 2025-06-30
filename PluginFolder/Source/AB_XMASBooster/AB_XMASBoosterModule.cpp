// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "AB_XMASBoosterModule.h"
#include "FGBlueprintHologram.h"
#include "ABCurvedDecorHologram.h"
#include "ABCurvedDecorBuildable.h"

#include "Patching/NativeHookManager.h"

void FAB_XMASBoosterModule::StartupModule() {

	AFGBlueprintHologram::FCreateBuildableVisualizationDelegate visualizingDelegate;
	visualizingDelegate = AFGBlueprintHologram::FCreateBuildableVisualizationDelegate::CreateStatic(&AABCurvedDecorHologram::BlueprintDataVisualize);
	AFGBlueprintHologram::RegisterCustomBuildableVisualization(AABCurvedDecorBuildable::StaticClass(), visualizingDelegate);

	// Hooking
	#if !WITH_EDITOR

	#endif
	// End Hooking

}

IMPLEMENT_GAME_MODULE(FAB_XMASBoosterModule, AB_XMASBooster);