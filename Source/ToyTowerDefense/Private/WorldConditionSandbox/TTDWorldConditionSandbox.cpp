#include "WorldConditionSandbox/TTDWorldConditionSandbox.h"

#include "WorldConditionContext.h"

DEFINE_LOG_CATEGORY(LogTTDWorldConditionSandbox);

namespace
{
	FWorldConditionResult MakeConditionResult(const bool bIsTrue)
	{
		return FWorldConditionResult(
			bIsTrue ? EWorldConditionResultValue::IsTrue : EWorldConditionResultValue::IsFalse,
			false);
	}

	const FTTDWorldConditionDataRow* ResolveConditionDataRow(const FDataTableRowHandle& DataRow, const TCHAR* ContextString)
	{
		if (!DataRow.DataTable || DataRow.RowName.IsNone())
		{
			UE_LOG(LogTTDWorldConditionSandbox, Warning, TEXT("WorldCondition sandbox row handle is empty for %s."), ContextString);
			return nullptr;
		}

		if (DataRow.DataTable->GetRowStruct() != FTTDWorldConditionDataRow::StaticStruct())
		{
			UE_LOG(
				LogTTDWorldConditionSandbox,
				Warning,
				TEXT("WorldCondition sandbox DataTable %s uses incompatible row struct %s for %s."),
				*GetNameSafe(DataRow.DataTable),
				*GetNameSafe(DataRow.DataTable->GetRowStruct()),
				ContextString);
			return nullptr;
		}

		const FTTDWorldConditionDataRow* Row = DataRow.DataTable->FindRow<FTTDWorldConditionDataRow>(DataRow.RowName, ContextString, false);
		if (!Row)
		{
			UE_LOG(
				LogTTDWorldConditionSandbox,
				Warning,
				TEXT("WorldCondition sandbox row %s is missing from %s for %s."),
				*DataRow.RowName.ToString(),
				*GetNameSafe(DataRow.DataTable),
				ContextString);
		}

		return Row;
	}

	template<typename PayloadType>
	const PayloadType* ResolveConditionPayload(const FDataTableRowHandle& DataRow, const TCHAR* ContextString)
	{
		const FTTDWorldConditionDataRow* Row = ResolveConditionDataRow(DataRow, ContextString);
		if (!Row)
		{
			return nullptr;
		}

		const PayloadType* Payload = Row->Payload.GetPtr<PayloadType>();
		if (!Payload)
		{
			UE_LOG(
				LogTTDWorldConditionSandbox,
				Warning,
				TEXT("WorldCondition sandbox row %s has payload type %s but %s expected %s."),
				*DataRow.RowName.ToString(),
				*GetNameSafe(Row->Payload.GetScriptStruct()),
				ContextString,
				*GetNameSafe(PayloadType::StaticStruct()));
		}

		return Payload;
	}
}

UTTDWorldConditionSandboxSchema::UTTDWorldConditionSandboxSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SandboxContextRef = AddContextDataDesc(TEXT("SandboxContext"), FTTDWorldConditionSandboxContext::StaticStruct(), EWorldConditionContextDataType::Dynamic);
}

bool UTTDWorldConditionSandboxSchema::IsStructAllowed(const UScriptStruct* InScriptStruct) const
{
	return InScriptStruct
		&& (InScriptStruct->IsChildOf(FTTDWorldCondition_HasCurrencyFromRow::StaticStruct())
			|| InScriptStruct->IsChildOf(FTTDWorldCondition_HasDiagramFromRow::StaticStruct()));
}

bool FTTDWorldCondition_HasCurrencyFromRow::Initialize(const UWorldConditionSchema& Schema)
{
	const UTTDWorldConditionSandboxSchema* SandboxSchema = Cast<UTTDWorldConditionSandboxSchema>(&Schema);
	if (!SandboxSchema)
	{
		return false;
	}

	SandboxContextRef = SandboxSchema->GetSandboxContextRef();
	bCanCacheResult = false;
	return SandboxContextRef.IsValid();
}

FWorldConditionResult FTTDWorldCondition_HasCurrencyFromRow::IsTrue(const FWorldConditionContext& Context) const
{
	const FTTDWorldConditionSandboxContext* RuntimeContext = Context.GetContextDataPtr<FTTDWorldConditionSandboxContext>(SandboxContextRef);
	const FTTDConditionData_HasCurrency* ConditionData = ResolveConditionPayload<FTTDConditionData_HasCurrency>(DataRow, TEXT("HasCurrencyFromRow"));
	return MakeConditionResult(RuntimeContext && ConditionData && RuntimeContext->Currency >= ConditionData->MinCurrency);
}

#if WITH_EDITOR
FText FTTDWorldCondition_HasCurrencyFromRow::GetDescription() const
{
	return FText::Format(
		NSLOCTEXT("ToyTowerDefense", "HasCurrencyFromRowDescription", "Currency from row {0}"),
		FText::FromName(DataRow.RowName));
}
#endif

bool FTTDWorldCondition_HasDiagramFromRow::Initialize(const UWorldConditionSchema& Schema)
{
	const UTTDWorldConditionSandboxSchema* SandboxSchema = Cast<UTTDWorldConditionSandboxSchema>(&Schema);
	if (!SandboxSchema)
	{
		return false;
	}

	SandboxContextRef = SandboxSchema->GetSandboxContextRef();
	bCanCacheResult = false;
	return SandboxContextRef.IsValid();
}

FWorldConditionResult FTTDWorldCondition_HasDiagramFromRow::IsTrue(const FWorldConditionContext& Context) const
{
	const FTTDWorldConditionSandboxContext* RuntimeContext = Context.GetContextDataPtr<FTTDWorldConditionSandboxContext>(SandboxContextRef);
	const FTTDConditionData_HasDiagram* ConditionData = ResolveConditionPayload<FTTDConditionData_HasDiagram>(DataRow, TEXT("HasDiagramFromRow"));
	return MakeConditionResult(RuntimeContext && ConditionData && RuntimeContext->DiagramIds.Contains(ConditionData->DiagramId));
}

#if WITH_EDITOR
FText FTTDWorldCondition_HasDiagramFromRow::GetDescription() const
{
	return FText::Format(
		NSLOCTEXT("ToyTowerDefense", "HasDiagramFromRowDescription", "Diagram from row {0}"),
		FText::FromName(DataRow.RowName));
}
#endif
