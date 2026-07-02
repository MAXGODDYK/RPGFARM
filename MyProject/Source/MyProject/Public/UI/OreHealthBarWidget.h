#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OreHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class MYPROJECT_API UOreHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Ore Health")
	void SetHealthPercent(float InHealthPercent);

	UFUNCTION(BlueprintCallable, Category = "Ore Health")
	void SetHealthValues(float CurrentValue, float MaxValue);

protected:
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HealthTextBlock;

private:
	void BuildWidgetTree();
	void UpdateHealthText(float CurrentValue, float MaxValue) const;

	float CachedCurrentValue = 0.0f;
	float CachedMaxValue = 0.0f;
};
