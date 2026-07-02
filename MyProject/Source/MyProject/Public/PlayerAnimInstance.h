#pragma once

// ============================================================================
//  UPlayerAnimInstance (.h) — интерфейс связи персонажа с анимацией. Ниже —
//  поля для Anim Blueprint (Speed, Direction, bIsInAir, bShouldMove,
//  bIsAttacking). Реализация — в .cpp.
//
//  ЗАЩИТА — по этапам 4-5 (движение/анимация): NativeUpdateAnimation.
// ============================================================================

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance.generated.h"

UCLASS()
class MYPROJECT_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	TObjectPtr<APawn> MyCharacter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float Speed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool bIsInAir;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float Direction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool bShouldMove;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool bIsAccelerating;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	bool bIsAttacking;
};
