#pragma once

class ObjectUtils
{
public:
	static PlayerRef CreatePlayer(GameSessionRef session);

private:
	static atomic<uint64> s_idGenerator;
};

