#pragma once

// ============================================================================
//  FMyProjectJsonSaveUtils (.h) — интерфейс слоя сохранений JSON.
//  Ниже — структуры сохраняемых данных (настройки, персонаж, заказы) и функции
//  записи/чтения файлов в Saved/JsonSaves. Реализация — в .cpp.
//
//  ЗАЩИТА — по этапу 10 (сохранение): Save/LoadCharacterData,
//  Save/LoadSettingsData, Save/LoadWorkOrderData, EnsureSaveDirectoryExists.
// ============================================================================

#include "CoreMinimal.h"
#include "ThiefPlayerController.h"
#include "MyProjectJsonSaveUtils.generated.h"

USTRUCT()
struct FMyProjectInputBindingSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 InputAction = static_cast<uint8>(ERemappableInputAction::None);

	UPROPERTY()
	FString KeyName;
};

USTRUCT()
struct FMyProjectSettingsSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	float MasterVolumeSetting = 1.0f;

	UPROPERTY()
	float MusicVolumeSetting = 0.65f;

	UPROPERTY()
	float SfxVolumeSetting = 1.0f;

	UPROPERTY()
	float MenuScaleSetting = 1.0f;

	UPROPERTY()
	float LookSensitivitySetting = 0.07f;

	UPROPERTY()
	bool bInvertLookY = false;

	UPROPERTY()
	uint8 Language = static_cast<uint8>(EGameLanguage::Russian);

	UPROPERTY()
	TArray<FMyProjectInputBindingSaveData> InputBindings;
};

USTRUCT()
struct FMyProjectCharacterSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	float MaxStamina = 100.0f;

	UPROPERTY()
	float CurrentStamina = 100.0f;

	UPROPERTY()
	int32 CollectedOreResources = 0;

	UPROPERTY()
	int32 CollectedGold = 0;

	UPROPERTY()
	int32 StaminaPotionCount = 0;

	UPROPERTY()
	int32 PlayerLevel = 1;

	UPROPERTY()
	int32 CurrentExperience = 0;

	UPROPERTY()
	int32 ExperienceToNextLevel = 100;

	UPROPERTY()
	int32 AvailableUpgradePoints = 0;

	UPROPERTY()
	float OreDamage = 1.0f;

	UPROPERTY()
	float WalkSpeed = 450.0f;

	UPROPERTY()
	float SprintSpeed = 700.0f;

	UPROPERTY()
	int32 TotalOreCollected = 0;

	UPROPERTY()
	int32 TotalOreSold = 0;

	UPROPERTY()
	int32 TotalOreNodesBroken = 0;

	UPROPERTY()
	int32 TotalGoldEarned = 0;

	UPROPERTY()
	int32 TotalPotionsBought = 0;

	UPROPERTY()
	int32 TotalPotionsUsed = 0;
};

USTRUCT()
struct FMyProjectWorkOrderSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Reputation = 0;

	UPROPERTY()
	int32 CompletedShiftCount = 0;

	UPROPERTY()
	int32 LastSelectedOrderIndex = 0;

	UPROPERTY()
	TArray<int32> CompletedOrderCounts;
};

class MYPROJECT_API FMyProjectJsonSaveUtils
{
public:
	static bool SaveSettingsData(const FMyProjectSettingsSaveData& SaveData);
	static bool LoadSettingsData(FMyProjectSettingsSaveData& OutSaveData);

	static bool SaveCharacterData(const FMyProjectCharacterSaveData& SaveData);
	static bool LoadCharacterData(FMyProjectCharacterSaveData& OutSaveData);

	static bool SaveWorkOrderData(const FMyProjectWorkOrderSaveData& SaveData);
	static bool LoadWorkOrderData(FMyProjectWorkOrderSaveData& OutSaveData);

private:
	static FString GetSettingsSavePath();
	static FString GetCharacterSavePath();
	static FString GetWorkOrderSavePath();
	static bool EnsureSaveDirectoryExists(const FString& FilePath);

	template<typename TSaveStruct>
	static bool SaveStructToJsonFile(const TSaveStruct& SaveData, const FString& FilePath);

	template<typename TSaveStruct>
	static bool LoadStructFromJsonFile(TSaveStruct& OutSaveData, const FString& FilePath);
};
