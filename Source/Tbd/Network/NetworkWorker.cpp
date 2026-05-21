// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/NetworkWorker.h"
#include "Sockets.h"
#include "Serialization/ArrayWriter.h"
#include "PacketSession.h"

RecvWorker::RecvWorker(FSocket* Socket, TSharedPtr<class PacketSession> Session) : Socket(Socket), SessionRef(Session)
{
	Thread = FRunnableThread::Create(this, TEXT("RecvWorker Thread"));
}

RecvWorker::~RecvWorker()
{

}

bool RecvWorker::Init()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Recv Thread Init"));

	return true;
}

uint32 RecvWorker::Run()
{
	while (Running)
	{
		TArray<uint8> Packet;

		if (ReceivePacket(OUT Packet))
		{
			if (TSharedPtr<PacketSession> Session = SessionRef.Pin())
			{
				if (Packet.Num() >= sizeof(PacketHeader))
				{
					PacketHeader* Header = reinterpret_cast<PacketHeader*>(Packet.GetData());

					UE_LOG(LogTemp, Warning, TEXT("[RecvWorker Enqueue] Session=%p PacketID=%d Size=%d"),
						Session.Get(),
						Header->id,
						Header->size);
				}

				Session->RecvPacketQueue.Enqueue(Packet);

				TWeakPtr<PacketSession> WeakSession = SessionRef;

				AsyncTask(ENamedThreads::GameThread, [WeakSession]()
					{
						if (TSharedPtr<PacketSession> PinnedSession = WeakSession.Pin())
						{
							PinnedSession->HandleRecvPackets();
						}
					});
			}
		}

		FPlatformProcess::Sleep(0.001f);
	}

	return 0;
}

void RecvWorker::Exit()
{
}

void RecvWorker::Destroy()
{
	Running = false;
}

bool RecvWorker::ReceivePacket(TArray<uint8>& OutPacket)
{
	const int32 HeaderSize = sizeof(PacketHeader);

	TArray<uint8> HeaderBuffer;
	HeaderBuffer.AddZeroed(HeaderSize);

	if (ReceiveDesiredBytes(HeaderBuffer.GetData(), HeaderSize) == false)
		return false;

	PacketHeader* Header = reinterpret_cast<PacketHeader*>(HeaderBuffer.GetData());

	const uint16 PacketSize = Header->size;
	const uint16 PacketId = Header->id;

	if (PacketSize < HeaderSize)
	{
		UE_LOG(LogTemp, Error, TEXT("[RecvWorker] Invalid packet size. id=%d size=%d headerSize=%d"),
			PacketId, PacketSize, HeaderSize);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[RecvWorker] Recv PacketID=%d PacketSize=%d"),
		PacketId, PacketSize);

	OutPacket = HeaderBuffer;

	const int32 PayloadSize = PacketSize - HeaderSize;

	if (PayloadSize > 0)
	{
		OutPacket.AddZeroed(PayloadSize);

		if (ReceiveDesiredBytes(OutPacket.GetData() + HeaderSize, PayloadSize) == false)
			return false;
	}

	return true;
}

bool RecvWorker::ReceiveDesiredBytes(uint8* Results, int32 Size)
{
	int32 Offset = 0;

	while (Size > 0 && Running)
	{
		uint32 PendingDataSize = 0;

		if (Socket->HasPendingData(PendingDataSize) == false || PendingDataSize <= 0)
		{
			FPlatformProcess::Sleep(0.001f);
			continue;
		}

		const int32 BytesToRead = FMath::Min<int32>(Size, PendingDataSize);

		int32 NumRead = 0;
		if (Socket->Recv(Results + Offset, BytesToRead, OUT NumRead) == false)
		{
			return false;
		}

		if (NumRead <= 0)
		{
			FPlatformProcess::Sleep(0.001f);
			continue;
		}

		Offset += NumRead;
		Size -= NumRead;
	}

	return Size == 0;
}

SendWorker::SendWorker(FSocket* Socket, TSharedPtr<PacketSession> Session) : Socket(Socket), SessionRef(Session)
{
	Thread = FRunnableThread::Create(this, TEXT("SendWorkerThread"));
}

SendWorker::~SendWorker()
{

}

bool SendWorker::Init()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Send Thread Init")));

	return true;
}

uint32 SendWorker::Run()
{
	while (Running)
	{
		SendBufferRef SendBuffer;

		if (TSharedPtr<PacketSession> Session = SessionRef.Pin())
		{
			if (Session->SendPacketQueue.Dequeue(OUT SendBuffer))
			{
				SendPacket(SendBuffer);
			}
		}
	}

	return 0;
}

void SendWorker::Exit()
{

}

bool SendWorker::SendPacket(SendBufferRef SendBuffer)
{
	if (SendDesiredBytes(SendBuffer->Buffer(), SendBuffer->WriteSize()) == false)
		return false;

	return true;
}

void SendWorker::Destroy()
{
	Running = false;
}

bool SendWorker::SendDesiredBytes(const uint8* Buffer, int32 Size)
{
	while (Size > 0)
	{
		int32 BytesSent = 0;
		if (Socket->Send(Buffer, Size, BytesSent) == false)
			return false;

		Size -= BytesSent;
		Buffer += BytesSent;
	}

	return true;
}
