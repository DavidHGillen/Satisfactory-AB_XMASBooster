// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "Math/UnrealMathUtility.h"
#include "ABCurvedDecorHologram.h"

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

int32 AABCurvedDecorHologram::GetBaseCostMultiplier() const
{
	//TODO:
	//TODO:
	//TODO:
	//TODO:
	//TODO:
	return 1;
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
			// snap into factory space
			//TODO: account for scaled factory things with snap
			snappedHitLocation = hitToWorld.TransformPosition(
				hitToWorld.InverseTransformPosition(hitResult.ImpactPoint).GridSnap(50)
			);
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
		SetActorRotation(FQuat(hitResult.ImpactNormal, 0));
	}

	// length = in tangent = out tanget >>>> evenly distributed straight spline
	if (eState == EBendHoloState::CDH_Zooping) {
		//TODO: point the buildable along this line, it's what users and other mods expect
		endPos = snappedHitLocation;
		startTangent = snappedHitLocation;
		endTangent = (endPos - snappedHitLocation);
	} else {
		if ((uint8)eState & (uint8)EBendHoloState::CDHM_BendIn) {
			startTangent = snappedHitLocation * 2.0f;
		}

		if ((uint8)eState & (uint8)EBendHoloState::CDHM_BendOut) {
			endTangent = (endPos - snappedHitLocation) * 2.0f;
		}
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

		UpdateAndReconstructSpline();

		UFGBlueprintFunctionLibrary::ShowOutline(splineRefHolo, EOutlineColor::OC_HOLOGRAM);
		UFGBlueprintFunctionLibrary::ApplyCustomizationPrimitiveData(this, mCustomizationData, this->mCustomizationData.ColorSlot, splineRefHolo);

		return splineRefHolo;
	}

	return Super::SetupComponent(attachParent, componentTemplate, componentName);
}

// Custom:
//////////////////////////////////////////////////////
void AABCurvedDecorHologram::UpdateAndReconstructSpline()
{
	// actually set data
	if (splineRefHolo != NULL) {
		splineRefHolo->SetStartTangent(startTangent, false);
		splineRefHolo->SetEndTangent(endTangent, false);
		splineRefHolo->SetEndPosition(endPos, true);
	}
	if (splineRefBuild != NULL) {
		splineRefBuild->SetStartTangent(startTangent, false);
		splineRefBuild->SetEndTangent(endTangent, false);
		splineRefBuild->SetEndPosition(endPos, true);
	}

	SetIsChanged(true);
}

void AABCurvedDecorHologram::ResetLineData()
{
	FVector defaultSplineLoc = FVector(0.0f, 0.0f, 400.0f);
	startTangent = defaultSplineLoc;
	endPos = defaultSplineLoc;
	endTangent = defaultSplineLoc;

	eState = EBendHoloState::CDH_Placing;
}
