#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TTDBattleTargetInterface.generated.h"

UINTERFACE(BlueprintType)
class TOYTOWERDEFENSE_API UTTDBattleTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class TOYTOWERDEFENSE_API ITTDBattleTargetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Toy Tower Defense|Battle")
	FVector GetBattleTargetLocation() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Toy Tower Defense|Battle")
	bool IsBattleTargetAlive() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Toy Tower Defense|Battle")
	void ApplyBattleDamage(float Damage, UObject* DamageSource);
};
