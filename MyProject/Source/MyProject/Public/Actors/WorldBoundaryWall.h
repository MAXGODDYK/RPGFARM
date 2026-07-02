#pragma once

// ============================================================================
//  AWorldBoundaryWall (.h) — невидимый барьер по краям карты.
//  ЗАЩИТА — по этапу 11 (границы мира): RebuildWalls() строит 4 коллизионные
//  стены по периметру. Спавнится автоматически из AThiefCatcerGameMode; вторая
//  страховка от падения — возврат на старт в AGameplayCharacterBase::Tick.
// ============================================================================

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldBoundaryWall.generated.h"

class UBoxComponent;

/**
 * Invisible collision barrier that surrounds the playable area with four walls so
 * the character can never fall off the edge of the world. Place one in the level,
 * move it to the centre of the playable area and size it with BoundaryHalfExtent.
 * The walls are hidden in game and only block movement (Pawn / physics).
 */
UCLASS()
class MYPROJECT_API AWorldBoundaryWall : public AActor
{
	GENERATED_BODY()

public:
	AWorldBoundaryWall();

	virtual void OnConstruction(const FTransform& Transform) override;

	// Half-size of the playable rectangle in X (east-west) and Y (north-south), in
	// world units. The walls are built just outside this rectangle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary", meta = (ClampMin = "100.0"))
	FVector2D BoundaryHalfExtent = FVector2D(20000.0f, 20000.0f);

	// How tall the walls are so the character cannot jump over them.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary", meta = (ClampMin = "100.0"))
	float WallHeight = 6000.0f;

	// How thick each wall is.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary", meta = (ClampMin = "10.0"))
	float WallThickness = 400.0f;

	// How far below the actor the walls start, so they cover dips in the terrain.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boundary", meta = (ClampMin = "0.0"))
	float WallDepthBelow = 2000.0f;

private:
	void RebuildWalls();

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TObjectPtr<UBoxComponent> WallNorth;

	UPROPERTY()
	TObjectPtr<UBoxComponent> WallSouth;

	UPROPERTY()
	TObjectPtr<UBoxComponent> WallEast;

	UPROPERTY()
	TObjectPtr<UBoxComponent> WallWest;

	void ConfigureWall(UBoxComponent* Wall, const FVector& RelativeLocation, const FVector& BoxExtent) const;
};
