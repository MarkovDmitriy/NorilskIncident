// Fill out your copyright notice in the Description page of Project Settings.

#include "NorilskDemo.h"
#include "BaseWeapon.h"
#include "Characters/BaseAIController.h"

// Sets default values
ABaseWeapon::ABaseWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh1P"));
	Mesh1P->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = Mesh1P;

	Mesh3P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh3P"));
	Mesh3P->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh3P->bReceivesDecals = false;
	Mesh3P->CastShadow = true;
	Mesh3P->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh3P->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh3P->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	Mesh3P->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh3P->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	Mesh3P->SetupAttachment(Mesh1P);

	bLoopedMuzzleFX = false;
	bLoopedFireAnim = false;
	bPlayingFireAnim = false;
	bIsEquipped = false;
	bWantsToFire = false;
	bPendingReload = false;
	bPendingEquip = false;
	CurrentState = EWeaponState::Idle;

	CurrentAmmo = 0;
	CurrentAmmoInClip = 0;
	BurstCounter = 0;
	LastFireTime = 0.0f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bNetUseOwnerRelevancy = true;

}

void ABaseWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (WeaponConfig.InitialClips > 0)
	{
		CurrentAmmoInClip = WeaponConfig.AmmoPerClip;
		CurrentAmmo = WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;
	}

	DetachMeshFromPawn();
}

void ABaseWeapon::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
}

//////////////////////////////////////////////////////////////////////////
// Inventory

void ABaseWeapon::OnEquip(const ABaseWeapon* LastWeapon)
{
	AttachMeshToPawn();

	bPendingEquip = true;
	DetermineWeaponState();

	// Only play animation if last weapon is valid
	if (LastWeapon)
	{
		float Duration = PlayWeaponAnimation(EquipAnim);
		if (Duration <= 0.0f)
		{
			// failsafe
			Duration = 0.5f;
		}
		EquipStartedTime = GetWorld()->GetTimeSeconds();
		EquipDuration = Duration;

		GetWorldTimerManager().SetTimer(TimerHandle_OnEquipFinished, this, &ABaseWeapon::OnEquipFinished, Duration, false);
	}
	else
	{
		OnEquipFinished();
	}

	if (m_owner && m_owner->IsLocallyControlled())
	{
		PlayWeaponSound(EquipSound);
	}
}

void ABaseWeapon::OnEquipFinished()
{
	AttachMeshToPawn();

	bIsEquipped = true;
	bPendingEquip = false;

	// Determine the state so that the can reload checks will work
	DetermineWeaponState(); 
	
	if (m_owner)
	{
		// try to reload empty clip
		if (m_owner->IsLocallyControlled() &&
			CurrentAmmoInClip <= 0 &&
			CanReload())
		{
			StartReload();
		}
	}

}

void ABaseWeapon::OnUnEquip()
{
	DetachMeshFromPawn();
	bIsEquipped = false;
	StopFire();

	if (bPendingReload)
	{
		StopWeaponAnimation(ReloadAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_StopReload);
		GetWorldTimerManager().ClearTimer(TimerHandle_ReloadWeapon);
	}

	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_OnEquipFinished);
	}

	DetermineWeaponState();
}

void ABaseWeapon::OnEnterInventory(ABaseCharacter* NewOwner)
{
	SetOwningPawn(NewOwner);
}

void ABaseWeapon::OnLeaveInventory()
{
	if (Role == ROLE_Authority)
	{
		SetOwningPawn(NULL);
	}

	if (IsAttachedToPawn())
	{
		OnUnEquip();
	}
}

void ABaseWeapon::AttachMeshToPawn()
{
	if (m_owner)
	{
		// Remove and hide both first and third person meshes
		DetachMeshFromPawn();

		// For locally controller players we attach both weapons and let the bOnlyOwnerSee, bOwnerNoSee flags deal with visibility.
		FName AttachPoint = m_owner->GetWeaponAttachPoint();
		if( m_owner->IsLocallyControlled() == true )
		{
			USkeletalMeshComponent* PawnMesh1p = m_owner->GetSpecifcPawnMesh(true);
			USkeletalMeshComponent* PawnMesh3p = m_owner->GetSpecifcPawnMesh(false);
			Mesh1P->SetHiddenInGame( false );
			Mesh3P->SetHiddenInGame( false );
			Mesh1P->AttachToComponent(PawnMesh1p, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
			Mesh3P->AttachToComponent(PawnMesh3p, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
		}
		else
		{
			USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
			USkeletalMeshComponent* UsePawnMesh = m_owner->GetPawnMesh();
			UseWeaponMesh->AttachToComponent(UsePawnMesh, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
			UseWeaponMesh->SetHiddenInGame( false );
		}
	}
}

void ABaseWeapon::DetachMeshFromPawn()
{
	Mesh1P->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	Mesh1P->SetHiddenInGame(true);

	Mesh3P->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	Mesh3P->SetHiddenInGame(true);
}

//////////////////////////////////////////////////////////////////////////
// Input

void ABaseWeapon::StartFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

void ABaseWeapon::StopFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

void ABaseWeapon::StartReload(bool bFromReplication)
{
	if (!bFromReplication && Role < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(ReloadAnim);		
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = WeaponConfig.NoAnimReloadDuration;
		}

		GetWorldTimerManager().SetTimer(TimerHandle_StopReload, this, &ABaseWeapon::StopReload, AnimDuration, false);
		if (Role == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_ReloadWeapon, this, &ABaseWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		}
		
		if (m_owner && m_owner->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}
}

void ABaseWeapon::StopReload()
{
	if (CurrentState == EWeaponState::Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}

bool ABaseWeapon::ServerStartFire_Validate()
{
	return true;
}

void ABaseWeapon::ServerStartFire_Implementation()
{
	StartFire();
}

bool ABaseWeapon::ServerStopFire_Validate()
{
	return true;
}

void ABaseWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

bool ABaseWeapon::ServerStartReload_Validate()
{
	return true;
}

void ABaseWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

bool ABaseWeapon::ServerStopReload_Validate()
{
	return true;
}

void ABaseWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

void ABaseWeapon::ClientStartReload_Implementation()
{
	StartReload();
}

//////////////////////////////////////////////////////////////////////////
// Control

bool ABaseWeapon::CanFire() const
{
	bool bCanFire = m_owner && m_owner->CanFire();
	bool bStateOKToFire = ( ( CurrentState ==  EWeaponState::Idle ) || ( CurrentState == EWeaponState::Firing) );	
	return (( bCanFire == true ) && ( bStateOKToFire == true ) && ( bPendingReload == false ));
}

bool ABaseWeapon::CanReload() const
{
	bool bCanReload = (!m_owner || m_owner->CanReload());
	bool bGotAmmo = ( CurrentAmmoInClip < WeaponConfig.AmmoPerClip) && (CurrentAmmo - CurrentAmmoInClip > 0 || HasInfiniteClip());
	bool bStateOKToReload = ( ( CurrentState ==  EWeaponState::Idle ) || ( CurrentState == EWeaponState::Firing) );
	return ( ( bCanReload == true ) && ( bGotAmmo == true ) && ( bStateOKToReload == true) );	
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage

void ABaseWeapon::GiveAmmo(int AddAmount)
{
	const int32 MissingAmmo = FMath::Max(0, WeaponConfig.MaxAmmo - CurrentAmmo);
	AddAmount = FMath::Min(AddAmount, MissingAmmo);
	CurrentAmmo += AddAmount;

	ABaseAIController* BotAI = m_owner ? Cast<ABaseAIController>(m_owner->GetController()) : NULL;
	if (BotAI)
	{
		BotAI->CheckAmmo(this);
	}

	// start reload if clip was empty
	if (GetCurrentAmmoInClip() <= 0 &&
		CanReload() &&
		m_owner->GetWeapon() == this)
	{
		ClientStartReload();
	}
}

void ABaseWeapon::UseAmmo()
{
	if (!HasInfiniteAmmo())
	{
		CurrentAmmoInClip--;
	}

	ABaseAIController* BotAI = m_owner ? Cast<ABaseAIController>(m_owner->GetController()) : NULL;
	if (BotAI)
	{
		BotAI->CheckAmmo(this);
	}

	if (!HasInfiniteAmmo() && !HasInfiniteClip())
	{
		CurrentAmmo--;
	}

	/*ANorilskPlayerController* PlayerController = owner ? Cast<AShooterPlayerController>(owner->GetController()) : NULL;
	else if(PlayerController)
	{
		AShooterPlayerState* PlayerState = Cast<AShooterPlayerState>(PlayerController->PlayerState);
		switch (GetAmmoType())
		{
			case EAmmoType::ERocket:
				PlayerState->AddRocketsFired(
				break;
			case EAmmoType::EBullet:
			default:
				PlayerState->AddBulletsFired(1);
				break;			
		}
	}*/
}

void ABaseWeapon::HandleFiring()
{
	if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (m_owner && m_owner->IsLocallyControlled())
		{
			FireWeapon();

			UseAmmo();
			
			// update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (m_owner && m_owner->IsLocallyControlled())
	{
		if (GetCurrentAmmo() <=10 && !bRefiring)
		{
			PlayWeaponSound(OutOfAmmoSound);
			ANorilskPlayerController* MyPC = Cast<ANorilskPlayerController>(m_owner->Controller);
			ABaseHUD* MyHUD = MyPC ? Cast<ABaseHUD>(MyPC->GetHUD()) : NULL;
			//if (MyHUD)
			//{
			//	MyHUD->NotifyLowAmmo();
			//}
		}
		
		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}

	if (m_owner && m_owner->IsLocallyControlled())
	{
		// local client will notify server
		if (Role < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		// reload after firing last round
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReload();
		}

		// setup refire timer
		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &ABaseWeapon::HandleFiring, WeaponConfig.TimeBetweenShots, false);
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}

bool ABaseWeapon::ServerHandleFiring_Validate()
{
	return true;
}

void ABaseWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

	HandleFiring();

	if (bShouldUpdateAmmo)
	{
		// update ammo
		UseAmmo();

		// update firing FX on remote clients
		BurstCounter++;
	}
}

void ABaseWeapon::ReloadWeapon()
{
	int32 ClipDelta = FMath::Min(WeaponConfig.AmmoPerClip - CurrentAmmoInClip, CurrentAmmo - CurrentAmmoInClip);

	if (HasInfiniteClip())
	{
		ClipDelta = WeaponConfig.AmmoPerClip - CurrentAmmoInClip;
	}

	if (ClipDelta > 0)
	{
		CurrentAmmoInClip += ClipDelta;
	}

	if (HasInfiniteClip())
	{
		CurrentAmmo = FMath::Max(CurrentAmmoInClip, CurrentAmmo);
	}
}

void ABaseWeapon::SetWeaponState(EWeaponState::Type NewState)
{
	const EWeaponState::Type PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

void ABaseWeapon::DetermineWeaponState()
{
	EWeaponState::Type NewState = EWeaponState::Idle;

	if (bIsEquipped)
	{
		if( bPendingReload  )
		{
			if( CanReload() == false )
			{
				NewState = CurrentState;
			}
			else
			{
				NewState = EWeaponState::Reloading;
			}
		}		
		else if ( (bPendingReload == false ) && ( bWantsToFire == true ) && ( CanFire() == true ))
		{
			NewState = EWeaponState::Firing;
		}
	}
	else if (bPendingEquip)
	{
		NewState = EWeaponState::Equipping;
	}

	SetWeaponState(NewState);
}

void ABaseWeapon::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
		LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &ABaseWeapon::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

void ABaseWeapon::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}
	
	GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

UAudioComponent* ABaseWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && m_owner)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, m_owner->GetRootComponent());
	}

	return AC;
}

float ABaseWeapon::PlayWeaponAnimation(const FWeaponAnim& Animation)
{
	float Duration = 0.0f;
	if (m_owner)
	{
		UAnimMontage* UseAnim = m_owner->IsFirstPerson() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			Duration = m_owner->PlayAnimMontage(UseAnim);
		}
	}

	return Duration;
}

void ABaseWeapon::StopWeaponAnimation(const FWeaponAnim& Animation)
{
	if (m_owner)
	{
		UAnimMontage* UseAnim = m_owner->IsFirstPerson() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			m_owner->StopAnimMontage(UseAnim);
		}
	}
}

FVector ABaseWeapon::GetCameraAim() const
{
	ANorilskPlayerController* const PlayerController = Instigator ? Cast<ANorilskPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;

	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		FinalAim = Instigator->GetBaseAimRotation().Vector();		
	}

	return FinalAim;
}

FVector ABaseWeapon::GetAdjustedAim() const
{
	ANorilskPlayerController* const PlayerController = Instigator ? Cast<ANorilskPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;
	// If we have a player controller use it for the aim
	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		// Now see if we have an AI controller - we will want to get the aim from there if we do
		ABaseAIController* AIController = m_owner ? Cast<ABaseAIController>(m_owner->Controller) : NULL;
		if(AIController != NULL )
		{
			FinalAim = AIController->GetControlRotation().Vector();
		}
		else
		{			
			FinalAim = Instigator->GetBaseAimRotation().Vector();
		}
	}

	return FinalAim;
}

FVector ABaseWeapon::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	ANorilskPlayerController* PC = m_owner ? Cast<ANorilskPlayerController>(m_owner->Controller) : NULL;
	ABaseAIController* AIPC = m_owner ? Cast<ABaseAIController>(m_owner->Controller) : NULL;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		// use player's camera
		FRotator UnusedRot;
		PC->GetPlayerViewPoint(OutStartTrace, UnusedRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * ((Instigator->GetActorLocation() - OutStartTrace) | AimDir);
	}
	else if (AIPC)
	{
		OutStartTrace = GetMuzzleLocation();
	}

	return OutStartTrace;
}

FVector ABaseWeapon::GetMuzzleLocation() const
{
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketLocation(MuzzleAttachPoint);
}

FVector ABaseWeapon::GetMuzzleDirection() const
{
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketRotation(MuzzleAttachPoint).Vector();
}

FHitResult ABaseWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
{
	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);

	return Hit;
}

void ABaseWeapon::SetOwningPawn(ABaseCharacter* NewOwner)
{
	if (m_owner != NewOwner)
	{
		Instigator = NewOwner;
		m_owner = NewOwner;
		// net owner for RPC calls
		SetOwner(NewOwner);
	}	
}

//////////////////////////////////////////////////////////////////////////
// Replication & effects

void ABaseWeapon::OnRep_Owner()
{
	if (m_owner)
	{
		OnEnterInventory(m_owner);
	}
	else
	{
		OnLeaveInventory();
	}
}

void ABaseWeapon::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}

void ABaseWeapon::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload(true);
	}
	else
	{
		StopReload();
	}
}

void ABaseWeapon::SimulateWeaponFire()
{
	if (Role == ROLE_Authority && CurrentState != EWeaponState::Firing)
	{
		return;
	}

	if (MuzzleFX)
	{
		USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
		if (!bLoopedMuzzleFX || MuzzlePSC == NULL)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if( (m_owner != NULL ) && ( m_owner->IsLocallyControlled() == true ) )
			{
				AController* PlayerCon = m_owner->GetController();				
				if( PlayerCon != NULL )
				{
					Mesh1P->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Mesh1P, MuzzleAttachPoint);
					MuzzlePSC->bOwnerNoSee = false;
					MuzzlePSC->bOnlyOwnerSee = true;

					Mesh3P->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSCSecondary = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Mesh3P, MuzzleAttachPoint);
					MuzzlePSCSecondary->bOwnerNoSee = true;
					MuzzlePSCSecondary->bOnlyOwnerSee = false;				
				}				
			}
			else
			{
				MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, UseWeaponMesh, MuzzleAttachPoint);
			}
		}
	}

	if (!bLoopedFireAnim || !bPlayingFireAnim)
	{
		PlayWeaponAnimation(FireAnim);
		bPlayingFireAnim = true;
	}

	if (bLoopedFireSound)
	{
		if (FireAC == NULL)
		{
			FireAC = PlayWeaponSound(FireLoopSound);
		}
	}
	else
	{
		PlayWeaponSound(FireSound);
	}

	ANorilskPlayerController* PC = (m_owner != NULL) ? Cast<ANorilskPlayerController>(m_owner->Controller) : NULL;
	if (PC != NULL && PC->IsLocalController())
	{
		if (FireCameraShake != NULL)
		{
			PC->ClientPlayCameraShake(FireCameraShake, 1);
		}
		if (FireForceFeedback != NULL)
		{
			PC->ClientPlayForceFeedback(FireForceFeedback, false, "Weapon");
		}
	}
}

void ABaseWeapon::StopSimulatingWeaponFire()
{
	if (bLoopedMuzzleFX )
	{
		if( MuzzlePSC != NULL )
		{
			MuzzlePSC->DeactivateSystem();
			MuzzlePSC = NULL;
		}
		if( MuzzlePSCSecondary != NULL )
		{
			MuzzlePSCSecondary->DeactivateSystem();
			MuzzlePSCSecondary = NULL;
		}
	}

	if (bLoopedFireAnim && bPlayingFireAnim)
	{
		StopWeaponAnimation(FireAnim);
		bPlayingFireAnim = false;
	}

	if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = NULL;

		PlayWeaponSound(FireFinishSound);
	}
}

void ABaseWeapon::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( ABaseWeapon, m_owner );

	DOREPLIFETIME_CONDITION( ABaseWeapon, CurrentAmmo,		COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( ABaseWeapon, CurrentAmmoInClip, COND_OwnerOnly );

	DOREPLIFETIME_CONDITION( ABaseWeapon, BurstCounter,		COND_SkipOwner );
	DOREPLIFETIME_CONDITION( ABaseWeapon, bPendingReload,	COND_SkipOwner );
}

USkeletalMeshComponent* ABaseWeapon::GetWeaponMesh() const
{
	return (m_owner != NULL && m_owner->IsFirstPerson()) ? Mesh1P : Mesh3P;
}

class ABaseCharacter* ABaseWeapon::GetPawnOwner() const
{
	return m_owner;
}

bool ABaseWeapon::IsEquipped() const
{
	return bIsEquipped;
}

bool ABaseWeapon::IsAttachedToPawn() const
{
	return bIsEquipped || bPendingEquip;
}

EWeaponState::Type ABaseWeapon::GetCurrentState() const
{
	return CurrentState;
}

int32 ABaseWeapon::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

int32 ABaseWeapon::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

int32 ABaseWeapon::GetAmmoPerClip() const
{
	return WeaponConfig.AmmoPerClip;
}

int32 ABaseWeapon::GetMaxAmmo() const
{
	return WeaponConfig.MaxAmmo;
}

bool ABaseWeapon::HasInfiniteAmmo() const
{
	const ANorilskPlayerController* MyPC = (m_owner != NULL) ? Cast<const ANorilskPlayerController>(m_owner->Controller) : NULL;
	return WeaponConfig.bInfiniteAmmo || (MyPC && MyPC->HasInfiniteAmmo());
}

bool ABaseWeapon::HasInfiniteClip() const
{
	const ANorilskPlayerController* MyPC = (m_owner != NULL) ? Cast<const ANorilskPlayerController>(m_owner->Controller) : NULL;
	return WeaponConfig.bInfiniteClip || (MyPC && MyPC->HasInfiniteClip());
}

float ABaseWeapon::GetEquipStartedTime() const
{
	return EquipStartedTime;
}

float ABaseWeapon::GetEquipDuration() const
{
	return EquipDuration;
}
