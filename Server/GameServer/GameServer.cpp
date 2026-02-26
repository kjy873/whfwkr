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
	
	// 서버 시작 시 Room 초기화 및 ID 생성기 초기화
	GRoom = make_shared<Room>();
	GRoom->Init();
	GRoom->Clear();
	ObjectUtils::ResetIdGenerator();

	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(L"0.0.0.0", 7777),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); }, 100);

	ASSERT_CRASH(service->Start());

	if (!service->Start())
	{
		cout<< "Service Start Failed!" << endl;
		return -1;
	}
	cout << "Server Start!" << endl;

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([&service]()
			{
				DoWorkerJob(service);
			});
	}

	while (true)
	{
		this_thread::sleep_for(1s);
	}

	GThreadManager->Join();
}