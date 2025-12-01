#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Protocol.pb.h"
#include "PlayerCharacter.generated.h"

UCLASS()
class TBD_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();
	virtual ~APlayerCharacter();

protected:
	virtual void BeginPlay();
	virtual void Tick(float DeltaSeconds) override;

public:
	bool IsMyPlayer();

public:
	void SetPlayerInfo(const Protocol::PlayerInfo& Info);
	void SetDestInfo(const Protocol::PlayerInfo& Info);
	Protocol::PlayerInfo* GetPlayerInfo() { return PlayerInfo; }

protected:
	class Protocol::PlayerInfo* PlayerInfo; // 현재 위치
	class Protocol::PlayerInfo* DestInfo; // 목적지
};