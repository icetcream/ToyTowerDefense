#include "Misc/AutomationTest.h"

#include "TTDDeveloperSettings.h"
#include "TTDGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTTDDeveloperSettingsTests,
	"ToyTowerDefense.Settings.DeveloperSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTDDeveloperSettingsTests::RunTest(const FString& Parameters)
{
	const UTTDDeveloperSettings* Settings = GetDefault<UTTDDeveloperSettings>();
	TestNotNull(TEXT("developer settings are registered"), Settings);

	TestTrue(TEXT("parts data table slot is configured by gameplay tag"), Settings->GameplayTagDataTables.Contains(TAG_TTD_DataTable_Research_Parts));
	TestTrue(TEXT("diagrams data table slot is configured by gameplay tag"), Settings->GameplayTagDataTables.Contains(TAG_TTD_DataTable_Research_Diagrams));
	TestTrue(TEXT("toy boxes data table slot is configured by gameplay tag"), Settings->GameplayTagDataTables.Contains(TAG_TTD_DataTable_Research_ToyBoxes));
	TestTrue(TEXT("object pool data table slot is configured by gameplay tag"), Settings->GameplayTagDataTables.Contains(TAG_TTD_DataTable_ObjectPool_Definitions));
	TestTrue(TEXT("battle levels data table slot is configured by gameplay tag"), Settings->GameplayTagDataTables.Contains(TAG_TTD_DataTable_Battle_Levels));
	TestTrue(TEXT("battle waves data table slot is configured by gameplay tag"), Settings->GameplayTagDataTables.Contains(TAG_TTD_DataTable_Battle_Waves));
	TestTrue(TEXT("battle enemies data table slot is configured by gameplay tag"), Settings->GameplayTagDataTables.Contains(TAG_TTD_DataTable_Battle_Enemies));
	TestTrue(TEXT("battle buildings data table slot is configured by gameplay tag"), Settings->GameplayTagDataTables.Contains(TAG_TTD_DataTable_Battle_Buildings));
	TestTrue(TEXT("battle toy box rewards data table slot is configured by gameplay tag"), Settings->GameplayTagDataTables.Contains(TAG_TTD_DataTable_Battle_ToyBoxRewards));
	TestTrue(TEXT("battle height effects data table slot is configured by gameplay tag"), Settings->GameplayTagDataTables.Contains(TAG_TTD_DataTable_Battle_HeightEffects));
	TestEqual(TEXT("crafting queue capacity comes from settings"), Settings->CraftingQueueMaxSlots, 5);
	TestEqual(TEXT("default battle level id comes from settings"), Settings->DefaultBattleLevelId, FName(TEXT("PrototypeBattle")));

	return true;
}

#endif
