
#include "PingIP.h"
#include "PingPrivatePCH.h"
#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
#include "WinPingThread.h"
#elif PLATFORM_MAC | PLATFORM_LINUX
#include "MacLinuxPingThread.h"
#else
#error "Platform is not supported"
#endif


#if PLATFORM_WINDOWS
typedef WinPingThread PingThreadType;
#elif PLATFORM_MAC | PLATFORM_LINUX
typedef MacLinuxPingThread PingThreadType;
#endif

DEFINE_LOG_CATEGORY(LogPing);

UPingIP::UPingIP(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UPingIP::SendPing(FString hostname)
{
	TargetHost = hostname;

	PingThreadObject = new PingThreadType(TargetHost, this);
	PingThread = PingThreadObject->StartThread();
	if (PingThread == NULL)
	{
		UE_LOG(LogPing, Error, TEXT("Unable to start ping thread."));
		ProcessEcho(-1, false);
	}
}

void UPingIP::ProcessEcho(int32 time, bool success)
{
	check(IsInGameThread());

	EchoTimeMs = time;

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

UPingIP::~UPingIP()
{
	delete PingThread;
	delete PingThreadObject;
}