// Fill out your copyright notice in the Description page of Project Settings.
#include "NorilskTypes.generated.h"
#pragma once

namespace ECrosshairDirection
{
	enum Type
	{
		Left = 0,
		Right = 1,
		Top = 2,
		Bottom = 3,
		Center = 4
	};
}

UENUM(BlueprintType)
enum class EGameFractions : uint8
{
	GF_PLAYER 	UMETA(DisplayName = "Player"),
	GF_NEUTRAL 	UMETA(DisplayName = "Neutral NPC"),
	GF_ENEMY	UMETA(DisplayName = "Very very bad NPC")
};

namespace EHudPosition
{
	enum Type
	{
		Left = 0,
		FrontLeft = 1,
		Front = 2,
		FrontRight = 3,
		Right = 4,
		BackRight = 5,
		Back = 6,
		BackLeft = 7,
	};
}

/** keep in sync with ShooterImpactEffect */
UENUM()
namespace EPhysMaterialType
{
	enum Type
	{
		Unknown,
		Concrete,
		Dirt,
		Water,
		Metal,
		Wood,
		Grass,
		Glass,
		Flesh,
	};
}

namespace EDialogType
{
	enum Type
	{
		None,
		Generic,
		ControllerDisconnected
	};
}

#define NORILSK_SURFACE_Default		SurfaceType_Default
#define NORILSK_SURFACE_Concrete	SurfaceType1
#define NORILSK_SURFACE_Dirt		SurfaceType2
#define NORILSK_SURFACE_Water		SurfaceType3
#define NORILSK_SURFACE_Metal		SurfaceType4
#define NORILSK_SURFACE_Wood		SurfaceType5
#define NORILSK_SURFACE_Grass		SurfaceType6
#define NORILSK_SURFACE_Glass		SurfaceType7
#define NORILSK_SURFACE_Flesh		SurfaceType8

USTRUCT()
struct FDecalData
{
	GENERATED_USTRUCT_BODY()

		/** material */
	UPROPERTY(EditDefaultsOnly, Category = Decal)
	UMaterial* DecalMaterial;

	/** quad size (width & height) */
	UPROPERTY(EditDefaultsOnly, Category = Decal)
		float DecalSize;

	/** lifespan */
	UPROPERTY(EditDefaultsOnly, Category = Decal)
		float LifeSpan;

	/** defaults */
	FDecalData()
		: DecalSize(256.f)
		, LifeSpan(10.f)
	{
	}
};

/** replicated information on a hit we've taken */
USTRUCT()
struct FTakeHitInfo
{
	GENERATED_USTRUCT_BODY()

	/** The amount of damage actually applied */
	UPROPERTY()
		float ActualDamage;

	/** The damage type we were hit with. */
	UPROPERTY()
		UClass* DamageTypeClass;

	/** Who hit us */
	UPROPERTY()
		TWeakObjectPtr<class ABaseCharacter> PawnInstigator;

	/** Who actually caused the damage */
	UPROPERTY()
		TWeakObjectPtr<class AActor> DamageCauser;

	/** Specifies which DamageEvent below describes the damage received. */
	UPROPERTY()
		int32 DamageEventClassID;

	/** Rather this was a kill */
	UPROPERTY()
		uint32 bKilled : 1;

private:

	/** A rolling counter used to ensure the struct is dirty and will replicate. */
	UPROPERTY()
		uint8 EnsureReplicationByte;

	/** Describes general damage. */
	UPROPERTY()
		FDamageEvent GeneralDamageEvent;

	/** Describes point damage, if that is what was received. */
	UPROPERTY()
		FPointDamageEvent PointDamageEvent;

	/** Describes radial damage, if that is what was received. */
	UPROPERTY()
		FRadialDamageEvent RadialDamageEvent;

public:
	FTakeHitInfo();

	FDamageEvent& GetDamageEvent();
	void SetDamageEvent(const FDamageEvent& DamageEvent);
	void EnsureReplication();
};
