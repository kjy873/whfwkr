// Fill out your copyright notice in the Description page of Project Settings.


#include "MainGameInstance.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "PacketSession.h"
#include "Protocol.pb.h"
#include "ClientPacketHandler.h"
#include "Player/MyPlayerCharacter.h"
#include "Player/UC_NetworkPlayerComponent.h"

void UMainGameInstance::ConnectToGameServer()
{
	Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(TEXT("Stream"), TEXT("Client Socket"));

	FIPv4Address Ip;
	FIPv4Address::Parse(IpAddress, Ip);

	TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddr->SetIp(Ip.Value);
	InternetAddr->SetPort(Port);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connecting To Server...")));

	bool Connected = Socket->Connect(*InternetAddr);

	if (Connected)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connection Success")));

		// Session
		GameServerSession = MakeShared<PacketSession>(Socket);
		GameServerSession->Run();

		{
			Protocol::C_LOGIN Pkt;
			SendBufferRef SendBuffer = ClientPacketHandler::MakeSendBuffer(Pkt);
			SendPacket(SendBuffer);
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connection Failed")));
	}
}

void UMainGameInstance::DisconnectToGameServer()
{

	if ( Socket == nullptr || GameServerSession == nullptr )
		return;

	Protocol::C_LEAVE_GAME LeavePkt;
	SEND_PACKET(LeavePkt);
}

void UMainGameInstance::HandleRecvPackets()
{
	if ( Socket == nullptr || GameServerSession == nullptr)
		return;
	GameServerSession->HandleRecvPackets();
}

void UMainGameInstance::SendPacket(SendBufferRef SendBuffer)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	GameServerSession->SendPacket(SendBuffer);
}

void UMainGameInstance::HandleSpawn(const Protocol::PlayerInfo& PlayerInfo, bool IsMine)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	const uint64 ObjectId = PlayerInfo.object_id();
	if (Players.Find(ObjectId) != nullptr)
		return;

	FVector SpawnLocation(PlayerInfo.x(), PlayerInfo.y(), PlayerInfo.z());

	if (IsMine)
	{
		auto PC = UGameplayStatics::GetPlayerController(this, 0);
		APlayerCharacter* Player = Cast<APlayerCharacter>(PC->GetPawn());
		if (Player == nullptr)
			return;

		MyPlayer = Player;
		Players.Add(PlayerInfo.object_id(), Player);

		if (UC_NetworkPlayerComponent* NetComp = Player->FindComponentByClass<UC_NetworkPlayerComponent>())
		{
			NetComp->SetObjectId(PlayerInfo.object_id());
			UE_LOG(LogTemp, Warning, TEXT("My ObjectId Set: %llu"), PlayerInfo.object_id());
		}
	}
	else
	{
		APlayerCharacter* Player = Cast<APlayerCharacter>(World->SpawnActor(OtherPlayerClass, &SpawnLocation));

		Players.Add(PlayerInfo.object_id(), Player);
	}
}

void UMainGameInstance::HandleSpawn(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
	HandleSpawn(EnterGamePkt.player(), true);
}

void UMainGameInstance::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
	for (auto& Player : SpawnPkt.players())
	{
		HandleSpawn(Player, false);
	}
}

void UMainGameInstance::HandleDespawn(uint64 ObjectId)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	APlayerCharacter** FindActor = Players.Find(ObjectId);
	if (FindActor == nullptr)
		return;

	World->DestroyActor(*FindActor);
}

void UMainGameInstance::HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt)
{
	for (auto& ObjectId : DespawnPkt.object_ids())
	{
		HandleDespawn(ObjectId);
	}
}
