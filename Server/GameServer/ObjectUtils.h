#pragma once

class ObjectUtils
{
public:
	static PlayerRef CreatePlayer(GameSessionRef session);
	static void ResetIdGenerator(); // ID 생성기 초기화

private:
	static atomic<uint64> s_idGenerator;
};

