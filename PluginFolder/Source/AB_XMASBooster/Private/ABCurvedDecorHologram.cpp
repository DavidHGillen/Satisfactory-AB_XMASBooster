// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "ABCurvedDecorHologram.h"
#include "Math/UnrealMathUtility.h"

// Ctor
//////////////////////////////////////////////////////
AABCurvedDecorHologram::AABCurvedDecorHologram() {
	isAnyCurvedBeamMode = IsCurrentBuildMode(mBuildModeCurved) || IsCurrentBuildMode(mBuildModeCompoundCurve);
	eState = EBendHoloState::CDH_Placing;

	ResetLineData();
}

void AABCurvedDecorHologram::GetSupportedBuildModes_Implementation(TArray<TSubclassOf<UFGHologramBuildModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mBuildModeCurved) { out_buildmodes.AddUnique(mBuildModeCurved); }
	if (mBuildModeCompoundCurve) { out_buildmodes.AddUnique(mBuildModeCompoundCurve); }
}

bool AABCurvedDecorHologram::IsValidHitResult(const FHitResult& hitResult) const
{
	// I mean this may have some unintended hits, but I want my beam to register and the rest is just funny.
	return true;
}

int32 AABCurvedDecorHologram::GetBaseCostMultiplier() const
{
	return FMath::RoundHalfFromZero(length / lengthPerCost);
}

void AABCurvedDecorHologram::OnBuildModeChanged()
{
	Super::OnBuildModeChanged();

	isAnyCurvedBeamMode = IsCurrentBuildMode(mBuildModeCurved) || IsCurrentBuildMode(mBuildModeCompoundCurve);
	eState = EBendHoloState::CDH_Placing;

	ResetLineData();
	UpdateAndReconstructSpline();
}

bool AABCurvedDecorHologram::DoMultiStepPlacement(bool isInputFromARelease)
{
	bool bComplete = false;

	switch (eState) {
		case EBendHoloState::CDH_Placing:
			eState = EBendHoloState::CDH_Zooping;
			break;

		case EBendHoloState::CDH_Zooping:
			if (length < mGridSnapSize) { return false; } // Don't allow short lines. //TODO: probably not where to do it

			if (IsCurrentBuildMode(mBuildModeCurved)) {
				eState = EBendHoloState::CDH_Bend_A1;
			} else if(IsCurrentBuildMode(mBuildModeCompoundCurve)) {
				eState = EBendHoloState::CDH_Bend_B1;
			} else {
				bComplete = true;
			}
			break;

		case EBendHoloState::CDH_Bend_A1:
			bComplete = true;
			break;

		case EBendHoloState::CDH_Bend_B1:
			eState = EBendHoloState::CDH_Bend_B2;
			break;

		case EBendHoloState::CDH_Bend_B2:
			bComplete = true;
			break;
	}

	return bComplete;
}

void AABCurvedDecorHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	AActor* hitActor = hitResult.GetActor();
	FTransform hitToWorld = hitActor != NULL ? hitActor->ActorToWorld() : FTransform::Identity;
	FVector snappedHitLocation;

	if (hitActor == NULL) {
		if (eState == EBendHoloState::CDH_Placing) {
			// stay where you were cause where the hell else would you go
			snappedHitLocation = lastHit;
		} else {
			// cast the point out into space along the look vector
			APawn* player = GetConstructionInstigator();
			snappedHitLocation = player->GetActorLocation() + player->GetControlRotation().RotateVector(FVector(maxLength,0.0f,0.0f));
		}
	} else {

		if (Cast<AFGBuildable>(hitActor) != NULL) {
			// snap into factory space accounting for scaled actor
			FVector scaledSnap = FVector(mGridSnapSize) / hitActor->GetActorScale();
			snappedHitLocation = hitToWorld.InverseTransformPosition(hitResult.ImpactPoint);
			snappedHitLocation = hitToWorld.TransformPosition(FVector(
				FMath::GridSnap(snappedHitLocation.X, scaledSnap.X),
				FMath::GridSnap(snappedHitLocation.Y, scaledSnap.Y),
				FMath::GridSnap(snappedHitLocation.Z, scaledSnap.Z)
			));
		}

		if (eState != EBendHoloState::CDH_Placing) {
			// reproject into local space for local co-ordinates
			snappedHitLocation = ActorToWorld().InverseTransformPosition(snappedHitLocation);
		}
	}

	// if our snap hasn't moved nothing needs changing
	FVector test = lastHit;
	lastHit = snappedHitLocation;
	if (snappedHitLocation.Equals(test)) {
		return;
	}

	// just move the object
	if (eState == EBendHoloState::CDH_Placing) {
		SetActorLocation(snappedHitLocation);
		SetActorRotation(hitResult.ImpactNormal.ToOrientationRotator());
		markerBall->SetRelativeLocation(FVector::ZeroVector);
	}

	// length = in tangent = out tanget >>>> evenly distributed straight spline
	if (eState == EBendHoloState::CDH_Zooping) {
		// point the buildable along the line and redo setup //TODO: optimize
		snappedHitLocation = ActorToWorld().TransformPosition(snappedHitLocation);
		SetActorRotation((snappedHitLocation - GetActorLocation()).ToOrientationRotator());
		snappedHitLocation = ActorToWorld().InverseTransformPosition(snappedHitLocation);

		endPos = snappedHitLocation;
		startTangent = snappedHitLocation * 0.99f; // If these are "exact" floating points can decide things are backwards
		endTangent = snappedHitLocation * 0.99f; // If these are "exact" floating points can decide things are backwards
		markerBall->SetRelativeLocation(snappedHitLocation);

	} else {
		if ((uint8)eState & (uint8)EBendHoloState::CDHM_BendIn) {
			startTangent = snappedHitLocation * 2.0f; // Overblow the movement so it's easier to make good curves
		}

		if ((uint8)eState & (uint8)EBendHoloState::CDHM_BendOut) {
			endTangent = (endPos - snappedHitLocation) * 2.0f; // Overblow the movement so it's easier to make good curves
		}

		markerBall->SetRelativeLocation(snappedHitLocation * 2.0f);
	}

	UpdateAndReconstructSpline();
}

USceneComponent* AABCurvedDecorHologram::SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName)
{
	// Lets keep track of our spline and set it up
	USplineMeshComponent* splineRefTemp = Cast<USplineMeshComponent>(componentTemplate);
	if (splineRefTemp != NULL) {
		if(splineRefHolo != NULL){ splineRefHolo->UnregisterComponent(); }
		splineRefHolo = DuplicateComponent<USplineMeshComponent>(attachParent, splineRefTemp, componentName);
		splineRefBuild = splineRefTemp;

		splineRefHolo->SetupAttachment(attachParent);
		splineRefHolo->SetCollisionProfileName(CollisionProfileHologram);
		splineRefHolo->SetSimulatePhysics(false);
		splineRefHolo->SetCastShadow(false);
		splineRefHolo->ComponentTags.Add(HOLOGRAM_MESH_TAG);
		splineRefHolo->RegisterComponent();

		splineRefHolo->SetBoundaryMin(0.0f, false);
		splineRefHolo->SetBoundaryMax(400.0f, false);

		UpdateAndReconstructSpline();

		UFGBlueprintFunctionLibrary::ShowOutline(splineRefHolo, EOutlineColor::OC_HOLOGRAM);
		UFGBlueprintFunctionLibrary::ApplyCustomizationPrimitiveData(this, mCustomizationData, this->mCustomizationData.ColorSlot, splineRefHolo);

		return splineRefHolo;
	}

	// Behave like normal
	USceneComponent* result = Super::SetupComponent(attachParent, componentTemplate, componentName);

	// There's only 1 other static mesh, so lets use it for the marker //TODO: this is wrong but I'm lazy and it's ?working?
	UMeshComponent* markerTemp = Cast<UMeshComponent>(result);
	if (markerTemp != NULL) { markerBall = markerTemp; }

	return result;
}

// Custom:
//////////////////////////////////////////////////////
void AABCurvedDecorHologram::UpdateAndReconstructSpline()
{
	//UE_LOG(LogTemp, Warning, TEXT("[[[ %s | %s | %s ]]]"), *startTangent.ToString(), *endTangent.ToString(), *endPos.ToString());
	
	if (eState == EBendHoloState::CDH_Placing) {
		length = lengthPerCost; // cost minimum until people draw a line
	} else {
		length = endPos.Size(); //TODO: curving should cost something but is expensive to calc
	}

	// actually set data
	if (splineRefHolo != NULL) {
		splineRefHolo->SetEndPosition(endPos, false);
		splineRefHolo->SetStartTangent(startTangent, false);
		splineRefHolo->SetEndTangent(endTangent, true);
	}
	if (splineRefBuild != NULL) {
		splineRefBuild->SetEndPosition(endPos, false);
		splineRefBuild->SetStartTangent(startTangent, false);
		splineRefBuild->SetEndTangent(endTangent, true);
	}

	SetIsChanged(true);
}

void AABCurvedDecorHologram::ResetLineData()
{
	FVector defaultSplineLoc = FVector(0.0f, 0.0f, 400.0f);
	startTangent = defaultSplineLoc;
	endPos = defaultSplineLoc * 0.99f;
	endTangent = defaultSplineLoc *0.99f;

	eState = EBendHoloState::CDH_Placing;

	if (markerBall) { markerBall->SetRelativeLocation(FVector::ZeroVector); }
}
