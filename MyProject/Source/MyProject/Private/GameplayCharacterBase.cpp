// ============================================================================
//  AGameplayCharacterBase — базовый класс персонажа и "сердце" геймплея.
//  Здесь собрана вся игровая логика персонажа: характеристики и стамина, удар
//  киркой, ресурсы и золото, опыт/уровни/прокачка, зелья и сохранение прогресса
//  в JSON. Управляемые пешки (AThifCatcher / Sandbox) наследуются отсюда, чтобы
//  не дублировать логику; за скорость отвечает UThiefCharacterMovementComponent.
//
//  ЗАЩИТА — по этапам показа (какие функции открывать):
//   • Этап 5 (стамина/спринт): SetSprintActive(), DecreaseStamina()/IncreaseStamina(), Tick().
//   • Этап 6 (добыча руды): Attack(), TryDamageOre().
//   • Этап 7 (торговля): SellOre(), BuyStaminaPotion(), UseStaminaPotion().
//   • Этап 8 (прокачка): AddExperience(), SpendUpgradePoint().
//   • Этап 10 (сохранение): BeginPlay()/EndPlay() (Load/SaveCharacterDataToJson).
//   • Этап 11 (возврат при падении): Tick() — нижний блок.
// ============================================================================
#include "GameplayCharacterBase.h"

#include "Actors/OreStoneBase.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Save/MyProjectJsonSaveUtils.h"
#include "ThiefCharacterMovementComponent.h"
#include "ThiefPlayerController.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/ResourceCounterWidget.h"
#include "UI/StaminaBarWidget.h"

namespace
{
	constexpr float MinimumSprintStamina = 1.0f;

	enum class EMiniQuestMetric : uint8
	{
		OreNodesBroken,
		OreCollected,
		OreSold,
		PotionsBought,
		PotionsUsed,
		PlayerLevel
	};

	struct FMiniQuestDefinition
	{
		EMiniQuestMetric Metric = EMiniQuestMetric::OreNodesBroken;
		int32 Target = 1;
	};

	const TArray<FMiniQuestDefinition>& GetMiniQuestDefinitions()
	{
		static const TArray<FMiniQuestDefinition> Definitions =
		{
			{ EMiniQuestMetric::OreNodesBroken, 3 },
			{ EMiniQuestMetric::OreCollected, 20 },
			{ EMiniQuestMetric::OreSold, 15 },
			{ EMiniQuestMetric::PotionsBought, 2 },
			{ EMiniQuestMetric::PotionsUsed, 2 },
			{ EMiniQuestMetric::PlayerLevel, 3 }
		};

		return Definitions;
	}
}

AGameplayCharacterBase::AGameplayCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UThiefCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	WalkSpeed = 450.0f;
	SprintSpeed = 700.0f;
	bIsSprint = false;
	MaxStamina = 100.0f;
	Stamina = MaxStamina;
	CurrentStamina = Stamina;
	MinusStamina = 15.0f;
	PlusStamina = 26.0f;
	MovingStaminaRegen = 6.0f;
	StaminaRegenDelay = 0.85f;
	StaminaSprintRecoveryThreshold = 25.0f;
	OreDamage = 50.0f;
	AttackRange = 350.0f;
	AttackCooldown = 0.45f;
	AttackStateDuration = 5.2f;
	AttackHitNormalizedTime = 0.55f;
	BaseExperienceToNextLevel = 100;
	MaxStaminaUpgradeAmount = 20.0f;
	OreDamageUpgradeAmount = 10.0f;
	MoveSpeedUpgradeAmount = 30.0f;
	bCanAttack = true;
	bIsAttacking = false;
	bStaminaExhausted = false;
	bHasGroundedLocation = false;
	LastGroundedLocation = FVector::ZeroVector;
	InitialSpawnLocation = FVector::ZeroVector;
	FallRecoveryDropDistance = 1500.0f;
	FallRecoveryKillZ = -2000.0f;
	TimeSinceLastStaminaUse = StaminaRegenDelay;
	CollectedOreResources = 0;
	CollectedGold = 0;
	StaminaPotionCount = 0;
	TemporaryStaminaDrainMultiplier = 1.0f;
	TemporaryStaminaRegenMultiplier = 1.0f;
	PlayerLevel = 1;
	CurrentExperience = 0;
	ExperienceToNextLevel = BaseExperienceToNextLevel;
	StaminaPotionRestoreAmount = 40.0f;
	ResourceCounterWidget = nullptr;
	StaminaBarWidget = nullptr;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = WalkSpeed;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> DefaultAttackAnimationFinder(
		TEXT("/Game/MainPlayer/Animations/AM_Pickaxe_Attack.AM_Pickaxe_Attack"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultAttackMontageFinder(
		TEXT("/Game/MainPlayer/Animations/AM_Pickaxe_Attack_Montage.AM_Pickaxe_Attack_Montage"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultPickaxeMeshFinder(
		TEXT("/Game/DEMO_MiningPack/StaticMeshes/SM_DEMOPickaxe2.SM_DEMOPickaxe2"));
	if (DefaultAttackMontageFinder.Succeeded())
	{
		AttackMontage = DefaultAttackMontageFinder.Object;
		AttackStateDuration = FMath::Max(AttackMontage->GetPlayLength(), 0.0f);
	}
	else
	{
		AttackMontage = nullptr;
	}

	if (DefaultAttackAnimationFinder.Succeeded())
	{
		AttackTimingAnimation = DefaultAttackAnimationFinder.Object;
		if (!AttackMontage)
		{
			AttackStateDuration = FMath::Max(AttackTimingAnimation->GetPlayLength(), 0.0f);
		}
	}
	else
	{
		AttackTimingAnimation = nullptr;
	}

	EquippedPickaxeMesh = DefaultPickaxeMeshFinder.Succeeded() ? DefaultPickaxeMeshFinder.Object : nullptr;
	PickaxeAttachSocketName = TEXT("LeftHandSocket");
	PickaxeRelativeLocation = FVector::ZeroVector;
	PickaxeRelativeRotation = FRotator::ZeroRotator;
	PickaxeRelativeScale = FVector(1.0f, 1.0f, 1.0f);
}

void AGameplayCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(true);

	LoadCharacterDataFromJson();
	UpdateResourceCounter();
	UpdateStaminaBar();

	InitialSpawnLocation = GetActorLocation();
	LastGroundedLocation = InitialSpawnLocation;
	bHasGroundedLocation = true;

	SyncAttachedVisualInputState();
	AttachPickaxeToAttachedVisual();
	GetWorldTimerManager().SetTimerForNextTick(this, &AGameplayCharacterBase::SyncAttachedVisualInputState);
	GetWorldTimerManager().SetTimerForNextTick(this, &AGameplayCharacterBase::AttachPickaxeToAttachedVisual);
	GetWorldTimerManager().SetTimer(
		AttachedVisualInputSyncTimerHandle,
		this,
		&AGameplayCharacterBase::SyncAttachedVisualInputState,
		0.25f,
		false);
}

void AGameplayCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AttachedVisualInputSyncTimerHandle);
	SaveCharacterDataToJson();
	Super::EndPlay(EndPlayReason);
}

void AGameplayCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		return;
	}

	PlayerInputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &AGameplayCharacterBase::Attack);
	PlayerInputComponent->BindAction(TEXT("UsePotion"), IE_Pressed, this, &AGameplayCharacterBase::HandleUseStaminaPotionInput);
}

// [этапы 5, 11] Каждый кадр: считаю стамину, выставляю скорость по состоянию
// спринта/истощения и проверяю возврат на старт при падении за край мира.
void AGameplayCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Stamina <= MinimumSprintStamina)
	{
		bStaminaExhausted = true;
		if (bIsSprint)
		{
			SetSprintActive(false);
		}
	}
	else if (bStaminaExhausted && Stamina >= FMath::Min(StaminaSprintRecoveryThreshold, MaxStamina))
	{
		bStaminaExhausted = false;
	}

	const bool bHasMovementIntent = HasMovementInputIntent();
	const bool bIsActivelySprinting = bIsSprint && bHasMovementIntent;

	if (bIsActivelySprinting && Stamina > MinimumSprintStamina)
	{
		DecreaseStamina();
	}
	else
	{
		TimeSinceLastStaminaUse += DeltaTime;
		if (!bIsSprint && !FMath::IsNearlyEqual(Stamina, MaxStamina) && TimeSinceLastStaminaUse >= StaminaRegenDelay)
		{
			IncreaseStamina();
		}
	}

	if (Stamina <= MinimumSprintStamina && bIsSprint)
	{
		SetSprintActive(false);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		const float DesiredWalkSpeed = (bIsSprint && bHasMovementIntent && Stamina > MinimumSprintStamina)
			? SprintSpeed
			: WalkSpeed;
		if (!FMath::IsNearlyEqual(MovementComponent->MaxWalkSpeed, DesiredWalkSpeed))
		{
			MovementComponent->MaxWalkSpeed = DesiredWalkSpeed;
		}

		// Fall-out-of-world recovery: remember the last spot we stood on, and if we
		// fall far below it (or below the absolute kill height), snap back to the
		// spawn point instead of falling out of the world.
		const FVector CurrentLocation = GetActorLocation();
		if (MovementComponent->IsMovingOnGround())
		{
			LastGroundedLocation = CurrentLocation;
			bHasGroundedLocation = true;
		}

		const bool bFellBelowGround = bHasGroundedLocation
			&& !MovementComponent->IsMovingOnGround()
			&& CurrentLocation.Z < LastGroundedLocation.Z - FallRecoveryDropDistance;
		const bool bFellBelowKillZ = CurrentLocation.Z < FallRecoveryKillZ;
		if (bFellBelowGround || bFellBelowKillZ)
		{
			MovementComponent->StopMovementImmediately();
			SetActorLocation(InitialSpawnLocation + FVector(0.0f, 0.0f, 80.0f), false, nullptr, ETeleportType::TeleportPhysics);
			MovementComponent->Velocity = FVector::ZeroVector;
			LastGroundedLocation = InitialSpawnLocation;
		}
	}
}

// [этап 6] Удар киркой: проверки (не в меню, не в прыжке), блокирую движение,
// запускаю анимацию и через таймер бью по руде в нужный момент замаха.
void AGameplayCharacterBase::Attack()
{
	if (bIsAttacking || !bCanAttack)
	{
		return;
	}

	if (const AThiefPlayerController* ThiefController = Cast<AThiefPlayerController>(GetController()))
	{
		if (ThiefController->IsAnyModalOpen())
		{
			return;
		}
	}

	if (const UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (MovementComponent->IsFalling())
		{
			return;
		}
	}

	bCanAttack = false;
	bIsAttacking = true;
	SetSprintActive(false);
	StopJumping();
	if (AController* CurrentController = GetController())
	{
		CurrentController->SetIgnoreMoveInput(true);
	}

	const float EffectiveAttackDuration = AttackMontage
		? FMath::Max(AttackMontage->GetPlayLength(), 0.0f)
		: AttackTimingAnimation
		? FMath::Max(AttackTimingAnimation->GetPlayLength(), 0.0f)
		: AttackStateDuration;
	const float EffectiveAttackCooldown = FMath::Max(AttackCooldown, EffectiveAttackDuration);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	SyncAttachedVisualInputState();
	const bool bTriggeredAttachedVisualAttack = TriggerAttachedVisualAttack();
	if (AttackMontage && !bTriggeredAttachedVisualAttack)
	{
		PlayAnimMontage(AttackMontage, 1.0f, NAME_None);
	}

	GetWorldTimerManager().ClearTimer(AttackStateTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackHitTimerHandle);

	if (EffectiveAttackDuration <= 0.0f)
	{
		TriggerAttackHit();
		FinishAttackState();
	}
	else
	{
		const float EffectiveHitDelay = FMath::Clamp(AttackHitNormalizedTime, 0.0f, 1.0f) * EffectiveAttackDuration;
		if (EffectiveHitDelay <= 0.0f)
		{
			TriggerAttackHit();
		}
		else
		{
			GetWorldTimerManager().SetTimer(
				AttackHitTimerHandle,
				this,
				&AGameplayCharacterBase::TriggerAttackHit,
				FMath::Min(EffectiveHitDelay, EffectiveAttackDuration),
				false);
		}

		GetWorldTimerManager().SetTimer(
			AttackStateTimerHandle,
			this,
			&AGameplayCharacterBase::FinishAttackState,
			EffectiveAttackDuration,
			false);
	}

	GetWorldTimerManager().ClearTimer(AttackCooldownHandle);
	if (EffectiveAttackCooldown <= 0.0f)
	{
		bCanAttack = true;
		return;
	}

	GetWorldTimerManager().SetTimer(
		AttackCooldownHandle,
		[this]()
		{
			bCanAttack = true;
		},
		EffectiveAttackCooldown,
		false);
}

// [этап 5] Включаю/выключаю спринт. Если стамина истощена или почти на нуле —
// спринт не включится (защита от бега на нуле). Меняю максимальную скорость.
void AGameplayCharacterBase::SetSprintActive(const bool bShouldSprint)
{
	if (bShouldSprint && (bStaminaExhausted || Stamina <= MinimumSprintStamina))
	{
		bIsSprint = false;
	}
	else
	{
		bIsSprint = bShouldSprint;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = bIsSprint ? SprintSpeed : WalkSpeed;
	}
}

// [этап 8] Начисляю опыт. Пока опыта хватает на уровень — повышаю уровень, даю
// очко улучшения и увеличиваю порог опыта до следующего уровня.
void AGameplayCharacterBase::AddExperience(const int32 ExperienceAmount)
{
	if (ExperienceAmount <= 0)
	{
		return;
	}

	CurrentExperience += ExperienceAmount;
	while (CurrentExperience >= ExperienceToNextLevel)
	{
		CurrentExperience -= ExperienceToNextLevel;
		PlayerLevel += 1;
		AvailableUpgradePoints += 1;
		ExperienceToNextLevel = FMath::Max(BaseExperienceToNextLevel, ExperienceToNextLevel + 50);
	}

	SaveCharacterDataToJson();
}

void AGameplayCharacterBase::AddGold(const int32 GoldAmount)
{
	if (GoldAmount <= 0)
	{
		return;
	}

	CollectedGold += GoldAmount;
	TotalGoldEarned += GoldAmount;
	UpdateResourceCounter();
	SaveCharacterDataToJson();
}

void AGameplayCharacterBase::ResetCharacterProgress()
{
	const AGameplayCharacterBase* DefaultCharacter = GetClass()
		? Cast<AGameplayCharacterBase>(GetClass()->GetDefaultObject())
		: nullptr;

	MaxStamina = DefaultCharacter ? DefaultCharacter->MaxStamina : 100.0f;
	Stamina = MaxStamina;
	CurrentStamina = Stamina;
	CollectedOreResources = 0;
	CollectedGold = 0;
	StaminaPotionCount = 0;
	PlayerLevel = 1;
	CurrentExperience = 0;
	ExperienceToNextLevel = DefaultCharacter ? DefaultCharacter->BaseExperienceToNextLevel : BaseExperienceToNextLevel;
	AvailableUpgradePoints = 0;
	OreDamage = FMath::Max(DefaultCharacter ? DefaultCharacter->OreDamage : 50.0f, 50.0f);
	WalkSpeed = DefaultCharacter ? DefaultCharacter->WalkSpeed : 450.0f;
	SprintSpeed = DefaultCharacter ? DefaultCharacter->SprintSpeed : 700.0f;
	StaminaPotionRestoreAmount = DefaultCharacter ? DefaultCharacter->StaminaPotionRestoreAmount : 40.0f;
	TotalOreCollected = 0;
	TotalOreSold = 0;
	TotalOreNodesBroken = 0;
	TotalGoldEarned = 0;
	TotalPotionsBought = 0;
	TotalPotionsUsed = 0;
	TemporaryStaminaDrainMultiplier = 1.0f;
	TemporaryStaminaRegenMultiplier = 1.0f;
	bIsSprint = false;
	bStaminaExhausted = false;
	TimeSinceLastStaminaUse = StaminaRegenDelay;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = WalkSpeed;
	}

	UpdateResourceCounter();
	UpdateStaminaBar();
	SaveCharacterDataToJson();
}

// [этап 7] Продажа руды: проверяю, что руды хватает, уменьшаю её и добавляю
// золото. Возвращаю false, если продать нельзя.
bool AGameplayCharacterBase::SellOre(const int32 OreAmount, const int32 GoldPerOre)
{
	if (OreAmount <= 0 || GoldPerOre <= 0 || CollectedOreResources < OreAmount)
	{
		return false;
	}

	CollectedOreResources -= OreAmount;
	const int32 GoldEarned = OreAmount * GoldPerOre;
	CollectedGold += GoldEarned;
	TotalGoldEarned += GoldEarned;
	TotalOreSold += OreAmount;
	UpdateResourceCounter();
	SaveCharacterDataToJson();
	return true;
}

// [этап 7] Покупка зелья стамины: проверяю, что хватает золота, списываю его и
// добавляю зелье в инвентарь.
bool AGameplayCharacterBase::BuyStaminaPotion(const int32 GoldCost, const float RestoreAmount)
{
	if (GoldCost <= 0 || RestoreAmount <= 0.0f || CollectedGold < GoldCost)
	{
		return false;
	}

	CollectedGold -= GoldCost;
	StaminaPotionCount += 1;
	StaminaPotionRestoreAmount = RestoreAmount;
	TotalPotionsBought += 1;
	SaveCharacterDataToJson();
	return true;
}

// [этап 7] Использование зелья (по клавише): если есть зелье и стамина не полная
// и не идёт атака/меню — трачу зелье и восстанавливаю стамину.
bool AGameplayCharacterBase::UseStaminaPotion()
{
	if (StaminaPotionCount <= 0 || FMath::IsNearlyEqual(Stamina, MaxStamina))
	{
		return false;
	}

	if (bIsAttacking)
	{
		return false;
	}

	if (const AThiefPlayerController* ThiefController = Cast<AThiefPlayerController>(GetController()))
	{
		if (ThiefController->IsAnyModalOpen())
		{
			return false;
		}
	}

	StaminaPotionCount -= 1;
	Stamina = FMath::Clamp(Stamina + StaminaPotionRestoreAmount, 0.0f, MaxStamina);
	CurrentStamina = Stamina;
	TotalPotionsUsed += 1;
	UpdateStaminaBar();
	SaveCharacterDataToJson();
	return true;
}

// [этап 8] Трата очка улучшения: повышаю выбранную характеристику — макс. стамину,
// урон по руде или скорость — и списываю одно очко.
bool AGameplayCharacterBase::SpendUpgradePoint(const EPlayerUpgradeType UpgradeType)
{
	if (AvailableUpgradePoints <= 0)
	{
		return false;
	}

	switch (UpgradeType)
	{
	case EPlayerUpgradeType::MaxStamina:
		MaxStamina += MaxStaminaUpgradeAmount;
		Stamina = MaxStamina;
		CurrentStamina = Stamina;
		UpdateStaminaBar();
		break;
	case EPlayerUpgradeType::OreDamage:
		OreDamage += FMath::Max(OreDamageUpgradeAmount, 10.0f);
		break;
	case EPlayerUpgradeType::MoveSpeed:
		WalkSpeed += MoveSpeedUpgradeAmount;
		SprintSpeed += MoveSpeedUpgradeAmount;
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->MaxWalkSpeed = bIsSprint ? SprintSpeed : WalkSpeed;
		}
		break;
	default:
		return false;
	}

	AvailableUpgradePoints -= 1;
	SaveCharacterDataToJson();
	return true;
}

int32 AGameplayCharacterBase::GetCollectedGold() const
{
	return CollectedGold;
}

int32 AGameplayCharacterBase::GetPlayerLevel() const
{
	return PlayerLevel;
}

int32 AGameplayCharacterBase::GetCurrentExperienceAmount() const
{
	return CurrentExperience;
}

int32 AGameplayCharacterBase::GetExperienceToNextLevelAmount() const
{
	return ExperienceToNextLevel;
}

int32 AGameplayCharacterBase::GetAvailableUpgradePoints() const
{
	return AvailableUpgradePoints;
}

float AGameplayCharacterBase::GetOreDamageAmount() const
{
	return OreDamage;
}

int32 AGameplayCharacterBase::GetMiniQuestCount() const
{
	return GetMiniQuestDefinitions().Num();
}

int32 AGameplayCharacterBase::GetCompletedMiniQuestCount() const
{
	int32 CompletedCount = 0;
	const TArray<FMiniQuestDefinition>& QuestDefinitions = GetMiniQuestDefinitions();
	for (int32 QuestIndex = 0; QuestIndex < QuestDefinitions.Num(); ++QuestIndex)
	{
		if (GetMiniQuestCurrentValue(QuestIndex) >= QuestDefinitions[QuestIndex].Target)
		{
			CompletedCount += 1;
		}
	}

	return CompletedCount;
}

int32 AGameplayCharacterBase::GetStaminaPotionCount() const
{
	return StaminaPotionCount;
}

int32 AGameplayCharacterBase::GetTotalOreCollected() const
{
	return TotalOreCollected;
}

int32 AGameplayCharacterBase::GetTotalOreSold() const
{
	return TotalOreSold;
}

int32 AGameplayCharacterBase::GetTotalOreNodesBroken() const
{
	return TotalOreNodesBroken;
}

int32 AGameplayCharacterBase::GetTotalGoldEarned() const
{
	return TotalGoldEarned;
}

int32 AGameplayCharacterBase::GetTotalPotionsBought() const
{
	return TotalPotionsBought;
}

int32 AGameplayCharacterBase::GetTotalPotionsUsed() const
{
	return TotalPotionsUsed;
}

bool AGameplayCharacterBase::GetMiniQuestProgress(int32 QuestIndex, int32& OutCurrentProgress, int32& OutTargetProgress, bool& bOutCompleted) const
{
	const TArray<FMiniQuestDefinition>& QuestDefinitions = GetMiniQuestDefinitions();
	if (!QuestDefinitions.IsValidIndex(QuestIndex))
	{
		OutCurrentProgress = 0;
		OutTargetProgress = 0;
		bOutCompleted = false;
		return false;
	}

	OutCurrentProgress = GetMiniQuestCurrentValue(QuestIndex);
	OutTargetProgress = QuestDefinitions[QuestIndex].Target;
	bOutCompleted = OutCurrentProgress >= OutTargetProgress;
	return true;
}

void AGameplayCharacterBase::SetTemporaryStaminaModifiers(const float DrainMultiplier, const float RegenMultiplier)
{
	TemporaryStaminaDrainMultiplier = FMath::Max(0.1f, DrainMultiplier);
	TemporaryStaminaRegenMultiplier = FMath::Max(0.0f, RegenMultiplier);
}

// [этап 5] Расход стамины при беге. Когда стамина падает в ноль — выставляю флаг
// истощения и выключаю спринт.
void AGameplayCharacterBase::DecreaseStamina()
{
	const UWorld* World = GetWorld();
	const float DeltaTime = World ? World->GetDeltaSeconds() : 0.0f;
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	TimeSinceLastStaminaUse = 0.0f;
	CurrentStamina = FMath::Clamp(Stamina - (MinusStamina * TemporaryStaminaDrainMultiplier * DeltaTime), 0.0f, MaxStamina);
	Stamina = CurrentStamina;
	if (Stamina <= MinimumSprintStamina)
	{
		bStaminaExhausted = true;
		if (bIsSprint)
		{
			SetSprintActive(false);
		}
	}
	UpdateStaminaBar();
}

// [этап 5] Восстановление стамины с задержкой после бега; в движении медленнее,
// в покое быстрее.
void AGameplayCharacterBase::IncreaseStamina()
{
	const UWorld* World = GetWorld();
	const float DeltaTime = World ? World->GetDeltaSeconds() : 0.0f;
	const bool bIsActivelySprinting = bIsSprint && HasMovementInputIntent();
	if (DeltaTime <= 0.0f || bIsActivelySprinting || TimeSinceLastStaminaUse < StaminaRegenDelay)
	{
		return;
	}

	const float RegenPerSecond = (HasMovementInputIntent() ? MovingStaminaRegen : PlusStamina) * TemporaryStaminaRegenMultiplier;
	if (RegenPerSecond <= 0.0f)
	{
		return;
	}

	CurrentStamina = FMath::Clamp(Stamina + (RegenPerSecond * DeltaTime), 0.0f, MaxStamina);
	Stamina = CurrentStamina;
	UpdateStaminaBar();
}

bool AGameplayCharacterBase::HasMovementInputIntent() const
{
	if (const UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (MovementComponent->GetCurrentAcceleration().SizeSquared2D() > FMath::Square(1.0f))
		{
			return true;
		}
	}

	return GetPendingMovementInputVector().SizeSquared2D() > FMath::Square(0.01f);
}

void AGameplayCharacterBase::AddOreResources(const int32 ResourceAmount)
{
	if (ResourceAmount <= 0)
	{
		return;
	}

	CollectedOreResources += ResourceAmount;
	TotalOreCollected += ResourceAmount;
	UpdateResourceCounter();
	SaveCharacterDataToJson();
}

void AGameplayCharacterBase::HandleUseStaminaPotionInput()
{
	UseStaminaPotion();
}

void AGameplayCharacterBase::TriggerAttackHit()
{
	GetWorldTimerManager().ClearTimer(AttackHitTimerHandle);
	TryDamageOre();
}

void AGameplayCharacterBase::FinishAttackState()
{
	GetWorldTimerManager().ClearTimer(AttackHitTimerHandle);
	bIsAttacking = false;
	if (AController* CurrentController = GetController())
	{
		CurrentController->SetIgnoreMoveInput(false);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		MovementComponent->MaxWalkSpeed = bIsSprint ? SprintSpeed : WalkSpeed;
	}
}

void AGameplayCharacterBase::UpdateResourceCounter() const
{
	if (!ResourceCounterWidget)
	{
		return;
	}

	ResourceCounterWidget->SetResourceAmount(CollectedOreResources);
}

void AGameplayCharacterBase::UpdateStaminaBar() const
{
	if (!StaminaBarWidget)
	{
		return;
	}

	StaminaBarWidget->SetStaminaValues(Stamina, MaxStamina);
}

// [этап 6] Поиск руды и нанесение урона: сначала "выстрел" сферой по направлению
// взгляда, если не попал — выбираю ближайшую руду перед персонажем в радиусе
// атаки и бью её. При разрушении начисляю руду и опыт.
void AGameplayCharacterBase::TryDamageOre()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	auto ResolveOreFromHit = [](const FHitResult& HitResult) -> AOreStoneBase*
	{
		if (AOreStoneBase* OreStone = Cast<AOreStoneBase>(HitResult.GetActor()))
		{
			return OreStone;
		}

		if (UPrimitiveComponent* HitComponent = HitResult.GetComponent())
		{
			if (AOreStoneBase* OreStoneOwner = Cast<AOreStoneBase>(HitComponent->GetOwner()))
			{
				return OreStoneOwner;
			}
		}

		return nullptr;
	};

	auto ApplyOreDamage = [this](AOreStoneBase* OreStone)
	{
		if (!OreStone || !OreStone->IsOreAvailable())
		{
			return false;
		}

		const bool bWillBreak = OreStone->GetCurrentHealth() <= OreDamage;
		OreStone->ApplyDamageToOre(OreDamage);
		if (bWillBreak && !OreStone->IsOreAvailable())
		{
			TotalOreNodesBroken += 1;
			AddOreResources(OreStone->GetOreResourceReward());
			AddExperience(OreStone->GetOreExperienceReward());
		}

		return true;
	};

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		FVector TraceStart;
		FRotator TraceRotation;
		PlayerController->GetPlayerViewPoint(TraceStart, TraceRotation);

		const FVector TraceEnd = TraceStart + (TraceRotation.Vector() * AttackRange);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OreAttackTrace), false, this);
		QueryParams.AddIgnoredActor(this);
		const FCollisionShape AttackShape = FCollisionShape::MakeSphere(65.0f);

		TArray<FHitResult> HitResults;
		if (World->SweepMultiByChannel(HitResults, TraceStart, TraceEnd, FQuat::Identity, ECC_Visibility, AttackShape, QueryParams))
		{
			for (const FHitResult& HitResult : HitResults)
			{
				if (ApplyOreDamage(ResolveOreFromHit(HitResult)))
				{
					return;
				}
			}
		}
	}

	const FVector CharacterLocation = GetActorLocation();
	FVector AttackDirection = GetActorForwardVector();
	if (AController* CurrentController = GetController())
	{
		FRotator ControlRotation = CurrentController->GetControlRotation();
		ControlRotation.Pitch = 0.0f;
		ControlRotation.Roll = 0.0f;
		AttackDirection = ControlRotation.Vector().GetSafeNormal();
	}

	TArray<AActor*> OreActors;
	UGameplayStatics::GetAllActorsOfClass(World, AOreStoneBase::StaticClass(), OreActors);

	AOreStoneBase* BestOreTarget = nullptr;
	float BestScore = -FLT_MAX;
	const float CloseRangeOverride = FMath::Min(AttackRange, 220.0f);
	for (AActor* OreActor : OreActors)
	{
		AOreStoneBase* OreStone = Cast<AOreStoneBase>(OreActor);
		if (!OreStone || !OreStone->IsOreAvailable())
		{
			continue;
		}

		const FVector ToOre = OreStone->GetActorLocation() - CharacterLocation;
		const float DistanceToOre = ToOre.Size();
		if (DistanceToOre > AttackRange || DistanceToOre <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector ToOreDirection = ToOre.GetSafeNormal();
		const float FacingDot = FVector::DotProduct(AttackDirection, ToOreDirection);
		const bool bIsVeryCloseOre = DistanceToOre <= CloseRangeOverride;
		if (!bIsVeryCloseOre && FacingDot < 0.15f)
		{
			continue;
		}

		const float Score = bIsVeryCloseOre
			? 10.0f - (DistanceToOre / CloseRangeOverride)
			: (FacingDot * 2.0f) - (DistanceToOre / AttackRange);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestOreTarget = OreStone;
		}
	}

	ApplyOreDamage(BestOreTarget);
}

void AGameplayCharacterBase::SyncAttachedVisualInputState()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors, true, true);
	for (AActor* AttachedActor : AttachedActors)
	{
		if (!AttachedActor || AttachedActor == this)
		{
			continue;
		}

		AttachedActor->DisableInput(PlayerController);
	}

	AttachPickaxeToAttachedVisual();
}

bool AGameplayCharacterBase::TriggerAttachedVisualAttack()
{
	static const FName PlayPickaxeAttackEventName(TEXT("PlayPickaxeAttack"));

	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors, true, true);
	bool bTriggeredVisualAttack = false;
	for (AActor* AttachedActor : AttachedActors)
	{
		if (!AttachedActor || AttachedActor == this)
		{
			continue;
		}

		if (UFunction* PlayPickaxeAttackFunction = AttachedActor->FindFunction(PlayPickaxeAttackEventName))
		{
			AttachedActor->ProcessEvent(PlayPickaxeAttackFunction, nullptr);
			bTriggeredVisualAttack = true;
		}
	}

	return bTriggeredVisualAttack;
}

void AGameplayCharacterBase::AttachPickaxeToAttachedVisual()
{
	if (!EquippedPickaxeMesh)
	{
		return;
	}

	static const FName PickaxeComponentTag(TEXT("EquippedPickaxe"));
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors, true, true);

	for (AActor* AttachedActor : AttachedActors)
	{
		if (!AttachedActor || AttachedActor == this)
		{
			continue;
		}

		USkeletalMeshComponent* TargetMeshComponent = nullptr;
		TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshComponents(AttachedActor);
		for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
		{
			if (!SkeletalMeshComponent)
			{
				continue;
			}

			if (SkeletalMeshComponent->GetName().Contains(TEXT("Manny")))
			{
				TargetMeshComponent = SkeletalMeshComponent;
				break;
			}

			if (!TargetMeshComponent)
			{
				TargetMeshComponent = SkeletalMeshComponent;
			}
		}

		if (!TargetMeshComponent)
		{
			continue;
		}

		FName ResolvedAttachSocket = PickaxeAttachSocketName;
		if (!TargetMeshComponent->DoesSocketExist(ResolvedAttachSocket))
		{
			static const FName CandidateSockets[] =
			{
				TEXT("LeftHandThumb3"),
				TEXT("LeftHand"),
				TEXT("leftHand"),
				TEXT("RightHand"),
				TEXT("rightHand"),
				TEXT("RightHandSocket"),
				TEXT("weapon_r"),
				TEXT("Weapon_R"),
				TEXT("hand_r"),
				TEXT("Hand_R"),
				TEXT("ik_hand_r"),
				TEXT("VB hand_r")
			};

			for (const FName CandidateSocket : CandidateSockets)
			{
				if (TargetMeshComponent->DoesSocketExist(CandidateSocket))
				{
					ResolvedAttachSocket = CandidateSocket;
					break;
				}
			}
		}

		UStaticMeshComponent* PickaxeComponent = nullptr;
		TInlineComponentArray<UStaticMeshComponent*> StaticMeshComponents(AttachedActor);
		for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
		{
			if (StaticMeshComponent && StaticMeshComponent->ComponentHasTag(PickaxeComponentTag))
			{
				PickaxeComponent = StaticMeshComponent;
				break;
			}
		}

		if (!PickaxeComponent)
		{
			PickaxeComponent = NewObject<UStaticMeshComponent>(AttachedActor, TEXT("EquippedPickaxe"));
			if (!PickaxeComponent)
			{
				continue;
			}

			PickaxeComponent->ComponentTags.Add(PickaxeComponentTag);
			PickaxeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			PickaxeComponent->SetGenerateOverlapEvents(false);
			PickaxeComponent->SetCanEverAffectNavigation(false);
			PickaxeComponent->SetMobility(EComponentMobility::Movable);
			AttachedActor->AddInstanceComponent(PickaxeComponent);
			PickaxeComponent->RegisterComponent();
		}

		PickaxeComponent->SetStaticMesh(EquippedPickaxeMesh);
		PickaxeComponent->SetVisibility(true, true);
		PickaxeComponent->AttachToComponent(
			TargetMeshComponent,
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			ResolvedAttachSocket);
		PickaxeComponent->SetRelativeLocation(PickaxeRelativeLocation);
		PickaxeComponent->SetRelativeRotation(PickaxeRelativeRotation);
		PickaxeComponent->SetRelativeScale3D(PickaxeRelativeScale);
	}
}

void AGameplayCharacterBase::LoadCharacterDataFromJson()
{
	FMyProjectCharacterSaveData SaveData;
	if (!FMyProjectJsonSaveUtils::LoadCharacterData(SaveData))
	{
		SaveCharacterDataToJson();
		return;
	}

	MaxStamina = FMath::Max(1.0f, SaveData.MaxStamina);
	Stamina = FMath::Clamp(SaveData.CurrentStamina, 0.0f, MaxStamina);
	CurrentStamina = Stamina;
	CollectedOreResources = FMath::Max(0, SaveData.CollectedOreResources);
	CollectedGold = FMath::Max(0, SaveData.CollectedGold);
	StaminaPotionCount = FMath::Max(0, SaveData.StaminaPotionCount);
	PlayerLevel = FMath::Max(1, SaveData.PlayerLevel);
	CurrentExperience = FMath::Max(0, SaveData.CurrentExperience);
	ExperienceToNextLevel = FMath::Max(BaseExperienceToNextLevel, SaveData.ExperienceToNextLevel);
	AvailableUpgradePoints = FMath::Max(0, SaveData.AvailableUpgradePoints);
	OreDamage = FMath::Max(50.0f, SaveData.OreDamage);
	WalkSpeed = FMath::Max(1.0f, SaveData.WalkSpeed);
	SprintSpeed = FMath::Max(WalkSpeed, SaveData.SprintSpeed);
	TotalOreCollected = FMath::Max(0, SaveData.TotalOreCollected);
	TotalOreSold = FMath::Max(0, SaveData.TotalOreSold);
	TotalOreNodesBroken = FMath::Max(0, SaveData.TotalOreNodesBroken);
	TotalGoldEarned = FMath::Max(0, SaveData.TotalGoldEarned);
	TotalPotionsBought = FMath::Max(0, SaveData.TotalPotionsBought);
	TotalPotionsUsed = FMath::Max(0, SaveData.TotalPotionsUsed);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = bIsSprint ? SprintSpeed : WalkSpeed;
	}
}

void AGameplayCharacterBase::SaveCharacterDataToJson() const
{
	FMyProjectCharacterSaveData SaveData;
	SaveData.MaxStamina = MaxStamina;
	SaveData.CurrentStamina = FMath::Clamp(Stamina, 0.0f, MaxStamina);
	SaveData.CollectedOreResources = CollectedOreResources;
	SaveData.CollectedGold = CollectedGold;
	SaveData.StaminaPotionCount = StaminaPotionCount;
	SaveData.PlayerLevel = PlayerLevel;
	SaveData.CurrentExperience = CurrentExperience;
	SaveData.ExperienceToNextLevel = ExperienceToNextLevel;
	SaveData.AvailableUpgradePoints = AvailableUpgradePoints;
	SaveData.OreDamage = OreDamage;
	SaveData.WalkSpeed = WalkSpeed;
	SaveData.SprintSpeed = SprintSpeed;
	SaveData.TotalOreCollected = TotalOreCollected;
	SaveData.TotalOreSold = TotalOreSold;
	SaveData.TotalOreNodesBroken = TotalOreNodesBroken;
	SaveData.TotalGoldEarned = TotalGoldEarned;
	SaveData.TotalPotionsBought = TotalPotionsBought;
	SaveData.TotalPotionsUsed = TotalPotionsUsed;
	FMyProjectJsonSaveUtils::SaveCharacterData(SaveData);
}

int32 AGameplayCharacterBase::GetMiniQuestCurrentValue(int32 QuestIndex) const
{
	const TArray<FMiniQuestDefinition>& QuestDefinitions = GetMiniQuestDefinitions();
	if (!QuestDefinitions.IsValidIndex(QuestIndex))
	{
		return 0;
	}

	switch (QuestDefinitions[QuestIndex].Metric)
	{
	case EMiniQuestMetric::OreNodesBroken:
		return TotalOreNodesBroken;
	case EMiniQuestMetric::OreCollected:
		return TotalOreCollected;
	case EMiniQuestMetric::OreSold:
		return TotalOreSold;
	case EMiniQuestMetric::PotionsBought:
		return TotalPotionsBought;
	case EMiniQuestMetric::PotionsUsed:
		return TotalPotionsUsed;
	case EMiniQuestMetric::PlayerLevel:
		return PlayerLevel;
	default:
		return 0;
	}
}
