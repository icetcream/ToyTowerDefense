#include "TTDDeveloperSettings.h"

#include "TTDGameplayTags.h"

UTTDDeveloperSettings::UTTDDeveloperSettings()
{
	GameplayTagDataTables.Add(TAG_TTD_DataTable_Research_Parts);
	GameplayTagDataTables.Add(TAG_TTD_DataTable_Research_Diagrams);
	GameplayTagDataTables.Add(TAG_TTD_DataTable_Research_ToyBoxes);
	GameplayTagDataTables.Add(TAG_TTD_DataTable_ObjectPool_Definitions);
	GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_Levels);
	GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_Waves);
	GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_Enemies);
	GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_Buildings);
	GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_ToyBoxRewards);
	GameplayTagDataTables.Add(TAG_TTD_DataTable_Battle_HeightEffects);

	DefaultUnlockedDiagramIds = { TEXT("JokerTower"), TEXT("BatteryBlaster") };
	DefaultUnlockedToyBoxIds = { TEXT("BasicToyBox"), TEXT("MechanicToyBox"), TEXT("EnergyToyBox") };
	DefaultUnlockedPartIds = { TEXT("Gear"), TEXT("Spring") };
}

UDataTable* UTTDDeveloperSettings::GetDataTableForTag(const FGameplayTag DataTableTag) const
{
	const TSoftObjectPtr<UDataTable>* DataTable = GameplayTagDataTables.Find(DataTableTag);
	return DataTable ? DataTable->LoadSynchronous() : nullptr;
}
