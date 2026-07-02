#pragma once

// ============================================================================
//  UWorkOrderSubsystem (.h) — интерфейс системы заказов и смен (Subsystem).
//  Ниже — структуры заказа/метрик и объявления функций выбора, запуска,
//  отслеживания и наград. Реализация — в .cpp.
//
//  ЗАЩИТА — по этапу 9 (заказы/смены): CycleSelectedWorkOrder,
//  PrepareSelectedOrderForLaunch, UpdateTrackedShift, CompleteTrackedShift,
//  AbandonTrackedShift, GetReputation.
// ============================================================================

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorkOrderSubsystem.generated.h"

class AGameplayCharacterBase;

enum class EWorkOrderMetric : uint8
{
	OreCollected,
	OreSold,
	OreNodesBroken,
	GoldEarned,
	PotionsBought,
	PotionsUsed,
	PlayerLevel,
	FinishBeforeTime
};

enum class EWorkOrderModifier : uint8
{
	None,
	IncreasedDemand,
	HeavyVeins,
	ExhaustingShift,
	TightSchedule,
	ReputationRush
};

enum class EWorkOrderShiftState : uint8
{
	None,
	Active,
	ReadyToTurnIn,
	Completed,
	Failed,
	Abandoned
};

struct FWorkOrderDefinition
{
	int32 OrderId = INDEX_NONE;
	EWorkOrderMetric PrimaryMetric = EWorkOrderMetric::OreCollected;
	int32 PrimaryTargetAmount = 0;
	EWorkOrderMetric BonusMetric = EWorkOrderMetric::OreNodesBroken;
	int32 BonusTargetAmount = 0;
	EWorkOrderModifier Modifier = EWorkOrderModifier::None;
	float ShiftDurationSeconds = 0.0f;
	int32 GoldReward = 0;
	int32 ExperienceReward = 0;
	int32 ReputationReward = 0;
	int32 BonusGoldReward = 0;
	int32 BonusExperienceReward = 0;
	int32 BonusReputationReward = 0;
	int32 RequiredCompletedShifts = 0;
	const TCHAR* EnglishTitle = TEXT("");
	const TCHAR* RussianTitle = TEXT("");
	const TCHAR* EnglishSummary = TEXT("");
	const TCHAR* RussianSummary = TEXT("");
	const TCHAR* EnglishPrimaryObjective = TEXT("");
	const TCHAR* RussianPrimaryObjective = TEXT("");
	const TCHAR* EnglishBonusObjective = TEXT("");
	const TCHAR* RussianBonusObjective = TEXT("");
};

UCLASS()
class MYPROJECT_API UWorkOrderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	int32 GetWorkOrderCount() const;
	int32 GetUnlockedWorkOrderCount() const;
	const FWorkOrderDefinition* GetWorkOrderDefinition(int32 OrderIndex) const;
	bool IsOrderUnlocked(int32 OrderIndex) const;

	bool SelectWorkOrder(int32 OrderIndex);
	bool CycleSelectedWorkOrder(int32 Direction);
	int32 GetSelectedWorkOrderIndex() const;
	const FWorkOrderDefinition* GetSelectedWorkOrder() const;

	void PrepareSelectedOrderForLaunch();
	bool TryStartPendingShift(AGameplayCharacterBase* PlayerCharacter, float WorldTimeSeconds);
	void UpdateTrackedShift(const AGameplayCharacterBase* PlayerCharacter, float WorldTimeSeconds);

	bool HasTrackedShift() const;
	bool IsShiftRunning() const;
	bool IsShiftReadyToTurnIn() const;
	bool HasShiftFailed() const;
	EWorkOrderShiftState GetShiftState() const;
	bool CompleteTrackedShift(AGameplayCharacterBase* PlayerCharacter, int32& OutGoldReward, int32& OutExperienceReward, int32& OutReputationReward);
	bool AbandonTrackedShift(const AGameplayCharacterBase* PlayerCharacter);
	void DismissShiftResult();

	int32 GetTrackedOrderIndex() const;
	const FWorkOrderDefinition* GetTrackedOrderDefinition() const;
	float GetRemainingShiftTime() const;
	int32 GetTrackedShiftProgress(const AGameplayCharacterBase* PlayerCharacter) const;
	int32 GetTrackedShiftTarget() const;
	float GetTrackedShiftProgressRatio(const AGameplayCharacterBase* PlayerCharacter) const;
	int32 GetTrackedBonusProgress(const AGameplayCharacterBase* PlayerCharacter) const;
	int32 GetTrackedBonusTarget() const;
	float GetTrackedBonusProgressRatio(const AGameplayCharacterBase* PlayerCharacter) const;
	bool IsTrackedBonusCompleted(const AGameplayCharacterBase* PlayerCharacter) const;

	int32 GetReputation() const;
	int32 GetCompletedShiftCount() const;
	int32 GetCompletedOrderCount(int32 OrderIndex) const;

	int32 GetLastRewardedGold() const;
	int32 GetLastRewardedExperience() const;
	int32 GetLastRewardedReputation() const;
	bool WasLastBonusCompleted() const;
	EWorkOrderShiftState GetLastResolvedState() const;
	bool HasResolvedShiftResult() const;

	FString GetOrderTitle(int32 OrderIndex, bool bRussian) const;
	FString GetOrderSummary(int32 OrderIndex, bool bRussian) const;
	FString GetOrderObjectiveText(int32 OrderIndex, bool bRussian) const;
	FString GetOrderBonusObjectiveText(int32 OrderIndex, bool bRussian) const;
	FString GetOrderModifierText(int32 OrderIndex, bool bRussian) const;
	FString GetOrderModifierEffectText(int32 OrderIndex, bool bRussian) const;
	int32 GetPreviewGoldReward(int32 OrderIndex, bool bIncludeBonus) const;
	int32 GetPreviewExperienceReward(int32 OrderIndex, bool bIncludeBonus) const;
	int32 GetPreviewReputationReward(int32 OrderIndex, bool bIncludeBonus) const;

	void ApplyActiveShiftEffects(AGameplayCharacterBase* PlayerCharacter) const;
	void NotifyReturnedToLobby();
	void ResetPersistentProgress();

private:
	void LoadPersistentData();
	void SavePersistentData() const;
	void ResetRuntimeState(bool bClearLastResolution);
	int32 GetMetricValue(const AGameplayCharacterBase* PlayerCharacter, EWorkOrderMetric Metric) const;
	int32 GetMetricStartValue(EWorkOrderMetric Metric) const;
	int32 GetObjectiveProgress(const AGameplayCharacterBase* PlayerCharacter, EWorkOrderMetric Metric) const;
	bool IsObjectiveComplete(const AGameplayCharacterBase* PlayerCharacter, EWorkOrderMetric Metric, int32 TargetAmount) const;
	void CacheTrackedProgress(const AGameplayCharacterBase* PlayerCharacter);
	float GetEffectiveShiftDuration(const FWorkOrderDefinition& Definition) const;
	int32 GetEffectivePrimaryTarget(const FWorkOrderDefinition& Definition) const;
	int32 GetEffectiveGoldReward(const FWorkOrderDefinition& Definition, bool bIncludeBonus) const;
	int32 GetEffectiveExperienceReward(const FWorkOrderDefinition& Definition, bool bIncludeBonus) const;
	int32 GetEffectiveReputationReward(const FWorkOrderDefinition& Definition, bool bIncludeBonus) const;
	int32 FindFirstUnlockedOrderIndex() const;

	TArray<FWorkOrderDefinition> WorkOrders;
	TArray<int32> CompletedOrderCounts;

	int32 SelectedOrderIndex = 0;
	int32 ActiveOrderIndex = INDEX_NONE;
	int32 LastResolvedOrderIndex = INDEX_NONE;

	int32 Reputation = 0;
	int32 CompletedShiftCount = 0;

	int32 LastRewardedGold = 0;
	int32 LastRewardedExperience = 0;
	int32 LastRewardedReputation = 0;
	bool bLastBonusCompleted = false;

	int32 CachedPrimaryProgress = 0;
	int32 CachedBonusProgress = 0;
	bool bCachedBonusCompleted = false;
	int32 StartOreCollected = 0;
	int32 StartOreSold = 0;
	int32 StartOreNodesBroken = 0;
	int32 StartGoldEarned = 0;
	int32 StartPotionsBought = 0;
	int32 StartPotionsUsed = 0;

	float ShiftStartWorldTime = 0.0f;
	float RemainingShiftTime = 0.0f;

	bool bPendingShiftLaunch = false;
	EWorkOrderShiftState ShiftState = EWorkOrderShiftState::None;
	EWorkOrderShiftState LastResolvedState = EWorkOrderShiftState::None;
};
