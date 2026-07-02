// ============================================================================
//  UWorkOrderSubsystem — система рабочих заказов и смен.
//  Это Subsystem движка: заказы, репутацию и прогресс смены я храню отдельно от
//  объектов на карте, чтобы данные жили независимо от уровня. Игрок выбирает
//  заказ в лобби, запускает смену, выполняет цель за отведённое время и получает
//  награду; всё сохраняется в WorkOrders.json.
//
//  ЗАЩИТА — по этапам показа:
//   • Этап 9 (заказы/смены): CycleSelectedWorkOrder() — выбор заказа стрелками;
//     PrepareSelectedOrderForLaunch()/TryStartPendingShift() — запуск смены;
//     UpdateTrackedShift() — прогресс и таймер; CompleteTrackedShift()/
//     AbandonTrackedShift() — сдача с наградой / отмена;
//     GetReputation()/GetCompletedShiftCount() — статистика.
// ============================================================================
#include "Systems/WorkOrderSubsystem.h"

#include "GameplayCharacterBase.h"
#include "Save/MyProjectJsonSaveUtils.h"

namespace
{
	TArray<FWorkOrderDefinition> BuildWorkOrderDefinitions()
	{
		return
		{
			{
				0,
				EWorkOrderMetric::OreCollected,
				18,
				EWorkOrderMetric::OreNodesBroken,
				3,
				EWorkOrderModifier::IncreasedDemand,
				420.0f,
				140,
				90,
				1,
				45,
				30,
				1,
				0,
				TEXT("Starter Claim"),
				TEXT("Стартовый участок"),
				TEXT("Bring back a first baseline haul before the shift whistle ends."),
				TEXT("Добудь первую стабильную партию руды до окончания рабочей смены."),
				TEXT("Collect 18 ore during the shift."),
				TEXT("Собери 18 руды за смену."),
				TEXT("Bonus: break 3 ore nodes before turn-in."),
				TEXT("Бонус: разбей 3 залежи до сдачи заказа.")
			},
			{
				1,
				EWorkOrderMetric::OreNodesBroken,
				6,
				EWorkOrderMetric::PotionsUsed,
				1,
				EWorkOrderModifier::HeavyVeins,
				420.0f,
				180,
				115,
				1,
				65,
				35,
				1,
				1,
				TEXT("Vein Clearance"),
				TEXT("Зачистка жил"),
				TEXT("Clear a rough vein cluster and leave the field ready for the next miner."),
				TEXT("Зачисти плотный участок жил и подготовь точку для следующей смены."),
				TEXT("Break 6 ore nodes during the shift."),
				TEXT("Разбей 6 залежей руды за смену."),
				TEXT("Bonus: use 1 stamina potion during the shift."),
				TEXT("Бонус: используй 1 зелье стамины во время смены.")
			},
			{
				2,
				EWorkOrderMetric::OreSold,
				20,
				EWorkOrderMetric::OreCollected,
				30,
				EWorkOrderModifier::ReputationRush,
				480.0f,
				260,
				150,
				2,
				80,
				45,
				2,
				3,
				TEXT("Merchant Supply"),
				TEXT("Поставка торговцу"),
				TEXT("Move a full trader order from the field into paid stock before time runs out."),
				TEXT("Закрой крупный заказ торговца и успей сдать сырьё до конца окна поставки."),
				TEXT("Sell 20 ore to the trader during the shift."),
				TEXT("Продай торговцу 20 руды за смену."),
				TEXT("Bonus: collect 30 ore during the same shift."),
				TEXT("Бонус: собери 30 руды в этой же смене.")
			},
			{
				3,
				EWorkOrderMetric::GoldEarned,
				220,
				EWorkOrderMetric::FinishBeforeTime,
				90,
				EWorkOrderModifier::TightSchedule,
				420.0f,
				320,
				190,
				2,
				120,
				60,
				1,
				5,
				TEXT("Gold Window"),
				TEXT("Золотое окно"),
				TEXT("Earn a fast pile of gold while the foreman keeps the clock tight."),
				TEXT("Заработай золото в сжатое окно, пока мастер давит по времени."),
				TEXT("Earn 220 gold during the shift."),
				TEXT("Заработай 220 золота за смену."),
				TEXT("Bonus: turn in with at least 90 seconds left."),
				TEXT("Бонус: сдай заказ, когда останется минимум 90 секунд.")
			},
			{
				4,
				EWorkOrderMetric::PotionsBought,
				2,
				EWorkOrderMetric::PotionsUsed,
				2,
				EWorkOrderModifier::ExhaustingShift,
				540.0f,
				360,
				220,
				3,
				130,
				70,
				1,
				7,
				TEXT("Supply Belt"),
				TEXT("Пояс припасов"),
				TEXT("Prepare and consume enough stamina supplies to survive a heavier work rhythm."),
				TEXT("Подготовь и используй припасы стамины для тяжёлого темпа смены."),
				TEXT("Buy 2 stamina potions during the shift."),
				TEXT("Купи 2 зелья стамины за смену."),
				TEXT("Bonus: use 2 stamina potions during the shift."),
				TEXT("Бонус: используй 2 зелья стамины за смену.")
			},
			{
				5,
				EWorkOrderMetric::PlayerLevel,
				4,
				EWorkOrderMetric::OreSold,
				25,
				EWorkOrderModifier::IncreasedDemand,
				600.0f,
				460,
				280,
				4,
				160,
				95,
				2,
				10,
				TEXT("Promotion Shift"),
				TEXT("Смена на повышение"),
				TEXT("Prove that the miner is ready for higher-value contracts."),
				TEXT("Докажи, что шахтёр готов к более дорогим заказам."),
				TEXT("Reach player level 4."),
				TEXT("Достигни 4 уровня персонажа."),
				TEXT("Bonus: sell 25 ore before the final turn-in."),
				TEXT("Бонус: продай 25 руды до финальной сдачи.")
			}
		};
	}

	float GetModifierGoldMultiplier(const EWorkOrderModifier Modifier)
	{
		switch (Modifier)
		{
		case EWorkOrderModifier::IncreasedDemand:
			return 1.25f;
		case EWorkOrderModifier::HeavyVeins:
			return 1.20f;
		case EWorkOrderModifier::TightSchedule:
			return 1.35f;
		default:
			return 1.0f;
		}
	}

	float GetModifierExperienceMultiplier(const EWorkOrderModifier Modifier)
	{
		return Modifier == EWorkOrderModifier::TightSchedule ? 1.15f : 1.0f;
	}

	float GetModifierReputationMultiplier(const EWorkOrderModifier Modifier)
	{
		return Modifier == EWorkOrderModifier::ReputationRush ? 1.50f : 1.0f;
	}
}

void UWorkOrderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WorkOrders = BuildWorkOrderDefinitions();
	CompletedOrderCounts.SetNumZeroed(WorkOrders.Num());
	LoadPersistentData();
}

void UWorkOrderSubsystem::Deinitialize()
{
	SavePersistentData();
	Super::Deinitialize();
}

int32 UWorkOrderSubsystem::GetWorkOrderCount() const
{
	return WorkOrders.Num();
}

int32 UWorkOrderSubsystem::GetUnlockedWorkOrderCount() const
{
	int32 UnlockedCount = 0;
	for (int32 OrderIndex = 0; OrderIndex < WorkOrders.Num(); ++OrderIndex)
	{
		if (IsOrderUnlocked(OrderIndex))
		{
			++UnlockedCount;
		}
	}

	return UnlockedCount;
}

const FWorkOrderDefinition* UWorkOrderSubsystem::GetWorkOrderDefinition(const int32 OrderIndex) const
{
	return WorkOrders.IsValidIndex(OrderIndex) ? &WorkOrders[OrderIndex] : nullptr;
}

bool UWorkOrderSubsystem::IsOrderUnlocked(const int32 OrderIndex) const
{
	const FWorkOrderDefinition* Definition = GetWorkOrderDefinition(OrderIndex);
	return Definition && CompletedShiftCount >= Definition->RequiredCompletedShifts;
}

bool UWorkOrderSubsystem::SelectWorkOrder(const int32 OrderIndex)
{
	if (!IsOrderUnlocked(OrderIndex))
	{
		return false;
	}

	SelectedOrderIndex = OrderIndex;
	SavePersistentData();
	return true;
}

bool UWorkOrderSubsystem::CycleSelectedWorkOrder(const int32 Direction)
{
	if (WorkOrders.Num() <= 0 || Direction == 0)
	{
		return false;
	}

	// Cycle through every order (including locked ones) so the lobby arrows always
	// respond. Locked orders can be browsed for their details but the launch path
	// (PrepareSelectedOrderForLaunch) still falls back to a valid unlocked order.
	const int32 Step = Direction > 0 ? 1 : -1;
	SelectedOrderIndex = (SelectedOrderIndex + Step + WorkOrders.Num()) % WorkOrders.Num();
	SavePersistentData();
	return true;
}

int32 UWorkOrderSubsystem::GetSelectedWorkOrderIndex() const
{
	return SelectedOrderIndex;
}

const FWorkOrderDefinition* UWorkOrderSubsystem::GetSelectedWorkOrder() const
{
	return GetWorkOrderDefinition(SelectedOrderIndex);
}

void UWorkOrderSubsystem::PrepareSelectedOrderForLaunch()
{
	if (!IsOrderUnlocked(SelectedOrderIndex))
	{
		SelectedOrderIndex = FindFirstUnlockedOrderIndex();
	}

	ResetRuntimeState(true);
	bPendingShiftLaunch = true;
	SavePersistentData();
}

bool UWorkOrderSubsystem::TryStartPendingShift(AGameplayCharacterBase* PlayerCharacter, const float WorldTimeSeconds)
{
	if (!bPendingShiftLaunch || !PlayerCharacter || !IsOrderUnlocked(SelectedOrderIndex))
	{
		return false;
	}

	const FWorkOrderDefinition* Definition = GetSelectedWorkOrder();
	if (!Definition)
	{
		return false;
	}

	bPendingShiftLaunch = false;
	ActiveOrderIndex = SelectedOrderIndex;
	ShiftState = EWorkOrderShiftState::Active;
	ShiftStartWorldTime = WorldTimeSeconds;
	RemainingShiftTime = GetEffectiveShiftDuration(*Definition);
	StartOreCollected = PlayerCharacter->GetTotalOreCollected();
	StartOreSold = PlayerCharacter->GetTotalOreSold();
	StartOreNodesBroken = PlayerCharacter->GetTotalOreNodesBroken();
	StartGoldEarned = PlayerCharacter->GetTotalGoldEarned();
	StartPotionsBought = PlayerCharacter->GetTotalPotionsBought();
	StartPotionsUsed = PlayerCharacter->GetTotalPotionsUsed();
	CachedPrimaryProgress = 0;
	CachedBonusProgress = 0;
	bCachedBonusCompleted = false;
	bLastBonusCompleted = false;
	LastRewardedGold = 0;
	LastRewardedExperience = 0;
	LastRewardedReputation = 0;
	LastResolvedOrderIndex = INDEX_NONE;
	LastResolvedState = EWorkOrderShiftState::None;
	ApplyActiveShiftEffects(PlayerCharacter);
	return true;
}

void UWorkOrderSubsystem::UpdateTrackedShift(const AGameplayCharacterBase* PlayerCharacter, const float WorldTimeSeconds)
{
	if (!PlayerCharacter || !HasTrackedShift())
	{
		return;
	}

	const FWorkOrderDefinition* Definition = GetTrackedOrderDefinition();
	if (!Definition)
	{
		return;
	}

	if (ShiftState == EWorkOrderShiftState::Completed || ShiftState == EWorkOrderShiftState::Failed || ShiftState == EWorkOrderShiftState::Abandoned)
	{
		return;
	}

	RemainingShiftTime = FMath::Max(0.0f, GetEffectiveShiftDuration(*Definition) - FMath::Max(0.0f, WorldTimeSeconds - ShiftStartWorldTime));
	CacheTrackedProgress(PlayerCharacter);

	if (RemainingShiftTime <= KINDA_SMALL_NUMBER)
	{
		ShiftState = EWorkOrderShiftState::Failed;
		LastResolvedOrderIndex = ActiveOrderIndex;
		LastResolvedState = EWorkOrderShiftState::Failed;
		LastRewardedGold = 0;
		LastRewardedExperience = 0;
		LastRewardedReputation = 0;
		bLastBonusCompleted = false;
		return;
	}

	if (IsObjectiveComplete(PlayerCharacter, Definition->PrimaryMetric, GetEffectivePrimaryTarget(*Definition)))
	{
		ShiftState = EWorkOrderShiftState::ReadyToTurnIn;
	}
	else
	{
		ShiftState = EWorkOrderShiftState::Active;
	}
}

bool UWorkOrderSubsystem::HasTrackedShift() const
{
	return ActiveOrderIndex != INDEX_NONE && ShiftState != EWorkOrderShiftState::None;
}

bool UWorkOrderSubsystem::IsShiftRunning() const
{
	return ShiftState == EWorkOrderShiftState::Active || ShiftState == EWorkOrderShiftState::ReadyToTurnIn;
}

bool UWorkOrderSubsystem::IsShiftReadyToTurnIn() const
{
	return ShiftState == EWorkOrderShiftState::ReadyToTurnIn;
}

bool UWorkOrderSubsystem::HasShiftFailed() const
{
	return ShiftState == EWorkOrderShiftState::Failed;
}

EWorkOrderShiftState UWorkOrderSubsystem::GetShiftState() const
{
	return ShiftState;
}

bool UWorkOrderSubsystem::CompleteTrackedShift(AGameplayCharacterBase* PlayerCharacter, int32& OutGoldReward, int32& OutExperienceReward, int32& OutReputationReward)
{
	OutGoldReward = 0;
	OutExperienceReward = 0;
	OutReputationReward = 0;

	if (!PlayerCharacter || ShiftState != EWorkOrderShiftState::ReadyToTurnIn)
	{
		return false;
	}

	const FWorkOrderDefinition* Definition = GetTrackedOrderDefinition();
	if (!Definition)
	{
		return false;
	}

	CacheTrackedProgress(PlayerCharacter);
	bLastBonusCompleted = IsTrackedBonusCompleted(PlayerCharacter);

	OutGoldReward = GetEffectiveGoldReward(*Definition, bLastBonusCompleted);
	OutExperienceReward = GetEffectiveExperienceReward(*Definition, bLastBonusCompleted);
	OutReputationReward = GetEffectiveReputationReward(*Definition, bLastBonusCompleted);

	PlayerCharacter->AddGold(OutGoldReward);
	PlayerCharacter->AddExperience(OutExperienceReward);
	PlayerCharacter->SetTemporaryStaminaModifiers(1.0f, 1.0f);

	Reputation += OutReputationReward;
	CompletedShiftCount += 1;
	if (CompletedOrderCounts.IsValidIndex(ActiveOrderIndex))
	{
		CompletedOrderCounts[ActiveOrderIndex] += 1;
	}

	LastRewardedGold = OutGoldReward;
	LastRewardedExperience = OutExperienceReward;
	LastRewardedReputation = OutReputationReward;
	LastResolvedOrderIndex = ActiveOrderIndex;
	LastResolvedState = EWorkOrderShiftState::Completed;
	ShiftState = EWorkOrderShiftState::Completed;
	RemainingShiftTime = 0.0f;
	SavePersistentData();
	return true;
}

bool UWorkOrderSubsystem::AbandonTrackedShift(const AGameplayCharacterBase* PlayerCharacter)
{
	if (!HasTrackedShift() || ShiftState == EWorkOrderShiftState::Completed || ShiftState == EWorkOrderShiftState::Failed || ShiftState == EWorkOrderShiftState::Abandoned)
	{
		return false;
	}

	if (PlayerCharacter)
	{
		CacheTrackedProgress(PlayerCharacter);
	}

	LastRewardedGold = 0;
	LastRewardedExperience = 0;
	LastRewardedReputation = 0;
	bLastBonusCompleted = false;
	LastResolvedOrderIndex = ActiveOrderIndex;
	LastResolvedState = EWorkOrderShiftState::Abandoned;
	ShiftState = EWorkOrderShiftState::Abandoned;
	RemainingShiftTime = 0.0f;
	return true;
}

void UWorkOrderSubsystem::DismissShiftResult()
{
	ResetRuntimeState(true);
}

int32 UWorkOrderSubsystem::GetTrackedOrderIndex() const
{
	return HasTrackedShift() ? ActiveOrderIndex : LastResolvedOrderIndex;
}

const FWorkOrderDefinition* UWorkOrderSubsystem::GetTrackedOrderDefinition() const
{
	const int32 DisplayOrderIndex = GetTrackedOrderIndex();
	return GetWorkOrderDefinition(DisplayOrderIndex);
}

float UWorkOrderSubsystem::GetRemainingShiftTime() const
{
	return RemainingShiftTime;
}

int32 UWorkOrderSubsystem::GetTrackedShiftProgress(const AGameplayCharacterBase* PlayerCharacter) const
{
	const FWorkOrderDefinition* Definition = GetTrackedOrderDefinition();
	if (!Definition)
	{
		return 0;
	}

	if (PlayerCharacter && (ShiftState == EWorkOrderShiftState::Active || ShiftState == EWorkOrderShiftState::ReadyToTurnIn))
	{
		return GetObjectiveProgress(PlayerCharacter, Definition->PrimaryMetric);
	}

	return CachedPrimaryProgress;
}

int32 UWorkOrderSubsystem::GetTrackedShiftTarget() const
{
	const FWorkOrderDefinition* Definition = GetTrackedOrderDefinition();
	return Definition ? GetEffectivePrimaryTarget(*Definition) : 0;
}

float UWorkOrderSubsystem::GetTrackedShiftProgressRatio(const AGameplayCharacterBase* PlayerCharacter) const
{
	const int32 TargetAmount = GetTrackedShiftTarget();
	if (TargetAmount <= 0)
	{
		return 0.0f;
	}

	return FMath::Clamp(static_cast<float>(GetTrackedShiftProgress(PlayerCharacter)) / static_cast<float>(TargetAmount), 0.0f, 1.0f);
}

int32 UWorkOrderSubsystem::GetTrackedBonusProgress(const AGameplayCharacterBase* PlayerCharacter) const
{
	const FWorkOrderDefinition* Definition = GetTrackedOrderDefinition();
	if (!Definition || Definition->BonusTargetAmount <= 0)
	{
		return 0;
	}

	if (PlayerCharacter && (ShiftState == EWorkOrderShiftState::Active || ShiftState == EWorkOrderShiftState::ReadyToTurnIn))
	{
		return GetObjectiveProgress(PlayerCharacter, Definition->BonusMetric);
	}

	return CachedBonusProgress;
}

int32 UWorkOrderSubsystem::GetTrackedBonusTarget() const
{
	const FWorkOrderDefinition* Definition = GetTrackedOrderDefinition();
	return Definition ? Definition->BonusTargetAmount : 0;
}

float UWorkOrderSubsystem::GetTrackedBonusProgressRatio(const AGameplayCharacterBase* PlayerCharacter) const
{
	const int32 TargetAmount = GetTrackedBonusTarget();
	if (TargetAmount <= 0)
	{
		return 0.0f;
	}

	return FMath::Clamp(static_cast<float>(GetTrackedBonusProgress(PlayerCharacter)) / static_cast<float>(TargetAmount), 0.0f, 1.0f);
}

bool UWorkOrderSubsystem::IsTrackedBonusCompleted(const AGameplayCharacterBase* PlayerCharacter) const
{
	const FWorkOrderDefinition* Definition = GetTrackedOrderDefinition();
	if (!Definition || Definition->BonusTargetAmount <= 0)
	{
		return false;
	}

	if (PlayerCharacter && (ShiftState == EWorkOrderShiftState::Active || ShiftState == EWorkOrderShiftState::ReadyToTurnIn))
	{
		return IsObjectiveComplete(PlayerCharacter, Definition->BonusMetric, Definition->BonusTargetAmount);
	}

	return bCachedBonusCompleted || bLastBonusCompleted;
}

int32 UWorkOrderSubsystem::GetReputation() const
{
	return Reputation;
}

int32 UWorkOrderSubsystem::GetCompletedShiftCount() const
{
	return CompletedShiftCount;
}

int32 UWorkOrderSubsystem::GetCompletedOrderCount(const int32 OrderIndex) const
{
	return CompletedOrderCounts.IsValidIndex(OrderIndex) ? CompletedOrderCounts[OrderIndex] : 0;
}

int32 UWorkOrderSubsystem::GetLastRewardedGold() const
{
	return LastRewardedGold;
}

int32 UWorkOrderSubsystem::GetLastRewardedExperience() const
{
	return LastRewardedExperience;
}

int32 UWorkOrderSubsystem::GetLastRewardedReputation() const
{
	return LastRewardedReputation;
}

bool UWorkOrderSubsystem::WasLastBonusCompleted() const
{
	return bLastBonusCompleted;
}

EWorkOrderShiftState UWorkOrderSubsystem::GetLastResolvedState() const
{
	return LastResolvedState;
}

bool UWorkOrderSubsystem::HasResolvedShiftResult() const
{
	return LastResolvedOrderIndex != INDEX_NONE
		&& (LastResolvedState == EWorkOrderShiftState::Completed
			|| LastResolvedState == EWorkOrderShiftState::Failed
			|| LastResolvedState == EWorkOrderShiftState::Abandoned);
}

FString UWorkOrderSubsystem::GetOrderTitle(const int32 OrderIndex, const bool bRussian) const
{
	const FWorkOrderDefinition* Definition = GetWorkOrderDefinition(OrderIndex);
	if (!Definition)
	{
		return FString();
	}

	return bRussian ? FString(Definition->RussianTitle) : FString(Definition->EnglishTitle);
}

FString UWorkOrderSubsystem::GetOrderSummary(const int32 OrderIndex, const bool bRussian) const
{
	const FWorkOrderDefinition* Definition = GetWorkOrderDefinition(OrderIndex);
	if (!Definition)
	{
		return FString();
	}

	return bRussian ? FString(Definition->RussianSummary) : FString(Definition->EnglishSummary);
}

FString UWorkOrderSubsystem::GetOrderObjectiveText(const int32 OrderIndex, const bool bRussian) const
{
	const FWorkOrderDefinition* Definition = GetWorkOrderDefinition(OrderIndex);
	if (!Definition)
	{
		return FString();
	}

	return bRussian ? FString(Definition->RussianPrimaryObjective) : FString(Definition->EnglishPrimaryObjective);
}

FString UWorkOrderSubsystem::GetOrderBonusObjectiveText(const int32 OrderIndex, const bool bRussian) const
{
	const FWorkOrderDefinition* Definition = GetWorkOrderDefinition(OrderIndex);
	if (!Definition)
	{
		return FString();
	}

	return bRussian ? FString(Definition->RussianBonusObjective) : FString(Definition->EnglishBonusObjective);
}

FString UWorkOrderSubsystem::GetOrderModifierText(const int32 OrderIndex, const bool bRussian) const
{
	const FWorkOrderDefinition* Definition = GetWorkOrderDefinition(OrderIndex);
	if (!Definition)
	{
		return FString();
	}

	switch (Definition->Modifier)
	{
	case EWorkOrderModifier::IncreasedDemand:
		return bRussian ? TEXT("Повышенный спрос") : TEXT("Increased Demand");
	case EWorkOrderModifier::HeavyVeins:
		return bRussian ? TEXT("Тяжёлые жилы") : TEXT("Heavy Veins");
	case EWorkOrderModifier::ExhaustingShift:
		return bRussian ? TEXT("Утомительная смена") : TEXT("Exhausting Shift");
	case EWorkOrderModifier::TightSchedule:
		return bRussian ? TEXT("Жёсткий график") : TEXT("Tight Schedule");
	case EWorkOrderModifier::ReputationRush:
		return bRussian ? TEXT("Ускоренная карьера") : TEXT("Reputation Rush");
	case EWorkOrderModifier::None:
	default:
		return bRussian ? TEXT("Без модификатора") : TEXT("No Modifier");
	}
}

FString UWorkOrderSubsystem::GetOrderModifierEffectText(const int32 OrderIndex, const bool bRussian) const
{
	const FWorkOrderDefinition* Definition = GetWorkOrderDefinition(OrderIndex);
	if (!Definition)
	{
		return FString();
	}

	switch (Definition->Modifier)
	{
	case EWorkOrderModifier::IncreasedDemand:
		return bRussian ? TEXT("+25% к золоту за заказ") : TEXT("+25% order gold");
	case EWorkOrderModifier::HeavyVeins:
		return bRussian ? TEXT("+25% к цели, +20% к золоту") : TEXT("+25% target, +20% gold");
	case EWorkOrderModifier::ExhaustingShift:
		return bRussian ? TEXT("Спринт тратит +20%, реген стамины -20%") : TEXT("+20% sprint drain, -20% stamina regen");
	case EWorkOrderModifier::TightSchedule:
		return bRussian ? TEXT("-15% времени, +35% к золоту") : TEXT("-15% time, +35% gold");
	case EWorkOrderModifier::ReputationRush:
		return bRussian ? TEXT("+50% к репутации за сдачу") : TEXT("+50% reputation reward");
	case EWorkOrderModifier::None:
	default:
		return bRussian ? TEXT("Обычные условия смены") : TEXT("Normal shift rules");
	}
}

int32 UWorkOrderSubsystem::GetPreviewGoldReward(const int32 OrderIndex, const bool bIncludeBonus) const
{
	const FWorkOrderDefinition* Definition = GetWorkOrderDefinition(OrderIndex);
	return Definition ? GetEffectiveGoldReward(*Definition, bIncludeBonus) : 0;
}

int32 UWorkOrderSubsystem::GetPreviewExperienceReward(const int32 OrderIndex, const bool bIncludeBonus) const
{
	const FWorkOrderDefinition* Definition = GetWorkOrderDefinition(OrderIndex);
	return Definition ? GetEffectiveExperienceReward(*Definition, bIncludeBonus) : 0;
}

int32 UWorkOrderSubsystem::GetPreviewReputationReward(const int32 OrderIndex, const bool bIncludeBonus) const
{
	const FWorkOrderDefinition* Definition = GetWorkOrderDefinition(OrderIndex);
	return Definition ? GetEffectiveReputationReward(*Definition, bIncludeBonus) : 0;
}

void UWorkOrderSubsystem::ApplyActiveShiftEffects(AGameplayCharacterBase* PlayerCharacter) const
{
	if (!PlayerCharacter)
	{
		return;
	}

	const FWorkOrderDefinition* Definition = GetTrackedOrderDefinition();
	if (!Definition || !IsShiftRunning())
	{
		PlayerCharacter->SetTemporaryStaminaModifiers(1.0f, 1.0f);
		return;
	}

	if (Definition->Modifier == EWorkOrderModifier::ExhaustingShift)
	{
		PlayerCharacter->SetTemporaryStaminaModifiers(1.2f, 0.8f);
		return;
	}

	PlayerCharacter->SetTemporaryStaminaModifiers(1.0f, 1.0f);
}

void UWorkOrderSubsystem::NotifyReturnedToLobby()
{
	ResetRuntimeState(true);
}

void UWorkOrderSubsystem::ResetPersistentProgress()
{
	Reputation = 0;
	CompletedShiftCount = 0;
	SelectedOrderIndex = 0;
	CompletedOrderCounts.SetNumZeroed(WorkOrders.Num());
	ResetRuntimeState(true);
	SavePersistentData();
}

void UWorkOrderSubsystem::LoadPersistentData()
{
	FMyProjectWorkOrderSaveData SaveData;
	if (!FMyProjectJsonSaveUtils::LoadWorkOrderData(SaveData))
	{
		SavePersistentData();
		return;
	}

	Reputation = FMath::Max(0, SaveData.Reputation);
	CompletedShiftCount = FMath::Max(0, SaveData.CompletedShiftCount);
	SelectedOrderIndex = FMath::Clamp(SaveData.LastSelectedOrderIndex, 0, FMath::Max(0, WorkOrders.Num() - 1));
	CompletedOrderCounts = SaveData.CompletedOrderCounts;
	CompletedOrderCounts.SetNum(WorkOrders.Num());
	for (int32& CompletedCount : CompletedOrderCounts)
	{
		CompletedCount = FMath::Max(0, CompletedCount);
	}

	if (!IsOrderUnlocked(SelectedOrderIndex))
	{
		SelectedOrderIndex = FindFirstUnlockedOrderIndex();
	}
}

void UWorkOrderSubsystem::SavePersistentData() const
{
	FMyProjectWorkOrderSaveData SaveData;
	SaveData.Reputation = Reputation;
	SaveData.CompletedShiftCount = CompletedShiftCount;
	SaveData.LastSelectedOrderIndex = SelectedOrderIndex;
	SaveData.CompletedOrderCounts = CompletedOrderCounts;
	FMyProjectJsonSaveUtils::SaveWorkOrderData(SaveData);
}

void UWorkOrderSubsystem::ResetRuntimeState(const bool bClearLastResolution)
{
	ActiveOrderIndex = INDEX_NONE;
	ShiftState = EWorkOrderShiftState::None;
	ShiftStartWorldTime = 0.0f;
	RemainingShiftTime = 0.0f;
	CachedPrimaryProgress = 0;
	CachedBonusProgress = 0;
	bCachedBonusCompleted = false;
	StartOreCollected = 0;
	StartOreSold = 0;
	StartOreNodesBroken = 0;
	StartGoldEarned = 0;
	StartPotionsBought = 0;
	StartPotionsUsed = 0;
	bPendingShiftLaunch = false;
	LastRewardedGold = 0;
	LastRewardedExperience = 0;
	LastRewardedReputation = 0;
	bLastBonusCompleted = false;

	if (bClearLastResolution)
	{
		LastResolvedOrderIndex = INDEX_NONE;
		LastResolvedState = EWorkOrderShiftState::None;
	}
}

int32 UWorkOrderSubsystem::GetMetricValue(const AGameplayCharacterBase* PlayerCharacter, const EWorkOrderMetric Metric) const
{
	if (!PlayerCharacter)
	{
		return 0;
	}

	switch (Metric)
	{
	case EWorkOrderMetric::OreSold:
		return PlayerCharacter->GetTotalOreSold();
	case EWorkOrderMetric::OreNodesBroken:
		return PlayerCharacter->GetTotalOreNodesBroken();
	case EWorkOrderMetric::GoldEarned:
		return PlayerCharacter->GetTotalGoldEarned();
	case EWorkOrderMetric::PotionsBought:
		return PlayerCharacter->GetTotalPotionsBought();
	case EWorkOrderMetric::PotionsUsed:
		return PlayerCharacter->GetTotalPotionsUsed();
	case EWorkOrderMetric::PlayerLevel:
		return PlayerCharacter->GetPlayerLevel();
	case EWorkOrderMetric::FinishBeforeTime:
		return FMath::FloorToInt(RemainingShiftTime);
	case EWorkOrderMetric::OreCollected:
	default:
		return PlayerCharacter->GetTotalOreCollected();
	}
}

int32 UWorkOrderSubsystem::GetMetricStartValue(const EWorkOrderMetric Metric) const
{
	switch (Metric)
	{
	case EWorkOrderMetric::OreSold:
		return StartOreSold;
	case EWorkOrderMetric::OreNodesBroken:
		return StartOreNodesBroken;
	case EWorkOrderMetric::GoldEarned:
		return StartGoldEarned;
	case EWorkOrderMetric::PotionsBought:
		return StartPotionsBought;
	case EWorkOrderMetric::PotionsUsed:
		return StartPotionsUsed;
	case EWorkOrderMetric::PlayerLevel:
	case EWorkOrderMetric::FinishBeforeTime:
		return 0;
	case EWorkOrderMetric::OreCollected:
	default:
		return StartOreCollected;
	}
}

int32 UWorkOrderSubsystem::GetObjectiveProgress(const AGameplayCharacterBase* PlayerCharacter, const EWorkOrderMetric Metric) const
{
	if (Metric == EWorkOrderMetric::PlayerLevel || Metric == EWorkOrderMetric::FinishBeforeTime)
	{
		return GetMetricValue(PlayerCharacter, Metric);
	}

	return FMath::Max(0, GetMetricValue(PlayerCharacter, Metric) - GetMetricStartValue(Metric));
}

bool UWorkOrderSubsystem::IsObjectiveComplete(const AGameplayCharacterBase* PlayerCharacter, const EWorkOrderMetric Metric, const int32 TargetAmount) const
{
	if (TargetAmount <= 0)
	{
		return false;
	}

	return GetObjectiveProgress(PlayerCharacter, Metric) >= TargetAmount;
}

void UWorkOrderSubsystem::CacheTrackedProgress(const AGameplayCharacterBase* PlayerCharacter)
{
	const FWorkOrderDefinition* Definition = GetTrackedOrderDefinition();
	if (!Definition)
	{
		CachedPrimaryProgress = 0;
		CachedBonusProgress = 0;
		bCachedBonusCompleted = false;
		return;
	}

	CachedPrimaryProgress = GetObjectiveProgress(PlayerCharacter, Definition->PrimaryMetric);
	CachedBonusProgress = GetObjectiveProgress(PlayerCharacter, Definition->BonusMetric);
	bCachedBonusCompleted = IsObjectiveComplete(PlayerCharacter, Definition->BonusMetric, Definition->BonusTargetAmount);
}

float UWorkOrderSubsystem::GetEffectiveShiftDuration(const FWorkOrderDefinition& Definition) const
{
	const float DurationMultiplier = Definition.Modifier == EWorkOrderModifier::TightSchedule ? 0.85f : 1.0f;
	return FMath::Max(30.0f, Definition.ShiftDurationSeconds * DurationMultiplier);
}

int32 UWorkOrderSubsystem::GetEffectivePrimaryTarget(const FWorkOrderDefinition& Definition) const
{
	const float TargetMultiplier = Definition.Modifier == EWorkOrderModifier::HeavyVeins ? 1.25f : 1.0f;
	return FMath::Max(1, FMath::CeilToInt(static_cast<float>(Definition.PrimaryTargetAmount) * TargetMultiplier));
}

int32 UWorkOrderSubsystem::GetEffectiveGoldReward(const FWorkOrderDefinition& Definition, const bool bIncludeBonus) const
{
	const int32 RawReward = Definition.GoldReward + (bIncludeBonus ? Definition.BonusGoldReward : 0);
	return FMath::RoundToInt(static_cast<float>(RawReward) * GetModifierGoldMultiplier(Definition.Modifier));
}

int32 UWorkOrderSubsystem::GetEffectiveExperienceReward(const FWorkOrderDefinition& Definition, const bool bIncludeBonus) const
{
	const int32 RawReward = Definition.ExperienceReward + (bIncludeBonus ? Definition.BonusExperienceReward : 0);
	return FMath::RoundToInt(static_cast<float>(RawReward) * GetModifierExperienceMultiplier(Definition.Modifier));
}

int32 UWorkOrderSubsystem::GetEffectiveReputationReward(const FWorkOrderDefinition& Definition, const bool bIncludeBonus) const
{
	const int32 RawReward = Definition.ReputationReward + (bIncludeBonus ? Definition.BonusReputationReward : 0);
	return FMath::RoundToInt(static_cast<float>(RawReward) * GetModifierReputationMultiplier(Definition.Modifier));
}

int32 UWorkOrderSubsystem::FindFirstUnlockedOrderIndex() const
{
	for (int32 OrderIndex = 0; OrderIndex < WorkOrders.Num(); ++OrderIndex)
	{
		if (IsOrderUnlocked(OrderIndex))
		{
			return OrderIndex;
		}
	}

	return 0;
}
