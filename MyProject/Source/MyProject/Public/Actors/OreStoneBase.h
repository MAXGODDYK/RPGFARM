#pragma once

// ============================================================================
//  AOreStoneBase (.h) — интерфейс рудного камня. Ниже — компоненты (меш, полоса
//  здоровья, триггер) и параметры (MaxHealth, RespawnDelayMinutes, награда),
//  которые настраиваются в BP_OreStone. Реализация — в .cpp.
//
//  ЗАЩИТА — по этапу 6 (добыча): ApplyDamageToOre, BreakOre, RespawnOre,
//  GetHealthPercent/RefreshHealthBar.
// ============================================================================

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OreStoneBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;
class UWidgetComponent;

UCLASS()
class MYPROJECT_API AOreStoneBase : public AActor
{
	GENERATED_BODY()

public:
	AOreStoneBase();

	UFUNCTION(BlueprintCallable, Category = "Ore Health")
	void ApplyDamageToOre(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Ore Health")
	void ResetOreHealth();

	UFUNCTION(BlueprintPure, Category = "Ore Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Ore Health")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintPure, Category = "Ore Health")
	bool IsOreAvailable() const;

	UFUNCTION(BlueprintPure, Category = "Ore Reward")
	int32 GetOreResourceReward() const;

	UFUNCTION(BlueprintPure, Category = "Ore Reward")
	int32 GetOreExperienceReward() const;

	void RefreshHealthBarVisibilityState();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> OreMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> HealthBarWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> HealthBarTriggerSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ore Health", meta = (ClampMin = "1.0"))
	float MaxHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ore Health")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ore Health")
	FVector HealthBarOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ore Health|UI", meta = (ClampMin = "1.0", ClampMax = "1024.0"))
	float HealthBarWidth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ore Health|UI", meta = (ClampMin = "1.0", ClampMax = "256.0"))
	float HealthBarHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ore Health|UI", meta = (ClampMin = "0.0"))
	float HealthBarVisibleDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ore Health|UI", meta = (ClampMin = "0.0"))
	float HealthBarTriggerRadius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ore Respawn", meta = (ClampMin = "0.0"))
	float RespawnDelayMinutes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ore Reward", meta = (ClampMin = "0"))
	int32 OreResourceReward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ore Reward", meta = (ClampMin = "0"))
	int32 OreExperienceReward;

private:
	void RefreshHealthBar();
	void UpdateHealthBarVisibility();
	void RefreshPlayerInHealthBarRange();
	void BreakOre();
	void RespawnOre();

	UFUNCTION()
	void HandleHealthBarTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleHealthBarTriggerEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	FTimerHandle RespawnTimerHandle;
	bool bOreAvailable;
	bool bPlayerInHealthBarRange;
};
