// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "States.h"
#include "Components/ActorComponent.h"
#include "ActionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEvaluationResult, FActionDecision, Decision);


UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TBD_API UActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UActionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FCurrentState CurrentState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuleSet")
	UActionRuleSet* ActionRuleSet = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	TMap<FGameplayTag, FTimerHandle> CooldownTimers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionQueue")
	FQueuedAction CurrentQueuedAction;

	UFUNCTION(BlueprintCallable, Category="State")
	void EnterState(EPlayerState NewState, FGameplayTag DestActionTag, float WorldSeconds);

	UFUNCTION(BlueprintCallable, Category="State")
	void ExitState();

	UFUNCTION(BlueprintCallable, Category = "State")
	void AddStateTag(FGameplayTag Tag) { CurrentState.AddTag(Tag); }
	
	UFUNCTION(BlueprintCallable, Category = "State")
	void AddStateTags(FGameplayTagContainer Tags) { CurrentState.RuntimeTags.AppendTags(Tags); }

	UFUNCTION(BlueprintCallable, Category = "State")
	void RemoveStateTag(FGameplayTag Tag) { CurrentState.RemoveTag(Tag); }

	UFUNCTION(BlueprintCallable, Category = "State")
	void RemoveStateTags(FGameplayTagContainer Tags) { CurrentState.RuntimeTags.RemoveTags(Tags); }

	UFUNCTION(BlueprintCallable, Category = "State")
	bool HasStateTag(FGameplayTag Tag) const { return CurrentState.HasTag(Tag); }

	UFUNCTION(BlueprintCallable, Category = "State")
	FGameplayTag GetCurrentActionTag() const { return CurrentState.GetActionTag(); }

	UFUNCTION(BlueprintCallable, Category = "State")
	bool HasCurrentActionTag() const { return CurrentState.HasActionTag(); }

	UFUNCTION(BlueprintCallable, Category = "State")
	void PushInput(FGameplayTag ActionTag, float WorldSeconds) { CurrentState.PushInputBuffer(ActionTag, WorldSeconds); }

	UFUNCTION(BlueprintCallable, Category = "State")
	bool PeekFastestInput(FGameplayTag& OutActionTag, float WorldSeconds) { return CurrentState.PeekFastestInputBuffer(OutActionTag, WorldSeconds); }

	UFUNCTION(BlueprintCallable, Category = "State")
	bool PeekLatestInput(FGameplayTag& OutActionTag, float WorldSeconds) { return CurrentState.PeekLatestInputBuffer(OutActionTag, WorldSeconds); }
	
	UFUNCTION(BlueprintCallable, Category = "State")
	bool PopFastestInput(FGameplayTag& OutActionTag, float WorldSeconds) { return CurrentState.PopFastestInputBuffer(OutActionTag, WorldSeconds); }

	UFUNCTION(BlueprintCallable, Category = "State")
	bool PopLatestInput(FGameplayTag& OutActionTag, float WorldSeconds) { return CurrentState.PopLatestInputBuffer(OutActionTag, WorldSeconds); }

	// 이동은 연속적으로 발생하므로, InputBuffer와 Evaluator를 거치지 않음
	// 이동 가능 여부 검사 + 상태 전환 후 이동

	UFUNCTION(BlueprintCallable, Category = "State")
	bool CanMove() const; // 상태에 따른 이동 가능 여부 판단

	UFUNCTION(BlueprintCallable, Category = "State")
	void UpdateMovementState(float WorldSeconds); // 이동에 따른 상태 전환

	UFUNCTION(BlueprintCallable, Category = "State")
	FActionDecision EvaluateActionBP(FGameplayTag ActionTag, float WorldSeconds);

	UPROPERTY(BlueprintAssignable, Category = "Evaluation");
	FEvaluationResult OnEvaluationResult;

	UFUNCTION(BlueprintCallable, Category = "Evaluation")
	void BroadcastEvaluationResult(FActionDecision Decision);

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugPrint(const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "State")
	EPlayerState ActionTagToState(const FGameplayTag& ActionTag);

	UFUNCTION(BlueprintCallable, Category = "ActionQueue")
	bool QueuedActionIsValid(float WorldSeconds) { 
		if (!CurrentQueuedAction.IsValid(WorldSeconds)) {
			CurrentQueuedAction.Reset();
			return false;
		}
		return true;
	}

	UFUNCTION(BlueprintCallable, Category = "ActionQueue")
	void PushQueuedAction(const FGameplayTag& ActionTag, float WorldSeconds, float MaxQueueTime) { CurrentQueuedAction = FQueuedAction{ ActionTag, WorldSeconds, MaxQueueTime }; }

	UFUNCTION(BlueprintCallable, Category = "ActionQueue")
	void ClearQueuedAction() { CurrentQueuedAction.Reset(); }

	UFUNCTION(BlueprintCallable, Category = "ActionQueue")
	FGameplayTag GetQueuedActionTag() const { return CurrentQueuedAction.ActionTag; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
