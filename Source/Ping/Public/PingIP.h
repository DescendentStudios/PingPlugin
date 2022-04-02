#pragma once
#include "UObject/Object.h"
#include "Engine/Engine.h"



#include "PingIP.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPingComplete, class UPingIP*, PingOperation, FString, Hostname, int32, TimeMS);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPingFailure, class UPingIP*, PingOperation, FString, Hostname);

DECLARE_LOG_CATEGORY_EXTERN(LogPing, Log, All);

class IPingThread;
class FRunnableThread;

/**
*	Container class for ping functionality.  Acts as interface between blueprint and ping code
*/
UCLASS(Blueprintable, BlueprintType, Config = Game)
class PING_API UPingIP : public UObject
{
	GENERATED_UCLASS_BODY()

private:
	FString TargetHost;
	int32 EchoTimeMs;
	FRunnableThread* PingThread;
	IPingThread* PingThreadObject;

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

	UFUNCTION(BlueprintCallable, Category = "Ping", Meta = (DeprecatedFunction, DeprecationMessage = "Polling has been made redundant and does nothing - remove your call to this function when convenient."))
		void PollThread() { }

	~UPingIP();
};
