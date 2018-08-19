// Fill out your copyright notice in the Description page of Project Settings.

#include "NorilskDemo.h"
#include "BaseCharacterMovement.h"



UBaseCharacterMovement::UBaseCharacterMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


float UBaseCharacterMovement::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	const ABaseCharacter* MyOwner = Cast<ABaseCharacter>(PawnOwner);
	if (MyOwner)
	{
		if (MyOwner->IsTargeting())
		{
			MaxSpeed *= MyOwner->GetTargetingSpeedModifier();
		}
		if (MyOwner->IsRunning())
		{
			MaxSpeed *= MyOwner->GetRunningSpeedModifier();
		}
	}

	return MaxSpeed;
}

