#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ResourceCounterWidget.generated.h"

class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class MYPROJECT_API UResourceCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Resources")
	void SetResourceAmount(int32 InResourceAmount);

protected:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResourceTextBlock;

private:
	void BuildWidgetTree();
	void UpdateResourceText() const;

	int32 CachedResourceAmount = 0;
};
