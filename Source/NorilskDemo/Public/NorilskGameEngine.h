// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/GameEngine.h"
#include "NorilskGameEngine.generated.h"

/**
 * 
 */
UCLASS()
class NORILSKDEMO_API UNorilskGameEngine : public UGameEngine
{
	GENERATED_UCLASS_BODY()

		/* Hook up specific callbacks */
		virtual void Init(IEngineLoop* InEngineLoop);

public:

	/**
	* 	All regular engine handling, plus update ShooterKing state appropriately.
	*/
	virtual void HandleNetworkFailure(UWorld *World, UNetDriver *NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString) override;
};