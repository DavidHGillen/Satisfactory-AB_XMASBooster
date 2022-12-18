// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "ABCurvedDecorHologram.h"

// Ctor
//////////////////////////////////////////////////////
AABCurvedDecorHologram::AABCurvedDecorHologram() {
	FVector defaultSplineLoc = FVector(0.0f, 0.0f, 400.0f);
	startTangent = defaultSplineLoc;
	endPos = defaultSplineLoc;
	endTangent = defaultSplineLoc;

	//mDecorMeshComponentName = "mSplineMesh";
	eState = EBendHoloState::CDH_Placing;
}

// AFGHologram interface
//////////////////////////////////////////////////////
void AABCurvedDecorHologram::GetSupportedBuildModes_Implementation(TArray<TSubclassOf<UFGHologramBuildModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mBuildModeCurved) { out_buildmodes.AddUnique(mBuildModeCurved); }
	if (mBuildModeCompoundCurve) { out_buildmodes.AddUnique(mBuildModeCompoundCurve); }
}

bool AABCurvedDecorHologram::IsValidHitResult(const FHitResult& hitResult) const
{
	return true;
}

void AABCurvedDecorHologram::OnBuildModeChanged()
{
	Super::OnBuildModeChanged();

	isAnyCurvedBeamMode = IsCurrentBuildMode(mBuildModeCurved) || IsCurrentBuildMode(mBuildModeCompoundCurve);

	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Change: %s"), isAnyCurvedBeamMode ? TEXT("CRV") : TEXT("STR"));
}

void AABCurvedDecorHologram::PreHologramPlacement()
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] //// PREPLACE: %s"), isAnyCurvedBeamMode ? TEXT("CRV") : TEXT("STR"));
	UpdateAndReconstructSpline();
	Super::PreHologramPlacement();
}

void AABCurvedDecorHologram::PostHologramPlacement()
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] POSTPLACE ////: %s"), isAnyCurvedBeamMode ? TEXT("CRV") : TEXT("STR"));
	UpdateAndReconstructSpline();
	Super::PostHologramPlacement();
}

bool AABCurvedDecorHologram::DoMultiStepPlacement(bool isInputFromARelease)
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Multistep: %s"), isAnyCurvedBeamMode ? TEXT("CRV") : TEXT("STR"));

	bool bComplete = false;

	switch (eState) {
		case EBendHoloState::CDH_Placing:
			Super::DoMultiStepPlacement(isInputFromARelease);
			bComplete = false;
			eState = EBendHoloState::CDH_Zooping;
			break;

		case EBendHoloState::CDH_Zooping:
			if (IsCurrentBuildMode(mBuildModeCurved)) {
				eState = EBendHoloState::CDH_Bend_A1;
				bComplete = false;
			} else if(IsCurrentBuildMode(mBuildModeCompoundCurve)) {
				eState = EBendHoloState::CDH_Bend_B1;
				bComplete = false;
			} else {
				bComplete = true;
			}
			break;

		case EBendHoloState::CDH_Bend_A1:
			bComplete = true;
			break;

		case EBendHoloState::CDH_Bend_B1:
			eState = EBendHoloState::CDH_Bend_B2;
			bComplete = false;
			break;

		case EBendHoloState::CDH_Bend_B2:
			bComplete = true;
			break;
	}

	UpdateAndReconstructSpline();
	return bComplete;
}

void AABCurvedDecorHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	AActor* hitActor = hitResult.GetActor();
	FTransform hitToWorld = hitActor != NULL ? hitActor->ActorToWorld() : FTransform::Identity;
	FVector localSnappedHitLocation;

	if (hitActor == NULL) {
		// cast the point out into space along the look vector
		localSnappedHitLocation = FVector(100.0f); // TODO:
	}
	else {
		// convert to the space of the hit object, snap in it, then convert back to world, then world to our space
		localSnappedHitLocation = ActorToWorld().InverseTransformPosition(
			hitToWorld.TransformPosition(
				hitToWorld.InverseTransformPosition(hitResult.ImpactPoint).GridSnap(mGridSnapSize)
			)
		);
	}

	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Set loc-rot: %s @%d,%d,%d "), isAnyCurvedBeamMode ? TEXT("CRV") : TEXT("STR"),
		localSnappedHitLocation.X / 100, localSnappedHitLocation.Y / 100, localSnappedHitLocation.Z / 100);

	// behave like normal till we enter further states
	if (eState == EBendHoloState::CDH_Placing) {
		Super::SetHologramLocationAndRotation(hitResult);
		return;
	}

	// if our snap hasn't moved nothing needs changing
	if (!localSnappedHitLocation.Equals(endPos)) {
		return;
	}

	// length = in tangent = out tanget >>>> evenly distributed straight spline
	if (eState == EBendHoloState::CDH_Zooping) {
		endPos = localSnappedHitLocation;
	}

	if ((uint8)eState & (uint8)EBendHoloState::CDHM_BendIn) {
		startTangent = localSnappedHitLocation;
	}

	if ((uint8)eState & (uint8)EBendHoloState::CDHM_BendOut) {
		endTangent = endPos - localSnappedHitLocation;
	}

	UpdateAndReconstructSpline();
}

USceneComponent* AABCurvedDecorHologram::SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName)
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Setup: %s %s"), isAnyCurvedBeamMode ? TEXT("CRV") : TEXT("STR"), *(componentName.ToString()));

	// Lets keep track of our spline
	USplineMeshComponent* splineRefTemp = Cast<USplineMeshComponent>(componentTemplate);
	if (splineRefTemp != NULL) {
		splineRefComp = splineRefTemp;
		UpdateAndReconstructSpline();
	}

	return Super::SetupComponent(attachParent, componentTemplate, componentName);
}

// Custom:
//////////////////////////////////////////////////////
void AABCurvedDecorHologram::UpdateAndReconstructSpline()
{
	if (splineRefComp == NULL) { return; }

	UE_LOG(LogTemp, Warning, TEXT("[ABCB] UPDATE Spline Component: "));

	// actually set data
	splineRefComp->SetStartTangent(startTangent, false);
	splineRefComp->SetEndTangent(endTangent, false);
	splineRefComp->SetEndPosition(endPos, true);
}

