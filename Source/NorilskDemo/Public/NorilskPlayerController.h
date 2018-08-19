// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerController.h"
#include "NorilskPlayerController.generated.h"

/**
 * 
 */
UCLASS(config=Game)
class NORILSKDEMO_API ANorilskPlayerController : public APlayerController
{
	GENERATED_UCLASS_BODY()
	
	
	/** get infinite ammo cheat */
	bool HasInfiniteAmmo() const;

	/** get infinite clip cheat */
	bool HasInfiniteClip() const;

	/** get health regen cheat */
	bool HasHealthRegen() const;

	/** get gode mode cheat */
	bool HasGodMode() const;

	virtual void SetCinematicMode(bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD, bool bAffectsMovement, bool bAffectsTurning) override;


	/** check if gameplay related actions (movement, weapon usage, etc) are allowed right now */
	bool IsGameInputAllowed() const;
	
};
