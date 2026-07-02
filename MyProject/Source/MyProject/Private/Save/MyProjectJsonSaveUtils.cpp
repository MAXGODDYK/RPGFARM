// ============================================================================
//  FMyProjectJsonSaveUtils — слой сохранений в формате JSON.
//  Чтение/запись файлов я вынес в отдельный класс, чтобы не смешивать сохранение
//  с игровой логикой. Структуры данных превращаются в JSON-строку и обратно;
//  файлы лежат в Saved/JsonSaves: Settings.json, Character.json, WorkOrders.json.
//  Формат текстовый и читаемый — можно открыть блокнотом и проверить.
//
//  ЗАЩИТА — по этапам показа:
//   • Этап 10 (сохранение): SaveCharacterData()/LoadCharacterData() — прогресс;
//     SaveSettingsData()/LoadSettingsData() — настройки;
//     SaveWorkOrderData()/LoadWorkOrderData() — заказы;
//     EnsureSaveDirectoryExists() — создаёт папку сохранений.
//   (Показываю сам файл Character.json, потом перезапуск — данные на месте.)
// ============================================================================
#include "Save/MyProjectJsonSaveUtils.h"

#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	const FString JsonSaveDirectory = TEXT("JsonSaves");
	const FString SettingsFileName = TEXT("Settings.json");
	const FString CharacterFileName = TEXT("Character.json");
	const FString WorkOrderFileName = TEXT("WorkOrders.json");
}

bool FMyProjectJsonSaveUtils::SaveSettingsData(const FMyProjectSettingsSaveData& SaveData)
{
	return SaveStructToJsonFile(SaveData, GetSettingsSavePath());
}

bool FMyProjectJsonSaveUtils::LoadSettingsData(FMyProjectSettingsSaveData& OutSaveData)
{
	return LoadStructFromJsonFile(OutSaveData, GetSettingsSavePath());
}

bool FMyProjectJsonSaveUtils::SaveCharacterData(const FMyProjectCharacterSaveData& SaveData)
{
	return SaveStructToJsonFile(SaveData, GetCharacterSavePath());
}

bool FMyProjectJsonSaveUtils::LoadCharacterData(FMyProjectCharacterSaveData& OutSaveData)
{
	return LoadStructFromJsonFile(OutSaveData, GetCharacterSavePath());
}

bool FMyProjectJsonSaveUtils::SaveWorkOrderData(const FMyProjectWorkOrderSaveData& SaveData)
{
	return SaveStructToJsonFile(SaveData, GetWorkOrderSavePath());
}

bool FMyProjectJsonSaveUtils::LoadWorkOrderData(FMyProjectWorkOrderSaveData& OutSaveData)
{
	return LoadStructFromJsonFile(OutSaveData, GetWorkOrderSavePath());
}

FString FMyProjectJsonSaveUtils::GetSettingsSavePath()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), JsonSaveDirectory, SettingsFileName);
}

FString FMyProjectJsonSaveUtils::GetCharacterSavePath()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), JsonSaveDirectory, CharacterFileName);
}

FString FMyProjectJsonSaveUtils::GetWorkOrderSavePath()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), JsonSaveDirectory, WorkOrderFileName);
}

bool FMyProjectJsonSaveUtils::EnsureSaveDirectoryExists(const FString& FilePath)
{
	return IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);
}

template<typename TSaveStruct>
bool FMyProjectJsonSaveUtils::SaveStructToJsonFile(const TSaveStruct& SaveData, const FString& FilePath)
{
	if (!EnsureSaveDirectoryExists(FilePath))
	{
		return false;
	}

	FString JsonString;
	if (!FJsonObjectConverter::UStructToJsonObjectString(SaveData, JsonString))
	{
		return false;
	}

	return FFileHelper::SaveStringToFile(JsonString, *FilePath);
}

template<typename TSaveStruct>
bool FMyProjectJsonSaveUtils::LoadStructFromJsonFile(TSaveStruct& OutSaveData, const FString& FilePath)
{
	if (!FPaths::FileExists(FilePath))
	{
		return false;
	}

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		return false;
	}

	return FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &OutSaveData, 0, 0);
}

template bool FMyProjectJsonSaveUtils::SaveStructToJsonFile(const FMyProjectSettingsSaveData& SaveData, const FString& FilePath);
template bool FMyProjectJsonSaveUtils::LoadStructFromJsonFile(FMyProjectSettingsSaveData& OutSaveData, const FString& FilePath);
template bool FMyProjectJsonSaveUtils::SaveStructToJsonFile(const FMyProjectCharacterSaveData& SaveData, const FString& FilePath);
template bool FMyProjectJsonSaveUtils::LoadStructFromJsonFile(FMyProjectCharacterSaveData& OutSaveData, const FString& FilePath);
template bool FMyProjectJsonSaveUtils::SaveStructToJsonFile(const FMyProjectWorkOrderSaveData& SaveData, const FString& FilePath);
template bool FMyProjectJsonSaveUtils::LoadStructFromJsonFile(FMyProjectWorkOrderSaveData& OutSaveData, const FString& FilePath);
