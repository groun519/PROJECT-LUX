#pragma once

#include "Containers/StaticArray.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LuxRevolver.generated.h"

UENUM(BlueprintType)
enum class ELuxRevolverRoundType : uint8
{
	Empty,
	Live,
	Blank,
	Rubber
};

UCLASS()
class PROJECT_LUX_API ALuxRevolver : public AActor
{
	GENERATED_BODY()

public:
	static constexpr uint8 ChamberCount = 6;

	ALuxRevolver();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	int32 GetChamberCount() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	uint8 GetLoadedMask() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	uint8 GetCurrentChamberIndex() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	bool IsCylinderOpen() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	bool IsChamberLoaded(int32 ChamberIndex) const;

	ELuxRevolverRoundType GetRoundTypeForAuthority(int32 ChamberIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Revolver|Development", meta = (DevelopmentOnly))
	bool SetChamberRoundTypeForDevelopment(int32 ChamberIndex, ELuxRevolverRoundType RoundType);

	UFUNCTION(BlueprintCallable, Category = "Revolver|Development", meta = (DevelopmentOnly))
	bool SetPublicStateForDevelopment(int32 ChamberIndex, bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Revolver|Development", meta = (DevelopmentOnly))
	FString DescribeChambersForDevelopment() const;

private:
	bool IsValidChamberIndex(int32 ChamberIndex) const;
	void RefreshLoadedMask();

	// Exact round types intentionally remain server-only and are never registered for replication.
	TStaticArray<ELuxRevolverRoundType, ChamberCount> ChamberRoundTypes;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	uint8 LoadedMask = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	uint8 CurrentChamberIndex = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	bool bCylinderOpen = false;
};
