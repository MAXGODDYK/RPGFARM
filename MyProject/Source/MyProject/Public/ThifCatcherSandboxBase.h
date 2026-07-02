#pragma once

// ============================================================================
//  AThifCatcherSandboxBase (.h) — играемая пешка (наследник
//  AGameplayCharacterBase) на основе Game Animation Sample. Ниже — ввод
//  движения/прыжка/спринта; вся игровая логика — в базовом классе.
//
//  ЗАЩИТА — по этапам: 4 (управление: MoveForwardBackward, MoveRightLeft, Jump),
//  5 (Sprint/StopSprint → SetSprintActive базы).
// ============================================================================

#include "Animation/AnimMontage.h"
#include "CoreMinimal.h"
#include "GameplayCharacterBase.h"
#include "ThifCatcherSandboxBase.generated.h"

UCLASS()
class MYPROJECT_API AThifCatcherSandboxBase : public AGameplayCharacterBase
{
	GENERATED_BODY()

public:
	AThifCatcherSandboxBase(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JumpAnimation")
	TObjectPtr<UAnimMontage> JumpAnimation;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void MoveForwardBackward(float Value);
	void MoveRightLeft(float Value);

	void Jump();
	void StopJump();

	void Sprint();
	void StopSprint();

protected:
	virtual bool HasMovementInputIntent() const override;

private:
	float ForwardMovementInputValue = 0.0f;
	float RightMovementInputValue = 0.0f;
};
