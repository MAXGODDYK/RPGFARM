#include "UI/ResourceCounterWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"
#include "Widgets/SWidget.h"

void UResourceCounterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateResourceText();
}

TSharedRef<SWidget> UResourceCounterWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}

	return Super::RebuildWidget();
}

void UResourceCounterWidget::SetResourceAmount(int32 InResourceAmount)
{
	CachedResourceAmount = FMath::Max(InResourceAmount, 0);
	UpdateResourceText();
}

void UResourceCounterWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
	RootSizeBox->SetWidthOverride(260.0f);
	RootSizeBox->SetHeightOverride(56.0f);
	WidgetTree->RootWidget = RootSizeBox;

	UBorder* ResourceBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResourceBorder"));
	ResourceBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.82f));
	ResourceBorder->SetPadding(FMargin(18.0f, 10.0f));
	RootSizeBox->AddChild(ResourceBorder);

	ResourceTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResourceTextBlock"));
	ResourceTextBlock->SetText(FText::FromString(TEXT("Ore: 0")));
	ResourceTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ResourceTextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
	ResourceTextBlock->SetShadowColorAndOpacity(FLinearColor::Black);
	ResourceTextBlock->SetRenderScale(FVector2D(1.25f, 1.25f));
	ResourceBorder->SetContent(ResourceTextBlock);
}

void UResourceCounterWidget::UpdateResourceText() const
{
	if (!ResourceTextBlock)
	{
		return;
	}

	ResourceTextBlock->SetText(FText::FromString(FString::Printf(TEXT("Ore: %d"), CachedResourceAmount)));
}
