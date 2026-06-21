#pragma once

#include "CoreMinimal.h"
#include "TTDTypes.h"

class UGameInstance;

namespace TTDUIDisplayNames
{
	FText DisplayNameOrFallback(const FText& DisplayName, FName Id);
	FText KnownId(FName Id);
	FText PartName(const UGameInstance* GameInstance, FName PartId);
	FText DiagramName(const UGameInstance* GameInstance, FName DiagramId);
	FText ToyBoxName(const UGameInstance* GameInstance, FName ToyBoxId, const FText& ConfigDisplayName = FText::GetEmpty());
	FString JoinPartIds(const UGameInstance* GameInstance, const TArray<FName>& PartIds, const TCHAR* EmptyText = TEXT("无"));
	FString JoinDiagramIds(const UGameInstance* GameInstance, const TArray<FName>& DiagramIds, const TCHAR* EmptyText = TEXT("无"));
	FString JoinPartStacks(const UGameInstance* GameInstance, const TArray<FTTDNameStack>& Stacks, const TCHAR* EmptyText = TEXT("无"));
	FString JoinToyBoxStacks(const UGameInstance* GameInstance, const TArray<FTTDNameStack>& Stacks, const TCHAR* EmptyText = TEXT("无"));
}
