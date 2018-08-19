// Fill out your copyright notice in the Description page of Project Settings.

#include "NorilskDemo.h"
#include "NorilskPlayerController.h"
#include "BaseWeapon.h"
#include "BasePlayerCameraManager.h"
#include "BaseCharacter.h"

ANorilskPlayerController::ANorilskPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerCameraManagerClass = ABasePlayerCameraManager::StaticClass();
}


bool ANorilskPlayerController::HasInfiniteAmmo() const
{
	return false;
}

bool ANorilskPlayerController::HasInfiniteClip() const
{
	return false;
}

bool ANorilskPlayerController::HasHealthRegen() const
{
	return false;
}

bool ANorilskPlayerController::HasGodMode() const
{
	return false;
}

bool ANorilskPlayerController::IsGameInputAllowed() const
{
	return !bCinematicMode;
}

void ANorilskPlayerController::SetCinematicMode(bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD, bool bAffectsMovement, bool bAffectsTurning)
{
	Super::SetCinematicMode(bInCinematicMode, bHidePlayer, bAffectsHUD, bAffectsMovement, bAffectsTurning);

	// If we have a pawn we need to determine if we should show/hide the weapon
	ABaseCharacter* MyPawn = Cast<ABaseCharacter>(GetPawn());
	ABaseWeapon* MyWeapon = MyPawn ? MyPawn->GetWeapon() : NULL;
	if (MyWeapon)
	{
		if (bInCinematicMode && bHidePlayer)
		{
			MyWeapon->SetActorHiddenInGame(true);
		}
		else if (!bCinematicMode)
		{
			MyWeapon->SetActorHiddenInGame(false);
		}
	}
}
