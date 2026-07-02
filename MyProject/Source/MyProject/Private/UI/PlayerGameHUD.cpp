// ============================================================================
//  APlayerGameHUD — весь интерфейс игры, нарисованный вручную через Canvas.
//  Один класс отвечает за все экраны: меню, загрузка, пауза, настройки,
//  торговля, прокачка, панели ресурсов/стамины/уровня и рабочие заказы.
//  Каждая кнопка и ползунок регистрируются в списке и проверяются при клике —
//  у верхнего открытого окна приоритет (поэтому нет "сквозных" нажатий).
//
//  ЗАЩИТА — по этапам показа (какую функцию открывать):
//   • Этап 1 (лобби/меню): DrawMainMenu().
//   • Этап 2 (настройки): DrawSettingsPanel(), DrawSettingValueRow() (ползунки),
//     HandleClick()/HandleScroll().
//   • Этап 3 (загрузка): DrawLoadingScreen().
//   • Этап 6 (игровой HUD): DrawResourcePanel(), DrawStaminaPanel().
//   • Этап 7 (торговля): DrawTradePanel().
//   • Этап 8 (прокачка): DrawProgressionPanel().
//   • Этап 9 (заказы): DrawWorkOrderPanel(), DrawShiftResultScreen().
//   Общее: DrawHUD() — диспетчер: решает, что рисовать по состоянию игры.
// ============================================================================
#include "UI/PlayerGameHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Math/UnrealMathUtility.h"
#include "ThiefPlayerController.h"
#include "GameplayCharacterBase.h"
#include "Systems/WorkOrderSubsystem.h"

namespace
{
	const FLinearColor PanelBackgroundColor(0.06f, 0.07f, 0.09f, 0.90f);
	const FLinearColor MenuOverlayColor(0.02f, 0.02f, 0.04f, 0.82f);
	const FLinearColor PanelBorderColor(0.68f, 0.56f, 0.34f, 0.98f);
	const FLinearColor SecondaryPanelColor(0.11f, 0.13f, 0.16f, 0.95f);
	const FLinearColor StaminaFillColor(0.12f, 0.80f, 0.18f, 1.0f);
	const FLinearColor AccentColor(0.90f, 0.68f, 0.27f, 1.0f);
	const FLinearColor PositiveColor(0.43f, 0.74f, 0.55f, 1.0f);
	const FLinearColor MutedTextColor(0.81f, 0.79f, 0.74f, 1.0f);
	const FLinearColor ButtonColor(0.18f, 0.21f, 0.27f, 0.96f);
	const FLinearColor ButtonHoveredColor(0.25f, 0.30f, 0.38f, 0.98f);
	const FLinearColor PrimaryButtonColor(0.45f, 0.58f, 0.38f, 0.98f);
	const FLinearColor PrimaryButtonHoveredColor(0.57f, 0.72f, 0.46f, 1.0f);
	const FLinearColor HeroPanelColor(0.09f, 0.11f, 0.14f, 0.94f);
	const FLinearColor HighlightPanelColor(0.13f, 0.16f, 0.20f, 0.96f);
	const FLinearColor DangerButtonColor(0.50f, 0.22f, 0.18f, 0.98f);
	const FLinearColor DangerButtonHoveredColor(0.64f, 0.28f, 0.22f, 1.0f);
	const FLinearColor ValueBarBackgroundColor(0.10f, 0.09f, 0.08f, 1.0f);
	const FLinearColor InnerFrameColor(0.07f, 0.09f, 0.12f, 0.98f);
	const FLinearColor CopperShadowColor(0.18f, 0.10f, 0.04f, 0.35f);
	const FLinearColor HeaderRibbonColor(0.16f, 0.12f, 0.08f, 0.96f);

	void DrawPanel(UCanvas* Canvas, const FVector2D& Position, const FVector2D& Size, const FLinearColor& FillColor = PanelBackgroundColor)
	{
		if (!Canvas)
		{
			return;
		}

		const float Edge = FMath::Clamp(FMath::Min(Size.X, Size.Y) * 0.03f, 2.0f, 8.0f);
		const FVector2D ShadowOffset(Edge * 0.65f, Edge * 0.85f);
		FCanvasTileItem ShadowTile(Position + ShadowOffset, Size, CopperShadowColor);
		ShadowTile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(ShadowTile);

		FCanvasTileItem BackgroundTile(Position, Size, FillColor);
		BackgroundTile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(BackgroundTile);

		const FVector2D InnerPosition = Position + FVector2D(Edge, Edge);
		const FVector2D InnerSize = Size - FVector2D(Edge * 2.0f, Edge * 2.0f);
		if (InnerSize.X > 0.0f && InnerSize.Y > 0.0f)
		{
			FCanvasTileItem InnerTile(InnerPosition, InnerSize, FLinearColor(FillColor.R + 0.015f, FillColor.G + 0.015f, FillColor.B + 0.02f, FillColor.A));
			InnerTile.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(InnerTile);
		}

		const bool bDecoratedPanel = Size.X >= 220.0f && Size.Y >= 88.0f;
		if (bDecoratedPanel)
		{
			const float StripHeight = FMath::Clamp(Edge * 0.9f, 2.0f, 6.0f);
			FCanvasTileItem AccentStrip(Position + FVector2D(0.0f, Edge * 0.35f), FVector2D(Size.X, StripHeight), AccentColor);
			AccentStrip.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(AccentStrip);

			FCanvasTileItem HeaderGlow(Position + FVector2D(0.0f, Edge * 0.35f), FVector2D(Size.X, StripHeight * 3.0f), HeaderRibbonColor);
			HeaderGlow.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(HeaderGlow);
		}

		FCanvasBoxItem InnerBorder(InnerPosition, InnerSize);
		InnerBorder.SetColor(FLinearColor(0.86f, 0.79f, 0.64f, 0.12f));
		Canvas->DrawItem(InnerBorder);

		FCanvasBoxItem BorderBox(Position, Size);
		BorderBox.SetColor(PanelBorderColor);
		Canvas->DrawItem(BorderBox);

		if (Size.X >= 320.0f && Size.Y >= 120.0f)
		{
			const FVector2D RivetSize(FMath::Clamp(Edge * 1.15f, 4.0f, 8.0f), FMath::Clamp(Edge * 1.15f, 4.0f, 8.0f));
			const float Inset = Edge + 3.0f;
			const TArray<FVector2D> RivetPositions =
			{
				Position + FVector2D(Inset, Inset),
				Position + FVector2D(Size.X - Inset - RivetSize.X, Inset),
				Position + FVector2D(Inset, Size.Y - Inset - RivetSize.Y),
				Position + FVector2D(Size.X - Inset - RivetSize.X, Size.Y - Inset - RivetSize.Y)
			};

			for (const FVector2D& RivetPosition : RivetPositions)
			{
				FCanvasTileItem RivetTile(RivetPosition, RivetSize, AccentColor);
				RivetTile.BlendMode = SE_BLEND_Translucent;
				Canvas->DrawItem(RivetTile);
			}
		}
	}

	void DrawTextLine(UCanvas* Canvas, UFont* Font, const FString& Text, const FVector2D& Position, const FLinearColor& Color, bool bCenterX = false, float Scale = 1.0f)
	{
		if (!Canvas || !Font)
		{
			return;
		}

		FVector2D DrawPosition = Position;
		if (bCenterX)
		{
			float TextWidth = 0.0f;
			float TextHeight = 0.0f;
			Canvas->StrLen(Font, Text, TextWidth, TextHeight);
			DrawPosition.X -= (TextWidth * Scale) * 0.5f;
		}

		FCanvasTextItem TextItem(DrawPosition, FText::FromString(Text), Font, Color);
		TextItem.Scale = FVector2D(Scale, Scale);
		TextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(TextItem);
	}

	void DrawWrappedTextBlock(
		UCanvas* Canvas,
		UFont* Font,
		const FString& Text,
		const FVector2D& Position,
		const float MaxWidth,
		const FLinearColor& Color,
		const float Scale = 1.0f,
		const float LineSpacingScale = 1.1f)
	{
		if (!Canvas || !Font || Text.IsEmpty() || MaxWidth <= 0.0f)
		{
			return;
		}

		float SampleWidth = 0.0f;
		float SampleHeight = 0.0f;
		Canvas->StrLen(Font, TEXT("Ag"), SampleWidth, SampleHeight);
		const float LineAdvance = FMath::Max(1.0f, SampleHeight * Scale * LineSpacingScale);

		TArray<FString> Paragraphs;
		Text.ParseIntoArray(Paragraphs, TEXT("\n"), false);
		if (Paragraphs.Num() == 0)
		{
			Paragraphs.Add(Text);
		}

		float CurrentY = Position.Y;
		for (int32 ParagraphIndex = 0; ParagraphIndex < Paragraphs.Num(); ++ParagraphIndex)
		{
			TArray<FString> Words;
			Paragraphs[ParagraphIndex].ParseIntoArray(Words, TEXT(" "), true);

			if (Words.Num() == 0)
			{
				CurrentY += LineAdvance;
				continue;
			}

			FString CurrentLine;
			for (const FString& Word : Words)
			{
				const FString CandidateLine = CurrentLine.IsEmpty() ? Word : FString::Printf(TEXT("%s %s"), *CurrentLine, *Word);
				float CandidateWidth = 0.0f;
				float CandidateHeight = 0.0f;
				Canvas->StrLen(Font, CandidateLine, CandidateWidth, CandidateHeight);

				if (!CurrentLine.IsEmpty() && CandidateWidth * Scale > MaxWidth)
				{
					DrawTextLine(Canvas, Font, CurrentLine, FVector2D(Position.X, CurrentY), Color, false, Scale);
					CurrentY += LineAdvance;
					CurrentLine = Word;
				}
				else
				{
					CurrentLine = CandidateLine;
				}
			}

			if (!CurrentLine.IsEmpty())
			{
				DrawTextLine(Canvas, Font, CurrentLine, FVector2D(Position.X, CurrentY), Color, false, Scale);
				CurrentY += LineAdvance;
			}

			if (ParagraphIndex < Paragraphs.Num() - 1)
			{
				CurrentY += LineAdvance * 0.2f;
			}
		}
	}

	FString GetCompactKeyLabel(const FText& KeyText)
	{
		const FString Label = KeyText.ToString();
		if (Label.Equals(TEXT("Left Mouse Button"), ESearchCase::IgnoreCase))
		{
			return TEXT("LMB");
		}

		if (Label.Equals(TEXT("Right Mouse Button"), ESearchCase::IgnoreCase))
		{
			return TEXT("RMB");
		}

		if (Label.Equals(TEXT("Middle Mouse Button"), ESearchCase::IgnoreCase))
		{
			return TEXT("MMB");
		}

		if (Label.Equals(TEXT("Mouse Wheel Axis"), ESearchCase::IgnoreCase))
		{
			return TEXT("Wheel");
		}

		return Label;
	}

	bool IsRussian(const AThiefPlayerController* ThiefController)
	{
		return ThiefController && ThiefController->IsRussianLanguage();
	}

	FString LocalizeText(const AThiefPlayerController* ThiefController, const TCHAR* EnglishText, const TCHAR* RussianText)
	{
		return IsRussian(ThiefController) ? FString(RussianText) : FString(EnglishText);
	}

	FString FormatShiftTimerText(const float RemainingShiftTime, const bool bRussian)
	{
		const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(RemainingShiftTime));
		const int32 Minutes = TotalSeconds / 60;
		const int32 Seconds = TotalSeconds % 60;
		return bRussian
			? FString::Printf(TEXT("Время смены: %02d:%02d"), Minutes, Seconds)
			: FString::Printf(TEXT("Shift Time: %02d:%02d"), Minutes, Seconds);
	}

	FString GetShiftStateLabel(const AThiefPlayerController* ThiefController, const EWorkOrderShiftState ShiftState)
	{
		switch (ShiftState)
		{
		case EWorkOrderShiftState::ReadyToTurnIn:
			return LocalizeText(ThiefController, TEXT("Ready to turn in"), TEXT("Готово к сдаче"));
		case EWorkOrderShiftState::Completed:
			return LocalizeText(ThiefController, TEXT("Shift completed"), TEXT("Смена закрыта"));
		case EWorkOrderShiftState::Failed:
			return LocalizeText(ThiefController, TEXT("Shift failed"), TEXT("Смена провалена"));
		case EWorkOrderShiftState::Abandoned:
			return LocalizeText(ThiefController, TEXT("Shift abandoned"), TEXT("Смена прервана"));
		case EWorkOrderShiftState::Active:
		default:
			return LocalizeText(ThiefController, TEXT("Shift in progress"), TEXT("Смена в работе"));
		}
	}

	FString GetShiftResultTitle(const AThiefPlayerController* ThiefController, const EWorkOrderShiftState ShiftState)
	{
		switch (ShiftState)
		{
		case EWorkOrderShiftState::Completed:
			return LocalizeText(ThiefController, TEXT("SHIFT COMPLETED"), TEXT("СМЕНА ЗАВЕРШЕНА"));
		case EWorkOrderShiftState::Abandoned:
			return LocalizeText(ThiefController, TEXT("SHIFT ABANDONED"), TEXT("СМЕНА ПРЕРВАНА"));
		case EWorkOrderShiftState::Failed:
		default:
			return LocalizeText(ThiefController, TEXT("SHIFT FAILED"), TEXT("СМЕНА ПРОВАЛЕНА"));
		}
	}

	FString GetMiniQuestTitle(const AThiefPlayerController* ThiefController, int32 QuestIndex)
	{
		switch (QuestIndex)
		{
		case 0:
			return LocalizeText(ThiefController, TEXT("Crack The First Veins"), TEXT("Первые жилы"));
		case 1:
			return LocalizeText(ThiefController, TEXT("Ore Runner"), TEXT("Рудный рейс"));
		case 2:
			return LocalizeText(ThiefController, TEXT("Merchant Route"), TEXT("Маршрут торговца"));
		case 3:
			return LocalizeText(ThiefController, TEXT("Supply Belt"), TEXT("Пояс припасов"));
		case 4:
			return LocalizeText(ThiefController, TEXT("Second Breath"), TEXT("Второе дыхание"));
		case 5:
			return LocalizeText(ThiefController, TEXT("First Promotion"), TEXT("Первое повышение"));
		default:
			return LocalizeText(ThiefController, TEXT("Mini Quest"), TEXT("Мини-задание"));
		}
	}

	FString GetMiniQuestDescription(const AThiefPlayerController* ThiefController, int32 QuestIndex)
	{
		switch (QuestIndex)
		{
		case 0:
			return LocalizeText(ThiefController, TEXT("Break 3 ore nodes."), TEXT("Разбей 3 залежи руды."));
		case 1:
			return LocalizeText(ThiefController, TEXT("Collect 20 ore in total."), TEXT("Собери всего 20 руды."));
		case 2:
			return LocalizeText(ThiefController, TEXT("Sell 15 ore to the trader."), TEXT("Продай торговцу 15 руды."));
		case 3:
			return LocalizeText(ThiefController, TEXT("Buy 2 stamina potions."), TEXT("Купи 2 зелья стамины."));
		case 4:
			return LocalizeText(ThiefController, TEXT("Use 2 stamina potions."), TEXT("Используй 2 зелья стамины."));
		case 5:
			return LocalizeText(ThiefController, TEXT("Reach player level 3."), TEXT("Достигни 3 уровня."));
		default:
			return LocalizeText(ThiefController, TEXT("Keep pushing the run."), TEXT("Продолжай забег."));
		}
	}
}

// [этапы 1-9] Главная отрисовка HUD — диспетчер: каждый кадр очищаю список кнопок
// и по состоянию игры (лобби/пауза/торговля/прокачка/настройки/заказы) решаю,
// какие панели нарисовать. Здесь же кэширую мышь и обновляю перетаскивание ползунков.
void APlayerGameHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || !PlayerOwner)
	{
		return;
	}

	ActiveButtons.Reset();
	ActiveSliders.Reset();

	const float ViewportWidth = Canvas->SizeX;
	const float ViewportHeight = Canvas->SizeY;
	if (ViewportWidth <= 0.0f || ViewportHeight <= 0.0f)
	{
		return;
	}

	// Cache the cursor position once per frame so per-button hover tests do not
	// query the player controller dozens of times while drawing the menus.
	CachedMousePosition = GetMousePosition();

	UpdateLevelUpPopupState();

	AThiefPlayerController* ThiefController = GetThiefPlayerController();
	if (ThiefController && ThiefController->IsInMenuMap())
	{
		DrawMainMenu(ViewportWidth, ViewportHeight);
		if (ThiefController->IsSettingsMenuOpen())
		{
			DrawSettingsPanel(ViewportWidth, ViewportHeight, false);
		}
		if (ThiefController->IsLoadingGameplayMap())
		{
			DrawLoadingScreen(ViewportWidth, ViewportHeight);
		}
		UpdateActiveSliderDrag();
		return;
	}

	const bool bHasOpenModal =
		ThiefController
		&& (ThiefController->IsTradeMenuOpen()
			|| ThiefController->IsProgressionMenuOpen()
			|| ThiefController->IsPauseMenuOpen()
			|| ThiefController->IsSettingsMenuOpen()
			|| ThiefController->IsShiftResultScreenOpen()
			|| ThiefController->IsLoadingGameplayMap());

	if (!bHasOpenModal)
	{
		DrawResourcePanel(ViewportWidth);
		DrawLevelPanel();
		DrawWorkOrderPanel(ViewportWidth, ViewportHeight);
		DrawStaminaPanel(ViewportHeight);
		DrawInteractionPrompt(ViewportWidth, ViewportHeight);
		DrawLevelUpPopup(ViewportWidth, ViewportHeight);
	}

	if (ThiefController)
	{
		if (ThiefController->IsShiftResultScreenOpen())
		{
			DrawShiftResultScreen(ViewportWidth, ViewportHeight);
			return;
		}

		if (ThiefController->IsTradeMenuOpen())
		{
			DrawTradePanel(ViewportWidth, ViewportHeight);
		}

		if (ThiefController->IsProgressionMenuOpen())
		{
			DrawProgressionPanel(ViewportWidth, ViewportHeight);
		}

		if (ThiefController->IsPauseMenuOpen())
		{
			DrawPauseMenu(ViewportWidth, ViewportHeight);
		}

		if (ThiefController->IsSettingsMenuOpen())
		{
			DrawSettingsPanel(ViewportWidth, ViewportHeight, ThiefController->IsPauseMenuOpen());
		}
	}

	UpdateActiveSliderDrag();
}

void APlayerGameHUD::UpdateLevelUpPopupState()
{
	AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter();
	if (!PlayerCharacter)
	{
		LastObservedPlayerLevel = INDEX_NONE;
		return;
	}

	const int32 CurrentLevel = PlayerCharacter->GetPlayerLevel();
	if (LastObservedPlayerLevel == INDEX_NONE)
	{
		LastObservedPlayerLevel = CurrentLevel;
		return;
	}

	if (CurrentLevel > LastObservedPlayerLevel)
	{
		LevelPopupDisplayedLevel = CurrentLevel;
		LevelPopupDisplayedPoints = PlayerCharacter->GetAvailableUpgradePoints();

		if (const UWorld* World = GetWorld())
		{
			LevelPopupStartTime = World->GetTimeSeconds();
		}
		else
		{
			LevelPopupStartTime = 0.0f;
		}
	}

	LastObservedPlayerLevel = CurrentLevel;
}

bool APlayerGameHUD::HandleClick(const FVector2D& ScreenPosition)
{
	AThiefPlayerController* ThiefController = GetThiefPlayerController();
	if (!ThiefController)
	{
		return false;
	}

	// Sliders take priority over buttons: a press on the bar grabs it for dragging
	// and immediately jumps the value to the click position.
	for (int32 Index = ActiveSliders.Num() - 1; Index >= 0; --Index)
	{
		const FHUDSliderData& Slider = ActiveSliders[Index];
		const bool bInsideX = ScreenPosition.X >= Slider.Position.X && ScreenPosition.X <= (Slider.Position.X + Slider.Size.X);
		const bool bInsideY = ScreenPosition.Y >= Slider.Position.Y && ScreenPosition.Y <= (Slider.Position.Y + Slider.Size.Y);
		if (!bInsideX || !bInsideY)
		{
			continue;
		}

		ActiveDragSlider = Slider.Target;
		const float Ratio = FMath::Clamp((ScreenPosition.X - Slider.Position.X) / FMath::Max(Slider.Size.X, 1.0f), 0.0f, 1.0f);
		ThiefController->SetSettingValueFromRatio(Slider.Target, Ratio);
		return true;
	}

	for (int32 Index = ActiveButtons.Num() - 1; Index >= 0; --Index)
	{
		const FHUDButtonData& ButtonData = ActiveButtons[Index];
		const bool bInsideX = ScreenPosition.X >= ButtonData.Position.X && ScreenPosition.X <= (ButtonData.Position.X + ButtonData.Size.X);
		const bool bInsideY = ScreenPosition.Y >= ButtonData.Position.Y && ScreenPosition.Y <= (ButtonData.Position.Y + ButtonData.Size.Y);
		if (!bInsideX || !bInsideY)
		{
			continue;
		}

		switch (ButtonData.Action)
		{
		case EHUDMenuAction::StartGame:
			ThiefController->StartGameplayFromMenu();
			return true;
		case EHUDMenuAction::WorkOrderPrev:
			if (UWorkOrderSubsystem* WorkOrderSubsystem = ThiefController->GetGameInstance()
				? ThiefController->GetGameInstance()->GetSubsystem<UWorkOrderSubsystem>()
				: nullptr)
			{
				return WorkOrderSubsystem->CycleSelectedWorkOrder(-1);
			}
			return false;
		case EHUDMenuAction::WorkOrderNext:
			if (UWorkOrderSubsystem* WorkOrderSubsystem = ThiefController->GetGameInstance()
				? ThiefController->GetGameInstance()->GetSubsystem<UWorkOrderSubsystem>()
				: nullptr)
			{
				return WorkOrderSubsystem->CycleSelectedWorkOrder(1);
			}
			return false;
		case EHUDMenuAction::OpenSettings:
			ThiefController->OpenSettingsMenu();
			return true;
		case EHUDMenuAction::CloseSettings:
			ThiefController->CloseSettingsMenu();
			return true;
		case EHUDMenuAction::ResumeGame:
			ThiefController->TogglePauseMenu();
			return true;
		case EHUDMenuAction::ReturnToLobby:
			ThiefController->ReturnToLobby();
			return true;
		case EHUDMenuAction::QuitGame:
			ThiefController->RequestQuitGame();
			return true;
		case EHUDMenuAction::SelectSettingsTab:
			ThiefController->SetActiveSettingsTab(static_cast<ESettingsPanelTab>(ButtonData.Payload));
			return true;
		case EHUDMenuAction::MasterVolumeDown:
			ThiefController->AdjustMasterVolume(-0.1f);
			return true;
		case EHUDMenuAction::MasterVolumeUp:
			ThiefController->AdjustMasterVolume(0.1f);
			return true;
		case EHUDMenuAction::MusicDown:
			ThiefController->AdjustMusicVolume(-0.1f);
			return true;
		case EHUDMenuAction::MusicUp:
			ThiefController->AdjustMusicVolume(0.1f);
			return true;
		case EHUDMenuAction::SfxDown:
			ThiefController->AdjustSfxVolume(-0.1f);
			return true;
		case EHUDMenuAction::SfxUp:
			ThiefController->AdjustSfxVolume(0.1f);
			return true;
		case EHUDMenuAction::LookSensitivityDown:
			ThiefController->AdjustLookSensitivity(-0.01f);
			return true;
		case EHUDMenuAction::LookSensitivityUp:
			ThiefController->AdjustLookSensitivity(0.01f);
			return true;
		case EHUDMenuAction::ToggleInvertLookY:
			ThiefController->ToggleLookYInversion();
			return true;
		case EHUDMenuAction::MenuScaleDown:
			ThiefController->AdjustMenuScale(-0.05f);
			return true;
		case EHUDMenuAction::MenuScaleUp:
			ThiefController->AdjustMenuScale(0.05f);
			return true;
		case EHUDMenuAction::LanguagePrev:
			ThiefController->CycleLanguage(-1);
			return true;
		case EHUDMenuAction::LanguageNext:
			ThiefController->CycleLanguage(1);
			return true;
		case EHUDMenuAction::BeginInputRebind:
			ThiefController->BeginRebindingInput(static_cast<ERemappableInputAction>(ButtonData.Payload));
			return true;
		case EHUDMenuAction::ResetControls:
			ThiefController->ResetControlsToDefaults();
			return true;
		case EHUDMenuAction::TradeSellOne:
			ThiefController->ExecuteMenuOption(1);
			return true;
		case EHUDMenuAction::TradeSellFive:
			ThiefController->ExecuteMenuOption(2);
			return true;
		case EHUDMenuAction::TradeBuyPotion:
			ThiefController->ExecuteMenuOption(3);
			return true;
		case EHUDMenuAction::UpgradeMaxStamina:
			ThiefController->ExecuteMenuOption(1);
			return true;
		case EHUDMenuAction::UpgradeOreDamage:
			ThiefController->ExecuteMenuOption(2);
			return true;
		case EHUDMenuAction::UpgradeMoveSpeed:
			ThiefController->ExecuteMenuOption(3);
			return true;
		case EHUDMenuAction::ResetCharacterProgress:
			ThiefController->ResetAllProgress();
			if (AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter())
			{
				LastObservedPlayerLevel = PlayerCharacter->GetPlayerLevel();
			}
			else
			{
				LastObservedPlayerLevel = INDEX_NONE;
			}
			LevelPopupDisplayedLevel = 0;
			LevelPopupDisplayedPoints = 0;
			LevelPopupStartTime = -1000.0f;
			return true;
		case EHUDMenuAction::CloseShiftResult:
			ThiefController->DismissShiftResultScreen();
			return true;
		case EHUDMenuAction::AbandonShift:
			ThiefController->AbandonActiveShift();
			return true;
		default:
			return false;
		}
	}

	return false;
}

bool APlayerGameHUD::HandleScroll(float WheelDelta)
{
	AThiefPlayerController* ThiefController = GetThiefPlayerController();
	if (!ThiefController
		|| !ThiefController->IsSettingsMenuOpen()
		|| ThiefController->GetActiveSettingsTab() != ESettingsPanelTab::Controls
		|| ThiefController->IsWaitingForInputRebind()
		|| ControlsBindingsMaxScroll <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float ScrollStep = 72.0f;
	ControlsBindingsScrollOffset = FMath::Clamp(
		ControlsBindingsScrollOffset + WheelDelta * ScrollStep,
		0.0f,
		ControlsBindingsMaxScroll);
	return true;
}

void APlayerGameHUD::DrawMainMenu(float ViewportWidth, float ViewportHeight)
{
	UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!LargeFont || !MediumFont || !SmallFont)
	{
		return;
	}

	const AThiefPlayerController* ThiefController = GetThiefPlayerController();
	if (!ThiefController)
	{
		return;
	}

	const UWorkOrderSubsystem* WorkOrderSubsystem = ThiefController->GetGameInstance()
		? ThiefController->GetGameInstance()->GetSubsystem<UWorkOrderSubsystem>()
		: nullptr;
	const float MenuScale = ThiefController ? ThiefController->GetMenuScaleSetting() : 1.0f;
	const float EffectiveScale = FMath::Clamp(
		MenuScale * FMath::Min(ViewportWidth / 1920.0f, ViewportHeight / 1080.0f),
		0.82f,
		1.08f);
	const float TextScale = FMath::Clamp(EffectiveScale, 0.72f, 1.0f);
	const float SummaryScale = FMath::Clamp(TextScale * 0.88f, 0.64f, 0.94f);
	const float FeatureDescriptionScale = FMath::Clamp(TextScale * 0.82f, 0.60f, 0.90f);
	FCanvasTileItem Overlay(FVector2D::ZeroVector, FVector2D(ViewportWidth, ViewportHeight), MenuOverlayColor);
	Overlay.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Overlay);

	const float HorizontalMargin = FMath::Clamp(ViewportWidth * 0.035f, 24.0f, 72.0f);
	const float VerticalMargin = FMath::Clamp(ViewportHeight * 0.04f, 24.0f, 60.0f);
	const FVector2D PanelPosition(HorizontalMargin, VerticalMargin);
	const FVector2D PanelSize(ViewportWidth - HorizontalMargin * 2.0f, ViewportHeight - VerticalMargin * 2.0f);
	DrawPanel(Canvas, PanelPosition, PanelSize, FLinearColor(0.03f, 0.04f, 0.05f, 0.92f));

	FCanvasTileItem AccentStrip(PanelPosition + FVector2D(0.0f, 0.0f), FVector2D(PanelSize.X, 10.0f * EffectiveScale), AccentColor);
	AccentStrip.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(AccentStrip);

	const float InnerGap = 28.0f * EffectiveScale;
	const float ActionPanelWidth = FMath::Clamp(PanelSize.X * 0.30f, 340.0f * EffectiveScale, 420.0f * EffectiveScale);
	const FVector2D HeroSize(PanelSize.X - ActionPanelWidth - InnerGap - 64.0f * EffectiveScale, PanelSize.Y - 72.0f * EffectiveScale);
	const FVector2D HeroPosition = PanelPosition + FVector2D(32.0f * EffectiveScale, 26.0f * EffectiveScale);
	DrawPanel(Canvas, HeroPosition, HeroSize, HeroPanelColor);

	const FVector2D ActionPanelSize(ActionPanelWidth, HeroSize.Y);
	const FVector2D ActionPanelPosition = PanelPosition + FVector2D(PanelSize.X - ActionPanelSize.X - 32.0f * EffectiveScale, 26.0f * EffectiveScale);
	DrawPanel(Canvas, ActionPanelPosition, ActionPanelSize, HighlightPanelColor);

	DrawTextLine(Canvas, LargeFont, TEXT("RPG FARM"), HeroPosition + FVector2D(28.0f * EffectiveScale, 34.0f * EffectiveScale), AccentColor, false, TextScale);
	DrawTextLine(
		Canvas,
		MediumFont,
		LocalizeText(ThiefController, TEXT("Set up the run in the lobby before entering Showcase."), TEXT("Подготовь заход в мире прямо из стартового лобби.")),
		HeroPosition + FVector2D(28.0f * EffectiveScale, 92.0f * EffectiveScale),
		FLinearColor::White,
		false,
		TextScale);
	DrawTextLine(
		Canvas,
		MediumFont,
		LocalizeText(ThiefController, TEXT("Choose controls, sound and UI first, then start."), TEXT("Сначала настрой звук, управление и UI, потом запускай игру.")),
		HeroPosition + FVector2D(28.0f * EffectiveScale, 126.0f * EffectiveScale),
		MutedTextColor,
		false,
		TextScale);

	const float FeatureGap = 18.0f * EffectiveScale;
	const FVector2D FeatureRowPosition = HeroPosition + FVector2D(28.0f * EffectiveScale, 188.0f * EffectiveScale);
	const FVector2D FeatureCardSize((HeroSize.X - 56.0f * EffectiveScale - FeatureGap * 2.0f) / 3.0f, 104.0f * EffectiveScale);
	const FVector2D MineCardPosition = FeatureRowPosition;
	const FVector2D TradeCardPosition = FeatureRowPosition + FVector2D(FeatureCardSize.X + FeatureGap, 0.0f);
	const FVector2D UpgradeCardPosition = FeatureRowPosition + FVector2D((FeatureCardSize.X + FeatureGap) * 2.0f, 0.0f);
	DrawPanel(Canvas, MineCardPosition, FeatureCardSize, SecondaryPanelColor);
	DrawPanel(Canvas, TradeCardPosition, FeatureCardSize, SecondaryPanelColor);
	DrawPanel(Canvas, UpgradeCardPosition, FeatureCardSize, SecondaryPanelColor);

	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Mine"), TEXT("Добыча")), MineCardPosition + FVector2D(18.0f * EffectiveScale, 16.0f * EffectiveScale), AccentColor, false, TextScale);
	DrawWrappedTextBlock(Canvas, SmallFont, LocalizeText(ThiefController, TEXT("Break ore and gather resources."), TEXT("Ломай руду и забирай ресурсы.")), MineCardPosition + FVector2D(18.0f * EffectiveScale, 48.0f * EffectiveScale), FeatureCardSize.X - 36.0f * EffectiveScale, FLinearColor::White, FeatureDescriptionScale);
	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Trade"), TEXT("Торговля")), TradeCardPosition + FVector2D(18.0f * EffectiveScale, 16.0f * EffectiveScale), AccentColor, false, TextScale);
	DrawWrappedTextBlock(Canvas, SmallFont, LocalizeText(ThiefController, TEXT("Sell ore and restore stamina."), TEXT("Продавай руду и пополняй стамину.")), TradeCardPosition + FVector2D(18.0f * EffectiveScale, 48.0f * EffectiveScale), FeatureCardSize.X - 36.0f * EffectiveScale, FLinearColor::White, FeatureDescriptionScale);
	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Upgrade"), TEXT("Прокачка")), UpgradeCardPosition + FVector2D(18.0f * EffectiveScale, 16.0f * EffectiveScale), AccentColor, false, TextScale);
	DrawWrappedTextBlock(Canvas, SmallFont, LocalizeText(ThiefController, TEXT("Boost stamina, damage and speed."), TEXT("Качай стамину, урон и скорость.")), UpgradeCardPosition + FVector2D(18.0f * EffectiveScale, 48.0f * EffectiveScale), FeatureCardSize.X - 36.0f * EffectiveScale, FLinearColor::White, FeatureDescriptionScale);

	const FVector2D SettingsCardPosition = HeroPosition + FVector2D(28.0f * EffectiveScale, 334.0f * EffectiveScale);
	const FVector2D SettingsCardSize(HeroSize.X - 56.0f * EffectiveScale, 214.0f * EffectiveScale);
	DrawPanel(Canvas, SettingsCardPosition, SettingsCardSize, FLinearColor(0.06f, 0.08f, 0.11f, 0.94f));

	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Ready Before Start"), TEXT("Подготовка до старта")), SettingsCardPosition + FVector2D(22.0f * EffectiveScale, 16.0f * EffectiveScale), AccentColor, false, TextScale);
	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Audio"), TEXT("Звук")), SettingsCardPosition + FVector2D(22.0f * EffectiveScale, 58.0f * EffectiveScale), MutedTextColor, false, TextScale);
	DrawTextLine(
		Canvas,
		MediumFont,
		FString::Printf(
			TEXT("%s %d%%   |   %s %d%%   |   %s %d%%"),
			*LocalizeText(ThiefController, TEXT("Master"), TEXT("Master")),
			FMath::RoundToInt((ThiefController ? ThiefController->GetMasterVolumeSetting() : 1.0f) * 100.0f),
			*LocalizeText(ThiefController, TEXT("Music"), TEXT("Music")),
			FMath::RoundToInt((ThiefController ? ThiefController->GetMusicVolumeSetting() : 1.0f) * 100.0f),
			*LocalizeText(ThiefController, TEXT("SFX"), TEXT("SFX")),
			FMath::RoundToInt((ThiefController ? ThiefController->GetSfxVolumeSetting() : 1.0f) * 100.0f)),
		SettingsCardPosition + FVector2D(138.0f * EffectiveScale, 58.0f * EffectiveScale),
		FLinearColor::White,
		false,
		SummaryScale);
	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Controls"), TEXT("Управление")), SettingsCardPosition + FVector2D(22.0f * EffectiveScale, 96.0f * EffectiveScale), MutedTextColor, false, TextScale);
	DrawTextLine(
		Canvas,
		MediumFont,
		FString::Printf(
			TEXT("%s %s   |   %s %s   |   %s %.2f"),
			*LocalizeText(ThiefController, TEXT("Forward"), TEXT("Вперёд")),
			*GetCompactKeyLabel(ThiefController->GetInputActionKeyText(ERemappableInputAction::MoveForward)),
			*LocalizeText(ThiefController, TEXT("Attack"), TEXT("Удар")),
			*GetCompactKeyLabel(ThiefController->GetInputActionKeyText(ERemappableInputAction::Attack)),
			*LocalizeText(ThiefController, TEXT("Look"), TEXT("Чувств.")),
			ThiefController ? ThiefController->GetLookSensitivitySetting() : 0.07f),
		SettingsCardPosition + FVector2D(138.0f * EffectiveScale, 96.0f * EffectiveScale),
		FLinearColor::White,
		false,
		SummaryScale);
	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Loop"), TEXT("Цикл")), SettingsCardPosition + FVector2D(22.0f * EffectiveScale, 134.0f * EffectiveScale), MutedTextColor, false, TextScale);
	DrawTextLine(
		Canvas,
		MediumFont,
		LocalizeText(ThiefController, TEXT("Lobby -> Loading -> Showcase -> Mine / Trade / Upgrade"), TEXT("Лобби -> Загрузка -> Showcase -> Добыча / Торговля / Прокачка")),
		SettingsCardPosition + FVector2D(138.0f * EffectiveScale, 134.0f * EffectiveScale),
		FLinearColor::White,
		false,
		SummaryScale);
	DrawTextLine(
		Canvas,
		SmallFont,
		LocalizeText(ThiefController, TEXT("Open settings before the first run and the game will remember them."), TEXT("Открой настройки до старта, и игра их запомнит.")),
		SettingsCardPosition + FVector2D(22.0f * EffectiveScale, 176.0f * EffectiveScale),
		MutedTextColor,
		false,
		TextScale);

	DrawTextLine(Canvas, LargeFont, LocalizeText(ThiefController, TEXT("Start Session"), TEXT("Старт сессии")), ActionPanelPosition + FVector2D(26.0f * EffectiveScale, 28.0f * EffectiveScale), AccentColor, false, TextScale);
	DrawTextLine(
		Canvas,
		MediumFont,
		LocalizeText(ThiefController, TEXT("All key settings are available directly in the lobby."), TEXT("Все важные настройки доступны прямо в лобби.")),
		ActionPanelPosition + FVector2D(26.0f * EffectiveScale, 72.0f * EffectiveScale),
		FLinearColor::White,
		false,
		TextScale);

	const FVector2D ButtonSize(ActionPanelSize.X - 52.0f * EffectiveScale, 60.0f * EffectiveScale);
	DrawMenuButton(LocalizeText(ThiefController, TEXT("Start Game"), TEXT("Начать игру")), ActionPanelPosition + FVector2D(26.0f * EffectiveScale, 146.0f * EffectiveScale), ButtonSize, EHUDMenuAction::StartGame, true);
	DrawMenuButton(LocalizeText(ThiefController, TEXT("Settings"), TEXT("Настройки")), ActionPanelPosition + FVector2D(26.0f * EffectiveScale, 222.0f * EffectiveScale), ButtonSize, EHUDMenuAction::OpenSettings);
	DrawMenuButton(LocalizeText(ThiefController, TEXT("Quit Game"), TEXT("Выйти из игры")), ActionPanelPosition + FVector2D(26.0f * EffectiveScale, 298.0f * EffectiveScale), ButtonSize, EHUDMenuAction::QuitGame);

	const FVector2D OrderPanelPosition = ActionPanelPosition + FVector2D(26.0f * EffectiveScale, 388.0f * EffectiveScale);
	const FVector2D OrderPanelSize(ButtonSize.X, 292.0f * EffectiveScale);
	DrawPanel(Canvas, OrderPanelPosition, OrderPanelSize, SecondaryPanelColor);
	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Work Order"), TEXT("Заказ на смену")), OrderPanelPosition + FVector2D(18.0f * EffectiveScale, 16.0f * EffectiveScale), AccentColor, false, TextScale);

	const int32 SelectedOrderIndex = WorkOrderSubsystem ? WorkOrderSubsystem->GetSelectedWorkOrderIndex() : 0;
	const int32 WorkOrderCount = WorkOrderSubsystem ? WorkOrderSubsystem->GetWorkOrderCount() : 0;
	const int32 UnlockedWorkOrderCount = WorkOrderSubsystem ? WorkOrderSubsystem->GetUnlockedWorkOrderCount() : 0;
	const int32 CompletedShiftCount = WorkOrderSubsystem ? WorkOrderSubsystem->GetCompletedShiftCount() : 0;
	const int32 Reputation = WorkOrderSubsystem ? WorkOrderSubsystem->GetReputation() : 0;
	const FWorkOrderDefinition* SelectedWorkOrder = WorkOrderSubsystem ? WorkOrderSubsystem->GetSelectedWorkOrder() : nullptr;

	const FVector2D SwitchButtonSize(42.0f * EffectiveScale, 38.0f * EffectiveScale);
	DrawMenuButton(TEXT("<"), OrderPanelPosition + FVector2D(18.0f * EffectiveScale, 52.0f * EffectiveScale), SwitchButtonSize, EHUDMenuAction::WorkOrderPrev);
	DrawMenuButton(TEXT(">"), OrderPanelPosition + FVector2D(OrderPanelSize.X - 18.0f * EffectiveScale - SwitchButtonSize.X, 52.0f * EffectiveScale), SwitchButtonSize, EHUDMenuAction::WorkOrderNext, true);

	const bool bSelectedOrderLocked = SelectedWorkOrder && WorkOrderSubsystem && !WorkOrderSubsystem->IsOrderUnlocked(SelectedOrderIndex);
	DrawTextLine(
		Canvas,
		SmallFont,
		SelectedWorkOrder
			? WorkOrderSubsystem->GetOrderTitle(SelectedOrderIndex, ThiefController->IsRussianLanguage())
			: LocalizeText(ThiefController, TEXT("No order"), TEXT("Заказ не выбран")),
		OrderPanelPosition + FVector2D(OrderPanelSize.X * 0.5f, 60.0f * EffectiveScale),
		bSelectedOrderLocked ? FLinearColor(1.0f, 0.55f, 0.45f, 1.0f) : FLinearColor::White,
		true,
		0.92f * TextScale);
	DrawTextLine(
		Canvas,
		SmallFont,
		FString::Printf(
			TEXT("%s %d/%d   |   %s %d"),
			*LocalizeText(ThiefController, TEXT("Unlocked"), TEXT("Открыто")),
			UnlockedWorkOrderCount,
			FMath::Max(1, WorkOrderCount),
			*LocalizeText(ThiefController, TEXT("Reputation"), TEXT("Репутация")),
			Reputation),
		OrderPanelPosition + FVector2D(OrderPanelSize.X * 0.5f, 88.0f * EffectiveScale),
		MutedTextColor,
		true,
		0.82f * TextScale);

	if (SelectedWorkOrder)
	{
		DrawWrappedTextBlock(
			Canvas,
			SmallFont,
			WorkOrderSubsystem->GetOrderObjectiveText(SelectedOrderIndex, ThiefController->IsRussianLanguage()),
			OrderPanelPosition + FVector2D(18.0f * EffectiveScale, 116.0f * EffectiveScale),
			OrderPanelSize.X - 36.0f * EffectiveScale,
			FLinearColor::White,
			0.86f * TextScale,
			1.0f);

		DrawTextLine(
			Canvas,
			SmallFont,
			FString::Printf(
				TEXT("%s %d   |   XP %d   |   %s +%d"),
				*LocalizeText(ThiefController, TEXT("Gold"), TEXT("Золото")),
				WorkOrderSubsystem->GetPreviewGoldReward(SelectedOrderIndex, false),
				WorkOrderSubsystem->GetPreviewExperienceReward(SelectedOrderIndex, false),
				*LocalizeText(ThiefController, TEXT("Rep"), TEXT("Реп.")),
				WorkOrderSubsystem->GetPreviewReputationReward(SelectedOrderIndex, false)),
			OrderPanelPosition + FVector2D(18.0f * EffectiveScale, 168.0f * EffectiveScale),
			AccentColor,
			false,
			0.84f * TextScale);
		DrawTextLine(
			Canvas,
			SmallFont,
			FString::Printf(
				TEXT("%s %d   |   %s %d"),
				*LocalizeText(ThiefController, TEXT("Minutes"), TEXT("Минуты")),
				FMath::RoundToInt(SelectedWorkOrder->ShiftDurationSeconds / 60.0f),
				*LocalizeText(ThiefController, TEXT("Done"), TEXT("Смен закрыто")),
				CompletedShiftCount),
			OrderPanelPosition + FVector2D(18.0f * EffectiveScale, 194.0f * EffectiveScale),
			PositiveColor,
			false,
			0.82f * TextScale);
		DrawWrappedTextBlock(
			Canvas,
			SmallFont,
			WorkOrderSubsystem->GetOrderBonusObjectiveText(SelectedOrderIndex, ThiefController->IsRussianLanguage()),
			OrderPanelPosition + FVector2D(18.0f * EffectiveScale, 222.0f * EffectiveScale),
			OrderPanelSize.X - 36.0f * EffectiveScale,
			PositiveColor,
			0.76f * TextScale,
			1.0f);
		DrawTextLine(
			Canvas,
			SmallFont,
			FString::Printf(
				TEXT("%s: %s"),
				*WorkOrderSubsystem->GetOrderModifierText(SelectedOrderIndex, ThiefController->IsRussianLanguage()),
				*WorkOrderSubsystem->GetOrderModifierEffectText(SelectedOrderIndex, ThiefController->IsRussianLanguage())),
			OrderPanelPosition + FVector2D(18.0f * EffectiveScale, 262.0f * EffectiveScale),
			MutedTextColor,
			false,
			0.70f * TextScale);
	}

	if (bSelectedOrderLocked)
	{
		const FWorkOrderDefinition* LockedDefinition = WorkOrderSubsystem ? WorkOrderSubsystem->GetWorkOrderDefinition(SelectedOrderIndex) : nullptr;
		const int32 RequiredShifts = LockedDefinition ? LockedDefinition->RequiredCompletedShifts : 0;
		DrawTextLine(
			Canvas,
			SmallFont,
			ThiefController->IsRussianLanguage()
				? FString::Printf(TEXT("Заблокировано - закрой смен: %d"), RequiredShifts)
				: FString::Printf(TEXT("Locked - complete %d shifts"), RequiredShifts),
			OrderPanelPosition + FVector2D(OrderPanelSize.X * 0.5f, OrderPanelSize.Y - 16.0f * EffectiveScale),
			FLinearColor(1.0f, 0.55f, 0.45f, 1.0f),
			true,
			0.74f * TextScale);
	}

	DrawTextLine(
		Canvas,
		MediumFont,
		LocalizeText(ThiefController, TEXT("Choose a work order, tune the lobby, then launch the shift."), TEXT("Выбери заказ, подготовь лобби и запускай смену.")),
		FVector2D(PanelPosition.X + PanelSize.X * 0.5f, PanelPosition.Y + PanelSize.Y - 34.0f * EffectiveScale),
		MutedTextColor,
		true,
		TextScale);
}

void APlayerGameHUD::DrawLoadingScreen(float ViewportWidth, float ViewportHeight)
{
	UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!LargeFont || !MediumFont)
	{
		return;
	}

	const AThiefPlayerController* ThiefController = GetThiefPlayerController();

	FCanvasTileItem Overlay(FVector2D::ZeroVector, FVector2D(ViewportWidth, ViewportHeight), FLinearColor(0.0f, 0.0f, 0.0f, 0.86f));
	Overlay.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Overlay);

	const FVector2D PanelSize(540.0f, 220.0f);
	const FVector2D PanelPosition((ViewportWidth - PanelSize.X) * 0.5f, (ViewportHeight - PanelSize.Y) * 0.5f);
	DrawPanel(Canvas, PanelPosition, PanelSize, FLinearColor(0.03f, 0.04f, 0.05f, 0.95f));

	FString LoadingText = LocalizeText(ThiefController, TEXT("Loading"), TEXT("Загрузка"));
	if (const UWorld* World = GetWorld())
	{
		const int32 DotCount = FMath::FloorToInt(World->GetTimeSeconds() * 4.0f) % 4;
		for (int32 Index = 0; Index < DotCount; ++Index)
		{
			LoadingText.AppendChar(TEXT('.'));
		}
	}

	DrawTextLine(Canvas, LargeFont, LoadingText, PanelPosition + FVector2D(PanelSize.X * 0.5f, 44.0f), AccentColor, true);
	DrawTextLine(
		Canvas,
		MediumFont,
		LocalizeText(ThiefController, TEXT("Preparing the world and entering gameplay"), TEXT("Подготавливаем мир и загружаем сцену")),
		PanelPosition + FVector2D(PanelSize.X * 0.5f, 102.0f),
		FLinearColor::White,
		true);

	const FVector2D BarPosition = PanelPosition + FVector2D(48.0f, 156.0f);
	const FVector2D BarSize(PanelSize.X - 96.0f, 18.0f);
	FCanvasTileItem BarBackground(BarPosition, BarSize, FLinearColor(0.08f, 0.08f, 0.08f, 1.0f));
	BarBackground.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BarBackground);

	const float AnimatedFill = 0.35f + 0.25f * FMath::Sin((GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) * 3.0f);
	FCanvasTileItem BarFill(BarPosition, FVector2D(BarSize.X * AnimatedFill, BarSize.Y), AccentColor);
	BarFill.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BarFill);
}

void APlayerGameHUD::DrawPauseMenu(float ViewportWidth, float ViewportHeight)
{
	UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!LargeFont || !MediumFont || !SmallFont)
	{
		return;
	}

	const AThiefPlayerController* ThiefController = GetThiefPlayerController();
	AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter();
	const float MenuScale = ThiefController ? ThiefController->GetMenuScaleSetting() : 1.0f;
	const float EffectiveScale = FMath::Clamp(
		MenuScale * FMath::Min(ViewportWidth / 1920.0f, ViewportHeight / 1080.0f),
		0.94f,
		1.16f);
	const float TextScale = FMath::Clamp(EffectiveScale * 1.08f, 1.02f, 1.22f);
	const float BodyScale = TextScale;
	const float DetailScale = FMath::Clamp(0.94f * TextScale, 0.92f, 1.08f);
	FCanvasTileItem Overlay(FVector2D::ZeroVector, FVector2D(ViewportWidth, ViewportHeight), MenuOverlayColor);
	Overlay.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Overlay);

	const float HorizontalMargin = FMath::Clamp(ViewportWidth * 0.05f, 36.0f, 84.0f);
	const float VerticalMargin = FMath::Clamp(ViewportHeight * 0.05f, 28.0f, 72.0f);
	const FVector2D PanelPosition(HorizontalMargin, VerticalMargin);
	const FVector2D PanelSize(ViewportWidth - HorizontalMargin * 2.0f, ViewportHeight - VerticalMargin * 2.0f);
	DrawPanel(Canvas, PanelPosition, PanelSize, FLinearColor(0.03f, 0.04f, 0.05f, 0.94f));
	FCanvasTileItem AccentStrip(PanelPosition, FVector2D(PanelSize.X, 10.0f * EffectiveScale), AccentColor);
	AccentStrip.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(AccentStrip);

	DrawTextLine(Canvas, LargeFont, LocalizeText(ThiefController, TEXT("Game Menu"), TEXT("Меню игры")), PanelPosition + FVector2D(PanelSize.X * 0.5f, 48.0f * EffectiveScale), AccentColor, true, TextScale);
	DrawTextLine(
		Canvas,
		MediumFont,
		LocalizeText(ThiefController, TEXT("Pause, inspect goals, tweak settings, or reset the run."), TEXT("Пауза, просмотр целей, настройки и сброс текущего прогресса.")),
		PanelPosition + FVector2D(PanelSize.X * 0.5f, 100.0f * EffectiveScale),
		FLinearColor::White,
		true,
		TextScale);

	const FVector2D InfoPanelPosition = PanelPosition + FVector2D(40.0f * EffectiveScale, 160.0f * EffectiveScale);
	const FVector2D InfoPanelSize(PanelSize.X * 0.48f, PanelSize.Y - 240.0f * EffectiveScale);
	DrawPanel(Canvas, InfoPanelPosition, InfoPanelSize, HeroPanelColor);
	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Mini Objectives"), TEXT("Мини-задания")), InfoPanelPosition + FVector2D(24.0f * EffectiveScale, 22.0f * EffectiveScale), AccentColor, false, TextScale);
	DrawWrappedTextBlock(
		Canvas,
		MediumFont,
		LocalizeText(ThiefController, TEXT("Open Esc any time to see what is left in the current run. Progress updates automatically while you mine, trade, level up and use consumables."), TEXT("Открывай Esc в любой момент, чтобы видеть цели текущего захода. Прогресс обновляется сам во время добычи, торговли, прокачки и использования зелий.")),
		InfoPanelPosition + FVector2D(24.0f * EffectiveScale, 62.0f * EffectiveScale),
		InfoPanelSize.X - 48.0f * EffectiveScale,
		FLinearColor::White,
		0.82f * BodyScale,
		1.0f);

	const int32 QuestCount = PlayerCharacter ? PlayerCharacter->GetMiniQuestCount() : 0;
	const int32 CompletedQuestCount = PlayerCharacter ? PlayerCharacter->GetCompletedMiniQuestCount() : 0;
	DrawTextLine(
		Canvas,
		MediumFont,
		LocalizeText(
			ThiefController,
			*FString::Printf(TEXT("Completed: %d / %d"), CompletedQuestCount, QuestCount),
			*FString::Printf(TEXT("Выполнено: %d / %d"), CompletedQuestCount, QuestCount)),
		InfoPanelPosition + FVector2D(24.0f * EffectiveScale, 138.0f * EffectiveScale),
		CompletedQuestCount >= QuestCount && QuestCount > 0 ? PositiveColor : FLinearColor::White,
		false,
		1.0f * TextScale);

	const FVector2D QuestCardPosition = InfoPanelPosition + FVector2D(24.0f * EffectiveScale, 178.0f * EffectiveScale);
	const FVector2D QuestCardSize(InfoPanelSize.X - 48.0f * EffectiveScale, 60.0f * EffectiveScale);
	const float QuestCardStep = 70.0f * EffectiveScale;
	for (int32 QuestIndex = 0; QuestIndex < QuestCount; ++QuestIndex)
	{
		int32 CurrentProgress = 0;
		int32 TargetProgress = 0;
		bool bCompleted = false;
		if (!PlayerCharacter || !PlayerCharacter->GetMiniQuestProgress(QuestIndex, CurrentProgress, TargetProgress, bCompleted))
		{
			continue;
		}

		const FVector2D CardPosition = QuestCardPosition + FVector2D(0.0f, QuestCardStep * QuestIndex);
		const FLinearColor CardColor = bCompleted
			? FLinearColor(0.16f, 0.27f, 0.18f, 0.96f)
			: FLinearColor(0.11f, 0.13f, 0.17f, 0.92f);
		DrawPanel(Canvas, CardPosition, QuestCardSize, CardColor);

		const FString TitleText = GetMiniQuestTitle(ThiefController, QuestIndex);
		const FString DescriptionText = GetMiniQuestDescription(ThiefController, QuestIndex);
		const FString ProgressText = FString::Printf(TEXT("%d / %d"), CurrentProgress, TargetProgress);
		const float ProgressRatio = TargetProgress > 0
			? FMath::Clamp(static_cast<float>(CurrentProgress) / static_cast<float>(TargetProgress), 0.0f, 1.0f)
			: 0.0f;

		DrawTextLine(Canvas, MediumFont, TitleText, CardPosition + FVector2D(14.0f * EffectiveScale, 7.0f * EffectiveScale), bCompleted ? PositiveColor : AccentColor, false, 0.82f * BodyScale);
		DrawTextLine(Canvas, MediumFont, ProgressText, CardPosition + FVector2D(QuestCardSize.X - 110.0f * EffectiveScale, 7.0f * EffectiveScale), FLinearColor::White, false, 0.76f * BodyScale);
		DrawTextLine(Canvas, SmallFont, DescriptionText, CardPosition + FVector2D(14.0f * EffectiveScale, 31.0f * EffectiveScale), MutedTextColor, false, DetailScale);

		const FVector2D BarPosition = CardPosition + FVector2D(14.0f * EffectiveScale, QuestCardSize.Y - 13.0f * EffectiveScale);
		const FVector2D BarSize(QuestCardSize.X - 28.0f * EffectiveScale, 6.0f * EffectiveScale);
		FCanvasTileItem BarBackground(BarPosition, BarSize, FLinearColor(0.08f, 0.08f, 0.09f, 1.0f));
		BarBackground.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(BarBackground);

		FCanvasTileItem BarFill(BarPosition, FVector2D(BarSize.X * ProgressRatio, BarSize.Y), bCompleted ? PositiveColor : AccentColor);
		BarFill.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(BarFill);
	}

	const UWorkOrderSubsystem* WorkOrderSubsystem = ThiefController && ThiefController->GetGameInstance()
		? ThiefController->GetGameInstance()->GetSubsystem<UWorkOrderSubsystem>()
		: nullptr;
	if (WorkOrderSubsystem && WorkOrderSubsystem->HasTrackedShift())
	{
		const FWorkOrderDefinition* Definition = WorkOrderSubsystem->GetTrackedOrderDefinition();
		if (Definition)
		{
			const FVector2D ShiftCardPosition(PanelPosition.X + PanelSize.X - PanelSize.X * 0.34f - 48.0f * EffectiveScale, PanelPosition.Y + 170.0f * EffectiveScale);
			const FVector2D ShiftCardSize(PanelSize.X * 0.34f, 230.0f * EffectiveScale);
			DrawPanel(Canvas, ShiftCardPosition, ShiftCardSize, SecondaryPanelColor);
			DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Current Shift"), TEXT("Текущая смена")), ShiftCardPosition + FVector2D(18.0f * EffectiveScale, 16.0f * EffectiveScale), AccentColor, false, TextScale);
			DrawTextLine(Canvas, MediumFont, WorkOrderSubsystem->GetOrderTitle(WorkOrderSubsystem->GetTrackedOrderIndex(), ThiefController->IsRussianLanguage()), ShiftCardPosition + FVector2D(18.0f * EffectiveScale, 46.0f * EffectiveScale), FLinearColor::White, false, 0.82f * BodyScale);
			DrawWrappedTextBlock(
				Canvas,
				MediumFont,
				WorkOrderSubsystem->GetOrderObjectiveText(WorkOrderSubsystem->GetTrackedOrderIndex(), ThiefController->IsRussianLanguage()),
				ShiftCardPosition + FVector2D(18.0f * EffectiveScale, 70.0f * EffectiveScale),
				ShiftCardSize.X - 36.0f * EffectiveScale,
				MutedTextColor,
				0.76f * BodyScale,
				1.0f);
			DrawWrappedTextBlock(
				Canvas,
				MediumFont,
				WorkOrderSubsystem->GetOrderBonusObjectiveText(WorkOrderSubsystem->GetTrackedOrderIndex(), ThiefController->IsRussianLanguage()),
				ShiftCardPosition + FVector2D(18.0f * EffectiveScale, 124.0f * EffectiveScale),
				ShiftCardSize.X - 36.0f * EffectiveScale,
				PositiveColor,
				0.68f * BodyScale,
				1.0f);
			DrawWrappedTextBlock(
				Canvas,
				MediumFont,
				FString::Printf(
					TEXT("%s: %s"),
					*WorkOrderSubsystem->GetOrderModifierText(WorkOrderSubsystem->GetTrackedOrderIndex(), ThiefController->IsRussianLanguage()),
					*WorkOrderSubsystem->GetOrderModifierEffectText(WorkOrderSubsystem->GetTrackedOrderIndex(), ThiefController->IsRussianLanguage())),
				ShiftCardPosition + FVector2D(18.0f * EffectiveScale, 174.0f * EffectiveScale),
				ShiftCardSize.X - 36.0f * EffectiveScale,
				AccentColor,
				0.62f * BodyScale,
				1.0f);
		}
	}

	const FVector2D ButtonSize(FMath::Min(420.0f * EffectiveScale, PanelSize.X * 0.32f), 62.0f * EffectiveScale);
	const float ButtonStartX = PanelPosition.X + PanelSize.X - ButtonSize.X - 48.0f * EffectiveScale;
	const bool bCanAbandonShift = WorkOrderSubsystem && WorkOrderSubsystem->IsShiftRunning();
	const float ButtonStartY = PanelPosition.Y + (WorkOrderSubsystem && WorkOrderSubsystem->HasTrackedShift() ? 426.0f : 188.0f) * EffectiveScale;
	DrawMenuButton(LocalizeText(ThiefController, TEXT("Resume"), TEXT("Продолжить")), FVector2D(ButtonStartX, ButtonStartY), ButtonSize, EHUDMenuAction::ResumeGame, true);
	DrawMenuButton(LocalizeText(ThiefController, TEXT("Settings"), TEXT("Настройки")), FVector2D(ButtonStartX, ButtonStartY + 84.0f * EffectiveScale), ButtonSize, EHUDMenuAction::OpenSettings);
	float NextButtonY = ButtonStartY + 168.0f * EffectiveScale;
	if (bCanAbandonShift)
	{
		DrawMenuButton(LocalizeText(ThiefController, TEXT("Abandon Shift"), TEXT("Прервать смену")), FVector2D(ButtonStartX, NextButtonY), ButtonSize, EHUDMenuAction::AbandonShift);
		NextButtonY += 84.0f * EffectiveScale;
	}
	DrawMenuButton(LocalizeText(ThiefController, TEXT("Reset Progress"), TEXT("Сбросить прогресс")), FVector2D(ButtonStartX, NextButtonY), ButtonSize, EHUDMenuAction::ResetCharacterProgress);
	NextButtonY += 84.0f * EffectiveScale;
	DrawMenuButton(LocalizeText(ThiefController, TEXT("Exit To Lobby"), TEXT("Выйти в лобби")), FVector2D(ButtonStartX, NextButtonY), ButtonSize, EHUDMenuAction::ReturnToLobby);
	NextButtonY += 84.0f * EffectiveScale;
	DrawMenuButton(LocalizeText(ThiefController, TEXT("Quit Game"), TEXT("Выйти из игры")), FVector2D(ButtonStartX, NextButtonY), ButtonSize, EHUDMenuAction::QuitGame);
}

void APlayerGameHUD::DrawWorkOrderPanel(float ViewportWidth, float ViewportHeight) const
{
	const AThiefPlayerController* ThiefController = GetThiefPlayerController();
	const AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter();
	if (!ThiefController || !PlayerCharacter)
	{
		return;
	}

	const UWorkOrderSubsystem* WorkOrderSubsystem = ThiefController->GetGameInstance()
		? ThiefController->GetGameInstance()->GetSubsystem<UWorkOrderSubsystem>()
		: nullptr;
	if (!WorkOrderSubsystem || !WorkOrderSubsystem->HasTrackedShift())
	{
		return;
	}

	const FWorkOrderDefinition* Definition = WorkOrderSubsystem->GetTrackedOrderDefinition();
	if (!Definition)
	{
		return;
	}

	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!MediumFont || !SmallFont)
	{
		return;
	}

	const bool bRussian = ThiefController->IsRussianLanguage();
	const EWorkOrderShiftState ShiftState = WorkOrderSubsystem->GetShiftState();
	const int32 CurrentProgress = WorkOrderSubsystem->GetTrackedShiftProgress(PlayerCharacter);
	const int32 TargetProgress = WorkOrderSubsystem->GetTrackedShiftTarget();
	const float ProgressRatio = WorkOrderSubsystem->GetTrackedShiftProgressRatio(PlayerCharacter);
	const int32 BonusProgress = WorkOrderSubsystem->GetTrackedBonusProgress(PlayerCharacter);
	const int32 BonusTarget = WorkOrderSubsystem->GetTrackedBonusTarget();
	const float BonusProgressRatio = WorkOrderSubsystem->GetTrackedBonusProgressRatio(PlayerCharacter);
	const bool bBonusCompleted = WorkOrderSubsystem->IsTrackedBonusCompleted(PlayerCharacter);
	const float HudScale = FMath::Clamp(FMath::Min(ViewportWidth / 1920.0f, ViewportHeight / 1080.0f), 0.96f, 1.12f);
	const FVector2D PanelPosition(34.0f, 112.0f);
	const FVector2D PanelSize(FMath::Clamp(ViewportWidth * 0.26f, 380.0f, 480.0f), FMath::Clamp(ViewportHeight * 0.25f, 214.0f, 270.0f));
	const FLinearColor FillColor =
		ShiftState == EWorkOrderShiftState::Completed ? FLinearColor(0.11f, 0.20f, 0.14f, 0.96f) :
		ShiftState == EWorkOrderShiftState::Failed ? FLinearColor(0.22f, 0.11f, 0.10f, 0.96f) :
		ShiftState == EWorkOrderShiftState::ReadyToTurnIn ? FLinearColor(0.15f, 0.18f, 0.09f, 0.96f) :
		FLinearColor(0.05f, 0.08f, 0.11f, 0.94f);
	DrawPanel(Canvas, PanelPosition, PanelSize, FillColor);

	const FString TitleText = WorkOrderSubsystem->GetOrderTitle(WorkOrderSubsystem->GetTrackedOrderIndex(), bRussian);
	const FString StateText = GetShiftStateLabel(ThiefController, ShiftState);
	DrawTextLine(Canvas, MediumFont, TitleText, PanelPosition + FVector2D(16.0f, 12.0f), AccentColor, false, 0.92f * HudScale);
	DrawTextLine(Canvas, MediumFont, StateText, PanelPosition + FVector2D(16.0f, 38.0f), ShiftState == EWorkOrderShiftState::Failed ? FLinearColor(1.0f, 0.72f, 0.68f, 1.0f) : FLinearColor::White, false, 0.68f * HudScale);

	if (ShiftState == EWorkOrderShiftState::Active || ShiftState == EWorkOrderShiftState::ReadyToTurnIn)
	{
		DrawTextLine(Canvas, MediumFont, FormatShiftTimerText(WorkOrderSubsystem->GetRemainingShiftTime(), bRussian), PanelPosition + FVector2D(PanelSize.X - 188.0f, 14.0f), PositiveColor, false, 0.54f * HudScale);
	}
	else if (ShiftState == EWorkOrderShiftState::Completed)
	{
		DrawTextLine(
			Canvas,
			MediumFont,
			FString::Printf(
				TEXT("+%d %s   |   XP +%d"),
				WorkOrderSubsystem->GetLastRewardedGold(),
				bRussian ? TEXT("золота") : TEXT("Gold"),
				WorkOrderSubsystem->GetLastRewardedExperience()),
			PanelPosition + FVector2D(PanelSize.X - 242.0f, 14.0f),
			PositiveColor,
			false,
			0.50f * HudScale);
	}

	DrawWrappedTextBlock(
		Canvas,
		MediumFont,
		WorkOrderSubsystem->GetOrderObjectiveText(WorkOrderSubsystem->GetTrackedOrderIndex(), bRussian),
		PanelPosition + FVector2D(16.0f, 62.0f),
		PanelSize.X - 32.0f,
		MutedTextColor,
		0.56f * HudScale,
		1.0f);

	const TCHAR* ProgressLabel = bRussian ? TEXT("Прогресс:") : TEXT("Progress:");
	const FString ProgressText = FString::Printf(
		TEXT("%s %d / %d"),
		ProgressLabel,
		CurrentProgress,
		TargetProgress);
	DrawTextLine(Canvas, MediumFont, ProgressText, PanelPosition + FVector2D(16.0f, 104.0f), FLinearColor::White, false, 0.60f * HudScale);

	const FVector2D BarPosition = PanelPosition + FVector2D(16.0f, 128.0f);
	const FVector2D BarSize(PanelSize.X - 32.0f, 7.0f);
	FCanvasTileItem BarBackground(BarPosition, BarSize, FLinearColor(0.09f, 0.08f, 0.07f, 1.0f));
	BarBackground.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BarBackground);

	FCanvasTileItem BarFill(
		BarPosition,
		FVector2D(BarSize.X * ProgressRatio, BarSize.Y),
		ShiftState == EWorkOrderShiftState::Failed ? DangerButtonHoveredColor : (ShiftState == EWorkOrderShiftState::Completed ? PositiveColor : AccentColor));
	BarFill.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BarFill);

	if (BonusTarget > 0)
	{
		const FString BonusTitle = bRussian ? TEXT("Бонус:") : TEXT("Bonus:");
		const FString BonusText = FString::Printf(TEXT("%s %d / %d"), *BonusTitle, BonusProgress, BonusTarget);
		DrawWrappedTextBlock(
			Canvas,
			SmallFont,
			WorkOrderSubsystem->GetOrderBonusObjectiveText(WorkOrderSubsystem->GetTrackedOrderIndex(), bRussian),
			PanelPosition + FVector2D(16.0f, 146.0f),
			PanelSize.X - 32.0f,
			bBonusCompleted ? PositiveColor : MutedTextColor,
			0.74f * HudScale,
			1.0f);
		DrawTextLine(Canvas, MediumFont, BonusText, PanelPosition + FVector2D(16.0f, 178.0f), bBonusCompleted ? PositiveColor : FLinearColor::White, false, 0.56f * HudScale);

		const FVector2D BonusBarPosition = PanelPosition + FVector2D(16.0f, 199.0f);
		FCanvasTileItem BonusBarBackground(BonusBarPosition, BarSize, FLinearColor(0.09f, 0.08f, 0.07f, 1.0f));
		BonusBarBackground.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(BonusBarBackground);

		FCanvasTileItem BonusBarFill(
			BonusBarPosition,
			FVector2D(BarSize.X * BonusProgressRatio, BarSize.Y),
			bBonusCompleted ? PositiveColor : FLinearColor(0.55f, 0.70f, 0.86f, 1.0f));
		BonusBarFill.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(BonusBarFill);
	}

	DrawWrappedTextBlock(
		Canvas,
		SmallFont,
		WorkOrderSubsystem->GetOrderModifierEffectText(WorkOrderSubsystem->GetTrackedOrderIndex(), bRussian),
		PanelPosition + FVector2D(16.0f, PanelSize.Y - 42.0f),
		PanelSize.X - 32.0f,
		MutedTextColor,
		0.66f * HudScale,
		1.0f);
}

void APlayerGameHUD::DrawShiftResultScreen(float ViewportWidth, float ViewportHeight)
{
	AThiefPlayerController* ThiefController = GetThiefPlayerController();
	const AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter();
	if (!ThiefController)
	{
		return;
	}

	const UWorkOrderSubsystem* WorkOrderSubsystem = ThiefController->GetGameInstance()
		? ThiefController->GetGameInstance()->GetSubsystem<UWorkOrderSubsystem>()
		: nullptr;
	if (!WorkOrderSubsystem || !WorkOrderSubsystem->HasResolvedShiftResult())
	{
		return;
	}

	UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!LargeFont || !MediumFont || !SmallFont)
	{
		return;
	}

	const bool bRussian = ThiefController->IsRussianLanguage();
	const int32 OrderIndex = WorkOrderSubsystem->GetTrackedOrderIndex();
	const EWorkOrderShiftState ResultState = WorkOrderSubsystem->GetLastResolvedState();
	const bool bCompleted = ResultState == EWorkOrderShiftState::Completed;
	const bool bAbandoned = ResultState == EWorkOrderShiftState::Abandoned;
	const bool bBonusCompleted = WorkOrderSubsystem->WasLastBonusCompleted();
	const int32 PrimaryProgress = WorkOrderSubsystem->GetTrackedShiftProgress(PlayerCharacter);
	const int32 PrimaryTarget = WorkOrderSubsystem->GetTrackedShiftTarget();
	const int32 BonusProgress = WorkOrderSubsystem->GetTrackedBonusProgress(PlayerCharacter);
	const int32 BonusTarget = WorkOrderSubsystem->GetTrackedBonusTarget();
	const float EffectiveScale = FMath::Clamp(FMath::Min(ViewportWidth / 1920.0f, ViewportHeight / 1080.0f), 0.90f, 1.12f);

	FCanvasTileItem Overlay(FVector2D::ZeroVector, FVector2D(ViewportWidth, ViewportHeight), FLinearColor(0.01f, 0.01f, 0.02f, 0.90f));
	Overlay.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Overlay);

	const FVector2D PanelSize(FMath::Clamp(ViewportWidth * 0.54f, 720.0f, 980.0f), FMath::Clamp(ViewportHeight * 0.62f, 520.0f, 680.0f));
	const FVector2D PanelPosition((ViewportWidth - PanelSize.X) * 0.5f, (ViewportHeight - PanelSize.Y) * 0.5f);
	const FLinearColor ResultColor = bCompleted
		? FLinearColor(0.08f, 0.16f, 0.11f, 0.97f)
		: (bAbandoned ? FLinearColor(0.17f, 0.13f, 0.10f, 0.97f) : FLinearColor(0.18f, 0.08f, 0.08f, 0.97f));
	DrawPanel(Canvas, PanelPosition, PanelSize, ResultColor);

	const FString TitleText = GetShiftResultTitle(ThiefController, ResultState);
	const FString OrderTitle = WorkOrderSubsystem->GetOrderTitle(OrderIndex, bRussian);
	const FLinearColor TitleColor = bCompleted ? PositiveColor : (bAbandoned ? AccentColor : DangerButtonHoveredColor);
	DrawTextLine(Canvas, LargeFont, TitleText, PanelPosition + FVector2D(PanelSize.X * 0.5f, 36.0f * EffectiveScale), TitleColor, true, 1.10f * EffectiveScale);
	DrawTextLine(Canvas, MediumFont, OrderTitle, PanelPosition + FVector2D(PanelSize.X * 0.5f, 86.0f * EffectiveScale), FLinearColor::White, true, 0.96f * EffectiveScale);

	const FVector2D SummaryPosition = PanelPosition + FVector2D(40.0f * EffectiveScale, 130.0f * EffectiveScale);
	const FVector2D SummarySize(PanelSize.X - 80.0f * EffectiveScale, 122.0f * EffectiveScale);
	DrawPanel(Canvas, SummaryPosition, SummarySize, HeroPanelColor);
	const FString SummaryText = bCompleted
		? LocalizeText(ThiefController, TEXT("The work order was closed successfully. Rewards were added to the character."), TEXT("Заказ успешно закрыт. Награды добавлены персонажу."))
		: (bAbandoned
			? LocalizeText(ThiefController, TEXT("The shift was abandoned manually. No reward was issued."), TEXT("Смена прервана вручную. Награда не выдана."))
			: LocalizeText(ThiefController, TEXT("The shift timer expired before turn-in. No reward was issued."), TEXT("Таймер смены закончился до сдачи. Награда не выдана.")));
	DrawWrappedTextBlock(Canvas, MediumFont, SummaryText, SummaryPosition + FVector2D(24.0f * EffectiveScale, 22.0f * EffectiveScale), SummarySize.X - 48.0f * EffectiveScale, FLinearColor::White, 0.82f * EffectiveScale, 1.05f);
	DrawWrappedTextBlock(Canvas, SmallFont, WorkOrderSubsystem->GetOrderModifierEffectText(OrderIndex, bRussian), SummaryPosition + FVector2D(24.0f * EffectiveScale, 78.0f * EffectiveScale), SummarySize.X - 48.0f * EffectiveScale, MutedTextColor, 0.80f * EffectiveScale, 1.0f);

	const FVector2D ObjectiveRowPosition = PanelPosition + FVector2D(40.0f * EffectiveScale, 280.0f * EffectiveScale);
	const FVector2D RowSize(PanelSize.X - 80.0f * EffectiveScale, 74.0f * EffectiveScale);
	DrawPanel(Canvas, ObjectiveRowPosition, RowSize, SecondaryPanelColor);
	DrawTextLine(Canvas, MediumFont, bRussian ? TEXT("Основная цель") : TEXT("Primary Objective"), ObjectiveRowPosition + FVector2D(20.0f * EffectiveScale, 10.0f * EffectiveScale), AccentColor, false, 0.78f * EffectiveScale);
	DrawWrappedTextBlock(Canvas, SmallFont, WorkOrderSubsystem->GetOrderObjectiveText(OrderIndex, bRussian), ObjectiveRowPosition + FVector2D(20.0f * EffectiveScale, 38.0f * EffectiveScale), RowSize.X - 220.0f * EffectiveScale, FLinearColor::White, 0.78f * EffectiveScale, 1.0f);
	DrawTextLine(
		Canvas,
		MediumFont,
		FString::Printf(TEXT("%d / %d"), PrimaryProgress, PrimaryTarget),
		ObjectiveRowPosition + FVector2D(RowSize.X - 150.0f * EffectiveScale, 28.0f * EffectiveScale),
		bCompleted ? PositiveColor : FLinearColor::White,
		false,
		0.86f * EffectiveScale);

	const FVector2D BonusRowPosition = ObjectiveRowPosition + FVector2D(0.0f, 88.0f * EffectiveScale);
	DrawPanel(Canvas, BonusRowPosition, RowSize, SecondaryPanelColor);
	DrawTextLine(Canvas, MediumFont, bRussian ? TEXT("Бонусная цель") : TEXT("Bonus Objective"), BonusRowPosition + FVector2D(20.0f * EffectiveScale, 10.0f * EffectiveScale), bBonusCompleted ? PositiveColor : AccentColor, false, 0.78f * EffectiveScale);
	DrawWrappedTextBlock(Canvas, SmallFont, WorkOrderSubsystem->GetOrderBonusObjectiveText(OrderIndex, bRussian), BonusRowPosition + FVector2D(20.0f * EffectiveScale, 38.0f * EffectiveScale), RowSize.X - 220.0f * EffectiveScale, FLinearColor::White, 0.78f * EffectiveScale, 1.0f);
	DrawTextLine(
		Canvas,
		MediumFont,
		BonusTarget > 0 ? FString::Printf(TEXT("%d / %d"), BonusProgress, BonusTarget) : TEXT("-"),
		BonusRowPosition + FVector2D(RowSize.X - 150.0f * EffectiveScale, 28.0f * EffectiveScale),
		bBonusCompleted ? PositiveColor : MutedTextColor,
		false,
		0.86f * EffectiveScale);

	const FVector2D RewardPosition = PanelPosition + FVector2D(40.0f * EffectiveScale, 470.0f * EffectiveScale);
	const FVector2D RewardSize(PanelSize.X - 80.0f * EffectiveScale, 76.0f * EffectiveScale);
	DrawPanel(Canvas, RewardPosition, RewardSize, bCompleted ? FLinearColor(0.10f, 0.18f, 0.11f, 0.96f) : FLinearColor(0.11f, 0.11f, 0.12f, 0.96f));
	const FString RewardText = bCompleted
		? (bRussian
			? FString::Printf(
				TEXT("Награда: +%d золота, +%d опыта, +%d репутации%s"),
				WorkOrderSubsystem->GetLastRewardedGold(),
				WorkOrderSubsystem->GetLastRewardedExperience(),
				WorkOrderSubsystem->GetLastRewardedReputation(),
				bBonusCompleted ? TEXT("  | бонус засчитан") : TEXT(""))
			: FString::Printf(
				TEXT("Reward: +%d Gold, +%d XP, +%d Reputation%s"),
				WorkOrderSubsystem->GetLastRewardedGold(),
				WorkOrderSubsystem->GetLastRewardedExperience(),
				WorkOrderSubsystem->GetLastRewardedReputation(),
				bBonusCompleted ? TEXT("  | bonus completed") : TEXT("")))
		: LocalizeText(ThiefController, TEXT("Reward: not issued"), TEXT("Награда: не выдана"));
	DrawWrappedTextBlock(Canvas, MediumFont, RewardText, RewardPosition + FVector2D(22.0f * EffectiveScale, 24.0f * EffectiveScale), RewardSize.X - 44.0f * EffectiveScale, bCompleted ? PositiveColor : MutedTextColor, 0.86f * EffectiveScale, 1.0f);

	const FVector2D ButtonSize(220.0f * EffectiveScale, 58.0f * EffectiveScale);
	const float ButtonY = PanelPosition.Y + PanelSize.Y - 86.0f * EffectiveScale;
	DrawMenuButton(LocalizeText(ThiefController, TEXT("Continue"), TEXT("Продолжить")), FVector2D(PanelPosition.X + PanelSize.X - ButtonSize.X * 2.0f - 64.0f * EffectiveScale, ButtonY), ButtonSize, EHUDMenuAction::CloseShiftResult, true);
	DrawMenuButton(LocalizeText(ThiefController, TEXT("Exit To Lobby"), TEXT("Выйти в лобби")), FVector2D(PanelPosition.X + PanelSize.X - ButtonSize.X - 40.0f * EffectiveScale, ButtonY), ButtonSize, EHUDMenuAction::ReturnToLobby);
}

void APlayerGameHUD::DrawSettingsPanel(float ViewportWidth, float ViewportHeight, bool bShowBackToPause)
{
	UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!LargeFont || !MediumFont || !SmallFont)
	{
		return;
	}

	AThiefPlayerController* ThiefController = GetThiefPlayerController();
	if (!ThiefController)
	{
		return;
	}

	// The settings panel is always the top-most full-screen modal. Drop any buttons
	// registered by the layers underneath it (lobby / pause menu) so a click that
	// lands on a gap in the settings panel can never trigger a hidden button such as
	// Quit Game or Reset Progress.
	ActiveButtons.Reset();

	const float MenuScale = ThiefController->GetMenuScaleSetting();
	const float EffectiveScale = FMath::Clamp(
		MenuScale * FMath::Min(ViewportWidth / 1920.0f, ViewportHeight / 1080.0f),
		0.85f,
		1.08f);
	FCanvasTileItem Overlay(FVector2D::ZeroVector, FVector2D(ViewportWidth, ViewportHeight), FLinearColor(0.01f, 0.01f, 0.02f, 0.88f));
	Overlay.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Overlay);

	const float HorizontalMargin = FMath::Clamp(ViewportWidth * 0.035f, 24.0f, 72.0f);
	const float VerticalMargin = FMath::Clamp(ViewportHeight * 0.04f, 24.0f, 60.0f);
	const FVector2D PanelPosition(HorizontalMargin, VerticalMargin);
	const FVector2D PanelSize(ViewportWidth - HorizontalMargin * 2.0f, ViewportHeight - VerticalMargin * 2.0f);
	DrawPanel(Canvas, PanelPosition, PanelSize, FLinearColor(0.03f, 0.04f, 0.05f, 0.97f));
	const float SidebarWidth = FMath::Clamp(PanelSize.X * 0.22f, 260.0f * EffectiveScale, 340.0f * EffectiveScale);
	const FVector2D SidebarPosition = PanelPosition + FVector2D(18.0f * EffectiveScale, 18.0f * EffectiveScale);
	const FVector2D SidebarSize(SidebarWidth, PanelSize.Y - 36.0f * EffectiveScale);
	DrawPanel(Canvas, SidebarPosition, SidebarSize, HeroPanelColor);

	DrawTextLine(Canvas, LargeFont, LocalizeText(ThiefController, TEXT("SETTINGS"), TEXT("НАСТРОЙКИ")), SidebarPosition + FVector2D(24.0f * EffectiveScale, 26.0f * EffectiveScale), AccentColor, false, 0.95f);
	DrawWrappedTextBlock(
		Canvas,
		SmallFont,
		LocalizeText(ThiefController, TEXT("Tune sound, controls and interface from one unified command deck."), TEXT("Настрой звук, управление и интерфейс в одной общей командной панели.")),
		SidebarPosition + FVector2D(24.0f * EffectiveScale, 78.0f * EffectiveScale),
		SidebarSize.X - 48.0f * EffectiveScale,
		FLinearColor::White,
		0.90f);

	const FVector2D SidebarButtonSize(SidebarSize.X - 40.0f * EffectiveScale, 56.0f * EffectiveScale);
	const float SidebarButtonsX = SidebarPosition.X + 20.0f * EffectiveScale;
	const float SidebarButtonsY = SidebarPosition.Y + 164.0f * EffectiveScale;
	DrawSidebarButton(LocalizeText(ThiefController, TEXT("Audio"), TEXT("Звук")), FVector2D(SidebarButtonsX, SidebarButtonsY), SidebarButtonSize, static_cast<int32>(ESettingsPanelTab::Audio), ThiefController->GetActiveSettingsTab() == ESettingsPanelTab::Audio);
	DrawSidebarButton(LocalizeText(ThiefController, TEXT("Controls"), TEXT("Управление")), FVector2D(SidebarButtonsX, SidebarButtonsY + 70.0f * EffectiveScale), SidebarButtonSize, static_cast<int32>(ESettingsPanelTab::Controls), ThiefController->GetActiveSettingsTab() == ESettingsPanelTab::Controls);
	DrawSidebarButton(LocalizeText(ThiefController, TEXT("Interface"), TEXT("Интерфейс")), FVector2D(SidebarButtonsX, SidebarButtonsY + 140.0f * EffectiveScale), SidebarButtonSize, static_cast<int32>(ESettingsPanelTab::Interface), ThiefController->GetActiveSettingsTab() == ESettingsPanelTab::Interface);

	const FVector2D ContentPanelPosition = SidebarPosition + FVector2D(SidebarSize.X + 22.0f * EffectiveScale, 0.0f);
	const FVector2D ContentPanelSize(PanelPosition.X + PanelSize.X - ContentPanelPosition.X - 18.0f * EffectiveScale, SidebarSize.Y);
	DrawPanel(Canvas, ContentPanelPosition, ContentPanelSize, SecondaryPanelColor);

	const FVector2D ContentPosition = ContentPanelPosition + FVector2D(28.0f * EffectiveScale, 28.0f * EffectiveScale);
	const float ContentWidth = ContentPanelSize.X - 56.0f * EffectiveScale;
	switch (ThiefController->GetActiveSettingsTab())
	{
	case ESettingsPanelTab::Audio:
		ControlsBindingsScrollOffset = 0.0f;
		ControlsBindingsMaxScroll = 0.0f;
		DrawAudioSettingsSection(ThiefController, ContentPosition, ContentWidth, EffectiveScale);
		break;
	case ESettingsPanelTab::Controls:
		DrawControlsSettingsSection(ThiefController, ContentPosition, ContentWidth, ContentPanelSize.Y - 56.0f * EffectiveScale, EffectiveScale);
		break;
	case ESettingsPanelTab::Interface:
	default:
		ControlsBindingsScrollOffset = 0.0f;
		ControlsBindingsMaxScroll = 0.0f;
		DrawInterfaceSettingsSection(ThiefController, ContentPosition, ContentWidth, EffectiveScale);
		break;
	}

	DrawMenuButton(
		bShowBackToPause
			? LocalizeText(ThiefController, TEXT("Back"), TEXT("Назад"))
			: LocalizeText(ThiefController, TEXT("Close"), TEXT("Закрыть")),
		SidebarPosition + FVector2D(20.0f * EffectiveScale, SidebarSize.Y - 72.0f * EffectiveScale),
		FVector2D(SidebarSize.X - 40.0f * EffectiveScale, 52.0f * EffectiveScale),
		EHUDMenuAction::CloseSettings,
		true);

	if (ThiefController->IsWaitingForInputRebind())
	{
		DrawInputCaptureOverlay(ThiefController, ViewportWidth, ViewportHeight);
	}
}

void APlayerGameHUD::DrawAudioSettingsSection(const AThiefPlayerController* ThiefController, const FVector2D& PanelPosition, float PanelWidth, float Scale)
{
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!ThiefController || !MediumFont || !SmallFont)
	{
		return;
	}

	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("AUDIO CONTROL DECK"), TEXT("ЗВУКОВАЯ ПАЛУБА")), PanelPosition, AccentColor);
	DrawTextLine(
		Canvas,
		SmallFont,
		LocalizeText(ThiefController, TEXT("Master affects the whole session, music controls the lobby track, SFX drives world and gameplay sounds."), TEXT("Master влияет на всю игру, Music управляет музыкой лобби, SFX отвечает за звуки мира и геймплея.")),
		PanelPosition + FVector2D(0.0f, 34.0f * Scale),
		MutedTextColor);

	const FVector2D CardPosition = PanelPosition + FVector2D(0.0f, 84.0f * Scale);
	DrawPanel(Canvas, CardPosition, FVector2D(PanelWidth, 404.0f * Scale), HeroPanelColor);

	DrawSettingValueRow(
		LocalizeText(ThiefController, TEXT("Master Volume"), TEXT("Громкость Master")),
		FString::Printf(TEXT("%d%%"), FMath::RoundToInt(ThiefController->GetMasterVolumeSetting() * 100.0f)),
		CardPosition + FVector2D(24.0f * Scale, 28.0f * Scale),
		PanelWidth - 48.0f * Scale,
		Scale,
		EHUDMenuAction::MasterVolumeDown,
		EHUDMenuAction::MasterVolumeUp,
		ThiefController->GetMasterVolumeSetting(),
		EHUDSliderTarget::MasterVolume);
	DrawSettingValueRow(
		LocalizeText(ThiefController, TEXT("Music Volume"), TEXT("Громкость музыки")),
		FString::Printf(TEXT("%d%%"), FMath::RoundToInt(ThiefController->GetMusicVolumeSetting() * 100.0f)),
		CardPosition + FVector2D(24.0f * Scale, 126.0f * Scale),
		PanelWidth - 48.0f * Scale,
		Scale,
		EHUDMenuAction::MusicDown,
		EHUDMenuAction::MusicUp,
		ThiefController->GetMusicVolumeSetting(),
		EHUDSliderTarget::MusicVolume);
	DrawSettingValueRow(
		LocalizeText(ThiefController, TEXT("SFX Volume"), TEXT("Громкость эффектов")),
		FString::Printf(TEXT("%d%%"), FMath::RoundToInt(ThiefController->GetSfxVolumeSetting() * 100.0f)),
		CardPosition + FVector2D(24.0f * Scale, 224.0f * Scale),
		PanelWidth - 48.0f * Scale,
		Scale,
		EHUDMenuAction::SfxDown,
		EHUDMenuAction::SfxUp,
		ThiefController->GetSfxVolumeSetting(),
		EHUDSliderTarget::SfxVolume);

	DrawTextLine(
		Canvas,
		SmallFont,
		LocalizeText(ThiefController, TEXT("Tip: set your overall mix in the lobby before entering Showcase so the first run already feels right."), TEXT("Подсказка: выстави микс ещё в лобби, чтобы первый заход в Showcase уже был комфортным.")),
		CardPosition + FVector2D(24.0f * Scale, 336.0f * Scale),
		MutedTextColor);
}

void APlayerGameHUD::DrawControlsSettingsSection(const AThiefPlayerController* ThiefController, const FVector2D& PanelPosition, float PanelWidth, float PanelHeight, float Scale)
{
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!ThiefController || !MediumFont || !SmallFont)
	{
		return;
	}

	const TArray<ERemappableInputAction> Bindings =
	{
		ERemappableInputAction::MoveForward,
		ERemappableInputAction::MoveBackward,
		ERemappableInputAction::MoveRight,
		ERemappableInputAction::MoveLeft,
		ERemappableInputAction::Jump,
		ERemappableInputAction::Sprint,
		ERemappableInputAction::Attack,
		ERemappableInputAction::Interact,
		ERemappableInputAction::ToggleTradeMenu,
		ERemappableInputAction::UsePotion,
		ERemappableInputAction::ToggleProgressionMenu,
		ERemappableInputAction::MenuConfirm,
		ERemappableInputAction::MenuBack,
		ERemappableInputAction::MenuOptionOne,
		ERemappableInputAction::MenuOptionTwo,
		ERemappableInputAction::MenuOptionThree
	};

	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("CONTROL DECK"), TEXT("КОНСОЛЬ УПРАВЛЕНИЯ")), PanelPosition, AccentColor);
	DrawTextLine(
		Canvas,
		SmallFont,
		LocalizeText(ThiefController, TEXT("Scroll the list with the wheel, click the key slot to rebind, Escape cancels capture."), TEXT("Листай список колёсиком, жми на слот клавиши для смены бинда, Escape отменяет захват.")),
		PanelPosition + FVector2D(0.0f, 34.0f * Scale),
		MutedTextColor);

	const FVector2D InputCardPosition = PanelPosition + FVector2D(0.0f, 78.0f * Scale);
	const FVector2D InputCardSize(PanelWidth, 170.0f * Scale);
	FCanvasTileItem InputCard(InputCardPosition, InputCardSize, FLinearColor(0.10f, 0.12f, 0.15f, 0.92f));
	InputCard.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(InputCard);
	FCanvasTileItem InputAccent(InputCardPosition + FVector2D(0.0f, 0.0f), FVector2D(InputCardSize.X, 3.0f * Scale), AccentColor);
	InputAccent.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(InputAccent);
	DrawTextLine(Canvas, SmallFont, LocalizeText(ThiefController, TEXT("LOOK AND INPUT"), TEXT("КАМЕРА И ВВОД")), InputCardPosition + FVector2D(18.0f * Scale, 14.0f * Scale), AccentColor);

	DrawSettingValueRow(
		LocalizeText(ThiefController, TEXT("Mouse Sensitivity"), TEXT("Чувствительность мыши")),
		FString::Printf(TEXT("%.2f"), ThiefController->GetLookSensitivitySetting()),
		InputCardPosition + FVector2D(18.0f * Scale, 30.0f * Scale),
		PanelWidth - 36.0f * Scale,
		Scale,
		EHUDMenuAction::LookSensitivityDown,
		EHUDMenuAction::LookSensitivityUp,
		(ThiefController->GetLookSensitivitySetting() - 0.02f) / (0.20f - 0.02f),
		EHUDSliderTarget::LookSensitivity);

	const FVector2D ToggleSize(220.0f * Scale, 44.0f * Scale);
	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Invert Y"), TEXT("Инверсия Y")), InputCardPosition + FVector2D(18.0f * Scale, 112.0f * Scale), FLinearColor::White);
	DrawMenuButton(
		ThiefController->IsLookYInverted()
			? LocalizeText(ThiefController, TEXT("Enabled"), TEXT("Включено"))
			: LocalizeText(ThiefController, TEXT("Disabled"), TEXT("Выключено")),
		InputCardPosition + FVector2D(PanelWidth - ToggleSize.X - 18.0f * Scale, 106.0f * Scale),
		ToggleSize,
		EHUDMenuAction::ToggleInvertLookY,
		ThiefController->IsLookYInverted());

	const FVector2D BindingsPanelPosition = InputCardPosition + FVector2D(0.0f, InputCardSize.Y + 18.0f * Scale);
	const float BindingsPanelHeight = FMath::Max(280.0f * Scale, PanelHeight - (BindingsPanelPosition.Y - PanelPosition.Y) - 12.0f * Scale);
	const FVector2D BindingsPanelSize(PanelWidth, BindingsPanelHeight);
	FCanvasTileItem BindingsPanel(BindingsPanelPosition, BindingsPanelSize, FLinearColor(0.09f, 0.11f, 0.14f, 0.94f));
	BindingsPanel.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BindingsPanel);
	FCanvasTileItem BindingsAccent(BindingsPanelPosition + FVector2D(0.0f, 0.0f), FVector2D(BindingsPanelSize.X, 3.0f * Scale), AccentColor);
	BindingsAccent.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BindingsAccent);
	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("BIND LIST"), TEXT("СПИСОК БИНДОВ")), BindingsPanelPosition + FVector2D(18.0f * Scale, 16.0f * Scale), AccentColor);
	DrawTextLine(
		Canvas,
		SmallFont,
		LocalizeText(ThiefController, TEXT("Movement, combat and fast menu keys are shown as a single scrollable command list."), TEXT("Передвижение, бой и быстрые меню собраны в один общий прокручиваемый список.")),
		BindingsPanelPosition + FVector2D(18.0f * Scale, 44.0f * Scale),
		MutedTextColor);

	const FVector2D ResetButtonSize(220.0f * Scale, 40.0f * Scale);
	const float ViewportTop = BindingsPanelPosition.Y + 78.0f * Scale;
	const float ViewportBottom = BindingsPanelPosition.Y + BindingsPanelHeight - 64.0f * Scale;
	const float ViewportHeight = FMath::Max(140.0f * Scale, ViewportBottom - ViewportTop);
	const float ScrollbarWidth = 12.0f * Scale;
	const float ListWidth = PanelWidth - 36.0f * Scale - ScrollbarWidth - 10.0f * Scale;
	const FVector2D RowSize(ListWidth, 52.0f * Scale);
	const float RowGap = 8.0f * Scale;
	const float ContentHeight = static_cast<float>(Bindings.Num()) * (RowSize.Y + RowGap) - RowGap;
	ControlsBindingsMaxScroll = FMath::Max(0.0f, ContentHeight - ViewportHeight);
	ControlsBindingsScrollOffset = FMath::Clamp(ControlsBindingsScrollOffset, 0.0f, ControlsBindingsMaxScroll);

	for (int32 Index = 0; Index < Bindings.Num(); ++Index)
	{
		const float RowY = ViewportTop + static_cast<float>(Index) * (RowSize.Y + RowGap) - ControlsBindingsScrollOffset;
		if ((RowY + RowSize.Y) < ViewportTop || RowY > ViewportBottom)
		{
			continue;
		}

		DrawBindingRow(
			ThiefController,
			FVector2D(BindingsPanelPosition.X + 18.0f * Scale, RowY),
			RowSize,
			Bindings[Index]);
	}

	if (ControlsBindingsMaxScroll > KINDA_SMALL_NUMBER)
	{
		const FVector2D ScrollTrackPosition(BindingsPanelPosition.X + PanelWidth - 18.0f * Scale - ScrollbarWidth, ViewportTop);
		const FVector2D ScrollTrackSize(ScrollbarWidth, ViewportHeight);
		FCanvasTileItem ScrollTrack(ScrollTrackPosition, ScrollTrackSize, FLinearColor(0.15f, 0.17f, 0.21f, 0.9f));
		ScrollTrack.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(ScrollTrack);

		const float ThumbHeight = FMath::Max(48.0f * Scale, ViewportHeight * (ViewportHeight / ContentHeight));
		const float ThumbTravel = FMath::Max(0.0f, ViewportHeight - ThumbHeight);
		const float ThumbOffset = ThumbTravel * (ControlsBindingsScrollOffset / ControlsBindingsMaxScroll);
		FCanvasTileItem ScrollThumb(ScrollTrackPosition + FVector2D(0.0f, ThumbOffset), FVector2D(ScrollbarWidth, ThumbHeight), AccentColor);
		ScrollThumb.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(ScrollThumb);
	}

	DrawMenuButton(
		LocalizeText(ThiefController, TEXT("Reset Controls"), TEXT("Сбросить управление")),
		BindingsPanelPosition + FVector2D(PanelWidth - ResetButtonSize.X - 18.0f * Scale, BindingsPanelHeight - ResetButtonSize.Y - 14.0f * Scale),
		ResetButtonSize,
		EHUDMenuAction::ResetControls);
}

void APlayerGameHUD::DrawInterfaceSettingsSection(const AThiefPlayerController* ThiefController, const FVector2D& PanelPosition, float PanelWidth, float Scale)
{
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!ThiefController || !MediumFont || !SmallFont)
	{
		return;
	}

	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("INTERFACE TUNING"), TEXT("НАСТРОЙКА ИНТЕРФЕЙСА")), PanelPosition, AccentColor);
	DrawTextLine(
		Canvas,
		SmallFont,
		LocalizeText(ThiefController, TEXT("Keep the menu readable while preserving the gameplay frame behind it."), TEXT("Настрой масштаб и язык так, чтобы интерфейс был читаемым, но не перегружал кадр.")),
		PanelPosition + FVector2D(0.0f, 34.0f * Scale),
		MutedTextColor);

	const FVector2D CardPosition = PanelPosition + FVector2D(0.0f, 84.0f * Scale);
	DrawPanel(Canvas, CardPosition, FVector2D(PanelWidth, 320.0f * Scale), HeroPanelColor);

	DrawSettingValueRow(
		LocalizeText(ThiefController, TEXT("Menu Scale"), TEXT("Масштаб меню")),
		FString::Printf(TEXT("%.2f"), ThiefController->GetMenuScaleSetting()),
		CardPosition + FVector2D(24.0f * Scale, 28.0f * Scale),
		PanelWidth - 48.0f * Scale,
		Scale,
		EHUDMenuAction::MenuScaleDown,
		EHUDMenuAction::MenuScaleUp,
		(ThiefController->GetMenuScaleSetting() - 0.85f) / (1.25f - 0.85f),
		EHUDSliderTarget::MenuScale);

	DrawTextLine(Canvas, MediumFont, LocalizeText(ThiefController, TEXT("Language"), TEXT("Язык")), CardPosition + FVector2D(24.0f * Scale, 144.0f * Scale), FLinearColor::White);
	DrawMenuButton(TEXT("<"), CardPosition + FVector2D(PanelWidth - 196.0f * Scale, 124.0f * Scale), FVector2D(54.0f * Scale, 44.0f * Scale), EHUDMenuAction::LanguagePrev);
	DrawMenuButton(TEXT(">"), CardPosition + FVector2D(PanelWidth - 68.0f * Scale, 124.0f * Scale), FVector2D(54.0f * Scale, 44.0f * Scale), EHUDMenuAction::LanguageNext, true);
	DrawPanel(Canvas, CardPosition + FVector2D(PanelWidth - 136.0f * Scale, 124.0f * Scale), FVector2D(60.0f * Scale, 44.0f * Scale), HighlightPanelColor);
	DrawTextLine(
		Canvas,
		MediumFont,
		ThiefController->GetCurrentLanguage() == EGameLanguage::Russian ? TEXT("RU") : TEXT("EN"),
		CardPosition + FVector2D(PanelWidth - 112.0f * Scale, 136.0f * Scale),
		FLinearColor::White);

	DrawTextLine(
		Canvas,
		SmallFont,
		LocalizeText(ThiefController, TEXT("Language and scale are saved together with audio and control settings."), TEXT("Язык и масштаб сохраняются вместе со звуком и управлением.")),
		CardPosition + FVector2D(24.0f * Scale, 236.0f * Scale),
		MutedTextColor);
}

void APlayerGameHUD::DrawSettingsTabButton(const FString& Label, const FVector2D& Position, const FVector2D& Size, int32 TabIndex, bool bSelected)
{
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Font)
	{
		return;
	}

	RegisterButton(EHUDMenuAction::SelectSettingsTab, Position, Size, TabIndex);
	const bool bHovered = IsButtonHovered(ActiveButtons.Last());
	const FLinearColor FillColor = bSelected
		? AccentColor
		: (bHovered ? ButtonHoveredColor : ButtonColor);

	DrawPanel(Canvas, Position, Size, FillColor);
	if (!bSelected)
	{
		FCanvasTileItem AccentBand(Position + FVector2D(0.0f, Size.Y - 6.0f), FVector2D(Size.X, 6.0f), FLinearColor(0.34f, 0.25f, 0.11f, 0.95f));
		AccentBand.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(AccentBand);
	}
	DrawTextLine(Canvas, Font, Label, Position + FVector2D(18.0f, (Size.Y * 0.5f) - 10.0f), bSelected ? FLinearColor::Black : FLinearColor(0.95f, 0.92f, 0.84f, 1.0f));
}

void APlayerGameHUD::DrawSidebarButton(const FString& Label, const FVector2D& Position, const FVector2D& Size, int32 TabIndex, bool bSelected)
{
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Font)
	{
		return;
	}

	RegisterButton(EHUDMenuAction::SelectSettingsTab, Position, Size, TabIndex);
	const bool bHovered = IsButtonHovered(ActiveButtons.Last());
	const FLinearColor FillColor = bSelected
		? AccentColor
		: (bHovered ? FLinearColor(0.19f, 0.23f, 0.30f, 0.98f) : FLinearColor(0.12f, 0.15f, 0.19f, 0.96f));
	DrawPanel(Canvas, Position, Size, FillColor);

	if (!bSelected)
	{
		FCanvasTileItem LeftAccent(Position + FVector2D(0.0f, 0.0f), FVector2D(6.0f, Size.Y), FLinearColor(0.34f, 0.25f, 0.11f, 0.95f));
		LeftAccent.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(LeftAccent);
	}

	DrawTextLine(Canvas, Font, Label, Position + FVector2D(22.0f, (Size.Y * 0.5f) - 10.0f), bSelected ? FLinearColor::Black : FLinearColor(0.95f, 0.92f, 0.84f, 1.0f));
}

void APlayerGameHUD::DrawSettingValueRow(
	const FString& Label,
	const FString& ValueText,
	const FVector2D& Position,
	float RowWidth,
	float Scale,
	EHUDMenuAction DecreaseAction,
	EHUDMenuAction IncreaseAction,
	float NormalizedValue,
	EHUDSliderTarget SliderTarget)
{
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!MediumFont || !SmallFont)
	{
		return;
	}

	const FVector2D RowSize(RowWidth, 58.0f * Scale);
	FCanvasTileItem RowBackground(Position, RowSize, FLinearColor(0.14f, 0.16f, 0.19f, 0.92f));
	RowBackground.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(RowBackground);
	FCanvasTileItem RowAccent(Position + FVector2D(0.0f, RowSize.Y - 2.0f * Scale), FVector2D(RowSize.X, 2.0f * Scale), FLinearColor(0.86f, 0.79f, 0.64f, 0.18f));
	RowAccent.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(RowAccent);

	DrawTextLine(Canvas, MediumFont, Label, Position + FVector2D(14.0f * Scale, 8.0f * Scale), FLinearColor::White);
	DrawTextLine(Canvas, SmallFont, ValueText, Position + FVector2D(14.0f * Scale, 33.0f * Scale), AccentColor);

	const FVector2D BarPosition = Position + FVector2D(210.0f * Scale, 22.0f * Scale);
	const FVector2D BarSize(RowWidth - 344.0f * Scale, 12.0f * Scale);

	// Register a draggable hit area over the bar (taller than the bar itself so the
	// knob is easy to grab). The horizontal extent matches the bar so the click
	// ratio maps directly onto the value.
	if (SliderTarget != EHUDSliderTarget::None && BarSize.X > 0.0f)
	{
		const float HitPadding = 12.0f * Scale;
		RegisterSlider(
			SliderTarget,
			FVector2D(BarPosition.X, BarPosition.Y - HitPadding),
			FVector2D(BarSize.X, BarSize.Y + HitPadding * 2.0f));
	}

	FCanvasTileItem BarBackground(BarPosition, BarSize, ValueBarBackgroundColor);
	BarBackground.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BarBackground);

	const float ClampedValue = FMath::Clamp(NormalizedValue, 0.0f, 1.0f);
	FCanvasTileItem BarFill(BarPosition, FVector2D(BarSize.X * ClampedValue, BarSize.Y), AccentColor);
	BarFill.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BarFill);

	const FVector2D KnobSize(10.0f * Scale, 22.0f * Scale);
	FCanvasTileItem SliderKnob(
		FVector2D(BarPosition.X + BarSize.X * ClampedValue - KnobSize.X * 0.5f, BarPosition.Y - 5.0f * Scale),
		KnobSize,
		FLinearColor(0.96f, 0.91f, 0.78f, 1.0f));
	SliderKnob.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(SliderKnob);

	const FVector2D ButtonSize(40.0f * Scale, 36.0f * Scale);
	const float ButtonsX = Position.X + RowWidth - 96.0f * Scale;
	DrawMenuButton(TEXT("-"), FVector2D(ButtonsX, Position.Y + 11.0f * Scale), ButtonSize, DecreaseAction);
	DrawMenuButton(TEXT("+"), FVector2D(ButtonsX + 46.0f * Scale, Position.Y + 11.0f * Scale), ButtonSize, IncreaseAction, true);
}

void APlayerGameHUD::DrawBindingRow(const AThiefPlayerController* ThiefController, const FVector2D& Position, const FVector2D& Size, const ERemappableInputAction InputAction)
{
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!ThiefController || !MediumFont)
	{
		return;
	}

	const bool bPending = ThiefController->IsWaitingForInputRebind() && ThiefController->GetPendingInputRebindAction() == InputAction;
	RegisterButton(EHUDMenuAction::BeginInputRebind, Position, Size, static_cast<int32>(InputAction));
	const bool bHovered = IsButtonHovered(ActiveButtons.Last());
	const FLinearColor RowColor = bPending
		? FLinearColor(0.23f, 0.18f, 0.07f, 0.98f)
		: (bHovered ? FLinearColor(0.17f, 0.20f, 0.25f, 0.96f) : FLinearColor(0.12f, 0.14f, 0.17f, 0.88f));
	FCanvasTileItem RowBackground(Position, Size, RowColor);
	RowBackground.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(RowBackground);
	FCanvasTileItem RowSeparator(Position + FVector2D(0.0f, Size.Y - 1.0f), FVector2D(Size.X, 1.0f), bPending ? AccentColor : FLinearColor(0.86f, 0.79f, 0.64f, 0.12f));
	RowSeparator.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(RowSeparator);

	const FVector2D ButtonSize(FMath::Clamp(Size.X * 0.24f, 120.0f, 168.0f), Size.Y - 12.0f);
	const FVector2D ButtonPosition = Position + FVector2D(Size.X - ButtonSize.X - 10.0f, 6.0f);
	DrawWrappedTextBlock(
		Canvas,
		MediumFont,
		ThiefController->GetInputActionDisplayName(InputAction).ToString(),
		Position + FVector2D(14.0f, 15.0f),
		Size.X - ButtonSize.X - 30.0f,
		bPending ? AccentColor : FLinearColor::White,
		0.82f,
		1.0f);

	FCanvasTileItem KeycapBackground(ButtonPosition, ButtonSize, bPending ? AccentColor : FLinearColor(0.30f, 0.34f, 0.42f, 0.95f));
	KeycapBackground.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(KeycapBackground);
	FCanvasBoxItem KeycapBorder(ButtonPosition, ButtonSize);
	KeycapBorder.SetColor(bPending ? FLinearColor::Black : FLinearColor(0.92f, 0.90f, 0.84f, 0.52f));
	Canvas->DrawItem(KeycapBorder);
	DrawTextLine(
		Canvas,
		MediumFont,
		GetCompactKeyLabel(ThiefController->GetInputActionKeyText(InputAction)),
		ButtonPosition + FVector2D(ButtonSize.X * 0.5f, 12.0f),
		bPending ? FLinearColor::Black : FLinearColor::White,
		true);
}

void APlayerGameHUD::DrawInputCaptureOverlay(const AThiefPlayerController* ThiefController, float ViewportWidth, float ViewportHeight)
{
	UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!ThiefController || !LargeFont || !MediumFont)
	{
		return;
	}

	const FVector2D PanelSize(520.0f, 180.0f);
	const FVector2D PanelPosition((ViewportWidth - PanelSize.X) * 0.5f, (ViewportHeight - PanelSize.Y) * 0.5f);
	DrawPanel(Canvas, PanelPosition, PanelSize, FLinearColor(0.03f, 0.03f, 0.04f, 0.98f));
	DrawTextLine(Canvas, LargeFont, LocalizeText(ThiefController, TEXT("Waiting For Input"), TEXT("Ожидание новой клавиши")), PanelPosition + FVector2D(PanelSize.X * 0.5f, 24.0f), AccentColor, true);
	DrawTextLine(
		Canvas,
		MediumFont,
		ThiefController->GetInputActionDisplayName(ThiefController->GetPendingInputRebindAction()).ToString(),
		PanelPosition + FVector2D(PanelSize.X * 0.5f, 78.0f),
		FLinearColor::White,
		true);
	DrawTextLine(
		Canvas,
		MediumFont,
		LocalizeText(ThiefController, TEXT("Press any key now. Esc cancels."), TEXT("Нажми новую клавишу. Esc отменяет.")),
		PanelPosition + FVector2D(PanelSize.X * 0.5f, 120.0f),
		MutedTextColor,
		true);
}

void APlayerGameHUD::DrawResourcePanel(float ViewportWidth) const
{
	AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter();
	if (!PlayerCharacter)
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	if (!Font)
	{
		return;
	}

	float OreTextWidth = 0.0f;
	float OreTextHeight = 0.0f;
	float GoldTextWidth = 0.0f;
	float GoldTextHeight = 0.0f;
	float PotionTextWidth = 0.0f;
	float PotionTextHeight = 0.0f;
	const AThiefPlayerController* ThiefController = GetThiefPlayerController();
	const FString OreText = ThiefController && ThiefController->IsRussianLanguage()
		? FString::Printf(TEXT("Руда: %d"), PlayerCharacter->CollectedOreResources)
		: FString::Printf(TEXT("Ore: %d"), PlayerCharacter->CollectedOreResources);
	const FString GoldText = ThiefController && ThiefController->IsRussianLanguage()
		? FString::Printf(TEXT("Золото: %d"), PlayerCharacter->GetCollectedGold())
		: FString::Printf(TEXT("Gold: %d"), PlayerCharacter->GetCollectedGold());
	const FString PotionKeyLabel = ThiefController
		? GetCompactKeyLabel(ThiefController->GetInputActionKeyText(ERemappableInputAction::UsePotion))
		: TEXT("Q");
	const FString PotionText = ThiefController && ThiefController->IsRussianLanguage()
		? FString::Printf(TEXT("Зелья [%s]: %d"), *PotionKeyLabel, PlayerCharacter->GetStaminaPotionCount())
		: FString::Printf(TEXT("Potions [%s]: %d"), *PotionKeyLabel, PlayerCharacter->GetStaminaPotionCount());
	Canvas->StrLen(Font, OreText, OreTextWidth, OreTextHeight);
	Canvas->StrLen(Font, GoldText, GoldTextWidth, GoldTextHeight);
	Canvas->StrLen(Font, PotionText, PotionTextWidth, PotionTextHeight);

	const float PanelWidth = FMath::Max3(OreTextWidth, GoldTextWidth, PotionTextWidth) + 40.0f;
	const FVector2D PanelSize(PanelWidth, OreTextHeight + GoldTextHeight + PotionTextHeight + 36.0f);
	const FVector2D PanelPosition(ViewportWidth - PanelSize.X - 40.0f, 26.0f);
	DrawPanel(Canvas, PanelPosition, PanelSize);
	DrawTextLine(Canvas, Font, OreText, PanelPosition + FVector2D(18.0f, 8.0f), FLinearColor::White);
	DrawTextLine(Canvas, Font, GoldText, PanelPosition + FVector2D(18.0f, 8.0f + OreTextHeight + 4.0f), AccentColor);
	DrawTextLine(Canvas, Font, PotionText, PanelPosition + FVector2D(18.0f, 8.0f + OreTextHeight + GoldTextHeight + 8.0f), PositiveColor);
}

void APlayerGameHUD::DrawLevelPanel() const
{
	AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter();
	if (!PlayerCharacter)
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Font)
	{
		return;
	}

	const FVector2D PanelPosition(34.0f, 26.0f);
	const FVector2D PanelSize(260.0f, 72.0f);
	DrawPanel(Canvas, PanelPosition, PanelSize);

	const AThiefPlayerController* ThiefController = GetThiefPlayerController();
	const FString LevelText = ThiefController && ThiefController->IsRussianLanguage()
		? FString::Printf(TEXT("Уровень: %d"), PlayerCharacter->GetPlayerLevel())
		: FString::Printf(TEXT("Level: %d"), PlayerCharacter->GetPlayerLevel());
	const FString ExperienceText = ThiefController && ThiefController->IsRussianLanguage()
		? FString::Printf(TEXT("Опыт: %d / %d"), PlayerCharacter->GetCurrentExperienceAmount(), PlayerCharacter->GetExperienceToNextLevelAmount())
		: FString::Printf(TEXT("XP: %d / %d"), PlayerCharacter->GetCurrentExperienceAmount(), PlayerCharacter->GetExperienceToNextLevelAmount());

	DrawTextLine(Canvas, Font, LevelText, PanelPosition + FVector2D(16.0f, 10.0f), AccentColor);
	DrawTextLine(Canvas, Font, ExperienceText, PanelPosition + FVector2D(16.0f, 36.0f), FLinearColor::White);
}

void APlayerGameHUD::DrawLevelUpPopup(float ViewportWidth, float ViewportHeight) const
{
	if (LevelPopupDisplayedLevel <= 0 || LevelPopupDuration <= KINDA_SMALL_NUMBER || LevelPopupStartTime < 0.0f)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float ElapsedTime = World->GetTimeSeconds() - LevelPopupStartTime;
	if (ElapsedTime < 0.0f || ElapsedTime >= LevelPopupDuration)
	{
		return;
	}

	UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!LargeFont || !MediumFont || !SmallFont)
	{
		return;
	}

	const AThiefPlayerController* ThiefController = GetThiefPlayerController();
	const float NormalizedTime = FMath::Clamp(ElapsedTime / LevelPopupDuration, 0.0f, 1.0f);
	const float FadeInAlpha = FMath::Clamp(NormalizedTime / 0.16f, 0.0f, 1.0f);
	const float FadeOutAlpha = 1.0f - FMath::Clamp((NormalizedTime - 0.74f) / 0.26f, 0.0f, 1.0f);
	const float PopupAlpha = FadeInAlpha * FadeOutAlpha;
	if (PopupAlpha <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float ViewScale = FMath::Clamp(FMath::Min(ViewportWidth / 1600.0f, ViewportHeight / 900.0f), 0.82f, 1.15f);
	const float PulseScale = 1.0f + 0.025f * FMath::Sin(NormalizedTime * PI);
	const float LayoutScale = ViewScale * PulseScale;
	const float VerticalDrift = FMath::Lerp(22.0f * ViewScale, 0.0f, FMath::Clamp(NormalizedTime / 0.40f, 0.0f, 1.0f));
	const FVector2D PanelSize(500.0f * LayoutScale, 200.0f * LayoutScale);
	const FVector2D PanelPosition(
		(ViewportWidth - PanelSize.X) * 0.5f,
		FMath::Max(36.0f, ViewportHeight * 0.16f - VerticalDrift));

	const auto ApplyAlpha = [PopupAlpha](const FLinearColor& Color, float AlphaScale = 1.0f)
	{
		return FLinearColor(Color.R, Color.G, Color.B, FMath::Clamp(Color.A * PopupAlpha * AlphaScale, 0.0f, 1.0f));
	};

	const FVector2D GlowPosition = PanelPosition - FVector2D(16.0f * ViewScale, 10.0f * ViewScale);
	const FVector2D GlowSize = PanelSize + FVector2D(32.0f * ViewScale, 20.0f * ViewScale);
	FCanvasTileItem GlowTile(GlowPosition, GlowSize, ApplyAlpha(FLinearColor(0.66f, 0.45f, 0.12f, 0.18f), 0.9f));
	GlowTile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(GlowTile);

	DrawPanel(Canvas, PanelPosition, PanelSize, ApplyAlpha(FLinearColor(0.07f, 0.09f, 0.12f, 0.96f)));

	const FVector2D RibbonPosition = PanelPosition + FVector2D(22.0f * ViewScale, 24.0f * ViewScale);
	const FVector2D RibbonSize(PanelSize.X - 44.0f * ViewScale, 18.0f * ViewScale);
	FCanvasTileItem RibbonTile(RibbonPosition, RibbonSize, ApplyAlpha(FLinearColor(0.23f, 0.17f, 0.08f, 0.74f)));
	RibbonTile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(RibbonTile);

	const FString TitleText = LocalizeText(ThiefController, TEXT("LEVEL UP"), TEXT("УРОВЕНЬ ПОВЫШЕН"));
	const FString SubtitleText = LocalizeText(
		ThiefController,
		*FString::Printf(TEXT("Reached level %d"), LevelPopupDisplayedLevel),
		*FString::Printf(TEXT("Достигнут уровень %d"), LevelPopupDisplayedLevel));
	const FString PointsText = LocalizeText(
		ThiefController,
		*FString::Printf(TEXT("Upgrade Points: %d"), LevelPopupDisplayedPoints),
		*FString::Printf(TEXT("Очки улучшений: %d"), LevelPopupDisplayedPoints));
	const FString BodyText = LocalizeText(
		ThiefController,
		TEXT("Spend points on stamina, ore damage and move speed."),
		TEXT("Трать очки на стамину, урон по руде и скорость."));
	const FString FooterText = LocalizeText(ThiefController, TEXT("[P] Open progression"), TEXT("[P] Открыть прокачку"));

	const float CenterX = PanelPosition.X + PanelSize.X * 0.5f;
	DrawTextLine(Canvas, LargeFont, TitleText, FVector2D(CenterX, PanelPosition.Y + 28.0f * LayoutScale), ApplyAlpha(AccentColor), true, 1.02f * ViewScale);
	DrawTextLine(Canvas, MediumFont, SubtitleText, FVector2D(CenterX, PanelPosition.Y + 66.0f * LayoutScale), ApplyAlpha(FLinearColor::White), true, 0.96f * ViewScale);
	DrawTextLine(Canvas, MediumFont, PointsText, FVector2D(CenterX, PanelPosition.Y + 94.0f * LayoutScale), ApplyAlpha(PositiveColor), true, 0.94f * ViewScale);
	DrawWrappedTextBlock(
		Canvas,
		SmallFont,
		BodyText,
		PanelPosition + FVector2D(30.0f * LayoutScale, 122.0f * LayoutScale),
		PanelSize.X - 60.0f * LayoutScale,
		ApplyAlpha(MutedTextColor),
		0.90f * ViewScale,
		1.0f);
	DrawTextLine(
		Canvas,
		MediumFont,
		FooterText,
		FVector2D(CenterX, PanelPosition.Y + PanelSize.Y - 36.0f * LayoutScale),
		ApplyAlpha(FLinearColor(0.96f, 0.92f, 0.84f, 1.0f)),
		true,
		0.90f * ViewScale);
}

void APlayerGameHUD::DrawStaminaPanel(float ViewportHeight) const
{
	AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter();
	if (!PlayerCharacter)
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Font)
	{
		return;
	}

	const FVector2D PanelPosition(34.0f, ViewportHeight - 90.0f);
	const FVector2D PanelSize(320.0f, 54.0f);
	const FVector2D InnerPosition = PanelPosition + FVector2D(10.0f, 8.0f);
	const FVector2D InnerSize(PanelSize.X - 20.0f, PanelSize.Y - 16.0f);

	const float SafeMaxStamina = FMath::Max(PlayerCharacter->MaxStamina, 1.0f);
	const float StaminaPercent = FMath::Clamp(PlayerCharacter->Stamina / SafeMaxStamina, 0.0f, 1.0f);
	const AThiefPlayerController* ThiefController = GetThiefPlayerController();

	DrawPanel(Canvas, PanelPosition, PanelSize);

	FCanvasTileItem BarBackground(InnerPosition, InnerSize, FLinearColor(0.08f, 0.08f, 0.08f, 1.0f));
	BarBackground.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BarBackground);

	FCanvasTileItem BarFill(InnerPosition, FVector2D(InnerSize.X * StaminaPercent, InnerSize.Y), StaminaFillColor);
	BarFill.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BarFill);

	const FString StaminaText = ThiefController && ThiefController->IsRussianLanguage()
		? FString::Printf(TEXT("Стамина: %d / %d"), FMath::RoundToInt(PlayerCharacter->Stamina), FMath::RoundToInt(SafeMaxStamina))
		: FString::Printf(TEXT("Stamina: %d / %d"), FMath::RoundToInt(PlayerCharacter->Stamina), FMath::RoundToInt(SafeMaxStamina));

	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	Canvas->StrLen(Font, StaminaText, TextWidth, TextHeight);

	FCanvasTextItem StaminaTextItem(
		PanelPosition + FVector2D((PanelSize.X - TextWidth) * 0.5f, (PanelSize.Y - TextHeight) * 0.5f),
		FText::FromString(StaminaText),
		Font,
		FLinearColor::White);
	StaminaTextItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(StaminaTextItem);
}

void APlayerGameHUD::DrawInteractionPrompt(float ViewportWidth, float ViewportHeight) const
{
	const AThiefPlayerController* ThiefController = GetThiefPlayerController();
	if (!ThiefController || ThiefController->IsAnyModalOpen() || !ThiefController->HasNearbyTrader())
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Font)
	{
		return;
	}

	const FText PromptText = ThiefController->GetNearbyTraderPromptText();
	if (PromptText.IsEmpty())
	{
		return;
	}

	float PromptTextWidth = 0.0f;
	float PromptTextHeight = 0.0f;
	Canvas->StrLen(Font, PromptText.ToString(), PromptTextWidth, PromptTextHeight);

	const FVector2D PanelSize(PromptTextWidth + 44.0f, PromptTextHeight + 26.0f);
	const FVector2D PanelPosition((ViewportWidth - PanelSize.X) * 0.5f, ViewportHeight - 156.0f);
	DrawPanel(Canvas, PanelPosition, PanelSize);
	DrawTextLine(Canvas, Font, PromptText.ToString(), PanelPosition + FVector2D(22.0f, 12.0f), PositiveColor);

	const UWorkOrderSubsystem* WorkOrderSubsystem = ThiefController->GetGameInstance()
		? ThiefController->GetGameInstance()->GetSubsystem<UWorkOrderSubsystem>()
		: nullptr;
	const FString SecondaryHint = WorkOrderSubsystem && WorkOrderSubsystem->IsShiftReadyToTurnIn()
		? (ThiefController->IsRussianLanguage()
			? TEXT("[B] Торговля   |   [P] Прокачка")
			: TEXT("[B] Trade   |   [P] Progression"))
		: (ThiefController->IsRussianLanguage()
			? TEXT("[B] Торговля   |   [P] Прокачка")
			: TEXT("[B] Trade   |   [P] Character Progression"));
	float SecondaryTextWidth = 0.0f;
	float SecondaryTextHeight = 0.0f;
	Canvas->StrLen(Font, SecondaryHint, SecondaryTextWidth, SecondaryTextHeight);
	DrawTextLine(
		Canvas,
		Font,
		SecondaryHint,
		FVector2D((ViewportWidth - SecondaryTextWidth) * 0.5f, PanelPosition.Y + PanelSize.Y + 12.0f),
		FLinearColor::White);
}

void APlayerGameHUD::DrawTradePanel(float ViewportWidth, float ViewportHeight)
{
	AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter();
	if (!PlayerCharacter)
	{
		return;
	}

	UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!LargeFont || !MediumFont)
	{
		return;
	}

	const AThiefPlayerController* ThiefController = GetThiefPlayerController();
	const bool bRussian = ThiefController && ThiefController->IsRussianLanguage();
	const int32 GoldPerOre = ThiefController ? ThiefController->GetGoldPerOre() : 10;
	const int32 PotionCost = ThiefController ? ThiefController->GetStaminaPotionCost() : 25;
	const int32 PotionRestoreAmount = ThiefController ? FMath::RoundToInt(ThiefController->GetStaminaPotionRestoreAmount()) : 40;
	const FString PotionKeyLabel = ThiefController
		? GetCompactKeyLabel(ThiefController->GetInputActionKeyText(ERemappableInputAction::UsePotion))
		: TEXT("Q");

	const float HorizontalMargin = FMath::Clamp(ViewportWidth * 0.04f, 28.0f, 72.0f);
	const float VerticalMargin = FMath::Clamp(ViewportHeight * 0.05f, 28.0f, 60.0f);
	const FVector2D PanelPosition(HorizontalMargin, VerticalMargin);
	const FVector2D PanelSize(ViewportWidth - HorizontalMargin * 2.0f, ViewportHeight - VerticalMargin * 2.0f);
	DrawPanel(Canvas, PanelPosition, PanelSize, FLinearColor(0.03f, 0.04f, 0.05f, 0.97f));

	FCanvasTileItem AccentStrip(PanelPosition, FVector2D(PanelSize.X, 8.0f), AccentColor);
	AccentStrip.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(AccentStrip);

	const FString TraderTitle = bRussian ? TEXT("Лавка торговца") : TEXT("Trader's Shop");
	DrawTextLine(Canvas, LargeFont, TraderTitle, PanelPosition + FVector2D(PanelSize.X * 0.5f, 28.0f), AccentColor, true);

	if (ThiefController && ThiefController->HasNearbyTrader())
	{
		DrawTextLine(
			Canvas,
			MediumFont,
			ThiefController->GetNearbyTraderName().ToString(),
			PanelPosition + FVector2D(PanelSize.X * 0.5f, 70.0f),
			MutedTextColor,
			true);
	}

	const float CardGap = 22.0f;
	const FVector2D ValueCardSize((PanelSize.X - 64.0f - CardGap * 2.0f) / 3.0f, 82.0f);
	const FVector2D OreCardPosition = PanelPosition + FVector2D(32.0f, 106.0f);
	const FVector2D GoldCardPosition = OreCardPosition + FVector2D(ValueCardSize.X + CardGap, 0.0f);
	const FVector2D PotionCardPosition = GoldCardPosition + FVector2D(ValueCardSize.X + CardGap, 0.0f);
	DrawPanel(Canvas, OreCardPosition, ValueCardSize, SecondaryPanelColor);
	DrawPanel(Canvas, GoldCardPosition, ValueCardSize, SecondaryPanelColor);
	DrawPanel(Canvas, PotionCardPosition, ValueCardSize, SecondaryPanelColor);

	DrawTextLine(Canvas, MediumFont, bRussian ? TEXT("Руда") : TEXT("Ore"), OreCardPosition + FVector2D(18.0f, 12.0f), MutedTextColor);
	DrawTextLine(Canvas, LargeFont, FString::FromInt(PlayerCharacter->CollectedOreResources), OreCardPosition + FVector2D(18.0f, 36.0f), FLinearColor::White);
	DrawTextLine(Canvas, MediumFont, bRussian ? TEXT("Золото") : TEXT("Gold"), GoldCardPosition + FVector2D(18.0f, 12.0f), MutedTextColor);
	DrawTextLine(Canvas, LargeFont, FString::FromInt(PlayerCharacter->GetCollectedGold()), GoldCardPosition + FVector2D(18.0f, 36.0f), AccentColor);
	DrawTextLine(Canvas, MediumFont, bRussian ? TEXT("Зелья") : TEXT("Potions"), PotionCardPosition + FVector2D(18.0f, 12.0f), MutedTextColor);
	DrawTextLine(Canvas, LargeFont, FString::FromInt(PlayerCharacter->GetStaminaPotionCount()), PotionCardPosition + FVector2D(18.0f, 36.0f), PositiveColor);

	DrawTextLine(
		Canvas,
		MediumFont,
		bRussian ? TEXT("Выбери действие торговли") : TEXT("Choose a trade action"),
		PanelPosition + FVector2D(32.0f, 200.0f),
		FLinearColor::White);

	const FVector2D TradeButtonSize(PanelSize.X - 64.0f, 72.0f);
	const FVector2D TradeButtonOne = PanelPosition + FVector2D(32.0f, 228.0f);
	const FVector2D TradeButtonTwo = PanelPosition + FVector2D(32.0f, 314.0f);
	const FVector2D TradeButtonThree = PanelPosition + FVector2D(32.0f, 400.0f);

	DrawActionButton(
		bRussian ? TEXT("[1] Продать 1 руду") : TEXT("[1] Sell 1 Ore"),
		FString::Printf(TEXT("+%d %s"), GoldPerOre, bRussian ? TEXT("золота") : TEXT("Gold")),
		TradeButtonOne,
		TradeButtonSize,
		EHUDMenuAction::TradeSellOne);
	DrawActionButton(
		bRussian ? TEXT("[2] Продать 5 руды") : TEXT("[2] Sell 5 Ore"),
		FString::Printf(TEXT("+%d %s"), GoldPerOre * 5, bRussian ? TEXT("золота") : TEXT("Gold")),
		TradeButtonTwo,
		TradeButtonSize,
		EHUDMenuAction::TradeSellFive);
	DrawActionButton(
		bRussian ? TEXT("[3] Купить зелье стамины") : TEXT("[3] Buy Stamina Potion"),
		bRussian
			? FString::Printf(TEXT("-%d золота   +1 зелье (%d стамины)"), PotionCost, PotionRestoreAmount)
			: FString::Printf(TEXT("-%d Gold   +1 Potion (%d Stamina)"), PotionCost, PotionRestoreAmount),
		TradeButtonThree,
		TradeButtonSize,
		EHUDMenuAction::TradeBuyPotion,
		true);

	const UWorkOrderSubsystem* WorkOrderSubsystem = ThiefController && ThiefController->GetGameInstance()
		? ThiefController->GetGameInstance()->GetSubsystem<UWorkOrderSubsystem>()
		: nullptr;
	if (WorkOrderSubsystem && WorkOrderSubsystem->IsShiftReadyToTurnIn())
	{
		const FVector2D TurnInBannerPosition = PanelPosition + FVector2D(32.0f, 492.0f);
		const FVector2D TurnInBannerSize(PanelSize.X - 64.0f, 76.0f);
		DrawPanel(Canvas, TurnInBannerPosition, TurnInBannerSize, FLinearColor(0.13f, 0.17f, 0.08f, 0.96f));
		DrawTextLine(Canvas, MediumFont, bRussian ? TEXT("Заказ готов к сдаче") : TEXT("Work order ready to turn in"), TurnInBannerPosition + FVector2D(18.0f, 10.0f), AccentColor);
		DrawTextLine(Canvas, MediumFont, bRussian ? TEXT("Нажми [E], чтобы закрыть смену и получить награду.") : TEXT("Press [E] to close the shift and collect the reward."), TurnInBannerPosition + FVector2D(18.0f, 30.0f), FLinearColor::White, false, 0.88f);
		DrawTextLine(
			Canvas,
			MediumFont,
			WorkOrderSubsystem->IsTrackedBonusCompleted(PlayerCharacter)
				? (bRussian ? TEXT("Бонусная цель выполнена.") : TEXT("Bonus objective completed."))
				: (bRussian ? TEXT("Бонусную цель ещё можно добить перед сдачей.") : TEXT("Bonus objective can still be finished before turn-in.")),
			TurnInBannerPosition + FVector2D(18.0f, 52.0f),
			WorkOrderSubsystem->IsTrackedBonusCompleted(PlayerCharacter) ? PositiveColor : MutedTextColor,
			false,
			0.78f);
	}

	DrawTextLine(
		Canvas,
		MediumFont,
		bRussian
			? FString::Printf(TEXT("Мышь или клавиши [1] [2] [3]   |   [%s] Выпить зелье   |   [Esc] Закрыть"), *PotionKeyLabel)
			: FString::Printf(TEXT("Mouse or keys [1] [2] [3]   |   [%s] Drink Potion   |   [Esc] Close"), *PotionKeyLabel),
		PanelPosition + FVector2D(32.0f, PanelSize.Y - 44.0f),
		MutedTextColor);
}

void APlayerGameHUD::DrawProgressionPanel(float ViewportWidth, float ViewportHeight)
{
	AGameplayCharacterBase* PlayerCharacter = GetPlayerCharacter();
	if (!PlayerCharacter)
	{
		return;
	}

	UFont* LargeFont = GEngine ? GEngine->GetLargeFont() : nullptr;
	UFont* MediumFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!LargeFont || !MediumFont)
	{
		return;
	}

	const float HorizontalMargin = FMath::Clamp(ViewportWidth * 0.05f, 32.0f, 84.0f);
	const float VerticalMargin = FMath::Clamp(ViewportHeight * 0.06f, 28.0f, 72.0f);
	const FVector2D PanelPosition(HorizontalMargin, VerticalMargin);
	const FVector2D PanelSize(ViewportWidth - HorizontalMargin * 2.0f, ViewportHeight - VerticalMargin * 2.0f);
	DrawPanel(Canvas, PanelPosition, PanelSize, FLinearColor(0.03f, 0.04f, 0.05f, 0.97f));
	FCanvasTileItem AccentStrip(PanelPosition, FVector2D(PanelSize.X, 8.0f), AccentColor);
	AccentStrip.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(AccentStrip);

	const AThiefPlayerController* ThiefController = GetThiefPlayerController();
	const bool bRussian = ThiefController && ThiefController->IsRussianLanguage();

	DrawTextLine(Canvas, LargeFont, bRussian ? TEXT("Прокачка персонажа") : TEXT("Character Progression"), PanelPosition + FVector2D(PanelSize.X * 0.5f, 24.0f), AccentColor, true);
	DrawTextLine(
		Canvas,
		MediumFont,
		bRussian
			? FString::Printf(TEXT("Уровень: %d"), PlayerCharacter->GetPlayerLevel())
			: FString::Printf(TEXT("Level: %d"), PlayerCharacter->GetPlayerLevel()),
		PanelPosition + FVector2D(32.0f, 84.0f),
		FLinearColor::White);
	DrawTextLine(
		Canvas,
		MediumFont,
		bRussian
			? FString::Printf(TEXT("Опыт: %d / %d"), PlayerCharacter->GetCurrentExperienceAmount(), PlayerCharacter->GetExperienceToNextLevelAmount())
			: FString::Printf(TEXT("XP: %d / %d"), PlayerCharacter->GetCurrentExperienceAmount(), PlayerCharacter->GetExperienceToNextLevelAmount()),
		PanelPosition + FVector2D(260.0f, 84.0f),
		FLinearColor::White);
	DrawTextLine(
		Canvas,
		MediumFont,
		bRussian
			? FString::Printf(TEXT("Очки улучшений: %d"), PlayerCharacter->GetAvailableUpgradePoints())
			: FString::Printf(TEXT("Upgrade Points: %d"), PlayerCharacter->GetAvailableUpgradePoints()),
		PanelPosition + FVector2D(520.0f, 84.0f),
		PositiveColor);
	DrawTextLine(
		Canvas,
		MediumFont,
		bRussian
			? FString::Printf(TEXT("Макс. стамина: %d"), FMath::RoundToInt(PlayerCharacter->MaxStamina))
			: FString::Printf(TEXT("Max Stamina: %d"), FMath::RoundToInt(PlayerCharacter->MaxStamina)),
		PanelPosition + FVector2D(32.0f, 138.0f),
		FLinearColor::White);
	DrawTextLine(
		Canvas,
		MediumFont,
		bRussian
			? FString::Printf(TEXT("Урон по руде: %.0f"), PlayerCharacter->GetOreDamageAmount())
			: FString::Printf(TEXT("Ore Damage: %.0f"), PlayerCharacter->GetOreDamageAmount()),
		PanelPosition + FVector2D(32.0f, 178.0f),
		FLinearColor::White);
	DrawTextLine(
		Canvas,
		MediumFont,
		bRussian
			? FString::Printf(TEXT("Скорость: %.0f"), PlayerCharacter->WalkSpeed)
			: FString::Printf(TEXT("Walk Speed: %.0f"), PlayerCharacter->WalkSpeed),
		PanelPosition + FVector2D(32.0f, 218.0f),
		FLinearColor::White);
	const float UpgradeColumnWidth = FMath::Min(420.0f, PanelSize.X * 0.42f);
	const FVector2D UpgradeButtonSize(UpgradeColumnWidth, 68.0f);
	const float UpgradeColumnX = PanelPosition.X + PanelSize.X - UpgradeColumnWidth - 36.0f;
	const FVector2D UpgradeButtonOne = FVector2D(UpgradeColumnX, PanelPosition.Y + 124.0f);
	const FVector2D UpgradeButtonTwo = FVector2D(UpgradeColumnX, PanelPosition.Y + 206.0f);
	const FVector2D UpgradeButtonThree = FVector2D(UpgradeColumnX, PanelPosition.Y + 288.0f);

	DrawActionButton(
		bRussian ? TEXT("[1] Макс. стамина") : TEXT("[1] Max Stamina"),
		bRussian ? TEXT("+20 к максимуму стамины") : TEXT("+20 Max Stamina"),
		UpgradeButtonOne,
		UpgradeButtonSize,
		EHUDMenuAction::UpgradeMaxStamina,
		true);
	DrawActionButton(
		bRussian ? TEXT("[2] Урон по руде") : TEXT("[2] Ore Damage"),
		bRussian ? TEXT("+10 к урону по руде") : TEXT("+10 Ore Damage"),
		UpgradeButtonTwo,
		UpgradeButtonSize,
		EHUDMenuAction::UpgradeOreDamage);
	DrawActionButton(
		bRussian ? TEXT("[3] Скорость") : TEXT("[3] Move Speed"),
		bRussian ? TEXT("+30 к скорости") : TEXT("+30 Move Speed"),
		UpgradeButtonThree,
		UpgradeButtonSize,
		EHUDMenuAction::UpgradeMoveSpeed);
	DrawTextLine(Canvas, MediumFont, bRussian ? TEXT("[Esc] Закрыть") : TEXT("[Esc] Close"), PanelPosition + FVector2D(32.0f, PanelSize.Y - 48.0f), FLinearColor::White);
}

void APlayerGameHUD::DrawMenuButton(const FString& Label, const FVector2D& Position, const FVector2D& Size, EHUDMenuAction Action, bool bPrimary, int32 Payload)
{
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Font)
	{
		return;
	}

	RegisterButton(Action, Position, Size, Payload);
	const bool bHovered = IsButtonHovered(ActiveButtons.Last());
	FLinearColor FillColor = bPrimary
		? (bHovered ? PrimaryButtonHoveredColor : PrimaryButtonColor)
		: (bHovered ? ButtonHoveredColor : ButtonColor);
	if (Action == EHUDMenuAction::QuitGame || Action == EHUDMenuAction::ResetCharacterProgress || Action == EHUDMenuAction::AbandonShift)
	{
		FillColor = bHovered ? DangerButtonHoveredColor : DangerButtonColor;
	}

	DrawPanel(Canvas, Position, Size, FillColor);

	const float TextScale = FMath::Clamp(Size.Y / 58.0f, 0.98f, 1.14f);
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	Canvas->StrLen(Font, Label, TextWidth, TextHeight);
	DrawTextLine(
		Canvas,
		Font,
		Label,
		Position + FVector2D((Size.X - TextWidth * TextScale) * 0.5f, (Size.Y - TextHeight * TextScale) * 0.5f),
		FLinearColor::White,
		false,
		TextScale);
}

void APlayerGameHUD::DrawActionButton(
	const FString& Title,
	const FString& Description,
	const FVector2D& Position,
	const FVector2D& Size,
	EHUDMenuAction Action,
	bool bPrimary,
	int32 Payload)
{
	UFont* TitleFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* DescriptionFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!TitleFont || !DescriptionFont)
	{
		return;
	}

	RegisterButton(Action, Position, Size, Payload);
	const bool bHovered = IsButtonHovered(ActiveButtons.Last());
	const FLinearColor FillColor = bPrimary
		? (bHovered ? PrimaryButtonHoveredColor : PrimaryButtonColor)
		: (bHovered ? ButtonHoveredColor : ButtonColor);

	DrawPanel(Canvas, Position, Size, FillColor);
	DrawTextLine(Canvas, TitleFont, Title, Position + FVector2D(18.0f, 8.0f), FLinearColor::White);
	DrawTextLine(Canvas, DescriptionFont, Description, Position + FVector2D(18.0f, 34.0f), MutedTextColor);
}

FVector2D APlayerGameHUD::GetMousePosition() const
{
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (PlayerOwner && PlayerOwner->GetMousePosition(MouseX, MouseY))
	{
		return FVector2D(MouseX, MouseY);
	}

	return FVector2D(-1.0f, -1.0f);
}

bool APlayerGameHUD::IsButtonHovered(const FHUDButtonData& ButtonData) const
{
	const FVector2D MousePosition = CachedMousePosition;
	return MousePosition.X >= ButtonData.Position.X
		&& MousePosition.X <= (ButtonData.Position.X + ButtonData.Size.X)
		&& MousePosition.Y >= ButtonData.Position.Y
		&& MousePosition.Y <= (ButtonData.Position.Y + ButtonData.Size.Y);
}

void APlayerGameHUD::RegisterButton(EHUDMenuAction Action, const FVector2D& Position, const FVector2D& Size, int32 Payload)
{
	FHUDButtonData ButtonData;
	ButtonData.Action = Action;
	ButtonData.Position = Position;
	ButtonData.Size = Size;
	ButtonData.Payload = Payload;
	ActiveButtons.Add(ButtonData);
}

void APlayerGameHUD::RegisterSlider(EHUDSliderTarget Target, const FVector2D& Position, const FVector2D& Size)
{
	FHUDSliderData SliderData;
	SliderData.Target = Target;
	SliderData.Position = Position;
	SliderData.Size = Size;
	ActiveSliders.Add(SliderData);
}

void APlayerGameHUD::UpdateActiveSliderDrag()
{
	if (ActiveDragSlider == EHUDSliderTarget::None)
	{
		return;
	}

	AThiefPlayerController* ThiefController = GetThiefPlayerController();
	const bool bMouseDown = PlayerOwner && PlayerOwner->IsInputKeyDown(EKeys::LeftMouseButton);
	if (!ThiefController || !bMouseDown)
	{
		// Drag finished: write the final value to disk once instead of every frame.
		if (ThiefController)
		{
			ThiefController->PersistSettingsToDisk();
		}
		ActiveDragSlider = EHUDSliderTarget::None;
		return;
	}

	for (const FHUDSliderData& Slider : ActiveSliders)
	{
		if (Slider.Target != ActiveDragSlider)
		{
			continue;
		}

		const float Ratio = FMath::Clamp((CachedMousePosition.X - Slider.Position.X) / FMath::Max(Slider.Size.X, 1.0f), 0.0f, 1.0f);
		ThiefController->SetSettingValueFromRatio(ActiveDragSlider, Ratio);
		break;
	}
}

AThiefPlayerController* APlayerGameHUD::GetThiefPlayerController() const
{
	return Cast<AThiefPlayerController>(PlayerOwner);
}

AGameplayCharacterBase* APlayerGameHUD::GetPlayerCharacter() const
{
	if (!PlayerOwner)
	{
		return nullptr;
	}

	return Cast<AGameplayCharacterBase>(PlayerOwner->GetPawn());
}
