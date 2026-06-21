#include "UI/TTDUIDisplayNames.h"

#include "Engine/GameInstance.h"
#include "TTDResearchSubsystem.h"

namespace
{
	FText FindPartDisplayName(const UGameInstance* GameInstance, const FName PartId)
	{
		if (!GameInstance)
		{
			return FText::GetEmpty();
		}

		const UTTDResearchSubsystem* ResearchSubsystem = GameInstance->GetSubsystem<UTTDResearchSubsystem>();
		if (!ResearchSubsystem)
		{
			return FText::GetEmpty();
		}

		for (const FTTDPartDefinition& Part : ResearchSubsystem->GetParts())
		{
			if (Part.PartId == PartId && !Part.DisplayName.IsEmpty())
			{
				return Part.DisplayName;
			}
		}

		return FText::GetEmpty();
	}

	FText FindCollectionDisplayName(const UGameInstance* GameInstance, const FName ItemId, const ETTDCollectionCategory Category)
	{
		if (!GameInstance)
		{
			return FText::GetEmpty();
		}

		const UTTDResearchSubsystem* ResearchSubsystem = GameInstance->GetSubsystem<UTTDResearchSubsystem>();
		if (!ResearchSubsystem)
		{
			return FText::GetEmpty();
		}

		for (const FTTDCollectionEntry& Entry : ResearchSubsystem->GetCollectionEntries(Category))
		{
			if (Entry.ItemId == ItemId && !Entry.DisplayName.IsEmpty())
			{
				return Entry.DisplayName;
			}
		}

		return FText::GetEmpty();
	}

	FString JoinTexts(const TArray<FText>& Texts, const TCHAR* EmptyText)
	{
		TArray<FString> Values;
		for (const FText& Text : Texts)
		{
			if (!Text.IsEmpty())
			{
				Values.Add(Text.ToString());
			}
		}

		return Values.IsEmpty() ? FString(EmptyText) : FString::Join(Values, TEXT("、"));
	}

	FString JoinStacks(
		const TArray<FTTDNameStack>& Stacks,
		const TFunctionRef<FText(FName)> NameResolver,
		const TCHAR* EmptyText)
	{
		TArray<FString> Values;
		for (const FTTDNameStack& Stack : Stacks)
		{
			if (!Stack.Id.IsNone() && Stack.Count > 0)
			{
				Values.Add(FString::Printf(TEXT("%s x%d"), *NameResolver(Stack.Id).ToString(), Stack.Count));
			}
		}

		return Values.IsEmpty() ? FString(EmptyText) : FString::Join(Values, TEXT("、"));
	}
}

namespace TTDUIDisplayNames
{
	FText KnownId(const FName Id)
	{
		if (Id == TEXT("BasicTower"))
		{
			return FText::FromString(TEXT("基础塔"));
		}
		if (Id == TEXT("ProjectileTower"))
		{
			return FText::FromString(TEXT("弹射塔"));
		}
		if (Id == TEXT("MissingDiagramTower"))
		{
			return FText::FromString(TEXT("缺图纸测试塔"));
		}
		if (Id == TEXT("ExpensiveTower"))
		{
			return FText::FromString(TEXT("高耗材测试塔"));
		}
		if (Id == TEXT("BasicToyBox") || Id == TEXT("BasicBox"))
		{
			return FText::FromString(TEXT("基础玩具盒"));
		}
		if (Id == TEXT("MechanicToyBox"))
		{
			return FText::FromString(TEXT("机械玩具盒"));
		}
		if (Id == TEXT("EnergyToyBox"))
		{
			return FText::FromString(TEXT("能量玩具盒"));
		}
		if (Id == TEXT("PartyToyBox"))
		{
			return FText::FromString(TEXT("派对玩具盒"));
		}
		if (Id == TEXT("RareToyBox"))
		{
			return FText::FromString(TEXT("稀有玩具盒"));
		}
		if (Id == TEXT("CardToyBox"))
		{
			return FText::FromString(TEXT("纸牌玩具盒"));
		}
		if (Id == TEXT("GuardToyBox"))
		{
			return FText::FromString(TEXT("守卫玩具盒"));
		}
		if (Id == TEXT("Gear"))
		{
			return FText::FromString(TEXT("齿轮"));
		}
		if (Id == TEXT("Spring"))
		{
			return FText::FromString(TEXT("弹簧"));
		}
		if (Id == TEXT("Battery"))
		{
			return FText::FromString(TEXT("电池"));
		}
		if (Id == TEXT("Wheel"))
		{
			return FText::FromString(TEXT("车轮"));
		}
		if (Id == TEXT("Ribbon"))
		{
			return FText::FromString(TEXT("丝带"));
		}
		if (Id == TEXT("Star"))
		{
			return FText::FromString(TEXT("星徽"));
		}
		if (Id == TEXT("Dice"))
		{
			return FText::FromString(TEXT("骰子"));
		}
		if (Id == TEXT("Card"))
		{
			return FText::FromString(TEXT("纸牌"));
		}
		if (Id == TEXT("JokerTower"))
		{
			return FText::FromString(TEXT("小丑塔图纸"));
		}
		if (Id == TEXT("BatteryBlaster"))
		{
			return FText::FromString(TEXT("电池炮台图纸"));
		}
		if (Id == TEXT("PrototypeBattle"))
		{
			return FText::FromString(TEXT("原型战斗"));
		}
		if (Id == TEXT("BasicEnemy"))
		{
			return FText::FromString(TEXT("基础敌人"));
		}

		return FText::FromName(Id);
	}

	FText DisplayNameOrFallback(const FText& DisplayName, const FName Id)
	{
		return DisplayName.IsEmpty() ? KnownId(Id) : DisplayName;
	}

	FText PartName(const UGameInstance* GameInstance, const FName PartId)
	{
		return DisplayNameOrFallback(FindPartDisplayName(GameInstance, PartId), PartId);
	}

	FText DiagramName(const UGameInstance* GameInstance, const FName DiagramId)
	{
		return DisplayNameOrFallback(FindCollectionDisplayName(GameInstance, DiagramId, ETTDCollectionCategory::Diagram), DiagramId);
	}

	FText ToyBoxName(const UGameInstance* GameInstance, const FName ToyBoxId, const FText& ConfigDisplayName)
	{
		const FText ResearchDisplayName = FindCollectionDisplayName(GameInstance, ToyBoxId, ETTDCollectionCategory::ToyBox);
		return DisplayNameOrFallback(ConfigDisplayName.IsEmpty() ? ResearchDisplayName : ConfigDisplayName, ToyBoxId);
	}

	FString JoinPartIds(const UGameInstance* GameInstance, const TArray<FName>& PartIds, const TCHAR* EmptyText)
	{
		TArray<FText> Values;
		for (const FName PartId : PartIds)
		{
			if (!PartId.IsNone())
			{
				Values.Add(PartName(GameInstance, PartId));
			}
		}

		return JoinTexts(Values, EmptyText);
	}

	FString JoinDiagramIds(const UGameInstance* GameInstance, const TArray<FName>& DiagramIds, const TCHAR* EmptyText)
	{
		TArray<FText> Values;
		for (const FName DiagramId : DiagramIds)
		{
			if (!DiagramId.IsNone())
			{
				Values.Add(DiagramName(GameInstance, DiagramId));
			}
		}

		return JoinTexts(Values, EmptyText);
	}

	FString JoinPartStacks(const UGameInstance* GameInstance, const TArray<FTTDNameStack>& Stacks, const TCHAR* EmptyText)
	{
		return JoinStacks(Stacks, [GameInstance](const FName Id) { return PartName(GameInstance, Id); }, EmptyText);
	}

	FString JoinToyBoxStacks(const UGameInstance* GameInstance, const TArray<FTTDNameStack>& Stacks, const TCHAR* EmptyText)
	{
		return JoinStacks(Stacks, [GameInstance](const FName Id) { return ToyBoxName(GameInstance, Id); }, EmptyText);
	}
}
