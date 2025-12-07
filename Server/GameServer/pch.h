#pragma once

#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

#ifdef _DEBUG
#pragma comment(lib, "ServerCore\\Debug\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "ServerCore\\Release\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif

#include "CorePch.h"

#include "Protocol.pb.h"
#include "Enum.pb.h"
#include "Struct.pb.h"
#include "ServerPacketHandler.h"
#include "Utils.h"

USING_SHARED_PTR(GameSession);
USING_SHARED_PTR(Player);
USING_SHARED_PTR(Room);

using GameSessionRef = shared_ptr<class GameSession>;

#define SEND_PACKET(pkt)																	\
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);			\
	session->Send(sendBuffer);