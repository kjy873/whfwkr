// Fill out your copyright notice in the Description page of Project Settings.


#include "PacketSession.h"
#include "NetworkWorker.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "ClientPacketHandler.h"


PacketSession::PacketSession(class FSocket* Socket) : Socket(Socket)
{
	ClientPacketHandler::Init();
	UE_LOG(LogTemp, Error, TEXT("Ŭ���̾�Ʈ��Ŷ�ڵ鷯 init"));
}

PacketSession::~PacketSession()
{
	Disconnect();
	UE_LOG(LogTemp, Error, TEXT("Ŭ���̾�Ʈ��Ŷ�ڵ鷯 disconnect"));
}

void PacketSession::Run()
{
	RecvWorkerThread = MakeShared<RecvWorker>(Socket, AsShared());
	SendWorkerThread = MakeShared<SendWorker>(Socket, AsShared());
	UE_LOG(LogTemp, Error, TEXT("��Ŷ���� ��"));
}

void PacketSession::HandleRecvPackets()
{
	int32 PacketCount = 0;
	while (true)
	{
		TArray<uint8> Packet;
		if (RecvPacketQueue.Dequeue(OUT Packet) == false)
			break;
		
		PacketCount++;
		PacketSessionRef ThisPtr = AsShared();
		
		if (Packet.Num() >= sizeof(PacketHeader))
		{
			PacketHeader* Header = reinterpret_cast<PacketHeader*>(Packet.GetData());
			UE_LOG(LogTemp, Warning, TEXT("[PacketSession::HandleRecvPackets] Processing packet ID=%d, Size=%d"), 
				Header->id, Header->size);
		}
		
		ClientPacketHandler::HandlePacket(ThisPtr, Packet.GetData(), Packet.Num());
	}
	
	if (PacketCount > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PacketSession::HandleRecvPackets] Processed %d packets"), PacketCount);
	}
}

void PacketSession::SendPacket(SendBufferRef SendBuffer)
{
	SendPacketQueue.Enqueue(SendBuffer);
}

void PacketSession::Disconnect()
{
	if (RecvWorkerThread)
	{
		RecvWorkerThread->Destroy();
		RecvWorkerThread = nullptr;
	}

	if (SendWorkerThread)
	{
		SendWorkerThread->Destroy();
		SendWorkerThread = nullptr;
	}
}
