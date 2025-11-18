// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionComponent.h"

// Sets default values for this component's properties
UActionComponent::UActionComponent()
{

	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
}

// Called when the game starts
void UActionComponent::BeginPlay()
{
	Super::BeginPlay();

	DebugPrint("ActionComponent::BeginPlay()");

}

// Called every frame
void UActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//UpdateMovementState(GetWorld()->GetDeltaSeconds());
}

void UActionComponent::EnterState(EPlayerState NewState, FGameplayTag DestActionTag, float WorldSeconds) {
	CurrentState.EnterState(NewState, DestActionTag, WorldSeconds);

	if (const FActionRule* Rule = ActionRuleSet->FindActionRuleByTag(CurrentState.CurrentActionTag)) {
		if (Rule->CooldownTag.IsValid()) {

			const FGameplayTag CoolTag = Rule->CooldownTag;
			const float CoolTime = Rule->CooldownTime;
			FTimerHandle& CooldownTimer = CooldownTimers.FindOrAdd(CoolTag);

			AddStateTag(CoolTag);

			GetWorld()->GetTimerManager().SetTimer(CooldownTimer, FTimerDelegate::CreateUObject(this, &UActionComponent::RemoveStateTag, CoolTag), CoolTime, false);

			// 쿨다운 태그를 제거하는 타이머 설정
			/*GetWorld()->GetTimerManager().SetTimerForNextTick([this, Rule]() {
				GetWorld()->GetTimerManager().SetTimer(
					CoolDownTimers.FindOrAdd(Rule->CooldownTag),
					FTimerDelegate::CreateUObject(this, &UActionComponent::RemoveStateTag, Rule->CooldownTag), Rule->CooldownTime, false);
			});*/
		}

		if (Rule->ProgressTags.IsValid()) {
			AddStateTags(Rule->ProgressTags);
		}
	}
}

void UActionComponent::ExitState() {

	if (const FActionRule* Rule = ActionRuleSet->FindActionRuleByTag(CurrentState.CurrentActionTag)) {
		if (Rule->ProgressTags.IsValid())
			CurrentState.RuntimeTags.RemoveTags(Rule->ProgressTags);
	}
	CurrentState.ClearActionTag();
}

bool UActionComponent::CanMove() const {

	// HasTag로 런타임 태그에 이동을 방해하는 태그가 있는지 확인

	return !CurrentState.HasTag(FGameplayTag::RequestGameplayTag("Stunned")) &&
		   !CurrentState.HasTag(FGameplayTag::RequestGameplayTag("LockMovement"));

	
	
}

void UActionComponent::UpdateMovementState(float WorldSeconds) {

	CurrentState.EnterState(CanMove() ? EPlayerState::Locomotion : EPlayerState::Idle, FGameplayTag(), WorldSeconds);

}

FActionDecision UActionComponent::EvaluateActionBP(FGameplayTag ActionTag, float WorldSeconds) {
	return ActionEvaluator::EvaluateAction(CurrentState, ActionTag, ActionRuleSet, WorldSeconds);
}

void UActionComponent::BroadcastEvaluationResult(FActionDecision Decision) {
	OnEvaluationResult.Broadcast(Decision);
}

void UActionComponent::DebugPrint(const FString& Message) {

	UE_LOG(LogTemp, Warning, TEXT("[CompDbg] %s"), *Message);

	if (AActor* Owner = GetOwner()) {
		UKismetSystemLibrary::PrintString(
			Owner,
			Message,
			true,
			true,
			FLinearColor::Red,
			2.0f
		);
	}

}

EPlayerState UActionComponent::ActionTagToState(const FGameplayTag& ActionTag) {
	if (ActionTag == FGameplayTag::RequestGameplayTag("ActionTag.Knight.Attack1")) return EPlayerState::Attack;
	if (ActionTag == FGameplayTag::RequestGameplayTag("ActionTag.Mage.Attack1")) return EPlayerState::Attack;
    else return EPlayerState::Idle;
}