// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "BehaviorTree/BTNode.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindPointNearEnemy.generated.h"

/**
 * 
 */
UCLASS()
class NORILSKDEMO_API UBTTask_FindPointNearEnemy : public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
};
