// ============================================================================
//  AThiefPlayerController — контроллер игрока: ввод, меню, настройки, карты.
//  Через него проходит весь ввод и состояния интерфейса. Я держу логику меню
//  отдельно от персонажа, чтобы не смешивать управление вводом с игровым
//  поведением. Также хранит и сохраняет настройки и переназначение клавиш.
//
//  ЗАЩИТА — по этапам показа (какие функции открывать):
//   • Этап 1 (меню): HandleLeftClick(), InputKey() — клики/клавиши по интерфейсу.
//   • Этап 2 (настройки): AdjustMasterVolume()/…, SetSettingValueFromRatio()
//     (ползунки), CycleLanguage(), BeginRebindingInput()/ApplyBindingKey(),
//     Load/SaveSettingsToJson().
//   • Этап 3 (старт→загрузка): StartGameplayFromMenu(), OpenGameplayMap(), ReturnToLobby().
//   • Этап 7 (торговля): ToggleTradeMenu(), ExecuteMenuOption().
//   • Этап 8 (окно прокачки): ToggleProgressionMenu().
//   Общее: ApplyMenuInputState() — переключение режима ввода: меню ↔ игра.
// ============================================================================
#include "ThiefPlayerController.h"

#include "Actors/OreStoneBase.h"
#include "Actors/TraderNPC.h"
#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Save/MyProjectJsonSaveUtils.h"
#include "Systems/WorkOrderSubsystem.h"
#include "GameplayCharacterBase.h"
#include "UI/PlayerGameHUD.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"

namespace
{
	constexpr float DefaultMasterVolume = 1.0f;
	constexpr float DefaultMusicVolume = 0.65f;
	constexpr float DefaultSfxVolume = 1.0f;
	constexpr float DefaultMenuScale = 1.0f;
	constexpr float DefaultLookSensitivity = 0.07f;
	constexpr float MinMenuScale = 0.85f;
	constexpr float MaxMenuScale = 1.25f;
	constexpr float MinLookSensitivity = 0.02f;
	constexpr float MaxLookSensitivity = 0.20f;
	const FName MoveForwardBackwardAxisName(TEXT("MoveForwardBackward"));
	const FName MoveRightLeftAxisName(TEXT("MoveRightLeft"));
	const FName LookUpDownAxisName(TEXT("lookUpDown"));
	const FName MenuConfirmActionName(TEXT("MenuConfirm"));
	const FName MenuBackActionName(TEXT("MenuBack"));
	const FName InteractActionName(TEXT("Interact"));
	const FName ToggleTradeActionName(TEXT("ToggleTradeMenu"));
	const FName ToggleProgressionActionName(TEXT("ToggleProgressionMenu"));
	const FName MenuOptionOneActionName(TEXT("MenuOptionOne"));
	const FName MenuOptionTwoActionName(TEXT("MenuOptionTwo"));
	const FName MenuOptionThreeActionName(TEXT("MenuOptionThree"));

	struct FRemappableBindingDefinition
	{
		ERemappableInputAction InputAction = ERemappableInputAction::None;
		FName MappingName = NAME_None;
		FKey DefaultKey;
		bool bIsAxis = false;
		float AxisScale = 1.0f;
		const TCHAR* EnglishLabel = TEXT("");
		const TCHAR* RussianLabel = TEXT("");
		const TCHAR* EnglishDescription = TEXT("");
		const TCHAR* RussianDescription = TEXT("");
	};

	FString GetShortMapName(const UWorld* World)
	{
		if (!World)
		{
			return FString();
		}

		FString MapName = World->GetMapName();
		MapName.RemoveFromStart(World->StreamingLevelsPrefix);
		return FPackageName::GetShortName(MapName);
	}

	const TArray<FRemappableBindingDefinition>& GetRemappableBindingDefinitions()
	{
		static const TArray<FRemappableBindingDefinition> Definitions =
		{
			{ ERemappableInputAction::MoveForward, MoveForwardBackwardAxisName, EKeys::W, true, 1.0f, TEXT("Move Forward"), TEXT("Шаг вперёд"), TEXT("Move the character forward"), TEXT("Движение персонажа вперёд") },
			{ ERemappableInputAction::MoveBackward, MoveForwardBackwardAxisName, EKeys::S, true, -1.0f, TEXT("Move Backward"), TEXT("Шаг назад"), TEXT("Move the character backward"), TEXT("Движение персонажа назад") },
			{ ERemappableInputAction::MoveRight, MoveRightLeftAxisName, EKeys::D, true, 1.0f, TEXT("Move Right"), TEXT("Шаг вправо"), TEXT("Move the character right"), TEXT("Движение персонажа вправо") },
			{ ERemappableInputAction::MoveLeft, MoveRightLeftAxisName, EKeys::A, true, -1.0f, TEXT("Move Left"), TEXT("Шаг влево"), TEXT("Move the character left"), TEXT("Движение персонажа влево") },
			{ ERemappableInputAction::Jump, TEXT("Jump"), EKeys::SpaceBar, false, 1.0f, TEXT("Jump"), TEXT("Прыжок"), TEXT("Jump over rocks and terrain"), TEXT("Прыжок через препятствия") },
			{ ERemappableInputAction::Sprint, TEXT("Sprint"), EKeys::LeftShift, false, 1.0f, TEXT("Sprint"), TEXT("Спринт"), TEXT("Run faster while stamina lasts"), TEXT("Быстрое движение за счёт стамины") },
			{ ERemappableInputAction::Attack, TEXT("Attack"), EKeys::LeftMouseButton, false, 1.0f, TEXT("Attack"), TEXT("Удар"), TEXT("Swing the pickaxe and damage ore"), TEXT("Удар киркой по руде") },
			{ ERemappableInputAction::Interact, InteractActionName, EKeys::E, false, 1.0f, TEXT("Interact"), TEXT("Взаимодействие"), TEXT("Interact with the trader"), TEXT("Взаимодействие с торговцем") },
			{ ERemappableInputAction::UsePotion, TEXT("UsePotion"), EKeys::Q, false, 1.0f, TEXT("Use Potion"), TEXT("Использовать зелье"), TEXT("Drink a stored stamina potion"), TEXT("Выпить сохранённое зелье стамины") },
			{ ERemappableInputAction::ToggleTradeMenu, ToggleTradeActionName, EKeys::B, false, 1.0f, TEXT("Quick Trade"), TEXT("Быстрая торговля"), TEXT("Open the trade menu near the trader"), TEXT("Открытие торговли рядом с NPC") },
			{ ERemappableInputAction::ToggleProgressionMenu, ToggleProgressionActionName, EKeys::P, false, 1.0f, TEXT("Progression"), TEXT("Прокачка"), TEXT("Open the character progression panel"), TEXT("Открытие окна прокачки") },
			{ ERemappableInputAction::MenuConfirm, MenuConfirmActionName, EKeys::Enter, false, 1.0f, TEXT("Confirm / Start"), TEXT("Подтвердить / Старт"), TEXT("Start the game or confirm a menu action"), TEXT("Старт игры или подтверждение меню") },
			{ ERemappableInputAction::MenuBack, MenuBackActionName, EKeys::Escape, false, 1.0f, TEXT("Back / Pause"), TEXT("Назад / Пауза"), TEXT("Open settings, go back, or pause"), TEXT("Открытие настроек, возврат или пауза") },
			{ ERemappableInputAction::MenuOptionOne, MenuOptionOneActionName, EKeys::One, false, 1.0f, TEXT("Menu Option 1"), TEXT("Пункт меню 1"), TEXT("Activate the first menu shortcut"), TEXT("Быстрый выбор первого пункта") },
			{ ERemappableInputAction::MenuOptionTwo, MenuOptionTwoActionName, EKeys::Two, false, 1.0f, TEXT("Menu Option 2"), TEXT("Пункт меню 2"), TEXT("Activate the second menu shortcut"), TEXT("Быстрый выбор второго пункта") },
			{ ERemappableInputAction::MenuOptionThree, MenuOptionThreeActionName, EKeys::Three, false, 1.0f, TEXT("Menu Option 3"), TEXT("Пункт меню 3"), TEXT("Activate the third menu shortcut"), TEXT("Быстрый выбор третьего пункта") }
		};

		return Definitions;
	}

	const FRemappableBindingDefinition* FindBindingDefinition(const ERemappableInputAction InputAction)
	{
		for (const FRemappableBindingDefinition& Definition : GetRemappableBindingDefinitions())
		{
			if (Definition.InputAction == InputAction)
			{
				return &Definition;
			}
		}

		return nullptr;
	}

	FInputActionKeyMapping MakeActionMapping(const FName MappingName, const FKey& Key)
	{
		return FInputActionKeyMapping(MappingName, Key);
	}

	FInputAxisKeyMapping MakeAxisMapping(const FName MappingName, const FKey& Key, const float Scale)
	{
		return FInputAxisKeyMapping(MappingName, Key, Scale);
	}

	FKey GetBindingKeyFromInputSettings(const UInputSettings* InputSettings, const FRemappableBindingDefinition& Definition)
	{
		if (!InputSettings)
		{
			return Definition.DefaultKey;
		}

		if (Definition.bIsAxis)
		{
			TArray<FInputAxisKeyMapping> ExistingMappings;
			InputSettings->GetAxisMappingByName(Definition.MappingName, ExistingMappings);
			for (const FInputAxisKeyMapping& ExistingMapping : ExistingMappings)
			{
				if (FMath::IsNearlyEqual(ExistingMapping.Scale, Definition.AxisScale))
				{
					return ExistingMapping.Key;
				}
			}
		}
		else
		{
			TArray<FInputActionKeyMapping> ExistingMappings;
			InputSettings->GetActionMappingByName(Definition.MappingName, ExistingMappings);
			if (ExistingMappings.Num() > 0)
			{
				return ExistingMappings[0].Key;
			}
		}

		return Definition.DefaultKey;
	}

	void SetBindingKeyOnInputSettings(UInputSettings* InputSettings, const FRemappableBindingDefinition& Definition, const FKey& NewKey)
	{
		if (!InputSettings || !NewKey.IsValid())
		{
			return;
		}

		if (Definition.bIsAxis)
		{
			TArray<FInputAxisKeyMapping> ExistingMappings;
			InputSettings->GetAxisMappingByName(Definition.MappingName, ExistingMappings);
			for (const FInputAxisKeyMapping& ExistingMapping : ExistingMappings)
			{
				if (FMath::IsNearlyEqual(ExistingMapping.Scale, Definition.AxisScale))
				{
					InputSettings->RemoveAxisMapping(ExistingMapping, false);
				}
			}

			InputSettings->AddAxisMapping(MakeAxisMapping(Definition.MappingName, NewKey, Definition.AxisScale), false);
			return;
		}

		TArray<FInputActionKeyMapping> ExistingMappings;
		InputSettings->GetActionMappingByName(Definition.MappingName, ExistingMappings);
		for (const FInputActionKeyMapping& ExistingMapping : ExistingMappings)
		{
			InputSettings->RemoveActionMapping(ExistingMapping, false);
		}

		InputSettings->AddActionMapping(MakeActionMapping(Definition.MappingName, NewKey), false);
	}

	void SetAxisSensitivity(UInputSettings* InputSettings, const FName AxisKeyName, const float Sensitivity)
	{
		if (!InputSettings)
		{
			return;
		}

		for (FInputAxisConfigEntry& AxisConfigEntry : InputSettings->AxisConfig)
		{
			if (AxisConfigEntry.AxisKeyName == AxisKeyName)
			{
				AxisConfigEntry.AxisProperties.Sensitivity = Sensitivity;
				return;
			}
		}

		FInputAxisConfigEntry NewEntry;
		NewEntry.AxisKeyName = AxisKeyName;
		NewEntry.AxisProperties = FInputAxisProperties();
		NewEntry.AxisProperties.Sensitivity = Sensitivity;
		InputSettings->AxisConfig.Add(NewEntry);
	}

	bool IsValidBindingKey(const FKey& Key)
	{
		if (!Key.IsValid() || Key == EKeys::Invalid || Key == EKeys::AnyKey)
		{
			return false;
		}

		if (Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D() || Key.IsTouch())
		{
			return false;
		}

		return Key.IsBindableToActions() || Key.IsMouseButton();
	}

	FString GetLocalizedString(const AThiefPlayerController* ThiefController, const TCHAR* EnglishText, const TCHAR* RussianText)
	{
		if (ThiefController && ThiefController->IsRussianLanguage())
		{
			return FString(RussianText);
		}

		return FString(EnglishText);
	}
}

AThiefPlayerController::AThiefPlayerController()
{
	bTradeMenuOpen = false;
	bProgressionMenuOpen = false;
	bPauseMenuOpen = false;
	bSettingsMenuOpen = false;
	bLoadingGameplayMap = false;
	bWaitingForInputRebind = false;
	MenuMapName = TEXT("NewMap");
	GameplayMapName = TEXT("/Game/Namaqualand/Levels/Showcase");
	LoadingScreenDelay = 0.15f;
	LobbyMusicVolume = DefaultMusicVolume;
	MasterVolumeSetting = DefaultMasterVolume;
	MusicVolumeSetting = LobbyMusicVolume;
	SfxVolumeSetting = DefaultSfxVolume;
	MenuScaleSetting = DefaultMenuScale;
	LookSensitivitySetting = DefaultLookSensitivity;
	bInvertLookY = false;
	CurrentLanguage = EGameLanguage::Russian;
	ActiveSettingsTab = ESettingsPanelTab::Audio;
	PendingInputRebindAction = ERemappableInputAction::None;
	GoldPerOre = 10;
	StaminaPotionCost = 25;
	StaminaPotionRestoreAmount = 40.0f;

	static ConstructorHelpers::FObjectFinder<USoundBase> LobbyMusicFinder(
		TEXT("/Game/Audio/Ambient/Starter_Music01.Starter_Music01"));
	if (LobbyMusicFinder.Succeeded())
	{
		LobbyMusic = LobbyMusicFinder.Object;
	}
}

void AThiefPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!GetHUD() || !GetHUD()->IsA(APlayerGameHUD::StaticClass()))
	{
		ClientSetHUD(APlayerGameHUD::StaticClass());
	}

	LoadSettingsFromJson();
	bLoadingGameplayMap = false;
	ApplyMenuInputState();
	ApplyLookSettings();
	ApplyAudioSettings();

	GetWorldTimerManager().ClearTimer(AudioRefreshTimerHandle);
	GetWorldTimerManager().SetTimer(
		AudioRefreshTimerHandle,
		this,
		&AThiefPlayerController::ApplyWorldAudioVolumes,
		1.0f,
		true,
		0.25f);
}

void AThiefPlayerController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bLoadingGameplayMap || IsInMenuMap())
	{
		return;
	}

	if (UWorkOrderSubsystem* WorkOrderSubsystem = GetWorkOrderSubsystem())
	{
		if (AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter())
		{
			WorkOrderSubsystem->TryStartPendingShift(PlayerCharacter, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
			WorkOrderSubsystem->UpdateTrackedShift(PlayerCharacter, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
			WorkOrderSubsystem->ApplyActiveShiftEffects(PlayerCharacter);
			if (WorkOrderSubsystem->HasResolvedShiftResult())
			{
				ApplyMenuInputState();
			}
		}
	}
}

void AThiefPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	FInputActionBinding& ConfirmBinding = InputComponent->BindAction(MenuConfirmActionName, IE_Pressed, this, &AThiefPlayerController::HandlePrimaryConfirm);
	ConfirmBinding.bConsumeInput = true;
	ConfirmBinding.bExecuteWhenPaused = true;

	FInputActionBinding& BackBinding = InputComponent->BindAction(MenuBackActionName, IE_Pressed, this, &AThiefPlayerController::HandleBackAction);
	BackBinding.bConsumeInput = true;
	BackBinding.bExecuteWhenPaused = true;

	FInputKeyBinding& LeftClickBinding = InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AThiefPlayerController::HandleLeftClick);
	LeftClickBinding.bConsumeInput = true;
	LeftClickBinding.bExecuteWhenPaused = true;

	FInputActionBinding& AttackBinding = InputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &AThiefPlayerController::HandleAttackAction);
	AttackBinding.bConsumeInput = true;

	FInputActionBinding& InteractBinding = InputComponent->BindAction(InteractActionName, IE_Pressed, this, &AThiefPlayerController::HandleInteractAction);
	InteractBinding.bConsumeInput = true;

	FInputActionBinding& TradeBinding = InputComponent->BindAction(ToggleTradeActionName, IE_Pressed, this, &AThiefPlayerController::ToggleTradeMenu);
	TradeBinding.bConsumeInput = true;

	FInputActionBinding& ProgressionBinding = InputComponent->BindAction(ToggleProgressionActionName, IE_Pressed, this, &AThiefPlayerController::ToggleProgressionMenu);
	ProgressionBinding.bConsumeInput = true;

	FInputActionBinding& OptionOneBinding = InputComponent->BindAction(MenuOptionOneActionName, IE_Pressed, this, &AThiefPlayerController::HandleOptionOne);
	OptionOneBinding.bConsumeInput = true;
	OptionOneBinding.bExecuteWhenPaused = true;

	FInputActionBinding& OptionTwoBinding = InputComponent->BindAction(MenuOptionTwoActionName, IE_Pressed, this, &AThiefPlayerController::HandleOptionTwo);
	OptionTwoBinding.bConsumeInput = true;
	OptionTwoBinding.bExecuteWhenPaused = true;

	FInputActionBinding& OptionThreeBinding = InputComponent->BindAction(MenuOptionThreeActionName, IE_Pressed, this, &AThiefPlayerController::HandleOptionThree);
	OptionThreeBinding.bConsumeInput = true;
	OptionThreeBinding.bExecuteWhenPaused = true;
}

bool AThiefPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	if (bWaitingForInputRebind && Params.Event == IE_Pressed)
	{
		if (Params.Key == EKeys::Escape)
		{
			CancelInputRebind();
			return true;
		}

		return TryCaptureInputRebind(Params.Key);
	}

	if (!bWaitingForInputRebind && Params.Event == IE_Pressed && Params.Key == EKeys::Escape)
	{
		HandleBackAction();
		return true;
	}

	if (!bWaitingForInputRebind
		&& Params.Event == IE_Pressed
		&& (Params.Key == EKeys::MouseScrollUp || Params.Key == EKeys::MouseScrollDown))
	{
		if (APlayerGameHUD* PlayerHUD = Cast<APlayerGameHUD>(GetHUD()))
		{
			const float WheelDelta = Params.Key == EKeys::MouseScrollDown ? 1.0f : -1.0f;
			if (PlayerHUD->HandleScroll(WheelDelta))
			{
				return true;
			}
		}
	}

	if (!bWaitingForInputRebind && Params.Event == IE_Pressed && Params.Key == EKeys::LeftMouseButton)
	{
		if (IsAnyModalOpen())
		{
			HandleLeftClick();
			return true;
		}

		HandleAttackAction();
		return true;
	}

	return Super::InputKey(Params);
}

bool AThiefPlayerController::IsInMenuMap() const
{
	return GetShortMapName(GetWorld()).Equals(MenuMapName.ToString(), ESearchCase::IgnoreCase);
}

bool AThiefPlayerController::IsTradeMenuOpen() const
{
	return bTradeMenuOpen;
}

bool AThiefPlayerController::IsProgressionMenuOpen() const
{
	return bProgressionMenuOpen;
}

bool AThiefPlayerController::IsPauseMenuOpen() const
{
	return bPauseMenuOpen;
}

bool AThiefPlayerController::IsSettingsMenuOpen() const
{
	return bSettingsMenuOpen;
}

bool AThiefPlayerController::IsAnyModalOpen() const
{
	return bTradeMenuOpen || bProgressionMenuOpen || bPauseMenuOpen || bSettingsMenuOpen || IsShiftResultScreenOpen() || IsInMenuMap();
}

bool AThiefPlayerController::IsLoadingGameplayMap() const
{
	return bLoadingGameplayMap;
}

bool AThiefPlayerController::IsShiftResultScreenOpen() const
{
	const UWorkOrderSubsystem* WorkOrderSubsystem = GetWorkOrderSubsystem();
	return WorkOrderSubsystem && WorkOrderSubsystem->HasResolvedShiftResult();
}

bool AThiefPlayerController::HasNearbyTrader() const
{
	return NearbyTrader.IsValid();
}

FText AThiefPlayerController::GetNearbyTraderPromptText() const
{
	if (!NearbyTrader.IsValid())
	{
		return FText::GetEmpty();
	}

	if (const UWorkOrderSubsystem* WorkOrderSubsystem = GetWorkOrderSubsystem())
	{
		if (WorkOrderSubsystem->IsShiftReadyToTurnIn())
		{
			return CurrentLanguage == EGameLanguage::Russian
				? FText::FromString(TEXT("[E] Сдать заказ"))
				: FText::FromString(TEXT("[E] Turn In Order"));
		}
	}

	return CurrentLanguage == EGameLanguage::Russian
		? FText::FromString(TEXT("[E] Открыть торговлю"))
		: FText::FromString(TEXT("[E] Open Trade"));
}

FText AThiefPlayerController::GetNearbyTraderName() const
{
	return NearbyTrader.IsValid() ? NearbyTrader->GetTraderName() : FText::GetEmpty();
}

float AThiefPlayerController::GetMasterVolumeSetting() const
{
	return MasterVolumeSetting;
}

float AThiefPlayerController::GetMusicVolumeSetting() const
{
	return MusicVolumeSetting;
}

float AThiefPlayerController::GetSfxVolumeSetting() const
{
	return SfxVolumeSetting;
}

float AThiefPlayerController::GetMenuScaleSetting() const
{
	return MenuScaleSetting;
}

float AThiefPlayerController::GetLookSensitivitySetting() const
{
	return LookSensitivitySetting;
}

bool AThiefPlayerController::IsLookYInverted() const
{
	return bInvertLookY;
}

EGameLanguage AThiefPlayerController::GetCurrentLanguage() const
{
	return CurrentLanguage;
}

bool AThiefPlayerController::IsRussianLanguage() const
{
	return CurrentLanguage == EGameLanguage::Russian;
}

ESettingsPanelTab AThiefPlayerController::GetActiveSettingsTab() const
{
	return ActiveSettingsTab;
}

bool AThiefPlayerController::IsWaitingForInputRebind() const
{
	return bWaitingForInputRebind;
}

ERemappableInputAction AThiefPlayerController::GetPendingInputRebindAction() const
{
	return PendingInputRebindAction;
}

FText AThiefPlayerController::GetInputActionDisplayName(const ERemappableInputAction InputAction) const
{
	const FRemappableBindingDefinition* Definition = FindBindingDefinition(InputAction);
	if (!Definition)
	{
		return FText::GetEmpty();
	}

	return FText::FromString(GetLocalizedString(this, Definition->EnglishLabel, Definition->RussianLabel));
}

FText AThiefPlayerController::GetInputActionDescription(const ERemappableInputAction InputAction) const
{
	const FRemappableBindingDefinition* Definition = FindBindingDefinition(InputAction);
	if (!Definition)
	{
		return FText::GetEmpty();
	}

	return FText::FromString(GetLocalizedString(this, Definition->EnglishDescription, Definition->RussianDescription));
}

FText AThiefPlayerController::GetInputActionKeyText(const ERemappableInputAction InputAction) const
{
	const FKey BoundKey = GetCurrentBindingKey(InputAction);
	if (BoundKey.IsValid())
	{
		return BoundKey.GetDisplayName();
	}

	return CurrentLanguage == EGameLanguage::Russian
		? FText::FromString(TEXT("Не назначено"))
		: FText::FromString(TEXT("Unbound"));
}

int32 AThiefPlayerController::GetGoldPerOre() const
{
	return GoldPerOre;
}

int32 AThiefPlayerController::GetStaminaPotionCost() const
{
	return StaminaPotionCost;
}

float AThiefPlayerController::GetStaminaPotionRestoreAmount() const
{
	return StaminaPotionRestoreAmount;
}

void AThiefPlayerController::CloseAllMenus()
{
	bTradeMenuOpen = false;
	bProgressionMenuOpen = false;
	bPauseMenuOpen = false;
	bSettingsMenuOpen = false;
	CancelInputRebind();
	ApplyMenuInputState();
}

void AThiefPlayerController::SetNearbyTrader(ATraderNPC* TraderActor)
{
	NearbyTrader = TraderActor;
}

void AThiefPlayerController::ClearNearbyTrader(const ATraderNPC* TraderActor)
{
	if (!NearbyTrader.IsValid() || NearbyTrader.Get() != TraderActor)
	{
		return;
	}

	NearbyTrader = nullptr;
	if (bTradeMenuOpen)
	{
		bTradeMenuOpen = false;
		ApplyMenuInputState();
	}
}

// [этап 3] Запуск игры из лобби: включаю экран загрузки, готовлю выбранный заказ
// и через короткую задержку открываю игровую карту Showcase.
void AThiefPlayerController::StartGameplayFromMenu()
{
	if (!IsInMenuMap() || bLoadingGameplayMap)
	{
		return;
	}

	CancelInputRebind();
	bSettingsMenuOpen = false;
	bLoadingGameplayMap = true;

	if (UWorkOrderSubsystem* WorkOrderSubsystem = GetWorkOrderSubsystem())
	{
		WorkOrderSubsystem->PrepareSelectedOrderForLaunch();
	}

	ApplyMenuInputState();
	UpdateLobbyMusic();

	GetWorldTimerManager().ClearTimer(LoadingMapTimerHandle);
	if (LoadingScreenDelay <= 0.0f)
	{
		OpenGameplayMap();
		return;
	}

	GetWorldTimerManager().SetTimer(
		LoadingMapTimerHandle,
		this,
		&AThiefPlayerController::OpenGameplayMap,
		LoadingScreenDelay,
		false);
}

void AThiefPlayerController::TogglePauseMenu()
{
	if (IsInMenuMap() || bLoadingGameplayMap)
	{
		return;
	}

	if (bSettingsMenuOpen)
	{
		CancelInputRebind();
		bSettingsMenuOpen = false;
		bPauseMenuOpen = true;
		ApplyMenuInputState();
		return;
	}

	if (bTradeMenuOpen || bProgressionMenuOpen)
	{
		CloseAllMenus();
		return;
	}

	bPauseMenuOpen = !bPauseMenuOpen;
	if (bPauseMenuOpen)
	{
		bTradeMenuOpen = false;
		bProgressionMenuOpen = false;
	}

	ApplyMenuInputState();
}

void AThiefPlayerController::OpenSettingsMenu()
{
	if (bLoadingGameplayMap)
	{
		return;
	}

	bSettingsMenuOpen = true;
	if (!IsInMenuMap())
	{
		bPauseMenuOpen = true;
	}
	bTradeMenuOpen = false;
	bProgressionMenuOpen = false;
	ApplyMenuInputState();
}

void AThiefPlayerController::CloseSettingsMenu()
{
	CancelInputRebind();
	bSettingsMenuOpen = false;
	ApplyMenuInputState();
}

void AThiefPlayerController::SetActiveSettingsTab(const ESettingsPanelTab NewTab)
{
	if (ActiveSettingsTab == NewTab)
	{
		return;
	}

	if (NewTab != ESettingsPanelTab::Controls)
	{
		CancelInputRebind();
	}

	ActiveSettingsTab = NewTab;
}

void AThiefPlayerController::ReturnToLobby()
{
	bLoadingGameplayMap = false;
	NearbyTrader = nullptr;
	CloseAllMenus();
	if (UWorkOrderSubsystem* WorkOrderSubsystem = GetWorkOrderSubsystem())
	{
		WorkOrderSubsystem->NotifyReturnedToLobby();
	}
	UpdateLobbyMusic();
	UGameplayStatics::OpenLevel(this, MenuMapName);
}

void AThiefPlayerController::RequestQuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AThiefPlayerController::ResetAllProgress()
{
	if (AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->ResetCharacterProgress();
	}

	if (UWorkOrderSubsystem* WorkOrderSubsystem = GetWorkOrderSubsystem())
	{
		WorkOrderSubsystem->ResetPersistentProgress();
	}
}

void AThiefPlayerController::DismissShiftResultScreen()
{
	if (UWorkOrderSubsystem* WorkOrderSubsystem = GetWorkOrderSubsystem())
	{
		WorkOrderSubsystem->DismissShiftResult();
	}

	if (AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->SetTemporaryStaminaModifiers(1.0f, 1.0f);
	}

	ApplyMenuInputState();
}

void AThiefPlayerController::AbandonActiveShift()
{
	UWorkOrderSubsystem* WorkOrderSubsystem = GetWorkOrderSubsystem();
	AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter();
	if (!WorkOrderSubsystem || !WorkOrderSubsystem->AbandonTrackedShift(PlayerCharacter))
	{
		return;
	}

	if (PlayerCharacter)
	{
		PlayerCharacter->SetTemporaryStaminaModifiers(1.0f, 1.0f);
	}

	bPauseMenuOpen = false;
	bTradeMenuOpen = false;
	bProgressionMenuOpen = false;
	bSettingsMenuOpen = false;
	CancelInputRebind();
	ApplyMenuInputState();
}

void AThiefPlayerController::AdjustMasterVolume(const float Delta)
{
	MasterVolumeSetting = FMath::Clamp(MasterVolumeSetting + Delta, 0.0f, 1.0f);
	ApplyAudioSettings();
	SaveSettingsToJson();
}

void AThiefPlayerController::AdjustMusicVolume(const float Delta)
{
	MusicVolumeSetting = FMath::Clamp(MusicVolumeSetting + Delta, 0.0f, 1.0f);
	UpdateLobbyMusic();
	SaveSettingsToJson();
}

void AThiefPlayerController::AdjustSfxVolume(const float Delta)
{
	SfxVolumeSetting = FMath::Clamp(SfxVolumeSetting + Delta, 0.0f, 1.0f);
	ApplyWorldAudioVolumes();
	SaveSettingsToJson();
}

void AThiefPlayerController::AdjustMenuScale(const float Delta)
{
	MenuScaleSetting = FMath::Clamp(MenuScaleSetting + Delta, MinMenuScale, MaxMenuScale);
	SaveSettingsToJson();
}

void AThiefPlayerController::AdjustLookSensitivity(const float Delta)
{
	LookSensitivitySetting = FMath::Clamp(LookSensitivitySetting + Delta, MinLookSensitivity, MaxLookSensitivity);
	ApplyLookSettings();
	SaveSettingsToJson();
}

void AThiefPlayerController::ToggleLookYInversion()
{
	bInvertLookY = !bInvertLookY;
	ApplyLookSettings();
	SaveSettingsToJson();
}

void AThiefPlayerController::CycleLanguage(const int32 Direction)
{
	constexpr int32 LanguageCount = 2;
	int32 NewLanguageIndex = static_cast<int32>(CurrentLanguage) + Direction;
	while (NewLanguageIndex < 0)
	{
		NewLanguageIndex += LanguageCount;
	}

	NewLanguageIndex %= LanguageCount;
	CurrentLanguage = static_cast<EGameLanguage>(NewLanguageIndex);
	SaveSettingsToJson();
}

void AThiefPlayerController::BeginRebindingInput(const ERemappableInputAction InputAction)
{
	if (!bSettingsMenuOpen || !FindBindingDefinition(InputAction))
	{
		return;
	}

	ActiveSettingsTab = ESettingsPanelTab::Controls;
	PendingInputRebindAction = InputAction;
	bWaitingForInputRebind = true;
}

void AThiefPlayerController::CancelInputRebind()
{
	bWaitingForInputRebind = false;
	PendingInputRebindAction = ERemappableInputAction::None;
}

void AThiefPlayerController::ResetControlsToDefaults()
{
	UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (!InputSettings)
	{
		return;
	}

	for (const FRemappableBindingDefinition& Definition : GetRemappableBindingDefinitions())
	{
		SetBindingKeyOnInputSettings(InputSettings, Definition, Definition.DefaultKey);
	}

	InputSettings->ForceRebuildKeymaps();
	InputSettings->SaveKeyMappings();
	InputSettings->SaveConfig();

	if (PlayerInput)
	{
		PlayerInput->ForceRebuildingKeyMaps(false);
	}

	LookSensitivitySetting = DefaultLookSensitivity;
	bInvertLookY = false;
	ApplyLookSettings();
	CancelInputRebind();
	SaveSettingsToJson();
}

// [этапы 7, 8] Действие кнопки [1]/[2]/[3] в открытом окне: в торговле — продажа/
// покупка, в прокачке — улучшение. Команда зависит от того, какое окно открыто.
void AThiefPlayerController::ExecuteMenuOption(const int32 OptionIndex)
{
	if (bPauseMenuOpen || bSettingsMenuOpen)
	{
		return;
	}

	AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter();
	if (!PlayerCharacter)
	{
		return;
	}

	if (bTradeMenuOpen)
	{
		switch (OptionIndex)
		{
		case 1:
			PlayerCharacter->SellOre(1, GoldPerOre);
			break;
		case 2:
			PlayerCharacter->SellOre(5, GoldPerOre);
			break;
		case 3:
			PlayerCharacter->BuyStaminaPotion(StaminaPotionCost, StaminaPotionRestoreAmount);
			break;
		default:
			break;
		}
		return;
	}

	if (bProgressionMenuOpen)
	{
		switch (OptionIndex)
		{
		case 1:
			PlayerCharacter->SpendUpgradePoint(EPlayerUpgradeType::MaxStamina);
			break;
		case 2:
			PlayerCharacter->SpendUpgradePoint(EPlayerUpgradeType::OreDamage);
			break;
		case 3:
			PlayerCharacter->SpendUpgradePoint(EPlayerUpgradeType::MoveSpeed);
			break;
		default:
			break;
		}
	}
}

// [этап 2] Установка значения ползунка по доле 0..1 (когда тяну его мышью):
// перевожу долю в нужный диапазон (громкость, чувствительность, масштаб) и применяю.
void AThiefPlayerController::SetSettingValueFromRatio(const EHUDSliderTarget Target, const float Ratio)
{
	const float ClampedRatio = FMath::Clamp(Ratio, 0.0f, 1.0f);
	switch (Target)
	{
	case EHUDSliderTarget::MasterVolume:
		MasterVolumeSetting = ClampedRatio;
		ApplyAudioSettings();
		break;
	case EHUDSliderTarget::MusicVolume:
		MusicVolumeSetting = ClampedRatio;
		UpdateLobbyMusic();
		break;
	case EHUDSliderTarget::SfxVolume:
		SfxVolumeSetting = ClampedRatio;
		ApplyWorldAudioVolumes();
		break;
	case EHUDSliderTarget::LookSensitivity:
		LookSensitivitySetting = FMath::Clamp(MinLookSensitivity + ClampedRatio * (MaxLookSensitivity - MinLookSensitivity), MinLookSensitivity, MaxLookSensitivity);
		ApplyLookSettings();
		break;
	case EHUDSliderTarget::MenuScale:
		MenuScaleSetting = FMath::Clamp(MinMenuScale + ClampedRatio * (MaxMenuScale - MinMenuScale), MinMenuScale, MaxMenuScale);
		break;
	default:
		break;
	}
}

void AThiefPlayerController::PersistSettingsToDisk() const
{
	SaveSettingsToJson();
}

void AThiefPlayerController::LoadSettingsFromJson()
{
	EnsureInputMappingsExist();

	FMyProjectSettingsSaveData SaveData;
	if (!FMyProjectJsonSaveUtils::LoadSettingsData(SaveData))
	{
		SaveSettingsToJson();
		return;
	}

	MasterVolumeSetting = FMath::Clamp(SaveData.MasterVolumeSetting, 0.0f, 1.0f);
	MusicVolumeSetting = FMath::Clamp(SaveData.MusicVolumeSetting, 0.0f, 1.0f);
	SfxVolumeSetting = FMath::Clamp(SaveData.SfxVolumeSetting, 0.0f, 1.0f);
	MenuScaleSetting = FMath::Clamp(SaveData.MenuScaleSetting, MinMenuScale, MaxMenuScale);
	LookSensitivitySetting = FMath::Clamp(SaveData.LookSensitivitySetting, MinLookSensitivity, MaxLookSensitivity);
	bInvertLookY = SaveData.bInvertLookY;

	constexpr uint8 LanguageCount = 2;
	const uint8 LanguageIndex = SaveData.Language < LanguageCount
		? SaveData.Language
		: static_cast<uint8>(EGameLanguage::Russian);
	CurrentLanguage = static_cast<EGameLanguage>(LanguageIndex);

	if (SaveData.InputBindings.Num() > 0)
	{
		UInputSettings* InputSettings = UInputSettings::GetInputSettings();
		TMap<uint8, FKey> SavedKeysByActionId;
		for (const FMyProjectInputBindingSaveData& SavedBinding : SaveData.InputBindings)
		{
			const FKey SavedKey(*SavedBinding.KeyName);
			if (SavedKey.IsValid())
			{
				SavedKeysByActionId.Add(SavedBinding.InputAction, SavedKey);
			}
		}

		const uint8 UsePotionId = static_cast<uint8>(ERemappableInputAction::UsePotion);
		const uint8 ToggleTradeMenuId = static_cast<uint8>(ERemappableInputAction::ToggleTradeMenu);
		const uint8 ToggleProgressionMenuId = static_cast<uint8>(ERemappableInputAction::ToggleProgressionMenu);
		const uint8 MenuConfirmId = static_cast<uint8>(ERemappableInputAction::MenuConfirm);
		const uint8 MenuBackId = static_cast<uint8>(ERemappableInputAction::MenuBack);
		const bool bNeedsLegacyBindingMigration =
			SavedKeysByActionId.Contains(UsePotionId) &&
			SavedKeysByActionId.Contains(ToggleTradeMenuId) &&
			SavedKeysByActionId.Contains(ToggleProgressionMenuId) &&
			SavedKeysByActionId.Contains(MenuConfirmId) &&
			SavedKeysByActionId.Contains(MenuBackId) &&
			SavedKeysByActionId[UsePotionId] == EKeys::B &&
			SavedKeysByActionId[ToggleTradeMenuId] == EKeys::P &&
			SavedKeysByActionId[MenuConfirmId] == EKeys::Escape;

		bool bMappingsChanged = false;
		if (bNeedsLegacyBindingMigration)
		{
			for (const FRemappableBindingDefinition& Definition : GetRemappableBindingDefinitions())
			{
				FKey NewKey = EKeys::Invalid;
				const uint8 CurrentActionId = static_cast<uint8>(Definition.InputAction);
				if (CurrentActionId <= static_cast<uint8>(ERemappableInputAction::Interact))
				{
					NewKey = SavedKeysByActionId.FindRef(CurrentActionId);
				}
				else if (CurrentActionId == UsePotionId)
				{
					NewKey = EKeys::Q;
				}
				else
				{
					NewKey = SavedKeysByActionId.FindRef(CurrentActionId - 1);
				}

				if (!NewKey.IsValid())
				{
					continue;
				}

				SetBindingKeyOnInputSettings(InputSettings, Definition, NewKey);
				bMappingsChanged = true;
			}
		}
		else
		{
			for (const FMyProjectInputBindingSaveData& SavedBinding : SaveData.InputBindings)
			{
				const FRemappableBindingDefinition* Definition = FindBindingDefinition(static_cast<ERemappableInputAction>(SavedBinding.InputAction));
				if (!Definition)
				{
					continue;
				}

				const FKey NewKey = SavedKeysByActionId.FindRef(SavedBinding.InputAction);
				if (!NewKey.IsValid())
				{
					continue;
				}

				SetBindingKeyOnInputSettings(InputSettings, *Definition, NewKey);
				bMappingsChanged = true;
			}
		}

		if (InputSettings && bMappingsChanged)
		{
			InputSettings->ForceRebuildKeymaps();
			InputSettings->SaveKeyMappings();
			InputSettings->SaveConfig();
			if (PlayerInput)
			{
				PlayerInput->ForceRebuildingKeyMaps(false);
			}
		}

		if (bNeedsLegacyBindingMigration)
		{
			SaveSettingsToJson();
		}
	}
}

void AThiefPlayerController::SaveSettingsToJson() const
{
	FMyProjectSettingsSaveData SaveData;
	SaveData.MasterVolumeSetting = MasterVolumeSetting;
	SaveData.MusicVolumeSetting = MusicVolumeSetting;
	SaveData.SfxVolumeSetting = SfxVolumeSetting;
	SaveData.MenuScaleSetting = MenuScaleSetting;
	SaveData.LookSensitivitySetting = LookSensitivitySetting;
	SaveData.bInvertLookY = bInvertLookY;
	SaveData.Language = static_cast<uint8>(CurrentLanguage);

	for (const FRemappableBindingDefinition& Definition : GetRemappableBindingDefinitions())
	{
		FMyProjectInputBindingSaveData BindingSaveData;
		BindingSaveData.InputAction = static_cast<uint8>(Definition.InputAction);
		BindingSaveData.KeyName = GetCurrentBindingKey(Definition.InputAction).GetFName().ToString();
		SaveData.InputBindings.Add(BindingSaveData);
	}

	FMyProjectJsonSaveUtils::SaveSettingsData(SaveData);
}

void AThiefPlayerController::ApplyMenuInputState()
{
	const bool bMenuLocked = bTradeMenuOpen || bProgressionMenuOpen || bPauseMenuOpen || bSettingsMenuOpen || IsShiftResultScreenOpen() || IsInMenuMap();
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	if (bMenuLocked)
	{
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
	}
	SetShowMouseCursor(bMenuLocked);
	bEnableClickEvents = bMenuLocked;
	bEnableMouseOverEvents = bMenuLocked;
	RefreshOreHealthBarVisibility();

	if (bMenuLocked)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		return;
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void AThiefPlayerController::RefreshOreHealthBarVisibility() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> OreActors;
	UGameplayStatics::GetAllActorsOfClass(World, AOreStoneBase::StaticClass(), OreActors);
	for (AActor* OreActor : OreActors)
	{
		if (AOreStoneBase* OreStone = Cast<AOreStoneBase>(OreActor))
		{
			OreStone->RefreshHealthBarVisibilityState();
		}
	}
}

void AThiefPlayerController::ApplyAudioSettings()
{
	if (UWorld* World = GetWorld())
	{
		if (FAudioDevice* AudioDevice = World->GetAudioDeviceRaw())
		{
			AudioDevice->SetTransientPrimaryVolume(MasterVolumeSetting);
		}
	}

	UpdateLobbyMusic();
	ApplyWorldAudioVolumes();
}

void AThiefPlayerController::ApplyLookSettings()
{
	UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (!InputSettings)
	{
		return;
	}

	SetAxisSensitivity(InputSettings, EKeys::MouseX.GetFName(), LookSensitivitySetting);
	SetAxisSensitivity(InputSettings, EKeys::MouseY.GetFName(), LookSensitivitySetting);

	TArray<FInputAxisKeyMapping> LookMappings;
	InputSettings->GetAxisMappingByName(LookUpDownAxisName, LookMappings);
	for (const FInputAxisKeyMapping& ExistingMapping : LookMappings)
	{
		if (ExistingMapping.Key == EKeys::MouseY)
		{
			InputSettings->RemoveAxisMapping(ExistingMapping, false);
		}
	}

	InputSettings->AddAxisMapping(MakeAxisMapping(LookUpDownAxisName, EKeys::MouseY, bInvertLookY ? 1.0f : -1.0f), false);
	InputSettings->ForceRebuildKeymaps();
	InputSettings->SaveKeyMappings();
	InputSettings->SaveConfig();

	if (PlayerInput)
	{
		PlayerInput->SetMouseSensitivity(LookSensitivitySetting, LookSensitivitySetting);
		PlayerInput->ForceRebuildingKeyMaps(false);
	}
}

void AThiefPlayerController::ApplyWorldAudioVolumes()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TObjectIterator<UAudioComponent> It; It; ++It)
	{
		UAudioComponent* AudioComponent = *It;
		if (!IsValid(AudioComponent) || AudioComponent->GetWorld() != World)
		{
			continue;
		}

		if (AudioComponent == LobbyMusicComponent.Get())
		{
			AudioComponent->SetVolumeMultiplier(MusicVolumeSetting);
			continue;
		}

		if (AudioComponent->bIsUISound)
		{
			continue;
		}

		AudioComponent->SetVolumeMultiplier(SfxVolumeSetting);
	}
}

void AThiefPlayerController::EnsureInputMappingsExist()
{
	UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (!InputSettings)
	{
		return;
	}

	bool bMappingsChanged = false;
	for (const FRemappableBindingDefinition& Definition : GetRemappableBindingDefinitions())
	{
		if (Definition.bIsAxis)
		{
			TArray<FInputAxisKeyMapping> ExistingMappings;
			InputSettings->GetAxisMappingByName(Definition.MappingName, ExistingMappings);

			bool bFoundMapping = false;
			for (const FInputAxisKeyMapping& ExistingMapping : ExistingMappings)
			{
				if (FMath::IsNearlyEqual(ExistingMapping.Scale, Definition.AxisScale))
				{
					bFoundMapping = true;
					break;
				}
			}

			if (!bFoundMapping)
			{
				InputSettings->AddAxisMapping(MakeAxisMapping(Definition.MappingName, Definition.DefaultKey, Definition.AxisScale), false);
				bMappingsChanged = true;
			}
			continue;
		}

		TArray<FInputActionKeyMapping> ExistingMappings;
		InputSettings->GetActionMappingByName(Definition.MappingName, ExistingMappings);
		if (ExistingMappings.Num() == 0)
		{
			InputSettings->AddActionMapping(MakeActionMapping(Definition.MappingName, Definition.DefaultKey), false);
			bMappingsChanged = true;
		}
	}

	if (bMappingsChanged)
	{
		InputSettings->ForceRebuildKeymaps();
		InputSettings->SaveKeyMappings();
		InputSettings->SaveConfig();
	}
}

void AThiefPlayerController::UpdateLobbyMusic()
{
	if (IsInMenuMap() && !bLoadingGameplayMap && LobbyMusic)
	{
		if (!LobbyMusicComponent || !LobbyMusicComponent->IsPlaying())
		{
			if (LobbyMusicComponent)
			{
				LobbyMusicComponent->Stop();
				LobbyMusicComponent = nullptr;
			}

			LobbyMusicComponent = UGameplayStatics::CreateSound2D(this, LobbyMusic, MusicVolumeSetting);
			if (LobbyMusicComponent)
			{
				LobbyMusicComponent->bIsUISound = true;
				LobbyMusicComponent->bAutoDestroy = false;
				LobbyMusicComponent->Play();
			}
		}
		else
		{
			LobbyMusicComponent->SetVolumeMultiplier(MusicVolumeSetting);
		}

		return;
	}

	if (LobbyMusicComponent)
	{
		LobbyMusicComponent->FadeOut(0.2f, 0.0f);
		LobbyMusicComponent = nullptr;
	}
}

void AThiefPlayerController::OpenGameplayMap()
{
	if (!IsInMenuMap())
	{
		return;
	}

	UGameplayStatics::OpenLevel(this, GameplayMapName);
}

void AThiefPlayerController::HandleAttackAction()
{
	if (IsInMenuMap() || bLoadingGameplayMap || IsAnyModalOpen())
	{
		return;
	}

	if (AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->Attack();
	}
}

void AThiefPlayerController::HandleInteractAction()
{
	if (IsInMenuMap() || !HasNearbyTrader() || bPauseMenuOpen || bSettingsMenuOpen)
	{
		return;
	}

	if (const AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter())
	{
		if (PlayerCharacter->bIsAttacking)
		{
			return;
		}
	}

	if (UWorkOrderSubsystem* WorkOrderSubsystem = GetWorkOrderSubsystem())
	{
		if (WorkOrderSubsystem->IsShiftReadyToTurnIn())
		{
			if (AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter())
			{
				int32 GoldReward = 0;
				int32 ExperienceReward = 0;
				int32 ReputationReward = 0;
				if (WorkOrderSubsystem->CompleteTrackedShift(PlayerCharacter, GoldReward, ExperienceReward, ReputationReward))
				{
					bTradeMenuOpen = false;
					bProgressionMenuOpen = false;
					ApplyMenuInputState();
					return;
				}
			}
		}
	}

	bTradeMenuOpen = !bTradeMenuOpen;
	if (bTradeMenuOpen)
	{
		bProgressionMenuOpen = false;
	}

	ApplyMenuInputState();
}

void AThiefPlayerController::HandlePrimaryConfirm()
{
	if (IsShiftResultScreenOpen())
	{
		DismissShiftResultScreen();
		return;
	}

	if (IsInMenuMap())
	{
		if (bSettingsMenuOpen)
		{
			CloseSettingsMenu();
			return;
		}

		StartGameplayFromMenu();
		return;
	}

	if (bPauseMenuOpen && !bSettingsMenuOpen)
	{
		TogglePauseMenu();
	}
}

void AThiefPlayerController::HandleBackAction()
{
	if (bLoadingGameplayMap)
	{
		return;
	}

	if (IsShiftResultScreenOpen())
	{
		DismissShiftResultScreen();
		return;
	}

	if (bSettingsMenuOpen)
	{
		CancelInputRebind();
		bSettingsMenuOpen = false;
		if (!IsInMenuMap())
		{
			bPauseMenuOpen = true;
		}
		ApplyMenuInputState();
		return;
	}

	if (bTradeMenuOpen || bProgressionMenuOpen)
	{
		bTradeMenuOpen = false;
		bProgressionMenuOpen = false;
		ApplyMenuInputState();
		return;
	}

	if (IsInMenuMap())
	{
		OpenSettingsMenu();
		return;
	}

	TogglePauseMenu();
}

void AThiefPlayerController::HandleLeftClick()
{
	if (IsAnyModalOpen())
	{
		APlayerGameHUD* PlayerHUD = Cast<APlayerGameHUD>(GetHUD());
		if (!PlayerHUD)
		{
			return;
		}

		float MouseX = 0.0f;
		float MouseY = 0.0f;
		if (!GetMousePosition(MouseX, MouseY))
		{
			return;
		}

		PlayerHUD->HandleClick(FVector2D(MouseX, MouseY));
		return;
	}

	HandleAttackAction();
}

void AThiefPlayerController::ToggleTradeMenu()
{
	if (IsInMenuMap() || !HasNearbyTrader() || bPauseMenuOpen || bSettingsMenuOpen)
	{
		return;
	}

	if (const AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter())
	{
		if (PlayerCharacter->bIsAttacking)
		{
			return;
		}
	}

	bTradeMenuOpen = !bTradeMenuOpen;
	if (bTradeMenuOpen)
	{
		bProgressionMenuOpen = false;
	}

	ApplyMenuInputState();
}

void AThiefPlayerController::ToggleProgressionMenu()
{
	if (IsInMenuMap() || bPauseMenuOpen || bSettingsMenuOpen)
	{
		return;
	}

	if (const AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter())
	{
		if (PlayerCharacter->bIsAttacking)
		{
			return;
		}
	}

	bProgressionMenuOpen = !bProgressionMenuOpen;
	if (bProgressionMenuOpen)
	{
		bTradeMenuOpen = false;
	}

	ApplyMenuInputState();
}

void AThiefPlayerController::HandleOptionOne()
{
	ExecuteMenuOption(1);
}

void AThiefPlayerController::HandleOptionTwo()
{
	ExecuteMenuOption(2);
}

void AThiefPlayerController::HandleOptionThree()
{
	ExecuteMenuOption(3);
}

bool AThiefPlayerController::TryCaptureInputRebind(const FKey& PressedKey)
{
	if (!bWaitingForInputRebind || PendingInputRebindAction == ERemappableInputAction::None)
	{
		return false;
	}

	if (!IsValidBindingKey(PressedKey))
	{
		return true;
	}

	if (!ApplyBindingKey(PendingInputRebindAction, PressedKey))
	{
		return true;
	}

	CancelInputRebind();
	SaveSettingsToJson();
	return true;
}

bool AThiefPlayerController::ApplyBindingKey(const ERemappableInputAction InputAction, const FKey& NewKey)
{
	if (!IsValidBindingKey(NewKey))
	{
		return false;
	}

	const FRemappableBindingDefinition* TargetDefinition = FindBindingDefinition(InputAction);
	UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (!TargetDefinition || !InputSettings)
	{
		return false;
	}

	const FKey PreviousKey = GetCurrentBindingKey(InputAction);
	if (PreviousKey == NewKey)
	{
		return true;
	}

	ERemappableInputAction SwappedInputAction = ERemappableInputAction::None;
	for (const FRemappableBindingDefinition& Definition : GetRemappableBindingDefinitions())
	{
		if (Definition.InputAction == InputAction)
		{
			continue;
		}

		if (GetCurrentBindingKey(Definition.InputAction) == NewKey)
		{
			SwappedInputAction = Definition.InputAction;
			break;
		}
	}

	SetBindingKeyOnInputSettings(InputSettings, *TargetDefinition, NewKey);
	if (SwappedInputAction != ERemappableInputAction::None)
	{
		const FRemappableBindingDefinition* SwappedDefinition = FindBindingDefinition(SwappedInputAction);
		if (SwappedDefinition)
		{
			const FKey SwapFallbackKey = PreviousKey.IsValid() ? PreviousKey : SwappedDefinition->DefaultKey;
			SetBindingKeyOnInputSettings(InputSettings, *SwappedDefinition, SwapFallbackKey);
		}
	}

	InputSettings->ForceRebuildKeymaps();
	InputSettings->SaveKeyMappings();
	InputSettings->SaveConfig();
	if (PlayerInput)
	{
		PlayerInput->ForceRebuildingKeyMaps(false);
	}

	return true;
}

FKey AThiefPlayerController::GetCurrentBindingKey(const ERemappableInputAction InputAction) const
{
	const FRemappableBindingDefinition* Definition = FindBindingDefinition(InputAction);
	if (!Definition)
	{
		return EKeys::Invalid;
	}

	return GetBindingKeyFromInputSettings(UInputSettings::GetInputSettings(), *Definition);
}

AGameplayCharacterBase* AThiefPlayerController::GetPlayerCharacter() const
{
	return Cast<AGameplayCharacterBase>(GetPawn());
}

UWorkOrderSubsystem* AThiefPlayerController::GetWorkOrderSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UWorkOrderSubsystem>() : nullptr;
}
