#pragma once

#include "HAL/Platform.h"

#if PLATFORM_MAC | PLATFORM_LINUX

#include "IPingThread.h"

class MacLinuxPingThread : public IPingThread
{
public:
	MacLinuxPingThread(FString const & host, UPingIP * pingOp) : IPingThread(host, pingOp) {}

	// FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

	virtual FRunnableThread* StartThread();
};

#endif