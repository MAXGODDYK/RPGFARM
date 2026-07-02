// ============================================================================
//  AOreStoneBase — рудный камень, который добывает игрок.
//  Класс хранит здоровье руды и отвечает за получение урона от кирки,
//  разрушение, повторное появление (respawn) и полосу здоровья, которая видна
//  только когда игрок рядом. При разрушении выдаёт ресурс и опыт персонажу.
//  Числа настраиваются в BP_OreStone → Class Defaults (без правки кода).
//
//  ЗАЩИТА — по этапам показа:
//   • Этап 6 (добыча руды): ApplyDamageToOre() — урон; BreakOre() — разрушение и
//     запуск таймера respawn; RespawnOre() — возврат руды; GetHealthPercent()/
//     RefreshHealthBar() — полоса здоровья.
//   Параметры: MaxHealth, RespawnDelayMinutes (минуты), OreResourceReward,
//   OreExperienceReward.
// ============================================================================
#include "Actors/OreStoneBase.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ThiefPlayerController.h"
#include "TimerManager.h"
#include "UI/OreHealthBarWidget.h"

AOreStoneBase::AOreStoneBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	OreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OreMesh"));
	OreMesh->SetupAttachment(SceneRoot);

	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidgetComponent->SetupAttachment(SceneRoot);
	HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidgetComponent->SetDrawAtDesiredSize(false);
	HealthBarWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HealthBarWidgetComponent->SetTwoSided(true);
	HealthBarWidgetComponent->SetWidgetClass(UOreHealthBarWidget::StaticClass());
	HealthBarWidgetComponent->SetVisibility(false);

	HealthBarTriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HealthBarTriggerSphere"));
	HealthBarTriggerSphere->SetupAttachment(SceneRoot);
	HealthBarTriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HealthBarTriggerSphere->SetCollisionObjectType(ECC_WorldDynamic);
	HealthBarTriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	HealthBarTriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HealthBarTriggerSphere->SetGenerateOverlapEvents(true);

	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	HealthBarOffset = FVector(0.0f, 0.0f, 160.0f);
	HealthBarWidth = 320.0f;
	HealthBarHeight = 72.0f;
	HealthBarVisibleDistance = 500.0f;
	HealthBarTriggerRadius = 500.0f;
	RespawnDelayMinutes = 60.0f;
	OreResourceReward = 1;
	OreExperienceReward = 25;
	bOreAvailable = true;
	bPlayerInHealthBarRange = false;
}

// [этап 6] Получаю урон от удара киркой: уменьшаю здоровье, обновляю полосу; если
// здоровье кончилось — вызываю разрушение руды.
void AOreStoneBase::ApplyDamageToOre(float DamageAmount)
{
	if (!bOreAvailable || DamageAmount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		BreakOre();
		return;
	}

	RefreshHealthBar();
	UpdateHealthBarVisibility();
}

void AOreStoneBase::ResetOreHealth()
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	bOreAvailable = true;
	CurrentHealth = MaxHealth;

	if (OreMesh)
	{
		OreMesh->SetVisibility(true, true);
		OreMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	RefreshHealthBar();
	RefreshPlayerInHealthBarRange();
	UpdateHealthBarVisibility();
}

float AOreStoneBase::GetHealthPercent() const
{
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}

	return CurrentHealth / MaxHealth;
}

float AOreStoneBase::GetCurrentHealth() const
{
	return CurrentHealth;
}

bool AOreStoneBase::IsOreAvailable() const
{
	return bOreAvailable;
}

int32 AOreStoneBase::GetOreResourceReward() const
{
	return OreResourceReward;
}

int32 AOreStoneBase::GetOreExperienceReward() const
{
	return OreExperienceReward;
}

void AOreStoneBase::BeginPlay()
{
	Super::BeginPlay();

	if (bOreAvailable)
	{
		CurrentHealth = MaxHealth;
	}
	else
	{
		CurrentHealth = 0.0f;
	}

	if (HealthBarTriggerSphere)
	{
		HealthBarTriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AOreStoneBase::HandleHealthBarTriggerBeginOverlap);
		HealthBarTriggerSphere->OnComponentEndOverlap.AddDynamic(this, &AOreStoneBase::HandleHealthBarTriggerEndOverlap);
	}

	RefreshHealthBar();
	RefreshPlayerInHealthBarRange();
	UpdateHealthBarVisibility();
}

void AOreStoneBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bOreAvailable)
	{
		CurrentHealth = MaxHealth;
	}
	else
	{
		CurrentHealth = 0.0f;
	}

	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->SetRelativeLocation(HealthBarOffset);
		HealthBarWidgetComponent->SetDrawSize(FVector2D(HealthBarWidth, HealthBarHeight));
		HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		HealthBarWidgetComponent->SetWidgetClass(UOreHealthBarWidget::StaticClass());
		HealthBarWidgetComponent->SetVisibility(false);
	}

	if (HealthBarTriggerSphere)
	{
		HealthBarTriggerSphere->SetSphereRadius(FMath::Max(HealthBarTriggerRadius, HealthBarVisibleDistance));
	}

	RefreshHealthBar();
	UpdateHealthBarVisibility();
}

void AOreStoneBase::RefreshHealthBar()
{
	if (!HealthBarWidgetComponent)
	{
		return;
	}

	HealthBarWidgetComponent->InitWidget();

	if (UOreHealthBarWidget* HealthBarWidget = Cast<UOreHealthBarWidget>(HealthBarWidgetComponent->GetUserWidgetObject()))
	{
		HealthBarWidget->SetHealthValues(CurrentHealth, MaxHealth);
	}
}

void AOreStoneBase::RefreshHealthBarVisibilityState()
{
	RefreshPlayerInHealthBarRange();
	UpdateHealthBarVisibility();
}

void AOreStoneBase::UpdateHealthBarVisibility()
{
	if (!HealthBarWidgetComponent)
	{
		return;
	}

	if (!bOreAvailable)
	{
		HealthBarWidgetComponent->SetVisibility(false);
		return;
	}

	if (const AThiefPlayerController* ThiefController = Cast<AThiefPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		if (ThiefController->IsAnyModalOpen() || ThiefController->IsLoadingGameplayMap())
		{
			HealthBarWidgetComponent->SetVisibility(false);
			return;
		}
	}

	HealthBarWidgetComponent->SetVisibility(bPlayerInHealthBarRange);
}

void AOreStoneBase::RefreshPlayerInHealthBarRange()
{
	if (!bOreAvailable || !HealthBarTriggerSphere)
	{
		bPlayerInHealthBarRange = false;
		return;
	}

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		bPlayerInHealthBarRange = HealthBarTriggerSphere->IsOverlappingActor(PlayerPawn);
		return;
	}

	bPlayerInHealthBarRange = false;
}

// [этап 6] Разрушение руды: прячу меш, отключаю коллизию и полосу, запускаю
// таймер на повторное появление (respawn) через RespawnDelayMinutes.
void AOreStoneBase::BreakOre()
{
	bOreAvailable = false;
	CurrentHealth = 0.0f;
	RefreshHealthBar();

	if (OreMesh)
	{
		OreMesh->SetVisibility(false, true);
		OreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->SetVisibility(false);
	}

	bPlayerInHealthBarRange = false;

	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);

	if (RespawnDelayMinutes <= 0.0f)
	{
		RespawnOre();
		return;
	}

	GetWorldTimerManager().SetTimer(
		RespawnTimerHandle,
		this,
		&AOreStoneBase::RespawnOre,
		RespawnDelayMinutes * 60.0f,
		false);
}

// [этап 6] Повторное появление руды: возвращаю полное здоровье, показываю меш и
// включаю коллизию — камень снова можно добывать.
void AOreStoneBase::RespawnOre()
{
	bOreAvailable = true;
	CurrentHealth = MaxHealth;

	if (OreMesh)
	{
		OreMesh->SetVisibility(true, true);
		OreMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	RefreshHealthBar();
	RefreshPlayerInHealthBarRange();
	UpdateHealthBarVisibility();
}

void AOreStoneBase::HandleHealthBarTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bOreAvailable)
	{
		return;
	}

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (OtherActor == PlayerPawn)
		{
			bPlayerInHealthBarRange = true;
			RefreshHealthBar();
			UpdateHealthBarVisibility();
		}
	}
}

void AOreStoneBase::HandleHealthBarTriggerEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (OtherActor == PlayerPawn)
		{
			bPlayerInHealthBarRange = false;
			UpdateHealthBarVisibility();
		}
	}
}
