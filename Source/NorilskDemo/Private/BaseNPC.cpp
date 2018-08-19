// Fill out your copyright notice in the Description page of Project Settings.

#include "NorilskDemo.h"
#include "Characters/BaseAIController.h"
#include "BaseNPC.h"



ABaseNPC::ABaseNPC(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = ABaseAIController::StaticClass();

	UpdatePawnMeshes();

	bUseControllerRotationYaw = true;
}

bool ABaseNPC::IsFirstPerson() const
{
	return false;
}

void ABaseNPC::FaceRotation(FRotator NewRotation, float DeltaTime)
{
	FRotator CurrentRotation = FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, 8.0f);

	Super::FaceRotation(CurrentRotation, DeltaTime);
}

