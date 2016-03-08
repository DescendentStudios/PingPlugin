#pragma once
#include "Object.h"
#include "Engine.h"

#include "IPingThread.h"
#if PLATFORM_WINDOWS
#include "WinPingThread.h"
#elif PLATFORM_MAC | PLATFORM_LINUX
#include "MacLinuxPingThread.h"
#endif

// begin
// see https://answers.unrealengine.com/questions/38930/cannot-open-include-files.html?sort=oldest
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include <istream>
#include <iostream>
#include <ostream>
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
// end"

#include "PingIP.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPingComplete, class UPingIP*, PingOperation, FString, Hostname, int32, TimeMS);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPingFailure, class UPingIP*, PingOperation, FString, Hostname);

DECLARE_LOG_CATEGORY_EXTERN(LogPing, Log, All);

#if PLATFORM_WINDOWS
typedef class WinPingThread PingThreadType;
#endif

/**
*	Container class for ping functionality.  Acts as interface between blueprint and ASIO code
*/
UCLASS(Blueprintable, Config = Game)
class UPingIP : public UObject
{
	GENERATED_UCLASS_BODY()

private:
	FString TargetHost;
	volatile int32 EchoThreadComplete; //using volatile int32 instead of bool to exploit atomic increment
	volatile int32 EchoTimeMs;
	FRunnableThread* PingThread;

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