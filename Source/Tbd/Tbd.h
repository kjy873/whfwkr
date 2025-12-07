// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct PacketHeader
{
	uint16 size;
	uint16 id;
};

class SendBuffer : public TSharedFromThis<SendBuffer>
{
public:
	SendBuffer(int32 bufferSize);
	~SendBuffer();

	BYTE* Buffer() { return _buffer.GetData(); }
	int32 WriteSize() { return _writeSize; }
	int32 Capacity() { return static_cast<int32>(_buffer.Num()); }

	void CopyData(void* data, int32 len);
	void Close(uint32 writeSize);

private:
	TArray<BYTE>	_buffer;
	int32			_writeSize = 0;
};

#define USING_SHARED_PTR(name)	using name##Ref = TSharedPtr<class name>;

USING_SHARED_PTR(Session);
USING_SHARED_PTR(PacketSession);
USING_SHARED_PTR(SendBuffer);

#include "ClientPacketHandler.h"
#include "MainGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

#define SEND_PACKET(pkt)																				\
	SendBufferRef SendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);						\
	Cast<UMainGameInstance>(GWorld->GetGameInstance())->SendPacket(SendBuffer);