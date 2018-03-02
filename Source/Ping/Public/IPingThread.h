#pragma once

#include "HAL/Runnable.h"
#include "Async/TaskGraphInterfaces.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "PingIP.h"


class UPingIP;

DECLARE_STATS_GROUP(TEXT("PingPlugin"), STATGROUP_PingPlugin, STATCAT_Advanced);


class IPingThread : public FRunnable
{
protected:
	FString const Hostname;
private:
	TWeakObjectPtr<UPingIP> PingOperation; // Only access from Game Thread.

public:
	IPingThread() : Hostname(), PingOperation(nullptr) {}
	IPingThread(FString const & host, UPingIP * pingOp) : Hostname(host), PingOperation(pingOp) {}

	virtual FRunnableThread* StartThread() = 0;

protected:
	// Safe to call from any thread.
	void ReturnResultToGameThread (bool bSuccess, int32 TimeMS = -1);
};