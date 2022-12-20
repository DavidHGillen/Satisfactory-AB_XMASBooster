// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "Math/UnrealMathUtility.h"
#include "ABCurvedDecorHologram.h"

// Ctor
//////////////////////////////////////////////////////
AABCurvedDecorHologram::AABCurvedDecorHologram() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	PrimaryActorTick.SetTickFunctionEnable(true);

	//mDecorMeshComponentName = "mSplineMesh";
	ResetLine();
	UpdateAndReconstructSpline();
}

// AFGHologram interface
//////////////////////////////////////////////////////
void AABCurvedDecorHologram::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] RepProps:"));
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AABCurvedDecorHologram::GetSupportedBuildModes_Implementation(TArray<TSubclassOf<UFGHologramBuildModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mBuildModeCurved) { out_buildmodes.AddUnique(mBuildModeCurved); }
	if (mBuildModeCompoundCurve) { out_buildmodes.AddUnique(mBuildModeCompoundCurve); }
}

bool AABCurvedDecorHologram::IsValidHitResult(const FHitResult& hitResult) const
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] IsValid:"));
	return true;
}

int32 AABCurvedDecorHologram::GetBaseCostMultiplier() const
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Cost: %d"), FMath::RoundHalfFromZero(length / lengthPerCost));
	//TODO:
	//TODO:
	//TODO:
	//TODO:
	//TODO:
	return 1;
}

void AABCurvedDecorHologram::BeginPlay()
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] BeginPlay:"));

	isAnyCurvedBeamMode = IsCurrentBuildMode(mBuildModeCurved) || IsCurrentBuildMode(mBuildModeCompoundCurve);

	eState = EBendHoloState::CDH_Placing;
	ResetLine();
	UpdateAndReconstructSpline();

	Super::BeginPlay();
}

void AABCurvedDecorHologram::OnBuildModeChanged()
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Change: %s"), isAnyCurvedBeamMode ? TEXT("CRV") : TEXT("STR"));
	Super::OnBuildModeChanged();

	isAnyCurvedBeamMode = IsCurrentBuildMode(mBuildModeCurved) || IsCurrentBuildMode(mBuildModeCompoundCurve);

	eState = EBendHoloState::CDH_Placing;
	ResetLine();
	UpdateAndReconstructSpline();
}

void AABCurvedDecorHologram::PreHologramPlacement()
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] //// PREPLACE:"));
	UpdateAndReconstructSpline();
	Super::PreHologramPlacement();
}

void AABCurvedDecorHologram::PostHologramPlacement()
{
	UpdateAndReconstructSpline();
	Super::PostHologramPlacement();
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] POSTPLACE ////:"));
}

bool AABCurvedDecorHologram::DoMultiStepPlacement(bool isInputFromARelease)
{
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Multistep begin: %d"), eState);

	bool bComplete = false;

	switch (eState) {
		case EBendHoloState::CDH_Placing:
			bComplete = !isAnyCurvedBeamMode;
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

	UE_LOG(LogTemp, Warning, TEXT("[ABCB] Multistep end: %d %s"), eState, bComplete ? TEXT("True") : TEXT("False"));
	return bComplete;
}

void AABCurvedDecorHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	AActor* hitActor = hitResult.GetActor();
	FTransform hitToWorld = hitActor != NULL ? hitActor->ActorToWorld() : FTransform::Identity;
	FVector localSnappedHitLocation;

	if (hitActor == NULL) {
		// cast the point out into space along the look vector
		/*
		GetConstructionInstigator()->GetControlRotation();
		GetConstructionInstigator()->GetActorLocation();
		*/
		localSnappedHitLocation = FVector(100.0f); // TODO:
	}
	else {
		// convert to the space of the hit object, snap in it, then convert back to world, then world to our space
		localSnappedHitLocation = ActorToWorld().InverseTransformPosition(
			hitToWorld.TransformPosition(
				hitToWorld.InverseTransformPosition(hitResult.ImpactPoint).GridSnap(50)
			)
		);
	}

	// if our snap hasn't moved nothing needs changing
	FVector test = lastHit;
	lastHit = localSnappedHitLocation;
	if (!localSnappedHitLocation.Equals(test)) {
		UE_LOG(LogTemp, Warning, TEXT("[ABCB] SET LOCROT: %s"), TEXT("NON"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ABCB] SET LOCROT: %d %d %d"), localSnappedHitLocation.X, localSnappedHitLocation.Y, localSnappedHitLocation.Z);

	// just move the object
	if (eState == EBendHoloState::CDH_Placing) {
		SetActorLocationAndRotation(localSnappedHitLocation, FQuat(hitResult.ImpactNormal, 0).Rotator());
	}

	// length = in tangent = out tanget >>>> evenly distributed straight spline
	if (eState == EBendHoloState::CDH_Zooping) {
		//TODO: point the buildable along this line, it's cleaner
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

void AABCurvedDecorHologram::CheckValidPlacement()
{
	Super::CheckValidPlacement();
	UE_LOG(LogTemp, Warning, TEXT("[ABCB] CheckValid:"));
}

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
