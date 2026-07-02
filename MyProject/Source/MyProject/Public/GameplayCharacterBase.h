#pragma once

// ============================================================================
//  AGameplayCharacterBase (.h) — интерфейс базового класса персонажа.
//  Ниже — поля характеристик (стамина, скорость, урон, опыт) и объявления
//  функций геймплея. Реализация и подробные пометки — в одноимённом .cpp.
//
//  ЗАЩИТА — по этапам: 5 (стамина/спринт: SetSprintActive, Decrease/IncreaseStamina,
//  Tick), 6 (добыча: Attack, TryDamageOre), 7 (торговля: SellOre, BuyStaminaPotion,
//  UseStaminaPotion), 8 (прокачка: AddExperience, SpendUpgradePoint),
//  10 (сохранение: BeginPlay/EndPlay), 11 (возврат при падении: Tick).
// ============================================================================

#include "CoreMinimal.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameplayCharacterBase.generated.h"

UENUM(BlueprintType)
enum class EPlayerUpgradeType : uint8
{
	MaxStamina,
	OreDamage,
	MoveSpeed
};

UCLASS()
class MYPROJECT_API AGameplayCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AGameplayCharacterBase(const FObjectInitializer& ObjectInitializer);

	// True while stamina is fully drained and sprint is locked out. Used by the
	// custom movement component to hard-cap speed even if a Blueprint forces it,
	// and exposed to Blueprints so the sprint/gait input can be gated by stamina.
	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool IsStaminaExhausted() const { return bStaminaExhausted; }

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Attack")
	virtual void Attack();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetSprintActive(bool bShouldSprint);

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void AddExperience(int32 ExperienceAmount);

	UFUNCTION(BlueprintCallable, Category = "Resources")
	void AddGold(int32 GoldAmount);

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void ResetCharacterProgress();

	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool SellOre(int32 OreAmount, int32 GoldPerOre);

	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool BuyStaminaPotion(int32 GoldCost, float RestoreAmount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseStaminaPotion();

	UFUNCTION(BlueprintCallable, Category = "Progression")
	bool SpendUpgradePoint(EPlayerUpgradeType UpgradeType);

	UFUNCTION(BlueprintPure, Category = "Resources")
	int32 GetCollectedGold() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetPlayerLevel() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCurrentExperienceAmount() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetExperienceToNextLevelAmount() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetAvailableUpgradePoints() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetOreDamageAmount() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetMiniQuestCount() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCompletedMiniQuestCount() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetStaminaPotionCount() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetTotalOreCollected() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetTotalOreSold() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetTotalOreNodesBroken() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetTotalGoldEarned() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetTotalPotionsBought() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetTotalPotionsUsed() const;

	bool GetMiniQuestProgress(int32 QuestIndex, int32& OutCurrentProgress, int32& OutTargetProgress, bool& bOutCompleted) const;

	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	bool bIsAttacking;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resources")
	int32 CollectedOreResources;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resources")
	int32 CollectedGold;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 StaminaPotionCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	int32 PlayerLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	int32 CurrentExperience;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	int32 ExperienceToNextLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	int32 AvailableUpgradePoints;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float WalkSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float SprintSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float CurrentStamina;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float MinusStamina;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float PlusStamina;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float MovingStaminaRegen;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float StaminaRegenDelay;

	// Once stamina is fully drained, sprint stays locked until it recovers back up to
	// this value. Prevents re-sprinting at (almost) zero stamina.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float StaminaSprintRecoveryThreshold;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0"))
	float Stamina;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "1.0"))
	float MaxStamina;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1.0"))
	float StaminaPotionRestoreAmount;

	// If the character ever drops more than this many units below the ground it was
	// last standing on, it is teleported back to the spawn point instead of falling
	// out of the world.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "200.0"))
	float FallRecoveryDropDistance;

	// Absolute world height below which the character is always recovered, no matter
	// how it got there (catches sliding off a slope into the void).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float FallRecoveryKillZ;

	void SetTemporaryStaminaModifiers(float DrainMultiplier, float RegenMultiplier);
	void DecreaseStamina();
	void IncreaseStamina();

protected:
	virtual bool HasMovementInputIntent() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimSequenceBase> AttackTimingAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<class UStaticMesh> EquippedPickaxeMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	FName PickaxeAttachSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	FVector PickaxeRelativeLocation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	FRotator PickaxeRelativeRotation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	FVector PickaxeRelativeScale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float OreDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float AttackRange;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float AttackCooldown;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float AttackStateDuration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AttackHitNormalizedTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "1"))
	int32 BaseExperienceToNextLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "1.0"))
	float MaxStaminaUpgradeAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "0.1"))
	float OreDamageUpgradeAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "1.0"))
	float MoveSpeedUpgradeAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resources")
	TSubclassOf<class UResourceCounterWidget> ResourceCounterWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	TSubclassOf<class UStaminaBarWidget> StaminaBarWidgetClass;

private:
	void AddOreResources(int32 ResourceAmount);
	void TriggerAttackHit();
	void FinishAttackState();
	void HandleUseStaminaPotionInput();
	void LoadCharacterDataFromJson();
	void SaveCharacterDataToJson() const;
	int32 GetMiniQuestCurrentValue(int32 QuestIndex) const;
	void UpdateResourceCounter() const;
	void UpdateStaminaBar() const;
	void TryDamageOre();
	void SyncAttachedVisualInputState();
	bool TriggerAttachedVisualAttack();
	void AttachPickaxeToAttachedVisual();

	bool bCanAttack;
	bool bStaminaExhausted;
	bool bHasGroundedLocation;
	FVector LastGroundedLocation;
	FVector InitialSpawnLocation;
	float TimeSinceLastStaminaUse;
	int32 TotalOreCollected = 0;
	int32 TotalOreSold = 0;
	int32 TotalOreNodesBroken = 0;
	int32 TotalGoldEarned = 0;
	int32 TotalPotionsBought = 0;
	int32 TotalPotionsUsed = 0;
	float TemporaryStaminaDrainMultiplier = 1.0f;
	float TemporaryStaminaRegenMultiplier = 1.0f;
	FTimerHandle AttackCooldownHandle;
	FTimerHandle AttackHitTimerHandle;
	FTimerHandle AttackStateTimerHandle;
	FTimerHandle AttachedVisualInputSyncTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<class UResourceCounterWidget> ResourceCounterWidget;

	UPROPERTY(Transient)
	TObjectPtr<class UStaminaBarWidget> StaminaBarWidget;
};
