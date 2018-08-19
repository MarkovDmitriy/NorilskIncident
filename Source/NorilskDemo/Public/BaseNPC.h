// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BaseCharacter.h"
#include "BaseNPC.generated.h"

/**
 * 
 */
UCLASS()
class NORILSKDEMO_API ABaseNPC : public ABaseCharacter
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(EditAnywhere, Category = Behavior)
	class UBehaviorTree* NPCBehavior;

	virtual bool IsFirstPerson() const override;

	virtual void FaceRotation(FRotator NewRotation, float DeltaTime = 0.f) override;

	
};
