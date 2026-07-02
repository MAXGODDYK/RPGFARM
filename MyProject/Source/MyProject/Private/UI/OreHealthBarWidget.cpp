#include "UI/OreHealthBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"
#include "Widgets/SWidget.h"

void UOreHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetHealthValues(CachedCurrentValue, CachedMaxValue);
}

TSharedRef<SWidget> UOreHealthBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}

	return Super::RebuildWidget();
}

void UOreHealthBarWidget::SetHealthPercent(float InHealthPercent)
{
	const float ClampedPercent = FMath::Clamp(InHealthPercent, 0.0f, 1.0f);
	SetHealthValues(ClampedPercent * 100.0f, 100.0f);
}

void UOreHealthBarWidget::SetHealthValues(float CurrentValue, float MaxValue)
{
	const float SafeMaxValue = FMath::Max(MaxValue, 1.0f);
	const float ClampedCurrentValue = FMath::Clamp(CurrentValue, 0.0f, SafeMaxValue);
	const float Percent = ClampedCurrentValue / SafeMaxValue;
	CachedCurrentValue = ClampedCurrentValue;
	CachedMaxValue = SafeMaxValue;

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(Percent);
	}

	UpdateHealthText(ClampedCurrentValue, SafeMaxValue);
}

void UOreHealthBarWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
	RootSizeBox->SetWidthOverride(320.0f);
	RootSizeBox->SetHeightOverride(72.0f);
	WidgetTree->RootWidget = RootSizeBox;

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f));
	RootBorder->SetPadding(FMargin(10.0f, 8.0f));
	RootSizeBox->AddChild(RootBorder);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	RootBorder->SetContent(Overlay);

	HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthProgressBar"));
	HealthProgressBar->SetFillColorAndOpacity(FLinearColor(0.88f, 0.17f, 0.12f, 1.0f));
	HealthProgressBar->SetPercent(1.0f);
	if (UOverlaySlot* ProgressSlot = Overlay->AddChildToOverlay(HealthProgressBar))
	{
		ProgressSlot->SetHorizontalAlignment(HAlign_Fill);
		ProgressSlot->SetVerticalAlignment(VAlign_Fill);
	}

	HealthTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthTextBlock"));
	HealthTextBlock->SetText(FText::FromString(TEXT("100 / 100")));
	HealthTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	HealthTextBlock->SetJustification(ETextJustify::Center);
	HealthTextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
	HealthTextBlock->SetShadowColorAndOpacity(FLinearColor::Black);
	HealthTextBlock->SetRenderScale(FVector2D(1.4f, 1.4f));
	if (UOverlaySlot* TextSlot = Overlay->AddChildToOverlay(HealthTextBlock))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UOreHealthBarWidget::UpdateHealthText(float CurrentValue, float MaxValue) const
{
	if (!HealthTextBlock)
	{
		return;
	}

	const int32 CurrentInt = FMath::RoundToInt(CurrentValue);
	const int32 MaxInt = FMath::RoundToInt(MaxValue);
	HealthTextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentInt, MaxInt)));
}
