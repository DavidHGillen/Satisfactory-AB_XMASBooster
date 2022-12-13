// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "ABCurveBeamHologram.h"

// Ctor
//////////////////////////////////////////////////////
AABCurveBeamHologram::AABCurveBeamHologram() {
	FVector defaultSplineLoc = FVector(0.0f, 0.0f, 400.0f);
	startTangent = defaultSplineLoc;
	endPos = defaultSplineLoc;
	endTangent = defaultSplineLoc;
}

// AFGHologram interface
//////////////////////////////////////////////////////
void AABCurveBeamHologram::GetSupportedBuildModes_Implementation(TArray<TSubclassOf<UFGHologramBuildModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mBuildModeCurved) {				out_buildmodes.AddUnique(mBuildModeCurved); }
	if (mBuildModeCompoundCurve) {		out_buildmodes.AddUnique(mBuildModeCompoundCurve); }
}

void AABCurveBeamHologram::OnBuildModeChanged()
{
	Super::OnBuildModeChanged();

	isBeamStarted = false;
	isBeamComplete = false;

	isAnyCurvedBeamMode = IsCurrentBuildMode(mBuildModeCurved) || IsCurrentBuildMode(mBuildModeCompoundCurve);

	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Change: %s"), isAnyCurvedBeamMode ? TEXT("CRV") : TEXT("STR"));
}

bool AABCurveBeamHologram::DoMultiStepPlacement(bool isInputFromARelease)
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Multistep: %s"), isAnyCurvedBeamMode ? TEXT("CRV") : TEXT("STR"));

	// hijack the first two interacts for default behaviour, then continue if our build mode
	if (!isBeamComplete) {
		isBeamStarted = true;
		isBeamComplete = Super::DoMultiStepPlacement(isInputFromARelease);

		return isAnyCurvedBeamMode ? false : isBeamComplete;
	}

	bool bCurveDone = false;

	if (IsCurrentBuildMode(mBuildModeCurved)) {
		// stuff

		bCurveDone = true;
	} else if (IsCurrentBuildMode(mBuildModeCompoundCurve)) {
		// stuff

		bCurveDone = true;
	}

	return bCurveDone;
}

void AABCurveBeamHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	AActor* hitActor = hitResult.GetActor();
	FTransform hitToWorld = hitActor != NULL ? hitActor->ActorToWorld() : FTransform::Identity;
	FVector localSnappedHitLocation;

	if (isAnyCurvedBeamMode) {
		if (hitActor == NULL) {
			// cast the point out into space along the look vector
			localSnappedHitLocation = FVector(100.0f); // TODO:
		} else {
			// convert to the space of the hit object, snap in it, then convert back to world, then world to our space
			localSnappedHitLocation = ActorToWorld().InverseTransformPosition(
				hitToWorld.TransformPosition(
					hitToWorld.InverseTransformPosition(hitResult.ImpactPoint).GridSnap(mGridSnapSize)
				)
			);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Set loc-rot: %s @%d,%d,%d "), isAnyCurvedBeamMode ? TEXT("CRV") : TEXT("STR"),
		localSnappedHitLocation.X / 100, localSnappedHitLocation.Y / 100, localSnappedHitLocation.Z / 100);

	// behave like normal till we enter curving steps
	if (!isBeamComplete) {
		Super::SetHologramLocationAndRotation(hitResult);

		if (isBeamStarted && isAnyCurvedBeamMode) {
			// length = in tangent = out tanget >>>> evenly distributed straight spline
			if (!localSnappedHitLocation.Equals(endPos)) {
				endPos = localSnappedHitLocation;
				startTangent = localSnappedHitLocation;
				endTangent = localSnappedHitLocation;
				UpdateAndReconstructSpline();
			}
		}
		return;
	}

	// set our tangents correctly for where we're looking and our build mode
	if (!localSnappedHitLocation.Equals(startTangent)) {
		startTangent = localSnappedHitLocation;
		endTangent = endPos - localSnappedHitLocation;
		UpdateAndReconstructSpline();
	}
}

USceneComponent* AABCurveBeamHologram::SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName)
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Setup: %s %s"), isAnyCurvedBeamMode?TEXT("CRV"):TEXT("STR"), *(componentName.ToString()));

	// Lets just keep track of our spline if we need it
	USplineMeshComponent* splineRefTemp = Cast<USplineMeshComponent>(componentTemplate);
	if (splineRefTemp != NULL) {
		splineRefComp = splineRefTemp;
	}

	return Super::SetupComponent(attachParent, componentTemplate, componentName);
}

// Custom:
//////////////////////////////////////////////////////
void AABCurveBeamHologram::UpdateAndReconstructSpline()
{
	if (splineRefComp == NULL) { return; }

	UE_LOG(LogTemp, Warning, TEXT("[ABCB] UPDATE Spline Component: "));

	if (isBeamStarted) {
		USplineMeshComponent* splineRefTemp;
		FCustomPrimitiveData cutomizerData;

		// copy form the correct start but don't kill the real reference
		if (splineRefGhost == NULL) {
			splineRefTemp = Cast<USplineMeshComponent>(CreateComponentFromTemplate(splineRefComp));
		} else {
			splineRefTemp = Cast<USplineMeshComponent>(CreateComponentFromTemplate(splineRefGhost));
			splineRefGhost->UnregisterComponent();
		}

		// properly attach new element
		this->ApplyMeshPrimitiveData(this->mCustomizationData);
		FinishAndRegisterComponent(splineRefTemp);
		splineRefTemp->AttachTo(this->RootComponent);
		splineRefGhost = splineRefTemp;
	}

	// actually set data
	splineRefComp->SetStartTangent(startTangent, false);
	splineRefComp->SetEndTangent(endTangent, false);
	splineRefComp->SetEndPosition(endPos, true);

	if (splineRefGhost != NULL) {
		splineRefGhost->SetStartTangent(startTangent, false);
		splineRefGhost->SetEndTangent(endTangent, false);
		splineRefGhost->SetEndPosition(endPos, true);
	}
}