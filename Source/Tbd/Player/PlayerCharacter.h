#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Protocol.pb.h"
#include "PlayerCharacter.generated.h"

UCLASS()
class TBD_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();
	virtual ~APlayerCharacter() override;

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	Protocol::PlayerInfo PlayerInfo;
	Protocol::PlayerInfo DestInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
	bool bIsMine = false;

	void SetPlayerInfo(const Protocol::PlayerInfo& Info);
	void SetDestInfo(const Protocol::PlayerInfo& Info);

	UFUNCTION(BlueprintCallable)
	bool IsMyPlayer() const { return bIsMine; }

private:
	void ApplyNetworkPosition(float DeltaTime);
};