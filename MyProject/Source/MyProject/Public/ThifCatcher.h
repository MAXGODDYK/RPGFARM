#pragma once

// ============================================================================
//  AThifCatcher (.h) — интерфейс управляемого персонажа (наследник
//  AGameplayCharacterBase). Ниже — компоненты камеры (SpringArm, Camera) и
//  объявления функций движения/прыжка/спринта. Реализация — в .cpp.
//
//  ЗАЩИТА — по этапам: 4 (управление/камера: конструктор SpringArm+Camera,
//  MoveForwardBackward, MoveRightLeft, Jump), 5 (Sprint/StopSprint).
// ============================================================================

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameplayCharacterBase.h"
#include "ThifCatcher.generated.h"

UCLASS()
class MYPROJECT_API AThifCatcher : public AGameplayCharacterBase
{
	GENERATED_BODY()

public:
	AThifCatcher(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JampAnimation")
	UAnimMontage* JumpAnimation;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void MoveForwardBackward(float Value);
	void MoveRightLeft(float Value);

	void Jump();
	void StopJump();

	void Sprint();
	void StopSprint();
};
