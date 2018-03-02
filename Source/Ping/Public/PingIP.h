#pragma once
#include "Object.h"
#include "Engine.h"

#include "IPingThread.h"
#if PLATFORM_WINDOWS
#include "WinPingThread.h"
#elif PLATFORM_MAC | PLATFORM_LINUX
#include "MacLinuxPingThread.h"
#else
#error "Platform is not supported"
#endif


#include "PingIP.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPingComplete, class UPingIP*, PingOperation, FString, Hostname, int32, TimeMS);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPingFailure, class UPingIP*, PingOperation, FString, Hostname);

DECLARE_LOG_CATEGORY_EXTERN(LogPing, Log, All);

#if PLATFORM_WINDOWS
typedef class WinPingThread PingThreadType;
#elif PLATFORM_MAC | PLATFORM_LINUX
typedef class MacLinuxPingThread PingThreadType;
#endif

/**
*	Container class for ping functionality.  Acts as interface between blueprint and ping code
*/
UCLASS(Blueprintable, BlueprintType, Config = Game)
class PING_API UPingIP : public UObject
{
	GENERATED_UCLASS_BODY()

private:
	FString TargetHost;
	volatile int32 EchoThreadComplete; //using volatile int32 instead of bool to exploit atomic increment
	volatile int32 EchoTimeMs;
	FRunnableThread* PingThread;
	PingThreadType* PingThreadObject;

public:
	UFUNCTION(BlueprintCallable, Category = "Ping")
	void SendPing(FString ipAddress);

	void ProcessEcho(int32 time, bool success);

	/** Event occured when the echo has been recieved */
	UPROPERTY(BlueprintAssignable, Category = "Ping")
		FOnPingComplete OnPingComplete;

	/** Event occured when the echo has NOT been recieved */
	UPROPERTY(BlueprintAssignable, Category = "Ping")
		FOnPingFailure OnPingFailure;

	UFUNCTION(BlueprintCallable, Category = "Ping")
		void PollThread();

	~UPingIP();
};
