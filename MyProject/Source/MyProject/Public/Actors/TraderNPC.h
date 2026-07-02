#pragma once

// ============================================================================
//  ATraderNPC (.h) — интерфейс торговца. Ниже — компоненты (меш, зона
//  InteractionSphere) и параметры. Реализация — в .cpp.
//
//  ЗАЩИТА — по этапу 7 (торговец): зона InteractionSphere запоминает торговца в
//  контроллере; сама торговля — в персонаже (SellOre/BuyStaminaPotion) через
//  ExecuteMenuOption контроллера.
// ============================================================================

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TraderNPC.generated.h"

UCLASS()
class MYPROJECT_API ATraderNPC : public AActor
{
	GENERATED_BODY()

public:
	ATraderNPC();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Trader")
	FText GetTraderName() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trader")
	TObjectPtr<class USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trader")
	TObjectPtr<class UStaticMeshComponent> TraderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trader")
	TObjectPtr<class USkeletalMeshComponent> TraderCharacterMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trader")
	TObjectPtr<class USphereComponent> InteractionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader")
	FText TraderName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader|Visual")
	TObjectPtr<class UAnimationAsset> TraderIdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader|LookAt", meta = (ClampMin = "0.0"))
	float LookAtPlayerDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader|LookAt", meta = (ClampMin = "0.0"))
	float LookAtRotationInterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader|LookAt", meta = (ClampMin = "-180.0", ClampMax = "180.0"))
	float LookAtYawOffsetDegrees;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader|Visual")
	FVector TraderCharacterRelativeLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader|Visual")
	FRotator TraderCharacterRelativeRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trader|Visual")
	FVector TraderCharacterRelativeScale;

private:
	UFUNCTION()
	void HandleInteractionSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleInteractionSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	FRotator InitialActorRotation;
};
