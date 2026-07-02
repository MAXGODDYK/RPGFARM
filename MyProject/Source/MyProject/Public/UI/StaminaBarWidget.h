#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StaminaBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class MYPROJECT_API UStaminaBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void SetStaminaValues(float CurrentValue, float MaxValue);

protected:
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> StaminaProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StaminaTextBlock;

private:
	void BuildWidgetTree();
	void UpdateStaminaText(float CurrentValue, float MaxValue) const;

	float CachedCurrentValue = 0.0f;
	float CachedMaxValue = 100.0f;
};
