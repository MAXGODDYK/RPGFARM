// ============================================================================
//  AThiefCatcerGameMode — игровой режим (правила сцены).
//  Задаёт, какой персонаж, контроллер и HUD используются. В лобби (NewMap)
//  намеренно НЕ создаёт персонажа — там только меню. На игровой карте Showcase
//  автоматически спавнит невидимый барьер мира по размерам ландшафта.
//
//  ЗАЩИТА — по этапам показа:
//   • Этап 1 (лобби): SpawnDefaultPawnFor_Implementation() — почему в меню нет
//     персонажа (на карте NewMap пешка не создаётся).
//   • Этап 11 (границы мира): BeginPlay() — спавн барьера AWorldBoundaryWall.
// ============================================================================
#include "ThiefCatcerGameMode.h"

#include "Actors/WorldBoundaryWall.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "ThiefPlayerController.h"
#include "UI/PlayerGameHUD.h"
#include "UObject/ConstructorHelpers.h"

AThiefCatcerGameMode::AThiefCatcerGameMode() : Super()
{
	ConstructorHelpers::FClassFinder<APawn> SandboxCharacterThiefCatcher(TEXT("/Game/Blueprints/BP_MyThifCatcher_Sandbox"));
	if (SandboxCharacterThiefCatcher.Succeeded())
	{
		DefaultPawnClass = SandboxCharacterThiefCatcher.Class;
	}
	else
	{
		ConstructorHelpers::FClassFinder<APawn> MainCharacterThiefCatcher(TEXT("/Game/Blueprints/Bp_MyThifCatcher"));
		if (MainCharacterThiefCatcher.Succeeded())
		{
			DefaultPawnClass = MainCharacterThiefCatcher.Class;
		}
	}

	PlayerControllerClass = AThiefPlayerController::StaticClass();
	HUDClass = APlayerGameHUD::StaticClass();
}

void AThiefCatcerGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!bSpawnWorldBoundary)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// No boundary in the lobby map.
	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix);
	if (FPackageName::GetShortName(MapName).Equals(TEXT("NewMap"), ESearchCase::IgnoreCase))
	{
		return;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, WorldBoundaryCenter);
	AWorldBoundaryWall* Boundary = World->SpawnActorDeferred<AWorldBoundaryWall>(
		AWorldBoundaryWall::StaticClass(),
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Boundary)
	{
		Boundary->BoundaryHalfExtent = WorldBoundaryHalfExtent;
		Boundary->WallHeight = WorldBoundaryHeight;
		Boundary->FinishSpawning(SpawnTransform);
	}
}

APawn* AThiefCatcerGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	if (GetWorld())
	{
		FString MapName = GetWorld()->GetMapName();
		MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
		if (FPackageName::GetShortName(MapName).Equals(TEXT("NewMap"), ESearchCase::IgnoreCase))
		{
			return nullptr;
		}
	}

	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}
