#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "States.generated.h"


UENUM(BlueprintType)
enum class EPlayerState : uint8
{
	Idle,
	Locomotion,
	Attack,
	Skill,
	Dodge,
	Interact,
	Stunned,
	Dead
};

UENUM(BlueprintType)
enum class EDecision : uint8
{
	Accept,
	Reject,
	Interrupt,
	Replace,
	Queue,
	Hold,
	Release
};

USTRUCT(BlueprintType)
struct FActionDecision
{
	GENERATED_BODY();

	FActionDecision() {};
	FActionDecision(EDecision InDecision, FName InReason, FGameplayTag InActionTag)
		: Decision(InDecision), Reason(InReason), ActionTag(InActionTag){
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite) EDecision Decision = EDecision::Reject;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Reason = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag ActionTag;
};

USTRUCT(BlueprintType)
struct FInputBuffer
{
	GENERATED_BODY();

	FInputBuffer() {};
	FInputBuffer(const FGameplayTag& InInputActionTag, float InInputTime)
		: InputActionTag(InInputActionTag), InputTime(InInputTime) {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag InputActionTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float InputTime = 0.0f; // world seconds

};

USTRUCT(BlueprintType)
struct FCurrentState
{
	GENERATED_BODY();

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State") EPlayerState State = EPlayerState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State") FGameplayTag CurrentActionTag = FGameplayTag::EmptyTag;

	// Runtime tags, for combo windows, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State") FGameplayTagContainer RuntimeTags;

	// endered time
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State") float EnteredStateTime = 0.0f;

	// Clear tags with transition
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State") FGameplayTagContainer ClearTagsOnTransition;

	// Input buffering limit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State") float InputBufferingTime = 1.0f;

	// Max input buffer size
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State") int32 MaxInputBufferSize = 3;

	// Inputs buffered, cleared input over MaxInputBufferSize
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State") TArray<FInputBuffer> InputBuffer;

	float ElapsedStateTime(float WorldSeconds) const { return WorldSeconds - EnteredStateTime; }

	
	bool HasTag(const FGameplayTag& Tag) const { return RuntimeTags.HasTag(Tag); }
	bool HasAnyTags(const FGameplayTagContainer& Tags) const { return RuntimeTags.HasAny(Tags); }
	bool HasAllTags(const FGameplayTagContainer& Tags) const { return RuntimeTags.HasAll(Tags); }
	bool HasTagExact(const FGameplayTag& Tag) const { return RuntimeTags.HasTagExact(Tag); }
	void AddTag(const FGameplayTag& Tag) { RuntimeTags.AddTag(Tag); }
	void RemoveTag(const FGameplayTag& Tag) { RuntimeTags.RemoveTag(Tag); }
	void ClearActionTag() { CurrentActionTag = FGameplayTag::EmptyTag; }

	FGameplayTag GetActionTag() const {
		return CurrentActionTag;  
	}

	bool HasActionTag() const {
		return CurrentActionTag.IsValid();
	}

	void EnterState(EPlayerState NewState, const FGameplayTag& DestActionTag, float WorldSeconds) {
		
		for (const FGameplayTag& Tag : ClearTagsOnTransition) RemoveTag(Tag);

		if (State == NewState && CurrentActionTag == DestActionTag) return;

		State = NewState;
		CurrentActionTag = DestActionTag;
		EnteredStateTime = WorldSeconds;

	}

	void RemoveInputBufferByTime(float WorldSeconds) {
		/*for (int32 i = InputBuffer.Num() - 1; i >= 0; --i) {
			if (InputBuffer[i].InputTime + InputBufferingTime <= WorldSeconds) InputBuffer.RemoveAt(i);
		}*/

		for (int32 i = InputBuffer.Num() - 1; i >= 0; --i) {
			float Age = WorldSeconds - InputBuffer[i].InputTime;
			if (InputBuffer[i].InputTime + InputBufferingTime <= WorldSeconds) {
				UE_LOG(LogTemp, Warning, TEXT("[Trim] Remove %s age=%.3f"),
					*InputBuffer[i].InputActionTag.ToString(), Age);
				InputBuffer.RemoveAt(i);
			}
			else {
				UE_LOG(LogTemp, Warning, TEXT("[Trim] Keep %s age=%.3f"),
					*InputBuffer[i].InputActionTag.ToString(), Age);
			}
		}
	}

	// first in, first out
	void PushInputBuffer(const FGameplayTag& InputActionTag, float WorldSeconds) {
		// 입력 버퍼에 새로운 입력을 push
		// Buffer 내에서 시간 초과된 입력 제거
		// MaxInputBufferSize 초과 시 가장 오래된 입력 제거
		// fifo

		RemoveInputBufferByTime(WorldSeconds);
		if (InputBuffer.Num() >= MaxInputBufferSize) {
			InputBuffer.RemoveAt(0);
		}
		
		InputBuffer.Add(FInputBuffer{ InputActionTag, WorldSeconds });
		
	}

	bool PeekFastestInputBuffer(FGameplayTag& OutActionTag, float WorldSeconds) {

		RemoveInputBufferByTime(WorldSeconds);

		if (InputBuffer.Num() > 0) {
			OutActionTag = InputBuffer[0].InputActionTag;
			return true;
		}
		
		return false;
	}

	bool PeekLatestInputBuffer(FGameplayTag& OutActionTag, float WorldSeconds) {

		RemoveInputBufferByTime(WorldSeconds);

		if (InputBuffer.Num() > 0) {
			OutActionTag = InputBuffer.Last().InputActionTag;
			return true;
		}
		return false;
	}

	bool PopFastestInputBuffer(FGameplayTag& OutActionTag, float WorldSeconds) {

		RemoveInputBufferByTime(WorldSeconds);

		if (InputBuffer.Num() > 0) {
			OutActionTag = InputBuffer[0].InputActionTag;
			InputBuffer.RemoveAt(0);

			return true;
		}

		else return false;
		
	}

	bool PopLatestInputBuffer(FGameplayTag& OutActionTag, float WorldSeconds) {

		RemoveInputBufferByTime(WorldSeconds);

		if (InputBuffer.Num() > 0) {
			OutActionTag = InputBuffer.Last().InputActionTag;
			InputBuffer.Pop();

			return true;
		}

		else return false;

	}

};

USTRUCT(BlueprintType)
struct FActionRule
{
	GENERATED_BODY();

	// Attack, Dodge 등 행동 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag ActionTag;

	// 실행에 필요한 태그, 실행을 막는 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTagContainer RequiredTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTagContainer BlockingTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTagContainer ProgressTags;
	// 이 태그가 있을 때 인터럽트 가능, 태그 컨테이너로 확장 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTagContainer InterruptWindowTags;

	// 현재 허용 불가 시 큐잉 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bQueueable = true;

	// 홀드 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHoldable = false;

	// 쿨다운 태그, 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag CooldownTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float CooldownTime = 0.0f;

	// 이동 블록 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float LockMovementTime = 0.0f;

    // 우선순위, 버퍼링 시 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Priority = 0;

};

UCLASS(BlueprintType)
class TBD_API UActionRuleSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FActionRule> ActionRules;

	const FActionRule* FindActionRuleByTag(const FGameplayTag& ActionTag) const {
		for (const FActionRule& Rule : ActionRules) {
			if (Rule.ActionTag == ActionTag) return &Rule;
		}
		return nullptr;
	}
};

namespace ActionEvaluator {

	// evaluator, ActionTag에 해당하는 FActionRule을 ActionRuleSet에서 찾아서 평가
	// CurrentState와 ActionRule을 비교해 EDecision, Reason 반환
	inline FActionDecision EvaluateAction(const FCurrentState& Current, const FGameplayTag& ActionTag, const UActionRuleSet* ActionRuleSet, float WorldSeconds) {

		if (!ActionTag.IsValid()) return FActionDecision{ EDecision::Reject, "Invalid ActionTag", ActionTag };
		if (!ActionRuleSet) return FActionDecision{ EDecision::Reject, "Invalid ActionRuleSet", ActionTag };

		// ActionRuleSet에 ActionTag에 해당하는 Rule이 있는지 검사, 없으면 거부
		FActionRule const* ReferenceRule = ActionRuleSet->FindActionRuleByTag(ActionTag);

		if (!ReferenceRule) return FActionDecision{ EDecision::Reject, "No Rule in ActionRuleSet", ActionTag};

		// 쿨다운 시간 검사
		if (ReferenceRule->CooldownTag.IsValid() && Current.HasTagExact(ReferenceRule->CooldownTag)) 
			return FActionDecision{ EDecision::Reject, "Cooldown", ActionTag };

		// RequiredTags 검사
		if (!Current.RuntimeTags.HasAll(ReferenceRule->RequiredTags)) return FActionDecision{ EDecision::Reject, "MissingRequired", ActionTag };

		// BlockingTags 검사
		if (Current.RuntimeTags.HasAny(ReferenceRule->BlockingTags)) return FActionDecision{ EDecision::Reject, "Blocked", ActionTag };

		// 위 조건을 만족했지만 다른 Action이 실행 중인 경우
		if (Current.CurrentActionTag.IsValid() /*&& Current.CurrentActionTag != ActionTag*/) {
			// 인터럽트 가능한지 검사

			if (ReferenceRule->InterruptWindowTags.IsValid() && Current.RuntimeTags.HasAny(ReferenceRule->InterruptWindowTags)) {
				if (Current.CurrentActionTag != ActionTag && ReferenceRule->bHoldable)
					return FActionDecision{ EDecision::Interrupt, "InterruptHold", ActionTag };
				return FActionDecision{ EDecision::Interrupt, "Interrupt", ActionTag };
			}

			if ((Current.CurrentActionTag == ActionTag) && ReferenceRule->bHoldable) 
				return FActionDecision{ EDecision::Release, "Release", ActionTag };

			return ReferenceRule->bQueueable ? FActionDecision{ EDecision::Queue, "Busy", ActionTag }
			: FActionDecision{ EDecision::Reject, "Busy", ActionTag };

		}

		if (ReferenceRule->bHoldable) return FActionDecision{ EDecision::Hold, "Hold", ActionTag };
		return FActionDecision{ EDecision::Accept, "OK", ActionTag };
	}

};

USTRUCT(BlueprintType)
struct FQueuedAction
{
	GENERATED_BODY();

	FQueuedAction() {};
	FQueuedAction(const FGameplayTag& ActionTag, float WorldSeconds, float MaxQueueTime)
		: ActionTag(ActionTag), ExpirationTime(WorldSeconds + MaxQueueTime) {};
	
	UPROPERTY() FGameplayTag ActionTag = FGameplayTag::EmptyTag;
	UPROPERTY() float ExpirationTime = 0.0f; // world seconds

public:
	inline bool IsValid(float WorldSeconds) const { return ActionTag.IsValid() && WorldSeconds <= ExpirationTime; }
	inline void Reset() { ActionTag = FGameplayTag::EmptyTag; ExpirationTime = 0.0f; }

};



//if (Current.CurrentActionTag != ActionTag) {
//	if (ReferenceRule->InterruptWindowTags.IsValid() && Current.RuntimeTags.HasAny(ReferenceRule->InterruptWindowTags)) {
//		if (ReferenceRule->bHoldable) return FActionDecision{ EDecision::Interrupt, "InterruptHold", ActionTag };
//		return FActionDecision{ EDecision::Interrupt, "Interrupt", ActionTag };
//	}
//	// 인터럽트 불가, 큐잉 가능한지 검사
//	return ReferenceRule->bQueueable ? FActionDecision{ EDecision::Queue, "Busy", ActionTag }
//	: FActionDecision{ EDecision::Reject, "Busy", ActionTag };
//}
//
//// 이미 같은 액션이 실행중인 경우, 거부, 추후에 각 Action의 정책에 따라 Replace/Reject를 결정하도록 변경 가능
//else if (Current.CurrentActionTag == ActionTag) {
//	if (ReferenceRule->bHoldable) return FActionDecision{ EDecision::Release, "Release", ActionTag };
//
//	if (ReferenceRule->InterruptWindowTags.IsValid() && Current.RuntimeTags.HasAny(ReferenceRule->InterruptWindowTags)) {
//		if (ReferenceRule->bHoldable) return FActionDecision{ EDecision::Interrupt, "InterruptHold", ActionTag };
//		return FActionDecision{ EDecision::Interrupt, "Interrupt", ActionTag };
//	}
//
//	// 인터럽트 불가, 큐잉 가능한지 검사
//	return ReferenceRule->bQueueable ? FActionDecision{ EDecision::Queue, "Busy", ActionTag }
//	: FActionDecision{ EDecision::Reject, "SameAction", ActionTag };
//
//}