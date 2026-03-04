#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "Room.h"
#include "ObjectUtils.h"
#include <tchar.h>
#include "Job.h"
#include "RoomManager.h"
#include "Protocol.pb.h"

enum
{
	WORKER_TICK = 64
};

void DoWorkerJob(ServerServiceRef& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->GetIocpCore()->Dispatch(10);

		// 예약된 일감 처리
		ThreadManager::DistributeReservedJobs();

		// 글로벌 큐
		ThreadManager::DoGlobalQueueWork();
	}
}

int main()
{
	/*
	try 
	{ 
		sql::mysql::MySQL_Driver* driver; 
		std::unique_ptr<sql::Connection> con; 
		driver = sql::mysql::get_mysql_driver_instance(); 
		con.reset(driver->connect("tcp://127.0.0.1:3306", "game", "game1")); 
		con->setSchema("tbd_game"); 
		std::cout << "DB CONNECT SUCCESS" << std::endl; 
	}
	catch (sql::SQLException& e) 
	{ 
		std::cout << "SQL ERROR: " << e.what() << std::endl; 
		std::cout << "ErrorCode: " << e.getErrorCode() << std::endl; 
		std::cout << "SQLState: " << e.getSQLState() << std::endl; 
	}
	*/

	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	ServerPacketHandler::Init();
	ObjectUtils::ResetIdGenerator();

	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(L"0.0.0.0", 7777),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); }, 100);

	if (service->Start())
	{
		cout << "Server Start! (Port: 7777)" << endl;
	}
	else
	{
		// 시작 실패 시 여기서 크래시를 내거나 종료
		ASSERT_CRASH(false);
		return -1;
	}

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([service]()
			{
				DoWorkerJob(const_cast<ServerServiceRef&>(service));
			});
	}

	GThreadManager->Launch([]()
		{
			cout << "Game Logic Thread Started!" << endl;
			uint64 lastTick = GetTickCount64();
			while (true)
			{
				uint64 currentTick = GetTickCount64();
				float deltaTime = (currentTick - lastTick) / 1000.0f;

				if (deltaTime > 0.0f)
				{
					lastTick = currentTick;

					GRoomManager.Update(deltaTime);
				}
				this_thread::sleep_for(10ms);
			}
		});

	GThreadManager->Join();
}