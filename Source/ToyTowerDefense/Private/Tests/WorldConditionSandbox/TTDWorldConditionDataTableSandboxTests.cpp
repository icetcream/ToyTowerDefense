#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "StructUtils/StructView.h"
#include "WorldConditionSandbox/TTDWorldConditionSandbox.h"
#include "UObject/StrongObjectPtr.h"
#include "WorldConditionContext.h"
#include "WorldConditionQuery.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FName CurrencyRowName(TEXT("CurrencyAtLeast10"));
	const FName DiagramRowName(TEXT("RequiresBasicTower"));

	UDataTable* CreateSandboxDataTable()
	{
		UDataTable* DataTable = NewObject<UDataTable>();
		DataTable->RowStruct = FTTDWorldConditionDataRow::StaticStruct();

		FTTDWorldConditionDataRow CurrencyRow;
		CurrencyRow.ConditionDataId = CurrencyRowName;
		CurrencyRow.Payload = FInstancedStruct::Make(FTTDConditionData_HasCurrency(10));
		DataTable->AddRow(CurrencyRowName, CurrencyRow);

		FTTDWorldConditionDataRow DiagramRow;
		DiagramRow.ConditionDataId = DiagramRowName;
		DiagramRow.Payload = FInstancedStruct::Make(FTTDConditionData_HasDiagram(TEXT("BasicTower")));
		DataTable->AddRow(DiagramRowName, DiagramRow);

		return DataTable;
	}

	FDataTableRowHandle MakeRowHandle(UDataTable* DataTable, const FName RowName)
	{
		FDataTableRowHandle Handle;
		Handle.DataTable = DataTable;
		Handle.RowName = RowName;
		return Handle;
	}

	FTTDWorldConditionSandboxContext MakeContext(const int32 Currency, const TSet<FName>& DiagramIds = {})
	{
		FTTDWorldConditionSandboxContext Context;
		Context.Currency = Currency;
		Context.DiagramIds = DiagramIds;
		return Context;
	}

	bool InitializeCurrencyQuery(FWorldConditionQuery& Query, UObject* Owner, const FDataTableRowHandle& DataRow)
	{
		FTTDWorldCondition_HasCurrencyFromRow Condition;
		Condition.DataRow = DataRow;
		return Query.DebugInitialize(Owner, UTTDWorldConditionSandboxSchema::StaticClass(),
			{
				FWorldConditionEditable(0, EWorldConditionOperator::Copy, FConstStructView::Make(Condition))
			});
	}

	bool InitializeDiagramQuery(FWorldConditionQuery& Query, UObject* Owner, const FDataTableRowHandle& DataRow)
	{
		FTTDWorldCondition_HasDiagramFromRow Condition;
		Condition.DataRow = DataRow;
		return Query.DebugInitialize(Owner, UTTDWorldConditionSandboxSchema::StaticClass(),
			{
				FWorldConditionEditable(0, EWorldConditionOperator::Copy, FConstStructView::Make(Condition))
			});
	}

	bool EvaluateQuery(FWorldConditionQuery& Query, UObject* Owner, const FTTDWorldConditionSandboxContext& RuntimeContext)
	{
		const UTTDWorldConditionSandboxSchema* Schema = GetDefault<UTTDWorldConditionSandboxSchema>();
		FWorldConditionContextData ContextData(*Schema);
		ContextData.SetContextData(Schema->GetSandboxContextRef(), &RuntimeContext);

		Query.Activate(*Owner, ContextData);
		const bool bResult = Query.IsTrue(ContextData);
		Query.Deactivate(ContextData);
		return bResult;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTTDWorldConditionDataTableSandboxTests,
	"ToyTowerDefense.WorldCondition.DataTableSandbox",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTDWorldConditionDataTableSandboxTests::RunTest(const FString& Parameters)
{
	UObject* Owner = GetTransientPackage();
	const TStrongObjectPtr<UDataTable> DataTable(CreateSandboxDataTable());

	FWorldConditionQuery FirstCurrencyQuery;
	FWorldConditionQuery SecondCurrencyQuery;
	TestTrue(TEXT("first query initializes from shared currency row"), InitializeCurrencyQuery(FirstCurrencyQuery, Owner, MakeRowHandle(DataTable.Get(), CurrencyRowName)));
	TestTrue(TEXT("second query initializes from shared currency row"), InitializeCurrencyQuery(SecondCurrencyQuery, Owner, MakeRowHandle(DataTable.Get(), CurrencyRowName)));

	TestFalse(TEXT("shared currency row rejects currency below threshold"), EvaluateQuery(FirstCurrencyQuery, Owner, MakeContext(9)));
	TestFalse(TEXT("second query shares the same row data and rejects below threshold"), EvaluateQuery(SecondCurrencyQuery, Owner, MakeContext(9)));
	TestTrue(TEXT("shared currency row accepts currency at threshold"), EvaluateQuery(FirstCurrencyQuery, Owner, MakeContext(10)));
	TestTrue(TEXT("second query accepts currency at threshold through same row"), EvaluateQuery(SecondCurrencyQuery, Owner, MakeContext(10)));

	FTTDWorldConditionDataRow* MutableCurrencyRow = DataTable->FindRow<FTTDWorldConditionDataRow>(CurrencyRowName, TEXT("WorldConditionDataTableSandbox"));
	TestNotNull(TEXT("mutable currency row exists"), MutableCurrencyRow);
	if (MutableCurrencyRow)
	{
		MutableCurrencyRow->Payload = FInstancedStruct::Make(FTTDConditionData_HasCurrency(20));
	}

	FWorldConditionQuery ReinitializedCurrencyQuery;
	TestTrue(TEXT("query reinitializes after shared row changes"), InitializeCurrencyQuery(ReinitializedCurrencyQuery, Owner, MakeRowHandle(DataTable.Get(), CurrencyRowName)));
	TestFalse(TEXT("reinitialized query reads updated row and rejects old threshold"), EvaluateQuery(ReinitializedCurrencyQuery, Owner, MakeContext(10)));
	TestTrue(TEXT("reinitialized query reads updated row and accepts new threshold"), EvaluateQuery(ReinitializedCurrencyQuery, Owner, MakeContext(20)));

	FWorldConditionQuery DiagramQuery;
	TestTrue(TEXT("diagram query initializes from diagram row"), InitializeDiagramQuery(DiagramQuery, Owner, MakeRowHandle(DataTable.Get(), DiagramRowName)));
	TestFalse(TEXT("diagram row rejects missing diagram id"), EvaluateQuery(DiagramQuery, Owner, MakeContext(0)));
	TestTrue(TEXT("diagram row accepts matching diagram id"), EvaluateQuery(DiagramQuery, Owner, MakeContext(0, { TEXT("BasicTower") })));

	AddExpectedError(TEXT("WorldCondition sandbox"), EAutomationExpectedErrorFlags::Contains, 3);

	FWorldConditionQuery EmptyHandleQuery;
	TestTrue(TEXT("empty handle query still initializes"), InitializeCurrencyQuery(EmptyHandleQuery, Owner, FDataTableRowHandle()));
	TestFalse(TEXT("empty row handle evaluates false"), EvaluateQuery(EmptyHandleQuery, Owner, MakeContext(100)));

	FWorldConditionQuery WrongPayloadQuery;
	TestTrue(TEXT("wrong payload query still initializes"), InitializeCurrencyQuery(WrongPayloadQuery, Owner, MakeRowHandle(DataTable.Get(), DiagramRowName)));
	TestFalse(TEXT("wrong payload type evaluates false"), EvaluateQuery(WrongPayloadQuery, Owner, MakeContext(100)));

	const TStrongObjectPtr<UDataTable> WrongStructTable(NewObject<UDataTable>());
	WrongStructTable->RowStruct = FTTDConditionData_HasCurrency::StaticStruct();
	FWorldConditionQuery WrongRowStructQuery;
	TestTrue(TEXT("wrong row struct query still initializes"), InitializeCurrencyQuery(WrongRowStructQuery, Owner, MakeRowHandle(WrongStructTable.Get(), CurrencyRowName)));
	TestFalse(TEXT("wrong row struct evaluates false"), EvaluateQuery(WrongRowStructQuery, Owner, MakeContext(100)));

	return true;
}

#endif
