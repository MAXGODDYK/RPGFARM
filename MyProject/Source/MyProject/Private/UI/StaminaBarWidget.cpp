#include "UI/StaminaBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"
#include "Widgets/SWidget.h"

void UStaminaBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetStaminaValues(CachedCurrentValue, CachedMaxValue);
}

TSharedRef<SWidget> UStaminaBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}

	return Super::RebuildWidget();
}

void UStaminaBarWidget::SetStaminaValues(float CurrentValue, float MaxValue)
{
	const float SafeMaxValue = FMath::Max(MaxValue, 1.0f);
	const float ClampedCurrentValue = FMath::Clamp(CurrentValue, 0.0f, SafeMaxValue);
	const float Percent = ClampedCurrentValue / SafeMaxValue;

	CachedCurrentValue = ClampedCurrentValue;
	CachedMaxValue = SafeMaxValue;

	if (StaminaProgressBar)
	{
		StaminaProgressBar->SetPercent(Percent);
	}

	UpdateStaminaText(ClampedCurrentValue, SafeMaxValue);
}

void UStaminaBarWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
	RootSizeBox->SetWidthOverride(320.0f);
	RootSizeBox->SetHeightOverride(54.0f);
	WidgetTree->RootWidget = RootSizeBox;

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.82f));
	RootBorder->SetPadding(FMargin(10.0f, 8.0f));
	RootSizeBox->AddChild(RootBorder);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	RootBorder->SetContent(Overlay);

	StaminaProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("StaminaProgressBar"));
	StaminaProgressBar->SetFillColorAndOpacity(FLinearColor(0.12f, 0.80f, 0.18f, 1.0f));
	StaminaProgressBar->SetPercent(1.0f);
	if (UOverlaySlot* ProgressSlot = Overlay->AddChildToOverlay(StaminaProgressBar))
	{
		ProgressSlot->SetHorizontalAlignment(HAlign_Fill);
		ProgressSlot->SetVerticalAlignment(VAlign_Fill);
	}

	StaminaTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StaminaTextBlock"));
	StaminaTextBlock->SetText(FText::FromString(TEXT("Stamina: 100 / 100")));
	StaminaTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	StaminaTextBlock->SetJustification(ETextJustify::Center);
	StaminaTextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
	StaminaTextBlock->SetShadowColorAndOpacity(FLinearColor::Black);
	if (UOverlaySlot* TextSlot = Overlay->AddChildToOverlay(StaminaTextBlock))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UStaminaBarWidget::UpdateStaminaText(float CurrentValue, float MaxValue) const
{
	if (!StaminaTextBlock)
	{
		return;
	}

	StaminaTextBlock->SetText(
		FText::FromString(
			FString::Printf(TEXT("Stamina: %d / %d"), FMath::RoundToInt(CurrentValue), FMath::RoundToInt(MaxValue))));
}
