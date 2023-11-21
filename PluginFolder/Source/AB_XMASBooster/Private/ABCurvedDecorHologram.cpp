// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "ABCurvedDecorHologram.h"

#include "Math/UnrealMathUtility.h"
#include "FGConstructDisqualifier.h"
#include "Net/UnrealNetwork.h"

// Ctor:
AABCurvedDecorHologram::AABCurvedDecorHologram() {
	mCanLockHologram = true;
	mCanNudgeHologram = true;
	mSnapToGuideLines = false;
	mCanSnapWithAttachmentPoints = false;

	isAnyCurvedBeamMode = false;
	eState = EBendHoloState::CDH_Placing;

	ResetLineData();
}

// Factory API:
void AABCurvedDecorHologram::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AABCurvedDecorHologram, endPos);
	DOREPLIFETIME(AABCurvedDecorHologram, startTangent);
	DOREPLIFETIME(AABCurvedDecorHologram, endTangent);
}

void AABCurvedDecorHologram::GetSupportedBuildModes_Implementation(TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const {
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mBuildModeCurved) { out_buildmodes.AddUnique(mBuildModeCurved); }
	if (mBuildModeCompoundCurve) { out_buildmodes.AddUnique(mBuildModeCompoundCurve); }
	if (mBuildModeDrawing) { out_buildmodes.AddUnique(mBuildModeDrawing); }
}

bool AABCurvedDecorHologram::IsValidHitResult(const FHitResult& hitResult) const {
	// all buildables are valid targets, but we also need to check for valid natural targets. We can let vanilla do that step.
	if (Cast<AFGBuildable>(hitResult.GetActor()) != NULL) { return true; }
	return Super::IsValidHitResult(hitResult);
}

int32 AABCurvedDecorHologram::GetBaseCostMultiplier() const {
	return FMath::RoundHalfFromZero(length / lengthPerCost);
}

void AABCurvedDecorHologram::OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode) {
	Super::OnBuildModeChanged(buildMode);

	// if we're in drawing mode we need the unique start state
	if(IsCurrentBuildMode(mBuildModeDrawing)) {
		eState = EBendHoloState::CDH_Draw_None;
	} else {
		//TODO: deal with the fact more complexity shouldn't reset the line
		eState = EBendHoloState::CDH_Placing;
		isAnyCurvedBeamMode = IsCurrentBuildMode(mBuildModeCurved) || IsCurrentBuildMode(mBuildModeCompoundCurve);
	}

	ResetLineData();
	UpdateAndRecalcSpline();
}

bool AABCurvedDecorHologram::DoMultiStepPlacement(bool isInputFromARelease) {
	bool bComplete = false;

	switch (eState) {
		// all precise modes start here and progress to zooping
		case EBendHoloState::CDH_Placing:
			eState = EBendHoloState::CDH_Zooping;
			bShowMarker = false; // zooping doesn't need the marker as it's obvious
			break;

		// once zooping is done where we go next depends on the build mode
		case EBendHoloState::CDH_Zooping:
			if (IsCurrentBuildMode(mBuildModeCurved)) {
				eState = EBendHoloState::CDH_BendBoth;
			} else if(IsCurrentBuildMode(mBuildModeCompoundCurve)) {
				eState = EBendHoloState::CDH_Bend_IN;
			} else {
				bComplete = true;
			}
			bShowMarker = true;
			break;

		// if we were bending both in and out we're done
		case EBendHoloState::CDH_BendBoth:
			bComplete = true;
			break;

		// step from in to out to done
		case EBendHoloState::CDH_Bend_IN:
			eState = EBendHoloState::CDH_Bend_OUT;
			break;

		case EBendHoloState::CDH_Bend_OUT:
			bComplete = true;
			break;

		// drawing mode goes from none, to painting, to confirm
		case EBendHoloState::CDH_Draw_None:
			eState = EBendHoloState::CDH_Draw_Live;
			break;

		case EBendHoloState::CDH_Draw_Live:
			eState = EBendHoloState::CDH_Draw_Done;
			bShowMarker = false;
			break;

		case EBendHoloState::CDH_Draw_Done:
			bComplete = true;
			bShowMarker = true;
			break;
	}

	return bComplete;
}

void AABCurvedDecorHologram::SetHologramLocationAndRotation(const FHitResult& hitResult) {
	AActor* hitActor = hitResult.GetActor();
	FTransform hitToWorld = hitActor != NULL ? hitActor->ActorToWorld() : FTransform::Identity;
	
	//
	// No, break out the snapping and run it before these two sets and then maybe we wont even need both of these?
	//
	if(IsCurrentBuildMode(mBuildModeDrawing)) {
		lastHit = LocationAndRotation_Drawn(hitResult, hitActor, hitToWorld);
	} else {
		lastHit = LocationAndRotation_Precise(hitResult, hitActor, hitToWorld);
	}

	markerPosition = lastHit;
	UpdateAndRecalcSpline();
}

void AABCurvedDecorHologram::ConfigureComponents(class AFGBuildable* inBuildable) const {
	Super::ConfigureComponents(inBuildable);

	// were there to be multiple spline mesh components this might be unwise, but like when will that come up
	USplineMeshComponent* splineMesh = inBuildable->GetComponentByClass<USplineMeshComponent>();
	splineMesh->SetEndPosition(endPos, false);
	splineMesh->SetStartTangent(startTangent, false);
	splineMesh->SetEndTangent(endTangent, true);
}

USceneComponent* AABCurvedDecorHologram::SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName, const FName& attachSocketName) {
	// We can't live modify the spline component for some reason, so remake it constantly with the right FX
	USplineMeshComponent* splineRefTemp = Cast<USplineMeshComponent>(componentTemplate);
	if (splineRefTemp != NULL) {
		if(splineRefHolo != NULL){ splineRefHolo->UnregisterComponent(); }
		splineRefHolo = DuplicateComponent<USplineMeshComponent>(attachParent, splineRefTemp, componentName);

		splineRefHolo->SetupAttachment(attachParent);
		splineRefHolo->SetCollisionProfileName(CollisionProfileHologram);
		splineRefHolo->SetSimulatePhysics(false);
		splineRefHolo->SetCastShadow(false);
		splineRefHolo->ComponentTags.Add(HOLOGRAM_MESH_TAG);
		splineRefHolo->RegisterComponent();

		UpdateAndRecalcSpline();

		UFGBlueprintFunctionLibrary::ShowOutline(splineRefHolo, EOutlineColor::OC_HOLOGRAM);
		UFGBlueprintFunctionLibrary::ApplyCustomizationPrimitiveData(this, mCustomizationData, this->mCustomizationData.ColorSlot, splineRefHolo);

		return splineRefHolo;
	}

	// Behave like normal
	USceneComponent* result = Super::SetupComponent(attachParent, componentTemplate, componentName, attachSocketName);

	return result;
}

// Custom:
FVector AABCurvedDecorHologram::LocationAndRotation_Precise(const FHitResult& hitResult, const AActor* hitActor, const FTransform& hitToWorld) {
	FVector snappedHitLocation;

	if (hitActor == NULL) {
		// we hit nothing, now what?
		if (eState == EBendHoloState::CDH_Placing) {
			// stay where you were last cause where the hell else would you go
			snappedHitLocation = lastHit;
			AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
		}
		else {
			// cast the point out into space along the look vector for maximum length style a-la beams
			APawn* player = GetConstructionInstigator();
			FVector farSight = FVector(maxLength, 0.0f, 0.0f);
			snappedHitLocation = player->GetActorLocation() + player->GetControlRotation().Vector() * farSight;
		}
	} else {
		//TODO: better snapping to more specific object types (walls, sloped foundations, spline meshes)
		if (Cast<AFGBuildable>(hitActor) != NULL) {
			// world location when snaped in local space on factory objects
			FVector scaledSnap = FVector(mGridSnapSize) / hitActor->GetActorScale();
			snappedHitLocation = hitToWorld.InverseTransformPosition(hitResult.ImpactPoint);
			snappedHitLocation = hitToWorld.TransformPosition(FVector(
				FMath::GridSnap(snappedHitLocation.X, scaledSnap.X),
				FMath::GridSnap(snappedHitLocation.Y, scaledSnap.Y),
				FMath::GridSnap(snappedHitLocation.Z, scaledSnap.Z)
			));
		} else {
			// if we don't have a smart way to snap, lets just use the raw position
			snappedHitLocation = hitResult.ImpactPoint;
		}

		// once we're past placing the object work in local space
		if (eState != EBendHoloState::CDH_Placing) {
			snappedHitLocation = ActorToWorld().InverseTransformPosition(snappedHitLocation);
		}
	}

	// if our snap hasn't moved nothing needs changing
	if (snappedHitLocation.Equals(lastHit)) {
		return lastHit;
	}

	UE_LOG(LogTemp, Warning, TEXT("[[[ Update: %s ]]]"), *snappedHitLocation.ToString());

	// move the building to our location
	if (eState == EBendHoloState::CDH_Placing) {
		SetActorLocation(snappedHitLocation);
		SetActorRotation(hitResult.ImpactNormal.ToOrientationRotator());
		markerPosition = FVector::ZeroVector;
	} else {

		// modify the building
		if (eState == EBendHoloState::CDH_Zooping) {
			// point the buildable along the line
			snappedHitLocation = ActorToWorld().TransformPosition(snappedHitLocation);
			SetActorRotation((snappedHitLocation - GetActorLocation()).ToOrientationRotator());
			snappedHitLocation = ActorToWorld().InverseTransformPosition(snappedHitLocation);

			endPos = snappedHitLocation;
			startTangent = snappedHitLocation * 0.99f; // If these are "exact" floating points can decide things are backwards
			endTangent = snappedHitLocation * 0.99f; // If these are "exact" floating points can decide things are backwards
		} else {
			if ((uint8)eState & (uint8)EBendHoloState::CDHM_isBendingIn) {
				startTangent = snappedHitLocation * 2.0f; // Overblow the movement so it's easier to make good curves
			}

			if ((uint8)eState & (uint8)EBendHoloState::CDHM_isBendingOut) {
				endTangent = (endPos - snappedHitLocation) * 2.0f; // Overblow the movement so it's easier to make good curves
			}
		}
	}

	return snappedHitLocation;
}

FVector AABCurvedDecorHologram::LocationAndRotation_Drawn(const FHitResult& hitResult, const AActor* hitActor, const FTransform& hitToWorld) {
	//TODO: DO
	return hitResult.ImpactPoint;
}

void AABCurvedDecorHologram::UpdateAndRecalcSpline() {
	UE_LOG(LogTemp, Warning, TEXT("[[[ %s | %s | %s ]]]"), *startTangent.ToString(), *endTangent.ToString(), *endPos.ToString());

	// actually set data
	if (splineRefHolo != NULL) {
		splineRefHolo->SetEndPosition(endPos, false);
		splineRefHolo->SetStartTangent(startTangent, false);
		splineRefHolo->SetEndTangent(endTangent, true);
	}
	
	if (eState == EBendHoloState::CDH_Placing || eState == EBendHoloState::CDH_Draw_None) {
		length = lengthPerCost; // cost minimum until people draw a line
	} else {
		length = calculateMeshLength(FVector::Zero(), endPos, startTangent, endTangent);

		if (length > maxLength) { AddConstructDisqualifier(UFGCDInvalidPlacement::StaticClass()); }
		if (length < minLength) { AddConstructDisqualifier(UFGCDInvalidPlacement::StaticClass()); }

		UE_LOG(LogTemp, Warning, TEXT("[[[ Length %f ]]]"), length);
	}

	SetIsChanged(true);
}

void AABCurvedDecorHologram::ResetLineData() {
	FVector defaultSplineLoc = FVector(0.0f, 0.0f, 400.0f);
	startTangent = defaultSplineLoc;
	endPos = defaultSplineLoc * 0.99f;
	endTangent = defaultSplineLoc *0.99f;
	localPointStore.Empty();
	bShowMarker = true;
	length = lengthPerCost;
}

// Static:
float AABCurvedDecorHologram::calculateMeshLength(FVector start, FVector end, FVector startTangent, FVector endTangent) {
	/**** Borrowed and trimmed from SplineComponenet.cpp ****/
	struct FLegendreGaussCoefficient { float Abscissa; float Weight; };
	static const FLegendreGaussCoefficient LegendreGaussCoefficients[] = {
		{ 0.0f, 0.5688889f },
		{ -0.5384693f, 0.47862867f },
		{ 0.5384693f, 0.47862867f },
		{ -0.90617985f, 0.23692688f },
		{ 0.90617985f, 0.23692688f }
	};

	// Cache the coefficients to be fed into the function to calculate the spline derivative at each sample point as they are constant.
	const FVector c1 = ((start - end) * 2.0f + startTangent + endTangent) * 3.0f;
	const FVector c2 = (end - start) * 6.0f - startTangent * 4.0f - endTangent * 2.0f;

	float calc = 0.0f;
	for (const auto& LegendreGaussCoefficient : LegendreGaussCoefficients) {
		// Calculate derivative at each Legendre-Gauss sample, and perform a weighted sum
		const float alpha = 0.5f * (1.0f + LegendreGaussCoefficient.Abscissa);
		const FVector derivative = ((c1 * alpha + c2) * alpha + startTangent);
		calc += derivative.Size() * LegendreGaussCoefficient.Weight;
	}
	
	return calc * 0.5f;
}