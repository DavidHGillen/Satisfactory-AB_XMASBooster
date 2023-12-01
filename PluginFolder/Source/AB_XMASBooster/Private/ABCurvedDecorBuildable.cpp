// Copyright David "Angry Beaver" Gillen, details listed on associated mods Readme

#include "ABCurvedDecorBuildable.h"

#include "Math/UnrealMathUtility.h"
#include "Net/UnrealNetwork.h"

void AABCurvedDecorBuildable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AABCurvedDecorBuildable, SplineLength);

	DOREPLIFETIME(AABCurvedDecorBuildable, StartPosition);
	DOREPLIFETIME(AABCurvedDecorBuildable, EndPosition);
	DOREPLIFETIME(AABCurvedDecorBuildable, StartTangent);
	DOREPLIFETIME(AABCurvedDecorBuildable, EndTangent);
}

void AABCurvedDecorBuildable::UpdateSplineMesh() {
	// were there to be multiple spline mesh components this might be unwise
	USplineMeshComponent* splineMesh = GetComponentByClass<USplineMeshComponent>();
	splineMesh->SetSplineUpDir(FVector::UnitZ());
	splineMesh->SetStartPosition(StartPosition, false);
	splineMesh->SetEndPosition(EndPosition, false);
	splineMesh->SetStartTangent(StartTangent, false);
	splineMesh->SetEndTangent(EndTangent, false);
	splineMesh->UpdateMesh();
	splineMesh->UpdateBounds();
}
