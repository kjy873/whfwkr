#include "Player/UC_NetworkPlayerComponent.h"
#include "MainGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UC_NetworkPlayerComponent::UC_NetworkPlayerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UC_NetworkPlayerComponent::BeginPlay()
{
    Super::BeginPlay();

    // GameInstance 캐싱
    CachedGameInstance = Cast<UMainGameInstance>(GetWorld()->GetGameInstance());
    if (CachedGameInstance == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("UC_NetworkPlayerComponent: GameInstance 캐스팅 실패!"));
        return;
    }

    // 50ms마다 자동으로 내 위치 전송
    GetWorld()->GetTimerManager().SetTimer(
        MoveSendTimer,
        this,
        &UC_NetworkPlayerComponent::SendMyMovement,
        MoveSendInterval,
        true
    );
}

void UC_NetworkPlayerComponent::SendMyMovement()
{
    if (MyObjectId == 0)
        return; // 아직 내 object_id 할당 안됨

    AActor* Owner = GetOwner();
    if (!Owner)
        return;

    FVector Location = Owner->GetActorLocation();
    float Yaw = Owner->GetActorRotation().Yaw;

    SendMoveToServer(Location, Yaw);
}

void UC_NetworkPlayerComponent::SendMoveToServer(const FVector& Location, float Yaw)
{
    if (CachedGameInstance == nullptr)
        return;
    
    if (MyObjectId == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ObjectId not set"));
        return;
    }

    // 패킷 생성
    Protocol::C_MOVE MovePkt;
    Protocol::PlayerInfo* Info = MovePkt.mutable_info();

    Info->set_object_id(MyObjectId);
    Info->set_x(Location.X);
    Info->set_y(Location.Y);
    Info->set_z(Location.Z);
    Info->set_yaw(Yaw);

    // 패킷 송신
    SEND_PACKET(MovePkt);
}