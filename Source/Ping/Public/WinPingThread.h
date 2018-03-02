#pragma once

#include "HAL/Platform.h"

#if PLATFORM_WINDOWS

#include "IPingThread.h"

class WinPingThread : public IPingThread
{
public:
	WinPingThread(FString const & host, UPingIP * pingOp) : IPingThread(host, pingOp) {}

	// FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

	virtual FRunnableThread* StartThread();
};

#endif