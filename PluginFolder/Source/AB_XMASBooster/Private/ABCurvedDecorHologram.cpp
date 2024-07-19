// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "ABCurvedDecorHologram.h"
#include "ABCurvedDecorBuildable.h"

#include "Math/UnrealMathUtility.h"
#include "FGConstructDisqualifier.h"
#include "Net/UnrealNetwork.h"

// Ctor:
AABCurvedDecorHologram::AABCurvedDecorHologram() {
	mCanLockHologram = true;
	mCanNudgeHologram = true;
	mSnapToGuideLines = false;
	mCanSnapWithAttachmentPoints = false;

	eState = EBendHoloState::CDH_Placing;

	ResetLineData();
}

// AActor:
void AABCurvedDecorHologram::BeginPlay() {
	Super::BeginPlay();

	TArray<AActor*> subActors;
	AABCurvedDecorBuildable* curvedDecor;

	GetAllChildActors(subActors);

	for (int i = 0; i < subActors.Num(); i++) {
		AActor* testActor = subActors[i];
		curvedDecor = Cast<AABCurvedDecorBuildable>(testActor);
		if (curvedDecor != NULL) {
			length = curvedDecor->SplineLength;
			endPos = curvedDecor->EndPosition;
			startTangent = curvedDecor->StartTangent;
			endTangent = curvedDecor->EndTangent;

			break;
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("[[[ [[READIT]] %s | %s | %s ]]]"), *startTangent.ToString(), *endTangent.ToString(), *endPos.ToString());
}

void AABCurvedDecorHologram::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AABCurvedDecorHologram, endPos);
	DOREPLIFETIME(AABCurvedDecorHologram, startTangent);
	DOREPLIFETIME(AABCurvedDecorHologram, endTangent);
}


// FactoryGame:
bool AABCurvedDecorHologram::IsValidHitResult(const FHitResult& hitResult) const {
	AActor* hitActor = hitResult.GetActor();

	// all buildables are valid targets, but we also need to check for valid natural targets. We can let vanilla do that step
	if (hitActor == NULL) {
		// Null is fine when not placing it.
		return eState != EBendHoloState::CDH_Placing;
	}
	
	if (Cast<AFGBuildable>(hitActor) != NULL) {
		return true;
	}

	return Super::IsValidHitResult(hitResult);
}

int32 AABCurvedDecorHologram::GetBaseCostMultiplier() const {
	return FMath::RoundHalfFromZero(length / lengthPerCost);
}

void AABCurvedDecorHologram::GetSupportedBuildModes_Implementation(TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const {
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mBuildModeCurved) { out_buildmodes.AddUnique(mBuildModeCurved); }
	if (mBuildModeCompoundCurve) { out_buildmodes.AddUnique(mBuildModeCompoundCurve); }
	if (mBuildModeDrawing) { out_buildmodes.AddUnique(mBuildModeDrawing); }
}

void AABCurvedDecorHologram::OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode) {
	Super::OnBuildModeChanged(buildMode);

	//TODO: deal with the fact more complexity than currently comitted shouldn't reset the line, remember there's a way to jump to a specific mode
	eState = EBendHoloState::CDH_Placing;

	ResetLineData();
	UpdateAndRecalcSpline();
	SetIsChanged(true);
}

bool AABCurvedDecorHologram::DoMultiStepPlacement(bool isInputFromARelease) {
	bool bComplete = false;

	switch (eState) {
		// now we're done placing the start point figure out the branch
		case EBendHoloState::CDH_Placing:
			if (IsCurrentBuildMode(mBuildModeDrawing)) {
				eState = EBendHoloState::CDH_Draw_Live;
			} else {
				eState = EBendHoloState::CDH_Zooping;
			}
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
		case EBendHoloState::CDH_Draw_Live:
			eState = EBendHoloState::CDH_Draw_Done;
			break;

		case EBendHoloState::CDH_Draw_Done:
			bComplete = true;
			break;
	}

	return bComplete;
}

void AABCurvedDecorHologram::SetHologramLocationAndRotation(const FHitResult& hitResult) {
	// where the hit occured. IMPORTANT: global for placing, local for other states.
	FVector snappedHitLocation = FindSnappedHitLocation(hitResult);
	
	// when placing simply move the hologram, and we're done
	if(eState == EBendHoloState::CDH_Placing) {
		SetActorLocation(snappedHitLocation);
		SetActorRotation(hitResult.ImpactNormal.ToOrientationRotator());
		markerPosition = FVector::ZeroVector; // marker is always local so centered when placing

		return;
	}

	// if our snap hasn't moved nothing needs changing
	if (snappedHitLocation.Equals(lastHit)) { return; }

	UE_LOG(LogTemp, Warning, TEXT("[[[ update: %s ]]]"), *snappedHitLocation.ToString());

	// drawing and precise behave differently
	if (IsCurrentBuildMode(mBuildModeDrawing)) {
		if (eState == EBendHoloState::CDH_Draw_Live && FVector::Distance(snappedHitLocation, localPointStore.Last()) > drawResolution) {
			int32 lastIndex = localPointStore.Num();
			localPointStore.Add(snappedHitLocation);

			for (int i = 0; i < localPointStore.Num(); i++) {
				UE_LOG(LogTemp, Warning, TEXT("[[[ local: %s"), *localPointStore[i].ToString());
			}

			if (lastIndex > 3) {
				endPos = localPointStore.Last();
				startTangent = FVector::Zero();
				endTangent = FVector::Zero();
				FVector inKick = FVector::Zero();
				FVector outKick = FVector::Zero();
				int kickContributors = FMath::Max<int>((lastIndex + 1.0f) * 0.15f, 1);

				//UE_LOG(LogTemp, Warning, TEXT("[[[   in kick: %s"), *inKick.ToString());
				//UE_LOG(LogTemp, Warning, TEXT("[[[   outKick: %s"), *outKick.ToString());

				// accumulate weighted attributes in respective tangents
				for (int i = 1; i <= lastIndex; i++) {
					float ratio = (float)i / ((float)lastIndex + 1.0f);
					//UE_LOG(LogTemp, Warning, TEXT("[[[   ratio: %f"), ratio);
					startTangent += ((1.0f - ratio) * 8.0f) * localPointStore[i];
					//UE_LOG(LogTemp, Warning, TEXT("[[[[[[   stTangent: %s"), *startTangent.ToString());
					endTangent += (ratio * 8.0f) * (localPointStore[i] - endPos);
					//UE_LOG(LogTemp, Warning, TEXT("[[[[[[   enTangent: %s"), *endTangent.ToString());

					if (i <= kickContributors) {            inKick += localPointStore[i]; }
					if (i > lastIndex-kickContributors) { outKick += localPointStore[i] - endPos; }
				}

				// hacktastic aproximation incoming
				// accumlated weights bend inwards too much, by pushing them back out we get a more accurate shape while skipping heavy math
				FVector temp, cross;

				inKick /= (float)kickContributors;
				outKick /= (float)kickContributors;

				temp = inKick;
				cross = endPos.Cross(temp);
				startTangent = startTangent.RotateAngleAxis(
					FMath::RadiansToDegrees(cross.Length() / (endPos.Length() * temp.Length())),
					cross / cross.Length()
				) / (float)lastIndex;

				temp = outKick - endPos;
				cross = (-endPos).Cross(temp);
				endTangent = endTangent.RotateAngleAxis(
					FMath::RadiansToDegrees(cross.Length() / (endPos.Length() * temp.Length())),
					cross / cross.Length()
				) / (float)lastIndex;

				startTangent += FVector(0.01f);
				endTangent = endTangent * -1.0f; // end tangents are backwards

				bShowMarker = false;
			} else {
				startTangent = FVector(0.01f);
				endTangent = FVector(0.01f);
				endPos = FVector(0.01f);
			}
		}

		if(localPointStore.Num() < 4) {
			AddConstructDisqualifier(UFGCDInvalidPlacement::StaticClass());
		}
	} else {
		// different modes change which points get affected
		if (eState == EBendHoloState::CDH_Zooping) {
			// point the buildable along the line
			SetActorRotation((ActorToWorld().TransformPosition(snappedHitLocation) - GetActorLocation()).ToOrientationRotator());

			endPos = snappedHitLocation;
			startTangent = snappedHitLocation + FVector(0.001f); // If these are "exact" floating points can decide things are twisty
			endTangent = snappedHitLocation - FVector(0.001f); // If these are "exact" floating points can decide things are twisty

			bShowMarker = false;
		} else {
			// use masking to see which points to affect, this handles several states
			if ((uint8)eState & (uint8)EBendHoloState::CDHM_isBendingIn) {
				startTangent = snappedHitLocation * 2.0f; // Overblow the movement so it's easier to make good curves
			}

			if ((uint8)eState & (uint8)EBendHoloState::CDHM_isBendingOut) {
				endTangent = (endPos - snappedHitLocation) * 2.0f; // Overblow the movement so it's easier to make good curves
			}

			bShowMarker = true;
		}
	}

	markerPosition = snappedHitLocation;
	lastHit = snappedHitLocation;

	UpdateAndRecalcSpline();
	SetIsChanged(true);
}

void AABCurvedDecorHologram::ConfigureActor(class AFGBuildable* inBuildable) const {
	Super::ConfigureActor(inBuildable);

	AABCurvedDecorBuildable* curvedDecor = Cast<AABCurvedDecorBuildable>(inBuildable);
	if (curvedDecor != NULL) {
		curvedDecor->SplineLength = length;
		curvedDecor->StartPosition = FVector::Zero();
		curvedDecor->EndPosition = endPos;
		curvedDecor->StartTangent = startTangent;
		curvedDecor->EndTangent = endTangent;
	}
}

void AABCurvedDecorHologram::ConfigureComponents(class AFGBuildable* inBuildable) const {
	Super::ConfigureComponents(inBuildable);

	// were there to be multiple spline mesh components this might be unwise, but like when will that come up
	USplineMeshComponent* splineMesh = inBuildable->GetComponentByClass<USplineMeshComponent>();
	splineMesh->SetEndPosition(endPos, false);
	splineMesh->SetStartTangent(startTangent, false); 
	splineMesh->SetEndTangent(endTangent, false);
	splineMesh->SetStartRoll(0.001f, false);
	splineMesh->SetEndRoll(0.001f, false);
	splineMesh->UpdateMesh();
}

USceneComponent* AABCurvedDecorHologram::SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName, const FName& attachSocketName) {
	// We can't live modify the spline component for some reason, so remake it constantly with the right FX
	USplineMeshComponent* splineRefTemp = Cast<USplineMeshComponent>(componentTemplate);
	if (splineRefTemp != NULL) {
		if(splineRefHolo != NULL){
			splineRefHolo->UnregisterComponent();
		}
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
FVector AABCurvedDecorHologram::FindSnappedHitLocation(const FHitResult& hitResult) const {
	AActor* hitActor = hitResult.GetActor();

	if (hitActor == NULL) {
		// cast the point out into space along the look vector for maximum length style a-la beams
		APawn* player = GetConstructionInstigator();
		FVector farSight = FVector(maxLength, 0.0f, 0.0f);
		return player->GetActorLocation() + player->GetControlRotation().Vector() * farSight;
	}

	FTransform hitToWorld = hitActor->ActorToWorld();
	FVector snappedHitLocation = FVector::Zero();

	//TODO: better snapping to more specific object types (walls, sloped foundations, spline meshes)
	/*if (Cast<AABCurvedDecorBuildable>(hitActor) != NULL) {
		USplineMeshComponent* hitSplineMesh = hitActor->GetComponentByClass<USplineMeshComponent>();
		hitSplineMesh->FindDistanceClosestToWorldLocation();

	} else */if (Cast<AFGBuildable>(hitActor) != NULL) {
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

	//UE_LOG(LogTemp, Warning, TEXT("[[[ snap: %s ]]]"), *snappedHitLocation.ToString());

	return snappedHitLocation;
}

void AABCurvedDecorHologram::UpdateAndRecalcSpline() {
	//UE_LOG(LogTemp, Warning, TEXT("[[[ %s | %s | %s ]]]"), *startTangent.ToString(), *endTangent.ToString(), *endPos.ToString());

	// actually set data
	if (splineRefHolo != NULL) {
		splineRefHolo->SetEndPosition(endPos, false);
		splineRefHolo->SetStartTangent(startTangent, false);
		splineRefHolo->SetEndTangent(endTangent, false);
		splineRefHolo->SetStartRoll(0.001f, false);
		splineRefHolo->SetEndRoll(0.001f, false);
		splineRefHolo->UpdateMesh();
	}
	
	if (eState == EBendHoloState::CDH_Placing) {
		length = lengthPerCost; // cost minimum until people draw a line
	} else {
		length = calculateMeshLength(FVector::Zero(), endPos, startTangent, endTangent);

		if (length < minLength || length > maxLength) { AddConstructDisqualifier(UFGCDInvalidPlacement::StaticClass()); }

		UE_LOG(LogTemp, Warning, TEXT("[[[ Length %f ]]]"), length);
	}
}

void AABCurvedDecorHologram::ResetLineData() {
	FVector defaultSplineLoc = FVector(0.0f, 0.0f, 400.0f);
	startTangent = defaultSplineLoc;
	endPos = defaultSplineLoc * 0.99f;
	endTangent = defaultSplineLoc *0.99f;
	localPointStore.Empty();
	localPointStore.Add(FVector::Zero());
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

FVector AABCurvedDecorHologram::nearestSplinePoint(USplineMeshComponent* spline, const FVector& test, int steps, float resolution) {
	float leftEdge = 0.0f;
	float rightEdge = 1.0f;
	FTransform leftT = spline->CalcSliceTransformAtSplineOffset(leftEdge);
	FTransform rightT = spline->CalcSliceTransformAtSplineOffset(rightEdge);
	FVector leftPoint = leftT.TransformPosition(FVector::Zero());
	FVector rightPoint = rightT.TransformPosition(FVector::Zero());
	float leftDist = FVector::DistSquared(test, leftPoint);
	float rightDist = FVector::DistSquared(test, rightPoint);
	FVector bestPoint;

	for (int i = 0; i <= steps; i++) {
		if (leftDist > rightDist) {
			bestPoint = rightPoint;
			if (i != steps) {
				leftEdge += (rightEdge - leftEdge) * resolution;
				leftT = spline->CalcSliceTransformAtSplineOffset(leftEdge);
				leftPoint = leftT.TransformPosition(FVector::Zero());
				leftDist = FVector::DistSquared(test, leftPoint);
			}
		} else {
			bestPoint = leftPoint;
			if (i != steps) {
				rightEdge -= (rightEdge - leftEdge) * resolution;
				rightT = spline->CalcSliceTransformAtSplineOffset(rightEdge);
				rightPoint = rightT.TransformPosition(FVector::Zero());
				rightDist = FVector::DistSquared(test, rightPoint);
			}
		}
	}

	return bestPoint;
}