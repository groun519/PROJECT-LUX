#pragma once

#include "Containers/StaticArray.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LuxRevolver.generated.h"

class ALuxCharacter;
class ALuxRevolver;

UENUM(BlueprintType)
enum class ELuxRevolverRoundType : uint8
{
	Empty,
	Live,
	Blank,
	Rubber
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FLuxNonLethalHitSignature,
	ALuxRevolver*, Revolver,
	ALuxCharacter*, Target
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLuxDryFireSignature, ALuxRevolver*, Revolver);

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

	UFUNCTION(BlueprintCallable, Category = "Revolver")
	void RequestFire();

	UFUNCTION(BlueprintPure, Category = "Revolver")
	int32 GetFireSequence() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	int32 GetDryFireSequence() const;

	ELuxRevolverRoundType GetRoundTypeForAuthority(int32 ChamberIndex) const;

	UPROPERTY(BlueprintAssignable, Category = "Revolver")
	FLuxNonLethalHitSignature OnNonLethalHit;

	UPROPERTY(BlueprintAssignable, Category = "Revolver")
	FLuxDryFireSignature OnDryFire;

	UFUNCTION(BlueprintCallable, Category = "Revolver|Development", meta = (DevelopmentOnly))
	bool SetChamberRoundTypeForDevelopment(int32 ChamberIndex, ELuxRevolverRoundType RoundType);

	UFUNCTION(BlueprintCallable, Category = "Revolver|Development", meta = (DevelopmentOnly))
	bool SetPublicStateForDevelopment(int32 ChamberIndex, bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Revolver|Development", meta = (DevelopmentOnly))
	FString DescribeChambersForDevelopment() const;

	UFUNCTION(BlueprintPure, Category = "Revolver|Development", meta = (DevelopmentOnly))
	FString DescribeLastFireResultForDevelopment() const;

	UFUNCTION(BlueprintPure, Category = "Revolver|Development", meta = (DevelopmentOnly))
	ALuxCharacter* GetLastNonLethalHitForDevelopment() const;

private:
	UFUNCTION(Server, Reliable)
	void ServerFire();

	bool CanFire(const ALuxCharacter* EquippedCharacter, double ServerTimeSeconds) const;
	bool IsValidChamberIndex(int32 ChamberIndex) const;
	ALuxCharacter* TraceCharacter(const ALuxCharacter* EquippedCharacter) const;
	void ConsumeCurrentRoundAndAdvance();
	void RefreshLoadedMask();

	// Exact round types intentionally remain server-only and are never registered for replication.
	TStaticArray<ELuxRevolverRoundType, ChamberCount> ChamberRoundTypes;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	uint8 LoadedMask = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	uint8 CurrentChamberIndex = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	bool bCylinderOpen = false;

	// Generic sequences support later presentation without revealing Live, Blank, or Rubber.
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	int32 FireSequence = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	int32 DryFireSequence = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Fire", meta = (ClampMin = "0.05"))
	float MinimumFireIntervalSeconds = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Fire", meta = (ClampMin = "100.0"))
	float TraceRange = 10000.0f;

	double LastFireServerTimeSeconds = -1.0;
	FName LastFireResultForDevelopment = TEXT("None");
	TWeakObjectPtr<ALuxCharacter> LastNonLethalHitForDevelopment;
};
