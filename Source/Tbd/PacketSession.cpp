// Fill out your copyright notice in the Description page of Project Settings.


#include "PacketSession.h"
#include "NetworkWorker.h"

PacketSession::PacketSession(class FSocket* Socket) : Socket(Socket)
{
}

PacketSession::~PacketSession()
{
	Disconnect();
}

void PacketSession::Run()
{
	RecvWorkerThread = MakeShared<RecvWorker>(Socket, AsShared());
}

void PacketSession::Recv()
{
	
}

void PacketSession::Disconnect()
{
	
}