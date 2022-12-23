// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "Math/UnrealMathUtility.h"
#include "ABCurvedDecorHologram.h"

// Ctor
//////////////////////////////////////////////////////
AABCurvedDecorHologram::AABCurvedDecorHologram() {
	eState = EBendHoloState::CDH_Placing;
	isAnyCurvedBeamMode = IsCurrentBuildMode(mBuildModeCurved) || IsCurrentBuildMode(mBuildModeCompoundCurve);

	ResetLine();
}

// AFGHologram interface
//////////////////////////////////////////////////////
//void AABCurvedDecorHologram::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
//{
//	UE_LOG(LogTemp, Warning, TEXT("[ABCB] RepProps:"));
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//}

void AABCurvedDecorHologram::GetSupportedBuildModes_Implementation(TArray<TSubclassOf<UFGHologramBuildModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mBuildModeCurved) { out_buildmodes.AddUnique(mBuildModeCurved); }
	if (mBuildModeCompoundCurve) { out_buildmodes.AddUnique(mBuildModeCompoundCurve); }
}

//bool AABCurvedDecorHologram::IsValidHitResult(const FHitResult& hitResult) const
//{
//	UE_LOG(LogTemp, Warning, TEXT("[ABCB] IsValid:"));
//	return true;
//}

int32 AABCurvedDecorHologram::GetBaseCostMultiplier() const
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Cost: %f"), FMath::RoundHalfFromZero(length / lengthPerCost));
	//TODO:
	//TODO:
	//TODO:
	//TODO:
	//TODO:
	return 1;
}

//void AABCurvedDecorHologram::ConfigureActor(AFGBuildable* inBuildable) const
//{
//	Super::ConfigureActor(inBuildable);
//
//	return;
//}
//
//void AABCurvedDecorHologram::BeginPlay()
//{
//	UE_LOG(LogTemp, Warning, TEXT("[ABCB] BeginPlay:"));
//
//	isAnyCurvedBeamMode = IsCurrentBuildMode(mBuildModeCurved) || IsCurrentBuildMode(mBuildModeCompoundCurve);
//
//	eState = EBendHoloState::CDH_Placing;
//	ResetLine();
//	UpdateAndReconstructSpline();
//
//	Super::BeginPlay();
//}

void AABCurvedDecorHologram::OnBuildModeChanged()
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Change: %s"), isAnyCurvedBeamMode ? TEXT("CRV") : TEXT("STR"));
	Super::OnBuildModeChanged();

	isAnyCurvedBeamMode = IsCurrentBuildMode(mBuildModeCurved) || IsCurrentBuildMode(mBuildModeCompoundCurve);

	eState = EBendHoloState::CDH_Placing;
	//ResetLine();
	//UpdateAndReconstructSpline();
}

//void AABCurvedDecorHologram::PreHologramPlacement()
//{
//	UE_LOG(LogTemp, Warning, TEXT("[ABCB] //// PREPLACE:"));
//	UpdateAndReconstructSpline();
//	Super::PreHologramPlacement();
//}
//
//void AABCurvedDecorHologram::PostHologramPlacement()
//{
//	UpdateAndReconstructSpline();
//	Super::PostHologramPlacement();
//	UE_LOG(LogTemp, Warning, TEXT("[ABCB] POSTPLACE ////:"));
//}

bool AABCurvedDecorHologram::DoMultiStepPlacement(bool isInputFromARelease)
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Multistep begin: %d"), (uint8)eState);

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

	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Multistep end: %d %s"), (uint8)eState, bComplete ? TEXT("True") : TEXT("False"));
	return bComplete;
}

void AABCurvedDecorHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	AActor* hitActor = hitResult.GetActor();
	FTransform hitToWorld = hitActor != NULL ? hitActor->ActorToWorld() : FTransform::Identity;
	FVector snappedHitLocation;

	if (hitActor == NULL) {
		if (eState == EBendHoloState::CDH_Placing) {
			UE_LOG(LogTemp, Warning, TEXT("[ABCB] NO ACTOR LOST"));
			// stay where you were cause where the hell else would you go
			snappedHitLocation = lastHit;
		} else {
			UE_LOG(LogTemp, Warning, TEXT("[ABCB] NO ACTOR RAYD"));
			// cast the point out into space along the look vector
			APawn* player = GetConstructionInstigator();
			snappedHitLocation = player->GetActorLocation() + player->GetControlRotation().RotateVector(FVector(maxLength,0.0f,0.0f));
		}
	} else {
		UE_LOG(LogTemp, Warning, TEXT("[ABCB] ACTOR, STATE: %d"), (uint8)eState);

		if (Cast<AFGBuildable>(hitActor) != NULL) {
			UE_LOG(LogTemp, Warning, TEXT("[ABCB] ACTOR, Factory Be Here"));
			// snap into factory space
			//TODO: account for scaled factory things with snap
			snappedHitLocation = hitToWorld.TransformPosition(
				hitToWorld.InverseTransformPosition(hitResult.ImpactPoint).GridSnap(50)
			);
		}

		if (eState != EBendHoloState::CDH_Placing) {
			UE_LOG(LogTemp, Warning, TEXT("[ABCB] ACTOR, LocalToMe"));
			// reproject into local space for local co-ordinates
			snappedHitLocation = ActorToWorld().InverseTransformPosition(snappedHitLocation);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[ABCB] IMPACT: %f %f %f"), hitResult.ImpactPoint.X, hitResult.ImpactPoint.Y, hitResult.ImpactPoint.Z);
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] SET LOCROT: %f %f %f"), snappedHitLocation.X, snappedHitLocation.Y, snappedHitLocation.Z);

	// if our snap hasn't moved nothing needs changing
	FVector test = lastHit;
	lastHit = snappedHitLocation;
	if (snappedHitLocation.Equals(test)) {
		UE_LOG(LogTemp, Warning, TEXT("[ABCB] SET LOCROT -- ABORT"));
		return;
	}

	// just move the object
	if (eState == EBendHoloState::CDH_Placing) {
		SetActorLocation(snappedHitLocation);
		SetActorRotation(FQuat(hitResult.ImpactNormal, 0));
	}

	// length = in tangent = out tanget >>>> evenly distributed straight spline
	if (eState == EBendHoloState::CDH_Zooping) {
		//TODO: point the buildable along this line, it's cleaner
		endPos = snappedHitLocation;
	}

	if ((uint8)eState & (uint8)EBendHoloState::CDHM_BendIn) {
		startTangent = snappedHitLocation;
	}

	if ((uint8)eState & (uint8)EBendHoloState::CDHM_BendOut) {
		endTangent = endPos - snappedHitLocation;
	}

	UpdateAndReconstructSpline();
}
/*
void AABCurvedDecorHologram::CheckValidPlacement()
{
	Super::CheckValidPlacement();
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] CheckValid:"));
}*/

/*void AABCurvedDecorHologram::ConfigureActor(AFGBuildable* inBuildable) const
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Config:"));
}*/

USceneComponent* AABCurvedDecorHologram::SetupComponent(USceneComponent* attachParent, UActorComponent* componentTemplate, const FName& componentName)
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Setup: %s"), *(componentName.ToString()));

	// Lets keep track of our spline
	USplineMeshComponent* splineRefTemp = Cast<USplineMeshComponent>(componentTemplate);
	if (splineRefTemp != NULL) {
		if(splineRefComp != NULL){ splineRefComp->UnregisterComponent(); }
		splineRefComp = DuplicateComponent<USplineMeshComponent>(attachParent, splineRefTemp, componentName);

		splineRefComp->SetupAttachment(attachParent);
		splineRefComp->SetCollisionProfileName(CollisionProfileHologram);
		splineRefComp->SetSimulatePhysics(false);
		splineRefComp->SetCastShadow(false);
		splineRefComp->ComponentTags.Add(HOLOGRAM_MESH_TAG);
		splineRefComp->RegisterComponent();

		UpdateAndReconstructSpline();

		UFGBlueprintFunctionLibrary::ShowOutline(splineRefComp, EOutlineColor::OC_HOLOGRAM);
		UFGBlueprintFunctionLibrary::ApplyCustomizationPrimitiveData(this, mCustomizationData, this->mCustomizationData.ColorSlot, splineRefComp);

		return splineRefComp;
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

	SetIsChanged(true);
}

void AABCurvedDecorHologram::ResetLine()
{
	FVector defaultSplineLoc = FVector(0.0f, 0.0f, 400.0f);
	startTangent = defaultSplineLoc;
	endPos = defaultSplineLoc;
	endTangent = defaultSplineLoc;

	eState = EBendHoloState::CDH_Placing;
}
