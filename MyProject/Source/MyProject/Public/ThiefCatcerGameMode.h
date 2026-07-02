// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// ============================================================================
//  AThiefCatcerGameMode (.h) — интерфейс игрового режима. Ниже — настройки
//  барьера мира и объявления функций режима. Реализация — в .cpp.
//
//  ЗАЩИТА — по этапам: 1 (SpawnDefaultPawnFor — нет персонажа в лобби),
//  11 (BeginPlay — авто-спавн барьера AWorldBoundaryWall).
// ============================================================================

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ThiefCatcerGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API AThiefCatcerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AThiefCatcerGameMode();

	virtual void BeginPlay() override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

protected:
	// Spawns an invisible boundary around the Showcase terrain so the player cannot
	// fall off the edge of the world. Values are sized to the Namaqualand landscape.
	UPROPERTY(EditDefaultsOnly, Category = "World Boundary")
	bool bSpawnWorldBoundary = true;

	UPROPERTY(EditDefaultsOnly, Category = "World Boundary")
	FVector WorldBoundaryCenter = FVector(0.0f, 1575.0f, 100.0f);

	UPROPERTY(EditDefaultsOnly, Category = "World Boundary")
	FVector2D WorldBoundaryHalfExtent = FVector2D(6200.0f, 7800.0f);

	UPROPERTY(EditDefaultsOnly, Category = "World Boundary", meta = (ClampMin = "100.0"))
	float WorldBoundaryHeight = 10000.0f;
};
