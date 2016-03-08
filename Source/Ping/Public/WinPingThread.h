#pragma once
#if PLATFORM_WINDOWS
#include "Object.h"
#include "Engine.h"
#include "IPingThread.h"

class WinPingThread : public IPingThread
{
public:
	WinPingThread(volatile int32* completeFlag, volatile int32* time, FString host) : IPingThread(completeFlag, time, host) {};

	// FRunnable interface
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();

	virtual FRunnableThread* StartThread();
};

#endif