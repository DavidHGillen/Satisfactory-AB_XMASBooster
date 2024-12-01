// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "ABCurvedDecorBuildable.h"

#include "Math/UnrealMathUtility.h"
#include "Net/UnrealNetwork.h"

AABCurvedDecorBuildable::AABCurvedDecorBuildable() {
	bReplicates = true;
}

void AABCurvedDecorBuildable::BeginPlay() {
	//UE_LOG(LogTemp, Warning, TEXT("[{[BeginPlay]]]"));
	Super::BeginPlay();
	UpdateSplineMesh();
}

void AABCurvedDecorBuildable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AABCurvedDecorBuildable, CostMultiplier);
	DOREPLIFETIME(AABCurvedDecorBuildable, SplineLength);

	DOREPLIFETIME(AABCurvedDecorBuildable, StartPosition);
	DOREPLIFETIME(AABCurvedDecorBuildable, EndPosition);
	DOREPLIFETIME(AABCurvedDecorBuildable, StartTangent);
	DOREPLIFETIME(AABCurvedDecorBuildable, EndTangent);
}

void AABCurvedDecorBuildable::PostSerializedFromBlueprint(bool isBlueprintWorld) {
	//UE_LOG(LogTemp, Warning, TEXT("[{[PostSerz]]]"));
	Super::PostSerializedFromBlueprint(isBlueprintWorld);

	GetWorld()->GetTimerManager().SetTimer(Timir, this, &AABCurvedDecorBuildable::UpdateSplineMesh, 0.1f, true);
}

int32 AABCurvedDecorBuildable::GetDismantleRefundReturnsMultiplier() const {
	return CostMultiplier;
}

void AABCurvedDecorBuildable::UpdateSplineMesh() {
	// were there to be multiple spline mesh components this might be unwise
	USplineMeshComponent* splineMesh = GetComponentByClass<USplineMeshComponent>();
	if (splineMesh == NULL) {
		//UE_LOG(LogTemp, Warning, TEXT("[{[NOMAS]]]"));
		return;
	}

	//UE_LOG(LogTemp, Warning, TEXT("[{[ [[UPDATE IT]] %s | %s ||| %s | %s ]]]"), *StartPosition.ToString(), *EndPosition.ToString(), *StartTangent.ToString(), *EndTangent.ToString());

	splineMesh->SetStartPosition(StartPosition, false);
	splineMesh->SetEndPosition(EndPosition, false);
	splineMesh->SetStartTangent(StartTangent, false);
	splineMesh->SetEndTangent(EndTangent, false);
	splineMesh->SetStartRoll(0.001f, false);
	splineMesh->SetEndRoll(0.001f, false);
	splineMesh->UpdateMesh_Concurrent();
	splineMesh->UpdateBounds();
}
