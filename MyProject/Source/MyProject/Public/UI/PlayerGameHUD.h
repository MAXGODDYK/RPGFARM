#pragma once

// ============================================================================
//  APlayerGameHUD (.h) — интерфейс HUD (весь UI рисуется через Canvas).
//  Ниже — перечисления действий/слайдеров, структуры кнопок и объявления всех
//  Draw-функций. Реализация и подробные пометки — в одноимённом .cpp.
//
//  ЗАЩИТА — по этапам: 1 (DrawMainMenu), 2 (DrawSettingsPanel, ползунки,
//  HandleClick/HandleScroll), 3 (DrawLoadingScreen), 6 (DrawResourcePanel,
//  DrawStaminaPanel), 7 (DrawTradePanel), 8 (DrawProgressionPanel),
//  9 (DrawWorkOrderPanel, DrawShiftResultScreen). Общее: DrawHUD — диспетчер.
// ============================================================================

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PlayerGameHUD.generated.h"

enum class ERemappableInputAction : uint8;

UENUM()
enum class EHUDMenuAction : uint8
{
	None,
	StartGame,
	WorkOrderPrev,
	WorkOrderNext,
	OpenSettings,
	CloseSettings,
	ResumeGame,
	ReturnToLobby,
	QuitGame,
	SelectSettingsTab,
	MasterVolumeDown,
	MasterVolumeUp,
	MusicDown,
	MusicUp,
	SfxDown,
	SfxUp,
	LookSensitivityDown,
	LookSensitivityUp,
	ToggleInvertLookY,
	MenuScaleDown,
	MenuScaleUp,
	LanguagePrev,
	LanguageNext,
	BeginInputRebind,
	ResetControls,
	TradeSellOne,
	TradeSellFive,
	TradeBuyPotion,
	UpgradeMaxStamina,
	UpgradeOreDamage,
	UpgradeMoveSpeed,
	ResetCharacterProgress,
	CloseShiftResult,
	AbandonShift
};

UENUM()
enum class EHUDSliderTarget : uint8
{
	None,
	MasterVolume,
	MusicVolume,
	SfxVolume,
	LookSensitivity,
	MenuScale
};

USTRUCT()
struct FHUDButtonData
{
	GENERATED_BODY()

	UPROPERTY()
	EHUDMenuAction Action = EHUDMenuAction::None;

	UPROPERTY()
	FVector2D Position = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D Size = FVector2D::ZeroVector;

	UPROPERTY()
	int32 Payload = 0;
};

USTRUCT()
struct FHUDSliderData
{
	GENERATED_BODY()

	UPROPERTY()
	EHUDSliderTarget Target = EHUDSliderTarget::None;

	UPROPERTY()
	FVector2D Position = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D Size = FVector2D::ZeroVector;
};

UCLASS()
class MYPROJECT_API APlayerGameHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	bool HandleClick(const FVector2D& ScreenPosition);
	bool HandleScroll(float WheelDelta);

private:
	void UpdateLevelUpPopupState();
	void DrawMainMenu(float ViewportWidth, float ViewportHeight);
	void DrawLoadingScreen(float ViewportWidth, float ViewportHeight);
	void DrawPauseMenu(float ViewportWidth, float ViewportHeight);
	void DrawWorkOrderPanel(float ViewportWidth, float ViewportHeight) const;
	void DrawShiftResultScreen(float ViewportWidth, float ViewportHeight);
	void DrawSettingsPanel(float ViewportWidth, float ViewportHeight, bool bShowBackToPause);
	void DrawAudioSettingsSection(const class AThiefPlayerController* ThiefController, const FVector2D& PanelPosition, float PanelWidth, float Scale);
	void DrawControlsSettingsSection(const class AThiefPlayerController* ThiefController, const FVector2D& PanelPosition, float PanelWidth, float PanelHeight, float Scale);
	void DrawInterfaceSettingsSection(const class AThiefPlayerController* ThiefController, const FVector2D& PanelPosition, float PanelWidth, float Scale);
	void DrawSettingsTabButton(
		const FString& Label,
		const FVector2D& Position,
		const FVector2D& Size,
		int32 TabIndex,
		bool bSelected);
	void DrawSettingValueRow(
		const FString& Label,
		const FString& ValueText,
		const FVector2D& Position,
		float RowWidth,
		float Scale,
		EHUDMenuAction DecreaseAction,
		EHUDMenuAction IncreaseAction,
		float NormalizedValue,
		EHUDSliderTarget SliderTarget = EHUDSliderTarget::None);
	void DrawBindingRow(
		const class AThiefPlayerController* ThiefController,
		const FVector2D& Position,
		const FVector2D& Size,
		enum class ERemappableInputAction InputAction);
	void DrawInputCaptureOverlay(const class AThiefPlayerController* ThiefController, float ViewportWidth, float ViewportHeight);
	void DrawResourcePanel(float ViewportWidth) const;
	void DrawLevelPanel() const;
	void DrawLevelUpPopup(float ViewportWidth, float ViewportHeight) const;
	void DrawStaminaPanel(float ViewportHeight) const;
	void DrawInteractionPrompt(float ViewportWidth, float ViewportHeight) const;
	void DrawTradePanel(float ViewportWidth, float ViewportHeight);
	void DrawProgressionPanel(float ViewportWidth, float ViewportHeight);
	void DrawMenuButton(const FString& Label, const FVector2D& Position, const FVector2D& Size, EHUDMenuAction Action, bool bPrimary = false, int32 Payload = 0);
	void DrawActionButton(
		const FString& Title,
		const FString& Description,
		const FVector2D& Position,
		const FVector2D& Size,
		EHUDMenuAction Action,
		bool bPrimary = false,
		int32 Payload = 0);
	void DrawSidebarButton(const FString& Label, const FVector2D& Position, const FVector2D& Size, int32 TabIndex, bool bSelected);
	FVector2D GetMousePosition() const;
	bool IsButtonHovered(const FHUDButtonData& ButtonData) const;
	void RegisterButton(EHUDMenuAction Action, const FVector2D& Position, const FVector2D& Size, int32 Payload = 0);
	void RegisterSlider(EHUDSliderTarget Target, const FVector2D& Position, const FVector2D& Size);
	void UpdateActiveSliderDrag();
	class AGameplayCharacterBase* GetPlayerCharacter() const;
	class AThiefPlayerController* GetThiefPlayerController() const;

	TArray<FHUDButtonData> ActiveButtons;
	TArray<FHUDSliderData> ActiveSliders;
	EHUDSliderTarget ActiveDragSlider = EHUDSliderTarget::None;
	FVector2D CachedMousePosition = FVector2D(-1.0f, -1.0f);
	float ControlsBindingsScrollOffset = 0.0f;
	float ControlsBindingsMaxScroll = 0.0f;
	int32 LastObservedPlayerLevel = INDEX_NONE;
	int32 LevelPopupDisplayedLevel = 0;
	int32 LevelPopupDisplayedPoints = 0;
	float LevelPopupStartTime = -1000.0f;
	float LevelPopupDuration = 3.6f;
};
