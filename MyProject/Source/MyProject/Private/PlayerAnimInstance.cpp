// ============================================================================
//  UPlayerAnimInstance — связь персонажа с его анимацией.
//  Каждый кадр считаю из реальной скорости персонажа данные для Anim Blueprint:
//  скорость, направление движения, в воздухе ли, есть ли движение, идёт ли атака.
//  Anim Blueprint по этим значениям выбирает анимацию (стоит/идёт/бежит/удар) —
//  поэтому анимация всегда совпадает с реальным движением.
//
//  ЗАЩИТА — по этапам показа:
//   • Этапы 4–5 (движение/анимация): NativeUpdateAnimation() — расчёт
//     Speed, Direction, bIsInAir, bShouldMove, bIsAttacking.
// ============================================================================
#include "PlayerAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"
#include "GameplayCharacterBase.h"

void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	MyCharacter = TryGetPawnOwner();
	Speed = 0.0f;
	bIsInAir = false;
	Direction = 0.0f;
	GroundSpeed = 0.0f;
	bShouldMove = false;
	bIsAccelerating = false;
	bIsAttacking = false;
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!MyCharacter)
	{
		MyCharacter = TryGetPawnOwner();
	}

	if (!MyCharacter)
	{
		Speed = 0.0f;
		bIsInAir = false;
		Direction = 0.0f;
		GroundSpeed = 0.0f;
		bShouldMove = false;
		bIsAccelerating = false;
		bIsAttacking = false;
		return;
	}

	const FVector Velocity = MyCharacter->GetVelocity();
	const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);

	Speed = HorizontalVelocity.Size();
	GroundSpeed = Speed;
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, MyCharacter->GetActorRotation());
	bShouldMove = GroundSpeed > 3.0f;

	if (const ACharacter* Character = Cast<ACharacter>(MyCharacter))
	{
		const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
		bIsInAir = MovementComponent->IsFalling();
		bIsAccelerating = !MovementComponent->GetCurrentAcceleration().IsNearlyZero();
	}
	else
	{
		bIsInAir = false;
		bIsAccelerating = false;
	}

	if (const AGameplayCharacterBase* GameplayCharacter = Cast<AGameplayCharacterBase>(MyCharacter))
	{
		bIsAttacking = GameplayCharacter->bIsAttacking;
	}
	else
	{
		bIsAttacking = false;
	}
}
