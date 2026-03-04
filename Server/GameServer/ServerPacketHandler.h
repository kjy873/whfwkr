#pragma once
#include "Protocol.pb.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
#include "Tbd.h"
#endif

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
	PKT_C_LOGIN = 1000,
	PKT_S_LOGIN = 1001,
	PKT_C_ENTER_GAME = 1002,
	PKT_S_ENTER_GAME = 1003,
	PKT_C_LEAVE_GAME = 1004,
	PKT_S_LEAVE_GAME = 1005,
	PKT_S_SPAWN = 1006,
	PKT_S_DESPAWN = 1007,
	PKT_C_MOVE = 1008,
	PKT_S_MOVE = 1009,
	PKT_C_CHAT = 1010,
	PKT_S_CHAT = 1011,
	PKT_C_ATTACK_PLAYER = 1012,
	PKT_S_DAMAGE_PLAYER = 1013,
	PKT_S_PLAYER_DEAD = 1014,
	PKT_S_SPAWN_MOB = 1015,
	PKT_S_DESPAWN_MOB = 1016,
	PKT_S_MOVE_MOB = 1017,
	PKT_C_MOVE_MOB = 1018,
	PKT_C_ATTACK_MOB = 1019,
	PKT_C_USE_SKILL = 1020,
	PKT_S_USE_SKILL = 1021,
	PKT_S_DAMAGE_MOB = 1022,
	PKT_S_PROJECTILE_HIT = 1023,
	PKT_S_PROJECTILE_DESTROY = 1024,
	PKT_S_CHANGE_LEVEL = 1025,
};

// Custom Handlers
bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt);
bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt);
bool Handle_C_LEAVE_GAME(PacketSessionRef& session, Protocol::C_LEAVE_GAME& pkt);
bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt);
bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt);
bool Handle_C_ATTACK_PLAYER(PacketSessionRef& session, Protocol::C_ATTACK_PLAYER& pkt);
bool Handle_C_MOVE_MOB(PacketSessionRef& session, Protocol::C_MOVE_MOB& pkt);
bool Handle_C_ATTACK_MOB(PacketSessionRef& session, Protocol::C_ATTACK_MOB& pkt);
bool Handle_C_USE_SKILL(PacketSessionRef& session, Protocol::C_USE_SKILL& pkt);

class ServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_C_LOGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LOGIN>(Handle_C_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_C_ENTER_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ENTER_GAME>(Handle_C_ENTER_GAME, session, buffer, len); };
		GPacketHandler[PKT_C_LEAVE_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LEAVE_GAME>(Handle_C_LEAVE_GAME, session, buffer, len); };
		GPacketHandler[PKT_C_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MOVE>(Handle_C_MOVE, session, buffer, len); };
		GPacketHandler[PKT_C_CHAT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_CHAT>(Handle_C_CHAT, session, buffer, len); };
		GPacketHandler[PKT_C_ATTACK_PLAYER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ATTACK_PLAYER>(Handle_C_ATTACK_PLAYER, session, buffer, len); };
		GPacketHandler[PKT_C_MOVE_MOB] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MOVE_MOB>(Handle_C_MOVE_MOB, session, buffer, len); };
		GPacketHandler[PKT_C_ATTACK_MOB] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ATTACK_MOB>(Handle_C_ATTACK_MOB, session, buffer, len); };
		GPacketHandler[PKT_C_USE_SKILL] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_USE_SKILL>(Handle_C_USE_SKILL, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::S_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_S_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_ENTER_GAME& pkt) { return MakeSendBuffer(pkt, PKT_S_ENTER_GAME); }
	static SendBufferRef MakeSendBuffer(Protocol::S_LEAVE_GAME& pkt) { return MakeSendBuffer(pkt, PKT_S_LEAVE_GAME); }
	static SendBufferRef MakeSendBuffer(Protocol::S_SPAWN& pkt) { return MakeSendBuffer(pkt, PKT_S_SPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_DESPAWN& pkt) { return MakeSendBuffer(pkt, PKT_S_DESPAWN); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MOVE& pkt) { return MakeSendBuffer(pkt, PKT_S_MOVE); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHAT& pkt) { return MakeSendBuffer(pkt, PKT_S_CHAT); }
	static SendBufferRef MakeSendBuffer(Protocol::S_DAMAGE_PLAYER& pkt) { return MakeSendBuffer(pkt, PKT_S_DAMAGE_PLAYER); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PLAYER_DEAD& pkt) { return MakeSendBuffer(pkt, PKT_S_PLAYER_DEAD); }
	static SendBufferRef MakeSendBuffer(Protocol::S_SPAWN_MOB& pkt) { return MakeSendBuffer(pkt, PKT_S_SPAWN_MOB); }
	static SendBufferRef MakeSendBuffer(Protocol::S_DESPAWN_MOB& pkt) { return MakeSendBuffer(pkt, PKT_S_DESPAWN_MOB); }
	static SendBufferRef MakeSendBuffer(Protocol::S_MOVE_MOB& pkt) { return MakeSendBuffer(pkt, PKT_S_MOVE_MOB); }
	static SendBufferRef MakeSendBuffer(Protocol::S_USE_SKILL& pkt) { return MakeSendBuffer(pkt, PKT_S_USE_SKILL); }
	static SendBufferRef MakeSendBuffer(Protocol::S_DAMAGE_MOB& pkt) { return MakeSendBuffer(pkt, PKT_S_DAMAGE_MOB); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PROJECTILE_HIT& pkt) { return MakeSendBuffer(pkt, PKT_S_PROJECTILE_HIT); }
	static SendBufferRef MakeSendBuffer(Protocol::S_PROJECTILE_DESTROY& pkt) { return MakeSendBuffer(pkt, PKT_S_PROJECTILE_DESTROY); }
	static SendBufferRef MakeSendBuffer(Protocol::S_CHANGE_LEVEL& pkt) { return MakeSendBuffer(pkt, PKT_S_CHANGE_LEVEL); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
		SendBufferRef sendBuffer = MakeShared<SendBuffer>(packetSize);
#else
		SendBufferRef sendBuffer = make_shared<SendBuffer>(packetSize);
#endif

		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		pkt.SerializeToArray(&header[1], dataSize);
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};