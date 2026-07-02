// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MainCheracter.generated.h"

UCLASS()
class MYPROJECT_API AMainCheracter : public ACharacter
{
	GENERATED_BODY()

public:
	AMainCheracter();

	UPROPERTY(EditDefaultsOnly, Category = "CheracterStamina")
	float ChacterStamina;

	bool pIsTired;


	float GetStaminaPercent();
	float AddStamina();
	float DecreaseStamina();

	bool StaminaIsZero(); //функция отвечает за смерть персонажа

	UPROPERTY(EditDefaultsOnly, Category = "CheracterStamina")
	USoundBase* TiredSound; //звук усталости персонажа. Мы унаследовали этот класс от ACharacter, поэтому можем использовать все его свойства и методы. USoundBase - класс звука в Unreal Engine.
	
	
	UPROPERTY(EditDefaultsOnly, Category = "CheracterStamina")
	UAnimMetaData* VoiceAnimation; //анимация усталости персонажа


};
