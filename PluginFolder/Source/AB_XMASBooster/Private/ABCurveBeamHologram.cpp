// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "ABCurveBeamHologram.h"

// Ctor
//////////////////////////////////////////////////////
AABCurveBeamHologram::AABCurveBeamHologram() {
}

// AFGHologram interface
//////////////////////////////////////////////////////
void AABCurveBeamHologram::GetSupportedBuildModes_Implementation(TArray<TSubclassOf<UFGHologramBuildModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mBuildModeCurved) {				out_buildmodes.AddUnique(mBuildModeCurved); }
	if (mBuildModeCompoundCurve) {		out_buildmodes.AddUnique(mBuildModeCompoundCurve); }
}

void AABCurveBeamHologram::ConfigureActor(AFGBuildable* inBuildable) const
{
	Super::ConfigureActor(inBuildable);
}

// Multistep
//////////////////////////////////////////////////////
bool AABCurveBeamHologram::DoMultiStepPlacement(bool isInputFromARelease)
{
	if (IsCurrentBuildMode(mBuildModeCurved)) {

		return true;
	}

	if (IsCurrentBuildMode(mBuildModeCurved)) {
		return true;
	}

	return Super::DoMultiStepPlacement(isInputFromARelease);
}