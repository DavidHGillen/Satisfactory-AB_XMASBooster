// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "ABCurvedDecorHologram.h"

#include "Math/UnrealMathUtility.h"
#include "FGConstructDisqualifier.h"
#include "Net/UnrealNetwork.h"

// Ctor
//////////////////////////////////////////////////////
AABCurvedDecorHologram::AABCurvedDecorHologram() {
	mCanLockHologram = true;
	mCanNudgeHologram = true;
	mSnapToGuideLines = false;
	mCanSnapWithAttachmentPoints = false;

	isAnyCurvedBeamMode = false;
	eState = EBendHoloState::CDH_Placing;

	ResetLineData();
}

void AABCurvedDecorHologram::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AABCurvedDecorHologram, endPos);
	DOREPLIFETIME(AABCurvedDecorHologram, startTangent);
	DOREPLIFETIME(AABCurvedDecorHologram, endTangent);
}

void AABCurvedDecorHologram::GetSupportedBuildModes_Implementation(TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mBuildModeCurved) { out_buildmodes.AddUnique(mBuildModeCurved); }
	if (mBuildModeCompoundCurve) { out_buildmodes.AddUnique(mBuildModeCompoundCurve); }
	if (mBuildModeDrawing) { out_buildmodes.AddUnique(mBuildModeDrawing); }
}

bool AABCurvedDecorHologram::IsValidHitResult(const FHitResult& hitResult) const
{
	// all buildables are valid targets, but we also need to check for valid natural targets. We can let vanilla do that step.
	if (Cast<AFGBuildable>(hitResult.GetActor()) != NULL) { return true; }
	return Super::IsValidHitResult(hitResult);
}

int32 AABCurvedDecorHologram::GetBaseCostMultiplier() const
{
	return FMath::RoundHalfFromZero(length / lengthPerCost);
}

void AABCurvedDecorHologram::OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode)
{
	Super::OnBuildModeChanged(buildMode);

	isAnyCurvedBeamMode = IsCurrentBuildMode(mBuildModeCurved) || IsCurrentBuildMode(mBuildModeCompoundCurve);
	eState = EBendHoloState::CDH_Placing;

	ResetLineData();
	UpdateAndReconstructSpline();
}

bool AABCurvedDecorHologram::DoMultiStepPlacement(bool isInputFromARelease)
{
	bool bComplete = false;

	switch (eState) {
		// precise modes each add another step after the last, from straight to defining one or two curved points
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

		// drawing mode goes from none, to painting, to confirm
		case EBendHoloState::CDH_Draw_None:
			eState = EBendHoloState::CDH_Draw_Live;
			break;

		case EBendHoloState::CDH_Draw_Live:
			eState = EBendHoloState::CDH_Draw_Done;
			break;

		case EBendHoloState::CDH_Draw_Done:
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
			AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
		} else {
			// cast the point out into space along the look vector
			APawn* player = GetConstructionInstigator();
			FVector farSight = FVector(eState == EBendHoloState::CDH_Zooping ? maxLength : maxBend, 0.0f, 0.0f);
			snappedHitLocation = player->GetActorLocation() + player->GetControlRotation().Vector() * farSight;
		}
	} else {
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
			// world location of the thing whatever it is
			snappedHitLocation = hitResult.ImpactPoint;
		}

		if (eState != EBendHoloState::CDH_Placing) {
			// reproject from world into local co-ordinates
			snappedHitLocation = ActorToWorld().InverseTransformPosition(snappedHitLocation);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[[[ %s ]]]"), *snappedHitLocation.ToString());

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
		markerPosition = FVector::ZeroVector;

		UpdateAndReconstructSpline();

		return;
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

	} else {
		if ((uint8)eState & (uint8)EBendHoloState::CDHM_BendIn) {
			startTangent = snappedHitLocation * 2.0f; // Overblow the movement so it's easier to make good curves
		}

		if ((uint8)eState & (uint8)EBendHoloState::CDHM_BendOut) {
			endTangent = (endPos - snappedHitLocation) * 2.0f; // Overblow the movement so it's easier to make good curves
		}
	}

	markerPosition = snappedHitLocation;
	UpdateAndReconstructSpline();

	if (length < mGridSnapSize) {
		AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
	}
	if (snappedHitLocation.Size() > maxBend * 1.01f) {
		AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
	}
}

bool AABCurvedDecorHologram::TrySnapToActor(const FHitResult& hitResult) {
	return Super::TrySnapToActor(hitResult);
}

void AABCurvedDecorHologram::ConfigureActor(class AFGBuildable* inBuildable) const {
	Super::ConfigureActor(inBuildable);
}

void AABCurvedDecorHologram::ConfigureComponents(class AFGBuildable* inBuildable) const {
	Super::ConfigureComponents(inBuildable);
}

// Custom:
//////////////////////////////////////////////////////
void AABCurvedDecorHologram::UpdateAndReconstructSpline()
{
	UE_LOG(LogTemp, Warning, TEXT("[[[ %s | %s | %s ]]]"), *startTangent.ToString(), *endTangent.ToString(), *endPos.ToString());
	
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
}
