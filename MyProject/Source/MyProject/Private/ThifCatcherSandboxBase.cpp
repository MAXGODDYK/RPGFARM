#include "ThifCatcherSandboxBase.h"

#include "Components/InputComponent.h"

AThifCatcherSandboxBase::AThifCatcherSandboxBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	JumpAnimation = nullptr;
}

void AThifCatcherSandboxBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		return;
	}

	PlayerInputComponent->BindAxis(TEXT("MoveForwardBackward"), this, &AThifCatcherSandboxBase::MoveForwardBackward);
	PlayerInputComponent->BindAxis(TEXT("MoveRightLeft"), this, &AThifCatcherSandboxBase::MoveRightLeft);

	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AThifCatcherSandboxBase::AddControllerYawInput);
	PlayerInputComponent->BindAxis(TEXT("lookUpDown"), this, &AThifCatcherSandboxBase::AddControllerPitchInput);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AThifCatcherSandboxBase::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AThifCatcherSandboxBase::StopJump);

	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AThifCatcherSandboxBase::Sprint);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AThifCatcherSandboxBase::StopSprint);
}

void AThifCatcherSandboxBase::MoveForwardBackward(const float Value)
{
	ForwardMovementInputValue = Value;

	if (bIsAttacking || !Controller || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	const FVector Direction = FRotationMatrix(YawRotation).GetScaledAxis(EAxis::X);
	AddMovementInput(Direction, Value);
}

void AThifCatcherSandboxBase::MoveRightLeft(const float Value)
{
	RightMovementInputValue = Value;

	if (bIsAttacking || !Controller || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	const FVector Direction = FRotationMatrix(YawRotation).GetScaledAxis(EAxis::Y);
	AddMovementInput(Direction, Value);
}

void AThifCatcherSandboxBase::Jump()
{
	if (bIsAttacking)
	{
		return;
	}

	ACharacter::Jump();

	if (JumpAnimation)
	{
		PlayAnimMontage(JumpAnimation, 1.0f, NAME_None);
	}
}

void AThifCatcherSandboxBase::StopJump()
{
	StopJumping();
}

void AThifCatcherSandboxBase::Sprint()
{
	SetSprintActive(true);
}

void AThifCatcherSandboxBase::StopSprint()
{
	SetSprintActive(false);
}

bool AThifCatcherSandboxBase::HasMovementInputIntent() const
{
	return !FMath::IsNearlyZero(ForwardMovementInputValue, 0.01f)
		|| !FMath::IsNearlyZero(RightMovementInputValue, 0.01f);
}
