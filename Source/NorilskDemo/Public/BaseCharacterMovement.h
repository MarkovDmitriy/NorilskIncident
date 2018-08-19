// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "BaseCharacterMovement.generated.h"

/**
 * 
 */
UCLASS()
class NORILSKDEMO_API UBaseCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

	virtual float GetMaxSpeed() const override;
	
	
	
	
};
