#pragma once

// ============================================================================
//  AThiefPlayerController (.h) — интерфейс контроллера игрока.
//  Ниже — перечисления (язык, вкладки настроек, биндинги) и объявления функций
//  ввода, меню и настроек. Реализация и подробные пометки — в одноимённом .cpp.
//
//  ЗАЩИТА — по этапам: 1 (меню: HandleLeftClick, InputKey), 2 (настройки:
//  Adjust*, SetSettingValueFromRatio, CycleLanguage, BeginRebindingInput),
//  3 (старт→загрузка: StartGameplayFromMenu, OpenGameplayMap, ReturnToLobby),
//  7 (торговля: ToggleTradeMenu, ExecuteMenuOption), 8 (ToggleProgressionMenu).
// ============================================================================

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"
#include "ThiefPlayerController.generated.h"

struct FInputKeyEventArgs;
enum class EHUDSliderTarget : uint8;

UENUM(BlueprintType)
enum class EGameLanguage : uint8
{
	English,
	Russian
};

UENUM(BlueprintType)
enum class ESettingsPanelTab : uint8
{
	Audio,
	Controls,
	Interface
};

UENUM(BlueprintType)
enum class ERemappableInputAction : uint8
{
	None,
	MoveForward,
	MoveBackward,
	MoveRight,
	MoveLeft,
	Jump,
	Sprint,
	Attack,
	Interact,
	UsePotion,
	ToggleTradeMenu,
	ToggleProgressionMenu,
	MenuConfirm,
	MenuBack,
	MenuOptionOne,
	MenuOptionTwo,
	MenuOptionThree
};

UCLASS()
class MYPROJECT_API AThiefPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AThiefPlayerController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsInMenuMap() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsTradeMenuOpen() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsProgressionMenuOpen() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsPauseMenuOpen() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsSettingsMenuOpen() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsAnyModalOpen() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsLoadingGameplayMap() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsShiftResultScreenOpen() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool HasNearbyTrader() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetNearbyTraderPromptText() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetNearbyTraderName() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	float GetMasterVolumeSetting() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	float GetMusicVolumeSetting() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	float GetSfxVolumeSetting() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	float GetMenuScaleSetting() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	float GetLookSensitivitySetting() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsLookYInverted() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	EGameLanguage GetCurrentLanguage() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsRussianLanguage() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	ESettingsPanelTab GetActiveSettingsTab() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsWaitingForInputRebind() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	ERemappableInputAction GetPendingInputRebindAction() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetInputActionDisplayName(ERemappableInputAction InputAction) const;

	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetInputActionDescription(ERemappableInputAction InputAction) const;

	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetInputActionKeyText(ERemappableInputAction InputAction) const;

	UFUNCTION(BlueprintPure, Category = "Trading")
	int32 GetGoldPerOre() const;

	UFUNCTION(BlueprintPure, Category = "Trading")
	int32 GetStaminaPotionCost() const;

	UFUNCTION(BlueprintPure, Category = "Trading")
	float GetStaminaPotionRestoreAmount() const;

	void CloseAllMenus();
	void SetNearbyTrader(class ATraderNPC* TraderActor);
	void ClearNearbyTrader(const class ATraderNPC* TraderActor);
	void StartGameplayFromMenu();
	void TogglePauseMenu();
	void OpenSettingsMenu();
	void CloseSettingsMenu();
	void SetActiveSettingsTab(ESettingsPanelTab NewTab);
	void ReturnToLobby();
	void RequestQuitGame();
	void ResetAllProgress();
	void DismissShiftResultScreen();
	void AbandonActiveShift();
	void AdjustMasterVolume(float Delta);
	void AdjustMusicVolume(float Delta);
	void AdjustSfxVolume(float Delta);
	void AdjustMenuScale(float Delta);
	void AdjustLookSensitivity(float Delta);
	void ToggleLookYInversion();
	void CycleLanguage(int32 Direction);
	void BeginRebindingInput(ERemappableInputAction InputAction);
	void CancelInputRebind();
	void ResetControlsToDefaults();
	void ExecuteMenuOption(int32 OptionIndex);
	void SetSettingValueFromRatio(EHUDSliderTarget Target, float Ratio);
	void PersistSettingsToDisk() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	FName MenuMapName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	FName GameplayMapName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Loading", meta = (ClampMin = "0.0"))
	float LoadingScreenDelay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Audio")
	TObjectPtr<USoundBase> LobbyMusic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Audio", meta = (ClampMin = "0.0"))
	float LobbyMusicVolume;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trading", meta = (ClampMin = "1"))
	int32 GoldPerOre;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trading", meta = (ClampMin = "1"))
	int32 StaminaPotionCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trading", meta = (ClampMin = "1.0"))
	float StaminaPotionRestoreAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Localization")
	EGameLanguage CurrentLanguage;

private:
	void LoadSettingsFromJson();
	void SaveSettingsToJson() const;
	void ApplyMenuInputState();
	void RefreshOreHealthBarVisibility() const;
	void ApplyAudioSettings();
	void ApplyLookSettings();
	void ApplyWorldAudioVolumes();
	void EnsureInputMappingsExist();
	void UpdateLobbyMusic();
	void OpenGameplayMap();
	void HandleAttackAction();
	void HandleInteractAction();
	void HandlePrimaryConfirm();
	void HandleBackAction();
	void HandleLeftClick();
	void ToggleTradeMenu();
	void ToggleProgressionMenu();
	void HandleOptionOne();
	void HandleOptionTwo();
	void HandleOptionThree();
	bool TryCaptureInputRebind(const FKey& PressedKey);
	bool ApplyBindingKey(ERemappableInputAction InputAction, const FKey& NewKey);
	FKey GetCurrentBindingKey(ERemappableInputAction InputAction) const;
	class AGameplayCharacterBase* GetPlayerCharacter() const;
	class UWorkOrderSubsystem* GetWorkOrderSubsystem() const;

	bool bTradeMenuOpen;
	bool bProgressionMenuOpen;
	bool bPauseMenuOpen;
	bool bSettingsMenuOpen;
	bool bLoadingGameplayMap;
	bool bWaitingForInputRebind;
	float MasterVolumeSetting;
	float MusicVolumeSetting;
	float SfxVolumeSetting;
	float MenuScaleSetting;
	float LookSensitivitySetting;
	bool bInvertLookY;
	ESettingsPanelTab ActiveSettingsTab;
	ERemappableInputAction PendingInputRebindAction;

	FTimerHandle LoadingMapTimerHandle;
	FTimerHandle AudioRefreshTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> LobbyMusicComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<class ATraderNPC> NearbyTrader;
};
