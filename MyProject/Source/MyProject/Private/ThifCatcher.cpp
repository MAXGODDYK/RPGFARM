// ============================================================================
//  AThifCatcher — управляемый персонаж (наследник AGameplayCharacterBase).
//  Этот класс отвечает только за "тело и камеру": камера от третьего лица на
//  SpringArm, ввод движения, прыжок и спринт. Вся игровая логика (стамина,
//  атака, ресурсы) остаётся в базовом классе — здесь только управление.
//
//  ЗАЩИТА — по этапам показа:
//   • Этап 4 (управление/камера): конструктор (SpringArm + Camera),
//     MoveForwardBackward()/MoveRightLeft(), Jump().
//   • Этап 5 (спринт): Sprint()/StopSprint() — вызывают SetSprintActive() базы.
// ============================================================================
#include "ThifCatcher.h"

#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AThifCatcher::AThifCatcher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 420.0f;
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 12.0f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 14.0f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bDoCollisionTest = true;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	Camera->SetupAttachment(SpringArm);
	Camera->bUsePawnControlRotation = false;
	Camera->SetRelativeRotation(FRotator(-10.0f, 0.0f, 0.0f));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		MovementComponent->JumpZVelocity = 650.0f;
		MovementComponent->AirControl = 0.35f;
		MovementComponent->BrakingDecelerationWalking = 1800.0f;
		MovementComponent->MaxAcceleration = 2048.0f;
		MovementComponent->MaxWalkSpeed = WalkSpeed;
	}
}

void AThifCatcher::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		return;
	}

	PlayerInputComponent->BindAxis(TEXT("MoveForwardBackward"), this, &AThifCatcher::MoveForwardBackward);
	PlayerInputComponent->BindAxis(TEXT("MoveRightLeft"), this, &AThifCatcher::MoveRightLeft);

	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AThifCatcher::AddControllerYawInput);
	PlayerInputComponent->BindAxis(TEXT("lookUpDown"), this, &AThifCatcher::AddControllerPitchInput);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AThifCatcher::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AThifCatcher::StopJump);

	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AThifCatcher::Sprint);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AThifCatcher::StopSprint);
}

void AThifCatcher::MoveForwardBackward(const float Value)
{
	if (bIsAttacking || !Controller || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	const FVector Direction = FRotationMatrix(YawRotation).GetScaledAxis(EAxis::X);
	AddMovementInput(Direction, Value);
}

void AThifCatcher::MoveRightLeft(const float Value)
{
	if (bIsAttacking || !Controller || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	const FVector Direction = FRotationMatrix(YawRotation).GetScaledAxis(EAxis::Y);
	AddMovementInput(Direction, Value);
}

void AThifCatcher::Jump()
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

void AThifCatcher::StopJump()
{
	StopJumping();
}

void AThifCatcher::Sprint()
{
	SetSprintActive(true);
}

void AThifCatcher::StopSprint()
{
	SetSprintActive(false);
}
