#include "PingPrivatePCH.h"
#include "PingIP.h"

DEFINE_LOG_CATEGORY(LogPing);

UPingIP::UPingIP(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	EchoThreadComplete = 0;
	EchoTimeMs = 0;
}

void UPingIP::SendPing(FString hostname)
{
	TargetHost = hostname;

	PingThreadObject = new PingThreadType(&EchoThreadComplete, &EchoTimeMs, TargetHost);
	PingThread = PingThreadObject->StartThread();
	if (PingThread == NULL)
	{
		UE_LOG(LogPing, Error, TEXT("Unable to start ping thread."));
		ProcessEcho(-1, false);
	}
}

void UPingIP::ProcessEcho(int32 time, bool success)
{
	if (success)
	{
		UE_LOG(LogPing, VeryVerbose, TEXT("Ping successful."));
		OnPingComplete.Broadcast(this, TargetHost, time);
	}
	else
	{
		UE_LOG(LogPing, VeryVerbose, TEXT("Ping failed."));
		OnPingFailure.Broadcast(this, TargetHost);
	}
}

void UPingIP::PollThread()
{
	//UE_LOG(LogPing, VeryVerbose, TEXT("Polling."));
	if (EchoThreadComplete)
	{
		if (EchoThreadComplete == 1)
		{
			ProcessEcho(EchoTimeMs, true);
		}
		else
		{
			ProcessEcho(-1, false);
		}
		EchoThreadComplete = 0;
	}
}

UPingIP::~UPingIP()
{
	delete PingThread;
	delete PingThreadObject;
}