// ============================================================================
//  ATraderNPC — торговец (NPC), у которого игрок продаёт руду и покупает зелья.
//  У торговца есть зона взаимодействия (InteractionSphere): когда игрок входит
//  в радиус, торговец "запоминается" в контроллере, появляется подсказка [E] и
//  можно открыть торговлю. Сам торговец плавно поворачивается лицом к игроку.
//
//  ЗАЩИТА — по этапам показа:
//   • Этап 7 (торговец): HandleInteractionSphereBeginOverlap()/EndOverlap() —
//     вход/выход игрока из зоны; BeginPlay() — подключение этих обработчиков;
//     Tick() — поворот к игроку. (Сама торговля — в персонаже: SellOre()/
//     BuyStaminaPotion(), вызывается через ExecuteMenuOption() контроллера.)
// ============================================================================
#include "Actors/TraderNPC.h"

#include "Animation/AnimationAsset.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameplayCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "ThiefPlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float DefaultRobotScoutYawCorrection = -90.0f;
}

ATraderNPC::ATraderNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TraderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TraderMesh"));
	TraderMesh->SetupAttachment(SceneRoot);
	TraderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TraderMesh->SetRelativeScale3D(FVector(0.9f, 0.9f, 2.0f));
	TraderMesh->SetHiddenInGame(true);
	TraderMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultMeshFinder.Succeeded())
	{
		TraderMesh->SetStaticMesh(DefaultMeshFinder.Object);
	}

	TraderCharacterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TraderCharacterMesh"));
	TraderCharacterMesh->SetupAttachment(SceneRoot);
	TraderCharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TraderCharacterMesh->SetGenerateOverlapEvents(false);
	TraderCharacterMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	TraderCharacterMesh->bEnableUpdateRateOptimizations = false;
	TraderCharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DefaultTraderMeshFinder(
		TEXT("/Game/Robot_scout_R_21/Mesh/SK_Robot_scout_R21.SK_Robot_scout_R21"));
	if (DefaultTraderMeshFinder.Succeeded())
	{
		TraderCharacterMesh->SetSkeletalMesh(DefaultTraderMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimationAsset> DefaultTraderIdleFinder(
		TEXT("/Game/Robot_scout_R_21/Demo/Animations/ThirdPersonIdle.ThirdPersonIdle"));
	if (DefaultTraderIdleFinder.Succeeded())
	{
		TraderIdleAnimation = DefaultTraderIdleFinder.Object;
	}

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(SceneRoot);
	InteractionSphere->SetSphereRadius(220.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	LookAtPlayerDistance = 420.0f;
	LookAtRotationInterpSpeed = 3.5f;
	LookAtYawOffsetDegrees = 0.0f;
	TraderCharacterRelativeLocation = FVector::ZeroVector;
	TraderCharacterRelativeRotation = FRotator(0.0f, DefaultRobotScoutYawCorrection, 0.0f);
	TraderCharacterRelativeScale = FVector(1.0f, 1.0f, 1.0f);
	TraderName = FText::FromString(TEXT("Trader"));
	InitialActorRotation = FRotator::ZeroRotator;
}

void ATraderNPC::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	AGameplayCharacterBase* PlayerCharacter = Cast<AGameplayCharacterBase>(UGameplayStatics::GetPlayerCharacter(this, 0));
	const bool bShouldTrackPlayer =
		IsValid(PlayerCharacter)
		&& LookAtPlayerDistance > 0.0f
		&& FVector::DistSquared2D(PlayerCharacter->GetActorLocation(), GetActorLocation()) <= FMath::Square(LookAtPlayerDistance);

	FRotator DesiredRotation = InitialActorRotation;
	if (bShouldTrackPlayer)
	{
		const FVector TraderLocation = GetActorLocation();
		FVector PlayerLocation = PlayerCharacter->GetActorLocation();
		PlayerLocation.Z = TraderLocation.Z;
		DesiredRotation = (PlayerLocation - TraderLocation).Rotation();
		DesiredRotation.Pitch = 0.0f;
		DesiredRotation.Roll = 0.0f;
		DesiredRotation.Yaw += LookAtYawOffsetDegrees;
		DesiredRotation.Normalize();
	}

	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, LookAtRotationInterpSpeed));
}

void ATraderNPC::BeginPlay()
{
	Super::BeginPlay();

	InitialActorRotation = GetActorRotation();
	TraderMesh->SetHiddenInGame(true);
	TraderMesh->SetVisibility(false);

	if (TraderCharacterMesh)
	{
		TraderCharacterMesh->SetRelativeLocation(TraderCharacterRelativeLocation);
		// Existing placed traders can still have the old zero rotation serialized in the map.
		const FRotator EffectiveRelativeRotation = TraderCharacterRelativeRotation.IsNearlyZero(0.01f)
			? FRotator(0.0f, DefaultRobotScoutYawCorrection, 0.0f)
			: TraderCharacterRelativeRotation;
		TraderCharacterMesh->SetRelativeRotation(EffectiveRelativeRotation);
		TraderCharacterMesh->SetRelativeScale3D(TraderCharacterRelativeScale);
		if (TraderIdleAnimation)
		{
			TraderCharacterMesh->SetAnimation(TraderIdleAnimation);
			TraderCharacterMesh->PlayAnimation(TraderIdleAnimation, true);
		}
	}

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATraderNPC::HandleInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ATraderNPC::HandleInteractionSphereEndOverlap);
}

FText ATraderNPC::GetTraderName() const
{
	return TraderName;
}

void ATraderNPC::HandleInteractionSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (AGameplayCharacterBase* PlayerCharacter = Cast<AGameplayCharacterBase>(OtherActor))
	{
		if (AThiefPlayerController* ThiefController = Cast<AThiefPlayerController>(PlayerCharacter->GetController()))
		{
			ThiefController->SetNearbyTrader(this);
		}
	}
}

void ATraderNPC::HandleInteractionSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (AGameplayCharacterBase* PlayerCharacter = Cast<AGameplayCharacterBase>(OtherActor))
	{
		if (AThiefPlayerController* ThiefController = Cast<AThiefPlayerController>(PlayerCharacter->GetController()))
		{
			ThiefController->ClearNearbyTrader(this);
		}
	}
}
