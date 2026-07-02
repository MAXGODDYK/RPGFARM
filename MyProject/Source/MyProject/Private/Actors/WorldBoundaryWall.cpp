// ============================================================================
//  AWorldBoundaryWall — невидимый барьер по краям карты.
//  Строит из 4 коллизионных стен прямоугольник по периметру игровой зоны, чтобы
//  персонаж не выпал за границу мира. В игре стены невидимы — только блокируют
//  движение. Спавнится автоматически из AThiefCatcerGameMode под размер карты.
//
//  ЗАЩИТА — по этапам показа:
//   • Этап 11 (границы мира): RebuildWalls() — строит 4 стены по размеру
//     BoundaryHalfExtent и высоте WallHeight. (Вторая страховка — возврат при
//     падении в AGameplayCharacterBase::Tick.)
// ============================================================================
#include "Actors/WorldBoundaryWall.h"

#include "Components/BoxComponent.h"

AWorldBoundaryWall::AWorldBoundaryWall()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WallNorth = CreateDefaultSubobject<UBoxComponent>(TEXT("WallNorth"));
	WallSouth = CreateDefaultSubobject<UBoxComponent>(TEXT("WallSouth"));
	WallEast = CreateDefaultSubobject<UBoxComponent>(TEXT("WallEast"));
	WallWest = CreateDefaultSubobject<UBoxComponent>(TEXT("WallWest"));

	for (UBoxComponent* Wall : { WallNorth, WallSouth, WallEast, WallWest })
	{
		if (!Wall)
		{
			continue;
		}

		Wall->SetupAttachment(SceneRoot);
		Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Wall->SetCollisionObjectType(ECC_WorldStatic);
		Wall->SetCollisionResponseToAllChannels(ECR_Block);
		Wall->SetCanEverAffectNavigation(false);
		Wall->SetGenerateOverlapEvents(false);
		Wall->SetHiddenInGame(true);
		Wall->bDrawOnlyIfSelected = false;
		Wall->ShapeColor = FColor(255, 80, 80, 255);
	}

	RebuildWalls();
}

void AWorldBoundaryWall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildWalls();
}

void AWorldBoundaryWall::ConfigureWall(UBoxComponent* Wall, const FVector& RelativeLocation, const FVector& BoxExtent) const
{
	if (!Wall)
	{
		return;
	}

	Wall->SetRelativeLocation(RelativeLocation);
	Wall->SetBoxExtent(BoxExtent, false);
}

void AWorldBoundaryWall::RebuildWalls()
{
	const float HalfX = FMath::Max(BoundaryHalfExtent.X, 100.0f);
	const float HalfY = FMath::Max(BoundaryHalfExtent.Y, 100.0f);
	const float HalfThickness = FMath::Max(WallThickness, 10.0f) * 0.5f;
	const float TotalHeight = FMath::Max(WallHeight, 100.0f) + FMath::Max(WallDepthBelow, 0.0f);
	const float HalfHeight = TotalHeight * 0.5f;
	// Centre the wall vertically so it extends WallDepthBelow downward and WallHeight up.
	const float CenterZ = (WallHeight * 0.5f) - (WallDepthBelow * 0.5f);

	// North / South run along X, offset on +/-Y.
	const FVector NorthSouthExtent(HalfX + WallThickness, HalfThickness, HalfHeight);
	ConfigureWall(WallNorth, FVector(0.0f, HalfY + HalfThickness, CenterZ), NorthSouthExtent);
	ConfigureWall(WallSouth, FVector(0.0f, -(HalfY + HalfThickness), CenterZ), NorthSouthExtent);

	// East / West run along Y, offset on +/-X.
	const FVector EastWestExtent(HalfThickness, HalfY + WallThickness, HalfHeight);
	ConfigureWall(WallEast, FVector(HalfX + HalfThickness, 0.0f, CenterZ), EastWestExtent);
	ConfigureWall(WallWest, FVector(-(HalfX + HalfThickness), 0.0f, CenterZ), EastWestExtent);
}
