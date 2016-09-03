#pragma once

#include "Object.h"
#include "Engine.h"
#include "PingIP.h"

class IPingThread : public FRunnable
{
protected:
	volatile int32* ThreadComplete;
	volatile int32* PingTime;
	FString Hostname;

public:
	IPingThread() { ThreadComplete = NULL; PingTime = NULL; Hostname = ""; };
	IPingThread(volatile int32* completeFlag, volatile int32* time, FString host) : ThreadComplete(completeFlag), PingTime(time), Hostname(host) {};

	// FRunnable interface
	virtual bool Init() = 0;
	virtual uint32 Run() = 0;
	virtual void Stop() = 0;

	virtual FRunnableThread* StartThread() = 0;
};