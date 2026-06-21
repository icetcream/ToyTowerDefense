#include "UI/TTDWarehouseWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "TTDMenuPlayerController.h"
#include "TTDResearchSubsystem.h"
#include "UI/TTDActionButtonWidget.h"
#include "UI/TTDUIDisplayNames.h"
#include "UI/TTDUIStyle.h"

namespace
{
	UBorder* MakePanel(UWidgetTree* WidgetTree, const FLinearColor& Color, const FMargin& Padding)
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		TTDUIStyle::ApplyPanel(Panel, Color, Padding);
		return Panel;
	}

	UTextBlock* MakeText(UWidgetTree* WidgetTree, const FText& TextValue, const int32 FontSize, const FLinearColor& Color = TTDUIStyle::Ink(), const ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(TextValue);
		TTDUIStyle::ApplyText(Text, FontSize, Color, Justification, true);
		return Text;
	}

	UTTDActionButtonWidget* MakeButton(UWidgetTree* WidgetTree, const FText& Label, const bool bEnabled, const FSimpleDelegate& Delegate, const ETTDActionButtonVariant Variant = ETTDActionButtonVariant::Secondary)
	{
		UTTDActionButtonWidget* Button = WidgetTree->ConstructWidget<UTTDActionButtonWidget>(UTTDActionButtonWidget::StaticClass());
		Button->Configure(Label, bEnabled, Delegate, Variant);
		return Button;
	}
}

void UTTDWarehouseWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UBorder* Root = MakePanel(WidgetTree, TTDUIStyle::Backdrop(), FMargin(32.0f, 28.0f));
	WidgetTree->RootWidget = Root;

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChild(RootBox);

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RootBox->AddChildToVerticalBox(HeaderRow);

	FSimpleDelegate BackDelegate;
	BackDelegate.BindUObject(this, &UTTDWarehouseWidget::HandleBackClicked);
	HeaderRow->AddChildToHorizontalBox(MakeButton(WidgetTree, FText::FromString(TEXT("返回")), true, BackDelegate, ETTDActionButtonVariant::Danger));

	UTextBlock* Title = MakeText(WidgetTree, FText::FromString(TEXT("仓库")), 34, TTDUIStyle::Ink(), ETextJustify::Center);
	UHorizontalBoxSlot* TitleSlot = HeaderRow->AddChildToHorizontalBox(Title);
	TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	TitleSlot->SetPadding(FMargin(18.0f, 0.0f, 0.0f, 0.0f));

	UBorder* ContentPanel = MakePanel(WidgetTree, TTDUIStyle::Paper(), FMargin(18.0f));
	UVerticalBoxSlot* ContentPanelSlot = RootBox->AddChildToVerticalBox(ContentPanel);
	ContentPanelSlot->SetPadding(FMargin(0.0f, 22.0f, 0.0f, 0.0f));
	ContentPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	ContentPanel->AddChild(ScrollBox);

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ScrollBox->AddChild(ContentBox);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameplayMessageSubsystem* MessageSubsystem = GameInstance->GetSubsystem<UGameplayMessageSubsystem>())
		{
			InventoryChangedListenerHandle = MessageSubsystem->RegisterListener<FTTDResearchInventoryChangedMessage>(
				TAG_TTD_Message_Research_Inventory_Changed,
				this,
				&UTTDWarehouseWidget::HandleInventoryChangedMessage);

			CollectionChangedListenerHandle = MessageSubsystem->RegisterListener<FTTDResearchCollectionChangedMessage>(
				TAG_TTD_Message_Research_Collection_Changed,
				this,
				&UTTDWarehouseWidget::HandleCollectionChangedMessage);
		}
	}

	RefreshContent();
}

void UTTDWarehouseWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TTDUIStyle::ResetFadeIn(this, EntranceElapsed);
}

void UTTDWarehouseWidget::NativeDestruct()
{
	InventoryChangedListenerHandle.Unregister();
	CollectionChangedListenerHandle.Unregister();
	Super::NativeDestruct();
}

void UTTDWarehouseWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TTDUIStyle::TickFadeIn(this, EntranceElapsed, InDeltaTime);
}

UTTDResearchSubsystem* UTTDWarehouseWidget::GetResearchSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UTTDResearchSubsystem>() : nullptr;
}

void UTTDWarehouseWidget::HandleBackClicked()
{
	if (ATTDMenuPlayerController* Controller = GetOwningPlayer<ATTDMenuPlayerController>())
	{
		Controller->ShowMainScreen();
	}
}

void UTTDWarehouseWidget::HandleInventoryChangedMessage(FGameplayTag Channel, const FTTDResearchInventoryChangedMessage& Message)
{
	RefreshContent();
}

void UTTDWarehouseWidget::HandleCollectionChangedMessage(FGameplayTag Channel, const FTTDResearchCollectionChangedMessage& Message)
{
	RefreshContent();
}

void UTTDWarehouseWidget::RefreshContent()
{
	if (!ContentBox)
	{
		return;
	}

	ContentBox->ClearChildren();

	const UTTDResearchSubsystem* ResearchSubsystem = GetResearchSubsystem();
	if (!ResearchSubsystem)
	{
		ContentBox->AddChildToVerticalBox(MakeText(WidgetTree, FText::FromString(TEXT("研究所系统未初始化。")), 18, TTDUIStyle::Danger()));
		return;
	}

	AddPartInventorySection(*ResearchSubsystem);
	AddCollectionSection(FText::FromString(TEXT("已解锁图纸")), ResearchSubsystem->GetCollectionEntries(ETTDCollectionCategory::Diagram), TEXT("暂无已解锁图纸"));
	AddCollectionSection(FText::FromString(TEXT("已解锁玩具盒")), ResearchSubsystem->GetCollectionEntries(ETTDCollectionCategory::ToyBox), TEXT("暂无已解锁玩具盒"));
}

void UTTDWarehouseWidget::AddPartInventorySection(const UTTDResearchSubsystem& ResearchSubsystem)
{
	UBorder* SectionPanel = MakePanel(WidgetTree, TTDUIStyle::Cream(), FMargin(18.0f));
	ContentBox->AddChildToVerticalBox(SectionPanel);
	UVerticalBox* SectionBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	SectionPanel->AddChild(SectionBox);
	SectionBox->AddChildToVerticalBox(MakeText(WidgetTree, FText::FromString(TEXT("零件库存")), 22, TTDUIStyle::Ink()));

	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	UVerticalBoxSlot* GridSlot = SectionBox->AddChildToVerticalBox(Grid);
	GridSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));

	const TArray<FTTDNameStack> Stacks = ResearchSubsystem.GetPartInventory();
	if (Stacks.IsEmpty())
	{
		Grid->AddChildToUniformGrid(MakeText(WidgetTree, FText::FromString(TEXT("暂无零件")), 17, TTDUIStyle::MutedInk()), 0, 0);
		return;
	}

	for (int32 Index = 0; Index < Stacks.Num(); ++Index)
	{
		const FTTDNameStack& Stack = Stacks[Index];
		if (!Stack.Id.IsNone() && Stack.Count > 0)
		{
			UBorder* Card = MakePanel(WidgetTree, TTDUIStyle::LightPaper(), FMargin(14.0f, 10.0f));
			Card->AddChild(MakeText(
				WidgetTree,
				FText::Format(
					FText::FromString(TEXT("{0}\nx{1}")),
					TTDUIDisplayNames::PartName(GetGameInstance(), Stack.Id),
					FText::AsNumber(Stack.Count)),
				17,
				TTDUIStyle::Ink(),
				ETextJustify::Center));
			Grid->AddChildToUniformGrid(Card, Index / 4, Index % 4);
		}
	}
}

void UTTDWarehouseWidget::AddCollectionSection(const FText& Title, const TArray<FTTDCollectionEntry>& Entries, const TCHAR* EmptyText)
{
	UBorder* SectionPanel = MakePanel(WidgetTree, TTDUIStyle::Cream(), FMargin(18.0f));
	UVerticalBoxSlot* SectionSlot = ContentBox->AddChildToVerticalBox(SectionPanel);
	SectionSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));

	UVerticalBox* SectionBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	SectionPanel->AddChild(SectionBox);
	SectionBox->AddChildToVerticalBox(MakeText(WidgetTree, Title, 22, TTDUIStyle::Ink()));

	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	UVerticalBoxSlot* GridSlot = SectionBox->AddChildToVerticalBox(Grid);
	GridSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));

	int32 VisibleIndex = 0;
	for (const FTTDCollectionEntry& Entry : Entries)
	{
		if (Entry.bUnlocked)
		{
			UBorder* Card = MakePanel(WidgetTree, TTDUIStyle::LightPaper(), FMargin(14.0f, 10.0f));
			Card->AddChild(MakeText(WidgetTree, Entry.DisplayName, 17, TTDUIStyle::Ink(), ETextJustify::Center));
			Grid->AddChildToUniformGrid(Card, VisibleIndex / 4, VisibleIndex % 4);
			++VisibleIndex;
		}
	}

	if (VisibleIndex == 0)
	{
		Grid->AddChildToUniformGrid(MakeText(WidgetTree, FText::FromString(EmptyText), 17, TTDUIStyle::MutedInk()), 0, 0);
	}
}
