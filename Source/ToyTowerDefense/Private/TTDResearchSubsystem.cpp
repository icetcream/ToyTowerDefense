#include "TTDResearchSubsystem.h"

#include "Engine/GameInstance.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "TTDDeveloperSettings.h"
#include "TTDGameplayMessages.h"
#include "TTDGameplayTags.h"
#include "TTDSaveGame.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTDResearch, Log, All);

namespace
{
	FText Text(const TCHAR* Value)
	{
		return FText::FromString(Value);
	}

	bool AddUniqueName(TArray<FName>& Names, const FName Name)
	{
		if (!Name.IsNone())
		{
			const int32 PreviousCount = Names.Num();
			Names.AddUnique(Name);
			return Names.Num() != PreviousCount;
		}

		return false;
	}

	void AddStackCount(TArray<FTTDNameStack>& Stacks, const FName Id, const int32 Count)
	{
		if (Id.IsNone() || Count <= 0)
		{
			return;
		}

		if (FTTDNameStack* ExistingStack = Stacks.FindByPredicate(
			[Id](const FTTDNameStack& Stack)
			{
				return Stack.Id == Id;
			}))
		{
			ExistingStack->Count += Count;
			return;
		}

		Stacks.Add(FTTDNameStack(Id, Count));
	}

	void SortStacksById(TArray<FTTDNameStack>& Stacks)
	{
		Stacks.Sort(
			[](const FTTDNameStack& Left, const FTTDNameStack& Right)
			{
				return Left.Id.LexicalLess(Right.Id);
			});
	}

	int32 CountCompletedQueueItems(const TArray<FTTDCraftQueueItem>& QueueItems)
	{
		int32 Count = 0;
		for (const FTTDCraftQueueItem& QueueItem : QueueItems)
		{
			if (QueueItem.bCompleted)
			{
				++Count;
			}
		}
		return Count;
	}

	template<typename RowType>
	bool IsCompatibleDataTable(const UDataTable* DataTable)
	{
		return DataTable && DataTable->GetRowStruct() == RowType::StaticStruct();
	}

	void LoadPartsFromDataTable(const UDataTable* DataTable, TArray<FTTDPartDefinition>& OutParts)
	{
		if (!IsCompatibleDataTable<FTTDPartDefinition>(DataTable))
		{
			return;
		}

		for (const TPair<FName, uint8*>& RowPair : DataTable->GetRowMap())
		{
			const FTTDPartDefinition* Row = reinterpret_cast<const FTTDPartDefinition*>(RowPair.Value);
			if (!Row)
			{
				continue;
			}

			FTTDPartDefinition Part = *Row;
			if (Part.PartId.IsNone())
			{
				Part.PartId = RowPair.Key;
			}
			OutParts.Add(Part);
		}
	}

	void LoadDiagramsFromDataTable(const UDataTable* DataTable, TArray<FTTDDiagramDefinition>& OutDiagrams)
	{
		if (!IsCompatibleDataTable<FTTDDiagramDefinition>(DataTable))
		{
			return;
		}

		for (const TPair<FName, uint8*>& RowPair : DataTable->GetRowMap())
		{
			const FTTDDiagramDefinition* Row = reinterpret_cast<const FTTDDiagramDefinition*>(RowPair.Value);
			if (!Row)
			{
				continue;
			}

			FTTDDiagramDefinition Diagram = *Row;
			if (Diagram.DiagramId.IsNone())
			{
				Diagram.DiagramId = RowPair.Key;
			}
			OutDiagrams.Add(Diagram);
		}
	}

	void LoadToyBoxesFromDataTable(const UDataTable* DataTable, TArray<FTTDToyBoxDefinition>& OutToyBoxes)
	{
		if (!IsCompatibleDataTable<FTTDToyBoxDefinition>(DataTable))
		{
			return;
		}

		for (const TPair<FName, uint8*>& RowPair : DataTable->GetRowMap())
		{
			const FTTDToyBoxDefinition* Row = reinterpret_cast<const FTTDToyBoxDefinition*>(RowPair.Value);
			if (!Row)
			{
				continue;
			}

			FTTDToyBoxDefinition ToyBox = *Row;
			if (ToyBox.ToyBoxId.IsNone())
			{
				ToyBox.ToyBoxId = RowPair.Key;
			}
			OutToyBoxes.Add(ToyBox);
		}
	}

	void AddDiagramIfMissing(
		TArray<FTTDDiagramDefinition>& OutDiagrams,
		const FName DiagramId,
		const FText& DisplayName,
		const FText& Description,
		TArray<FName> RequiredPartIds,
		TArray<FTTDNameStack> PartCosts)
	{
		if (OutDiagrams.ContainsByPredicate(
			[DiagramId](const FTTDDiagramDefinition& Diagram)
			{
				return Diagram.DiagramId == DiagramId;
			}))
		{
			return;
		}

		FTTDDiagramDefinition Diagram(DiagramId, DisplayName, Description, MoveTemp(RequiredPartIds));
		Diagram.PartCosts = MoveTemp(PartCosts);
		OutDiagrams.Add(Diagram);
	}

	void EnsureBattleDiagramDefinitions(TArray<FTTDDiagramDefinition>& OutDiagrams)
	{
		AddDiagramIfMissing(
			OutDiagrams,
			TEXT("BasicTower"),
			Text(TEXT("基础塔图纸")),
			Text(TEXT("基础攻击塔图纸，需要齿轮与弹簧搭建。")),
			{ TEXT("Gear"), TEXT("Spring") },
			{ FTTDNameStack(TEXT("Gear"), 1), FTTDNameStack(TEXT("Spring"), 1) });
		AddDiagramIfMissing(
			OutDiagrams,
			TEXT("ProjectileTower"),
			Text(TEXT("弹射塔图纸")),
			Text(TEXT("投射物塔图纸，需要齿轮与弹簧搭建。")),
			{ TEXT("Gear"), TEXT("Spring") },
			{ FTTDNameStack(TEXT("Gear"), 1), FTTDNameStack(TEXT("Spring"), 1) });
	}
}

void UTTDResearchSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SeedDefinitions();
	const UTTDDeveloperSettings* Settings = GetDefault<UTTDDeveloperSettings>();
	CraftingQueue = FTTDCraftingQueueModel(Settings ? Settings->CraftingQueueMaxSlots : 5);
	LoadOrCreateProgress();
	ApplyOfflineCraftingProgress();
}

void UTTDResearchSubsystem::Deinitialize()
{
	SaveProgress();
	Super::Deinitialize();
}

void UTTDResearchSubsystem::Tick(const float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	const bool bHadActiveItems = CraftingQueue.HasActiveItems();
	const int32 CompletedItemsBeforeTick = CountCompletedQueueItems(CraftingQueue.GetQueue());
	CraftingQueue.AdvanceTime(DeltaTime);
	const int32 CompletedItemsAfterTick = CountCompletedQueueItems(CraftingQueue.GetQueue());

	if (bHadActiveItems)
	{
		bProgressDirty = true;
		AutosaveAccumulatorSeconds += DeltaTime;

		if (CompletedItemsAfterTick != CompletedItemsBeforeTick)
		{
			BroadcastQueueChanged();
		}

		if (AutosaveAccumulatorSeconds >= 5.0f)
		{
			SaveProgress();
		}
	}
}

TStatId UTTDResearchSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTTDResearchSubsystem, STATGROUP_Tickables);
}

bool UTTDResearchSubsystem::IsTickable() const
{
	return !IsTemplate() && !HasAnyFlags(RF_ClassDefaultObject);
}

UWorld* UTTDResearchSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

void UTTDResearchSubsystem::ResetProgress()
{
	const FString SaveSlotName = GetSaveSlotName();
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
	{
		UGameplayStatics::DeleteGameInSlot(SaveSlotName, UserIndex);
	}

	LoadOrCreateProgress();
	bProgressDirty = true;
	SaveProgress();
	BroadcastQueueChanged();
	BroadcastCollectionChanged();
	BroadcastInventoryChanged();
}

TArray<FTTDCollectionEntry> UTTDResearchSubsystem::GetCollectionEntries(const ETTDCollectionCategory Category) const
{
	TArray<FTTDCollectionEntry> Entries;

	if (!ActiveSave)
	{
		return Entries;
	}

	if (Category == ETTDCollectionCategory::Diagram)
	{
		for (const FTTDDiagramDefinition& Diagram : Diagrams)
		{
			FTTDCollectionEntry Entry;
			Entry.Category = Category;
			Entry.ItemId = Diagram.DiagramId;
			Entry.DisplayName = Diagram.DisplayName;
			Entry.Description = Diagram.Description;
			for (const FTTDNameStack& PartCost : GetDiagramPartCosts(Diagram))
			{
				Entry.PartIds.Add(PartCost.Id);
			}
			Entry.bUnlocked = IsUnlocked(ActiveSave->UnlockedDiagramIds, Diagram.DiagramId);
			Entries.Add(Entry);
		}
	}
	else
	{
		for (const FTTDToyBoxDefinition& ToyBox : ToyBoxes)
		{
			FTTDCollectionEntry Entry;
			Entry.Category = Category;
			Entry.ItemId = ToyBox.ToyBoxId;
			Entry.DisplayName = ToyBox.DisplayName;
			Entry.Description = ToyBox.Description;
			Entry.PartIds = ToyBox.PossiblePartIds;
			Entry.CraftDurationSeconds = ToyBox.CraftDurationSeconds;
			Entry.bUnlocked = IsUnlocked(ActiveSave->UnlockedToyBoxIds, ToyBox.ToyBoxId);
			Entries.Add(Entry);
		}
	}

	return Entries;
}

TArray<FTTDToyBoxDefinition> UTTDResearchSubsystem::GetToyBoxes() const
{
	return ToyBoxes;
}

TArray<FTTDDiagramDefinition> UTTDResearchSubsystem::GetDiagrams() const
{
	return Diagrams;
}

TArray<FTTDPartDefinition> UTTDResearchSubsystem::GetParts() const
{
	return Parts;
}

TArray<FTTDPartDefinition> UTTDResearchSubsystem::GetPartsForToyBox(const FName ToyBoxId) const
{
	TArray<FTTDPartDefinition> Result;
	const FTTDToyBoxDefinition* ToyBox = FindToyBox(ToyBoxId);

	if (!ToyBox)
	{
		return Result;
	}

	for (const FName PartId : ToyBox->PossiblePartIds)
	{
		if (const FTTDPartDefinition* Part = FindPart(PartId))
		{
			Result.Add(*Part);
		}
	}

	return Result;
}

TArray<FTTDNameStack> UTTDResearchSubsystem::GetPartInventory() const
{
	TArray<FTTDNameStack> Result;
	if (!ActiveSave)
	{
		return Result;
	}

	for (const FTTDNameStack& Stack : ActiveSave->PartInventory)
	{
		if (!Stack.Id.IsNone() && Stack.Count > 0)
		{
			Result.Add(Stack);
		}
	}

	SortStacksById(Result);
	return Result;
}

TArray<FTTDNameStack> UTTDResearchSubsystem::GetToyBoxInventory() const
{
	TArray<FTTDNameStack> Result;
	if (!ActiveSave)
	{
		return Result;
	}

	for (const FTTDNameStack& Stack : ActiveSave->ToyBoxInventory)
	{
		if (!Stack.Id.IsNone() && Stack.Count > 0)
		{
			Result.Add(Stack);
		}
	}

	SortStacksById(Result);
	return Result;
}

int32 UTTDResearchSubsystem::GetPartCount(const FName PartId) const
{
	if (!ActiveSave || PartId.IsNone())
	{
		return 0;
	}

	for (const FTTDNameStack& Stack : ActiveSave->PartInventory)
	{
		if (Stack.Id == PartId)
		{
			return FMath::Max(0, Stack.Count);
		}
	}

	return 0;
}

int32 UTTDResearchSubsystem::GetToyBoxCount(const FName ToyBoxId) const
{
	if (!ActiveSave || ToyBoxId.IsNone())
	{
		return 0;
	}

	for (const FTTDNameStack& Stack : ActiveSave->ToyBoxInventory)
	{
		if (Stack.Id == ToyBoxId)
		{
			return FMath::Max(0, Stack.Count);
		}
	}

	return 0;
}

TArray<FTTDCraftQueueItem> UTTDResearchSubsystem::GetCraftingQueue() const
{
	return CraftingQueue.GetQueue();
}

int32 UTTDResearchSubsystem::GetCraftingQueueMaxSlots() const
{
	return CraftingQueue.GetMaxSlots();
}

bool UTTDResearchSubsystem::EnqueueToyBox(const FName ToyBoxId, FText& OutFailureReason)
{
	if (!ActiveSave)
	{
		OutFailureReason = Text(TEXT("研究所进度尚未初始化"));
		return false;
	}

	const FTTDToyBoxDefinition* ToyBox = FindToyBox(ToyBoxId);
	if (!ToyBox)
	{
		OutFailureReason = Text(TEXT("未找到该玩具盒"));
		return false;
	}

	if (!IsUnlocked(ActiveSave->UnlockedToyBoxIds, ToyBoxId))
	{
		OutFailureReason = Text(TEXT("该玩具盒尚未解锁"));
		return false;
	}

	FTTDCraftQueueItem QueueItem;
	if (!CraftingQueue.EnqueueToyBox(*ToyBox, QueueItem))
	{
		OutFailureReason = Text(TEXT("制作队列已满"));
		return false;
	}

	bProgressDirty = true;
	SaveProgress();
	BroadcastToyBoxQueued(QueueItem);
	BroadcastQueueChanged();
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UTTDResearchSubsystem::ClaimCompletedToyBox(const FGuid QueueId, TArray<FTTDPartDefinition>& OutUnlockedParts)
{
	OutUnlockedParts.Reset();

	if (!ActiveSave)
	{
		return false;
	}

	FTTDCraftQueueItem ClaimedItem;
	if (!CraftingQueue.ClaimCompletedItem(QueueId, ClaimedItem))
	{
		return false;
	}

	const FTTDToyBoxDefinition* ToyBox = FindToyBox(ClaimedItem.ToyBoxId);
	TArray<FTTDNameStack> GrantedParts;
	TArray<FTTDNameStack> GrantedToyBoxes;
	if (ToyBox)
	{
		AddToyBoxCount(ToyBox->ToyBoxId, 1);
		AddStackCount(GrantedToyBoxes, ToyBox->ToyBoxId, 1);
	}

	bProgressDirty = true;
	SaveProgress();
	SortStacksById(GrantedParts);
	SortStacksById(GrantedToyBoxes);
	BroadcastToyBoxClaimed(ClaimedItem, OutUnlockedParts, GrantedParts, GrantedToyBoxes);
	BroadcastQueueChanged();
	BroadcastInventoryChanged();
	if (OutUnlockedParts.Num() > 0)
	{
		BroadcastCollectionChanged();
	}
	return true;
}

bool UTTDResearchSubsystem::CanResearchDiagram(const FName DiagramId, FText& OutFailureReason) const
{
	OutFailureReason = FText::GetEmpty();

	if (!ActiveSave)
	{
		OutFailureReason = Text(TEXT("研究所进度尚未初始化"));
		return false;
	}

	const FTTDDiagramDefinition* Diagram = FindDiagram(DiagramId);
	if (!Diagram)
	{
		OutFailureReason = Text(TEXT("未找到该图纸"));
		return false;
	}

	if (IsUnlocked(ActiveSave->UnlockedDiagramIds, DiagramId))
	{
		OutFailureReason = Text(TEXT("该图纸已解锁"));
		return false;
	}

	return HasPartCosts(GetDiagramPartCosts(*Diagram), OutFailureReason);
}

bool UTTDResearchSubsystem::ResearchDiagram(const FName DiagramId, FText& OutFailureReason)
{
	if (!CanResearchDiagram(DiagramId, OutFailureReason))
	{
		return false;
	}

	const FTTDDiagramDefinition* Diagram = FindDiagram(DiagramId);
	if (!ActiveSave || !Diagram)
	{
		OutFailureReason = Text(TEXT("研究所进度尚未初始化"));
		return false;
	}

	ApplyPartCosts(GetDiagramPartCosts(*Diagram));
	AddUniqueName(ActiveSave->UnlockedDiagramIds, DiagramId);
	bProgressDirty = true;
	SaveProgress();
	BroadcastCollectionChanged();
	BroadcastInventoryChanged();
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UTTDResearchSubsystem::CanConsumeToyBoxes(const TArray<FTTDNameStack>& ToyBoxesToConsume, FText& OutFailureReason) const
{
	if (!ActiveSave)
	{
		OutFailureReason = Text(TEXT("研究所进度尚未初始化"));
		return false;
	}

	TArray<FTTDNameStack> NormalizedCosts;
	for (const FTTDNameStack& ToyBoxCost : ToyBoxesToConsume)
	{
		AddStackCount(NormalizedCosts, ToyBoxCost.Id, ToyBoxCost.Count);
	}

	for (const FTTDNameStack& ToyBoxCost : NormalizedCosts)
	{
		if (ToyBoxCost.Id.IsNone() || ToyBoxCost.Count <= 0)
		{
			continue;
		}

		const int32 AvailableCount = GetToyBoxCount(ToyBoxCost.Id);
		if (AvailableCount < ToyBoxCost.Count)
		{
			OutFailureReason = FText::Format(
				FText::FromString(TEXT("玩具盒库存不足：{0} 需要 {1}，当前 {2}")),
				FText::FromName(ToyBoxCost.Id),
				FText::AsNumber(ToyBoxCost.Count),
				FText::AsNumber(AvailableCount));
			return false;
		}
	}

	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UTTDResearchSubsystem::ConsumeToyBoxes(const TArray<FTTDNameStack>& ToyBoxesToConsume, FText& OutFailureReason)
{
	if (!CanConsumeToyBoxes(ToyBoxesToConsume, OutFailureReason))
	{
		return false;
	}

	ApplyToyBoxCosts(ToyBoxesToConsume);
	bProgressDirty = true;
	SaveProgress();
	BroadcastInventoryChanged();
	OutFailureReason = FText::GetEmpty();
	return true;
}

void UTTDResearchSubsystem::BroadcastCurrentResearchState() const
{
	BroadcastQueueChanged();
	BroadcastCollectionChanged();
	BroadcastInventoryChanged();
}

void UTTDResearchSubsystem::SeedDefinitions()
{
	Parts.Reset();
	Diagrams.Reset();
	ToyBoxes.Reset();

	if (const UTTDDeveloperSettings* Settings = GetDefault<UTTDDeveloperSettings>())
	{
		LoadDefinitionsFromSettings(*Settings);
	}

	if (Parts.IsEmpty())
	{
		UE_LOG(LogTTDResearch, Warning, TEXT("Research parts DataTable is missing or empty; using C++ fallback sample parts."));
	Parts = {
		{ TEXT("Gear"), Text(TEXT("齿轮")), Text(TEXT("基础机械零件，可驱动玩具机关。")) },
		{ TEXT("Spring"), Text(TEXT("弹簧")), Text(TEXT("为塔防玩具提供弹射和回弹能力。")) },
		{ TEXT("Battery"), Text(TEXT("电池")), Text(TEXT("提供持续能量的核心零件。")) },
		{ TEXT("Wheel"), Text(TEXT("车轮")), Text(TEXT("适用于移动或旋转类玩具。")) },
		{ TEXT("Ribbon"), Text(TEXT("丝带")), Text(TEXT("装饰类零件，可用于提升玩具表现。")) },
		{ TEXT("Star"), Text(TEXT("星徽")), Text(TEXT("稀有装饰零件，常用于高级图纸。")) },
		{ TEXT("Dice"), Text(TEXT("骰子")), Text(TEXT("带有随机主题的趣味零件。")) },
		{ TEXT("Card"), Text(TEXT("纸牌")), Text(TEXT("用于扑克主题玩具的基础材料。")) }
	};
	}

	if (Diagrams.IsEmpty())
	{
		UE_LOG(LogTTDResearch, Warning, TEXT("Research diagrams DataTable is missing or empty; using C++ fallback sample diagrams."));
	Diagrams = {
		{ TEXT("BasicTower"), Text(TEXT("基础塔图纸")), Text(TEXT("基础攻击塔图纸，需要齿轮与弹簧搭建。")), { TEXT("Gear"), TEXT("Spring") } },
		{ TEXT("ProjectileTower"), Text(TEXT("弹射塔图纸")), Text(TEXT("投射物塔图纸，需要齿轮与弹簧搭建。")), { TEXT("Gear"), TEXT("Spring") } },
		{ TEXT("JokerTower"), Text(TEXT("小丑塔图纸")), Text(TEXT("基础攻击塔图纸，需要齿轮与弹簧搭建。")), { TEXT("Gear"), TEXT("Spring") } },
		{ TEXT("BatteryBlaster"), Text(TEXT("电池炮台图纸")), Text(TEXT("能量型炮台图纸，需要电池和齿轮。")), { TEXT("Battery"), TEXT("Gear") } },
		{ TEXT("RollingGuard"), Text(TEXT("滚轮守卫图纸")), Text(TEXT("控制路径的玩具守卫，需要车轮和弹簧。")), { TEXT("Wheel"), TEXT("Spring") } },
		{ TEXT("RibbonTrap"), Text(TEXT("丝带陷阱图纸")), Text(TEXT("减速类陷阱图纸，需要丝带和齿轮。")), { TEXT("Ribbon"), TEXT("Gear") } },
		{ TEXT("StarBeacon"), Text(TEXT("星徽信标图纸")), Text(TEXT("高级辅助塔图纸，需要星徽、电池和纸牌。")), { TEXT("Star"), TEXT("Battery"), TEXT("Card") } },
		{ TEXT("DiceLauncher"), Text(TEXT("骰子发射器图纸")), Text(TEXT("随机弹道塔图纸，需要骰子和弹簧。")), { TEXT("Dice"), TEXT("Spring") } },
		{ TEXT("CardWall"), Text(TEXT("纸牌墙图纸")), Text(TEXT("防御障碍图纸，需要纸牌和丝带。")), { TEXT("Card"), TEXT("Ribbon") } },
		{ TEXT("ClockworkToy"), Text(TEXT("发条玩具图纸")), Text(TEXT("复合型玩具图纸，需要齿轮、弹簧和车轮。")), { TEXT("Gear"), TEXT("Spring"), TEXT("Wheel") } },
		{ TEXT("PrismJoker"), Text(TEXT("棱彩小丑图纸")), Text(TEXT("稀有攻击塔图纸，需要星徽和骰子。")), { TEXT("Star"), TEXT("Dice") } },
		{ TEXT("CarnivalCore"), Text(TEXT("狂欢核心图纸")), Text(TEXT("终阶核心图纸，需要多种高级零件组合。")), { TEXT("Star"), TEXT("Battery"), TEXT("Dice"), TEXT("Card") } }
	};
	}

	if (ToyBoxes.IsEmpty())
	{
		UE_LOG(LogTTDResearch, Warning, TEXT("Research toy boxes DataTable is missing or empty; using C++ fallback sample toy boxes."));
	ToyBoxes = {
		{ TEXT("BasicToyBox"), Text(TEXT("基础玩具盒")), Text(TEXT("常见零件玩具盒，适合早期制作。")), { TEXT("Gear"), TEXT("Spring"), TEXT("Card") }, 20.0f },
		{ TEXT("MechanicToyBox"), Text(TEXT("机械玩具盒")), Text(TEXT("偏向机械结构零件的玩具盒。")), { TEXT("Gear"), TEXT("Wheel"), TEXT("Spring") }, 35.0f },
		{ TEXT("EnergyToyBox"), Text(TEXT("能量玩具盒")), Text(TEXT("更容易开出电池类零件。")), { TEXT("Battery"), TEXT("Gear"), TEXT("Star") }, 45.0f },
		{ TEXT("PartyToyBox"), Text(TEXT("派对玩具盒")), Text(TEXT("包含装饰和随机主题零件。")), { TEXT("Ribbon"), TEXT("Dice"), TEXT("Card") }, 50.0f },
		{ TEXT("RareToyBox"), Text(TEXT("稀有玩具盒")), Text(TEXT("有机会获得高级图纸所需零件。")), { TEXT("Star"), TEXT("Battery"), TEXT("Dice") }, 70.0f },
		{ TEXT("CardToyBox"), Text(TEXT("纸牌玩具盒")), Text(TEXT("纸牌主题零件集合。")), { TEXT("Card"), TEXT("Dice"), TEXT("Ribbon") }, 40.0f },
		{ TEXT("GuardToyBox"), Text(TEXT("守卫玩具盒")), Text(TEXT("防御类玩具常用零件集合。")), { TEXT("Wheel"), TEXT("Gear"), TEXT("Battery") }, 55.0f }
	};
	}

	EnsureBattleDiagramDefinitions(Diagrams);
}

void UTTDResearchSubsystem::LoadDefinitionsFromSettings(const UTTDDeveloperSettings& Settings)
{
	LoadPartsFromDataTable(Settings.GetDataTableForTag(TAG_TTD_DataTable_Research_Parts), Parts);
	LoadDiagramsFromDataTable(Settings.GetDataTableForTag(TAG_TTD_DataTable_Research_Diagrams), Diagrams);
	LoadToyBoxesFromDataTable(Settings.GetDataTableForTag(TAG_TTD_DataTable_Research_ToyBoxes), ToyBoxes);
}

void UTTDResearchSubsystem::LoadOrCreateProgress()
{
	ActiveSave = nullptr;
	const UTTDDeveloperSettings* Settings = GetDefault<UTTDDeveloperSettings>();

	const FString SaveSlotName = GetSaveSlotName();
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
	{
		ActiveSave = Cast<UTTDSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));
		if (ActiveSave && ActiveSave->SaveVersion < 2)
		{
			UE_LOG(LogTTDResearch, Warning, TEXT("Resetting research progress because save version %d is older than %d."), ActiveSave->SaveVersion, UTTDSaveGame::CurrentSaveVersion);
			UGameplayStatics::DeleteGameInSlot(SaveSlotName, UserIndex);
			ActiveSave = nullptr;
		}
	}

	if (!ActiveSave)
	{
		ActiveSave = Cast<UTTDSaveGame>(UGameplayStatics::CreateSaveGameObject(UTTDSaveGame::StaticClass()));
		ActiveSave->SaveVersion = UTTDSaveGame::CurrentSaveVersion;
		ActiveSave->LastSavedUnixSeconds = FDateTime::UtcNow().ToUnixTimestamp();
	}
	else
	{
		bProgressDirty |= ActiveSave->SaveVersion < UTTDSaveGame::CurrentSaveVersion;
		ActiveSave->SaveVersion = UTTDSaveGame::CurrentSaveVersion;
		NormalizePartInventory();
		NormalizeToyBoxInventory();
	}

	const int32 RemovedQueueItems = CraftingQueue.SetQueue(ActiveSave->CraftingQueue);
	AutosaveAccumulatorSeconds = 0.0f;
	bProgressDirty |= RemovedQueueItems > 0;
	if (RemovedQueueItems > 0)
	{
		UE_LOG(LogTTDResearch, Warning, TEXT("Removed %d invalid or overflow crafting queue item(s) while loading research progress."), RemovedQueueItems);
	}

	if (Settings)
	{
		ApplyDefaultUnlocks(*Settings);
	}
}

void UTTDResearchSubsystem::ApplyDefaultUnlocks(const UTTDDeveloperSettings& Settings)
{
	if (!ActiveSave)
	{
		return;
	}

	for (const FName DiagramId : Settings.DefaultUnlockedDiagramIds)
	{
		bProgressDirty |= AddUniqueName(ActiveSave->UnlockedDiagramIds, DiagramId);
	}

	for (const FName ToyBoxId : Settings.DefaultUnlockedToyBoxIds)
	{
		bProgressDirty |= AddUniqueName(ActiveSave->UnlockedToyBoxIds, ToyBoxId);
	}

	for (const FName PartId : Settings.DefaultUnlockedPartIds)
	{
		bProgressDirty |= AddUniqueName(ActiveSave->UnlockedPartIds, PartId);
	}
}

void UTTDResearchSubsystem::ApplyOfflineCraftingProgress()
{
	if (!ActiveSave)
	{
		return;
	}

	const int64 Now = FDateTime::UtcNow().ToUnixTimestamp();
	const int64 ElapsedSeconds = ActiveSave->LastSavedUnixSeconds > 0
		? FMath::Max<int64>(0, Now - ActiveSave->LastSavedUnixSeconds)
		: 0;

	if (ElapsedSeconds > 0)
	{
		CraftingQueue.AdvanceTime(static_cast<float>(ElapsedSeconds));
		bProgressDirty = true;
	}

	ActiveSave->LastSavedUnixSeconds = Now;
	SaveProgress();

	if (ElapsedSeconds > 0)
	{
		BroadcastQueueChanged();
	}
}

void UTTDResearchSubsystem::SaveProgress()
{
	if (!ActiveSave)
	{
		return;
	}

	NormalizePartInventory();
	NormalizeToyBoxInventory();
	ActiveSave->SaveVersion = UTTDSaveGame::CurrentSaveVersion;
	ActiveSave->CraftingQueue = CraftingQueue.GetQueue();
	ActiveSave->LastSavedUnixSeconds = FDateTime::UtcNow().ToUnixTimestamp();
	UGameplayStatics::SaveGameToSlot(ActiveSave, GetSaveSlotName(), UserIndex);

	AutosaveAccumulatorSeconds = 0.0f;
	bProgressDirty = false;
}

void UTTDResearchSubsystem::BroadcastQueueChanged() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
		{
			FTTDResearchQueueChangedMessage Message;
			Message.QueueItems = CraftingQueue.GetQueue();
			Message.MaxSlots = CraftingQueue.GetMaxSlots();
			MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Research_Queue_Changed, Message);
		}
	}
}

void UTTDResearchSubsystem::BroadcastToyBoxQueued(const FTTDCraftQueueItem& QueueItem) const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
		{
			FTTDToyBoxQueuedMessage Message;
			Message.QueueItem = QueueItem;
			MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Research_Queue_Enqueued, Message);
		}
	}
}

void UTTDResearchSubsystem::BroadcastToyBoxClaimed(const FTTDCraftQueueItem& QueueItem, const TArray<FTTDPartDefinition>& NewlyUnlockedParts, const TArray<FTTDNameStack>& GrantedParts, const TArray<FTTDNameStack>& GrantedToyBoxes) const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
		{
			FTTDToyBoxClaimedMessage Message;
			Message.QueueItem = QueueItem;
			Message.NewlyUnlockedParts = NewlyUnlockedParts;
			Message.GrantedParts = GrantedParts;
			Message.GrantedToyBoxes = GrantedToyBoxes;
			Message.PartInventory = GetPartInventory();
			Message.ToyBoxInventory = GetToyBoxInventory();
			MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Research_Queue_Claimed, Message);
		}
	}
}

void UTTDResearchSubsystem::BroadcastCollectionChanged() const
{
	if (!ActiveSave)
	{
		return;
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
		{
			FTTDResearchCollectionChangedMessage Message;
			Message.UnlockedDiagramIds = ActiveSave->UnlockedDiagramIds;
			Message.UnlockedToyBoxIds = ActiveSave->UnlockedToyBoxIds;
			Message.UnlockedPartIds = ActiveSave->UnlockedPartIds;
			MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Research_Collection_Changed, Message);
		}
	}
}

void UTTDResearchSubsystem::BroadcastInventoryChanged() const
{
	if (!ActiveSave)
	{
		return;
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
		{
			FTTDResearchInventoryChangedMessage Message;
			Message.PartInventory = GetPartInventory();
			Message.ToyBoxInventory = GetToyBoxInventory();
			MessageSubsystem->BroadcastMessage(TAG_TTD_Message_Research_Inventory_Changed, Message);
		}
	}
}

const FTTDToyBoxDefinition* UTTDResearchSubsystem::FindToyBox(const FName ToyBoxId) const
{
	return ToyBoxes.FindByPredicate(
		[ToyBoxId](const FTTDToyBoxDefinition& ToyBox)
		{
			return ToyBox.ToyBoxId == ToyBoxId;
		});
}

const FTTDDiagramDefinition* UTTDResearchSubsystem::FindDiagram(const FName DiagramId) const
{
	return Diagrams.FindByPredicate(
		[DiagramId](const FTTDDiagramDefinition& Diagram)
		{
			return Diagram.DiagramId == DiagramId;
		});
}

const FTTDPartDefinition* UTTDResearchSubsystem::FindPart(const FName PartId) const
{
	return Parts.FindByPredicate(
		[PartId](const FTTDPartDefinition& Part)
		{
			return Part.PartId == PartId;
		});
}

FString UTTDResearchSubsystem::GetSaveSlotName() const
{
	const UTTDDeveloperSettings* Settings = GetDefault<UTTDDeveloperSettings>();
	return Settings && !Settings->SaveSlotName.IsEmpty()
		? Settings->SaveSlotName
		: FString(TEXT("ToyTowerDefenseResearch"));
}

bool UTTDResearchSubsystem::IsUnlocked(const TArray<FName>& UnlockedIds, const FName Id) const
{
	return UnlockedIds.Contains(Id);
}

TArray<FTTDNameStack> UTTDResearchSubsystem::GetDiagramPartCosts(const FTTDDiagramDefinition& Diagram) const
{
	TArray<FTTDNameStack> Result;
	for (const FTTDNameStack& PartCost : Diagram.PartCosts)
	{
		if (!PartCost.Id.IsNone() && PartCost.Count > 0)
		{
			AddStackCount(Result, PartCost.Id, PartCost.Count);
		}
	}

	if (Result.IsEmpty())
	{
		for (const FName PartId : Diagram.RequiredPartIds)
		{
			AddStackCount(Result, PartId, 1);
		}
	}

	SortStacksById(Result);
	return Result;
}

bool UTTDResearchSubsystem::HasPartCosts(const TArray<FTTDNameStack>& PartCosts, FText& OutFailureReason) const
{
	for (const FTTDNameStack& PartCost : PartCosts)
	{
		if (PartCost.Id.IsNone() || PartCost.Count <= 0)
		{
			continue;
		}

		const int32 AvailableCount = GetPartCount(PartCost.Id);
		if (AvailableCount < PartCost.Count)
		{
			OutFailureReason = FText::Format(
				FText::FromString(TEXT("零件不足：{0} 需要 {1}，当前 {2}")),
				FText::FromName(PartCost.Id),
				FText::AsNumber(PartCost.Count),
				FText::AsNumber(AvailableCount));
			return false;
		}
	}

	OutFailureReason = FText::GetEmpty();
	return true;
}

void UTTDResearchSubsystem::ApplyPartCosts(const TArray<FTTDNameStack>& PartCosts)
{
	if (!ActiveSave)
	{
		return;
	}

	for (const FTTDNameStack& PartCost : PartCosts)
	{
		if (PartCost.Id.IsNone() || PartCost.Count <= 0)
		{
			continue;
		}

		for (FTTDNameStack& Stack : ActiveSave->PartInventory)
		{
			if (Stack.Id == PartCost.Id)
			{
				Stack.Count = FMath::Max(0, Stack.Count - PartCost.Count);
				break;
			}
		}
	}

	NormalizePartInventory();
}

void UTTDResearchSubsystem::AddPartCount(const FName PartId, const int32 Count)
{
	if (!ActiveSave)
	{
		return;
	}

	AddStackCount(ActiveSave->PartInventory, PartId, Count);
	NormalizePartInventory();
}

void UTTDResearchSubsystem::AddToyBoxCount(const FName ToyBoxId, const int32 Count)
{
	if (!ActiveSave)
	{
		return;
	}

	AddStackCount(ActiveSave->ToyBoxInventory, ToyBoxId, Count);
	NormalizeToyBoxInventory();
}

void UTTDResearchSubsystem::ApplyToyBoxCosts(const TArray<FTTDNameStack>& ToyBoxCosts)
{
	if (!ActiveSave)
	{
		return;
	}

	TArray<FTTDNameStack> NormalizedCosts;
	for (const FTTDNameStack& ToyBoxCost : ToyBoxCosts)
	{
		AddStackCount(NormalizedCosts, ToyBoxCost.Id, ToyBoxCost.Count);
	}

	for (const FTTDNameStack& ToyBoxCost : NormalizedCosts)
	{
		if (ToyBoxCost.Id.IsNone() || ToyBoxCost.Count <= 0)
		{
			continue;
		}

		for (FTTDNameStack& Stack : ActiveSave->ToyBoxInventory)
		{
			if (Stack.Id == ToyBoxCost.Id)
			{
				Stack.Count = FMath::Max(0, Stack.Count - ToyBoxCost.Count);
				break;
			}
		}
	}

	NormalizeToyBoxInventory();
}

void UTTDResearchSubsystem::NormalizePartInventory()
{
	if (!ActiveSave)
	{
		return;
	}

	TArray<FTTDNameStack> NormalizedInventory;
	for (const FTTDNameStack& Stack : ActiveSave->PartInventory)
	{
		if (!Stack.Id.IsNone() && Stack.Count > 0)
		{
			AddStackCount(NormalizedInventory, Stack.Id, Stack.Count);
		}
	}

	SortStacksById(NormalizedInventory);
	ActiveSave->PartInventory = MoveTemp(NormalizedInventory);
}

void UTTDResearchSubsystem::NormalizeToyBoxInventory()
{
	if (!ActiveSave)
	{
		return;
	}

	TArray<FTTDNameStack> NormalizedInventory;
	for (const FTTDNameStack& Stack : ActiveSave->ToyBoxInventory)
	{
		if (!Stack.Id.IsNone() && Stack.Count > 0)
		{
			AddStackCount(NormalizedInventory, Stack.Id, Stack.Count);
		}
	}

	SortStacksById(NormalizedInventory);
	ActiveSave->ToyBoxInventory = MoveTemp(NormalizedInventory);
}
