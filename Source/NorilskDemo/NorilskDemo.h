// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "ParticleDefinitions.h"
#include "SoundDefinitions.h"
#include "Net/UnrealNetwork.h"
#include "NorilskGameMode.h"
#include "BaseCharacter.h"
#include "NorilskPlayerController.h"

class UBehaviorTreeComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogNorilsk, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNorilskWeapon, Log, All);

/** when you modify this, please note that this information can be saved with instances
 * also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list **/
#define COLLISION_WEAPON		ECC_GameTraceChannel1
#define COLLISION_PROJECTILE	ECC_GameTraceChannel2
#define COLLISION_PICKUP		ECC_GameTraceChannel3

#define MAX_PLAYER_NAME_LENGTH 16

