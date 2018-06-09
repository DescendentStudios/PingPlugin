#pragma once

#include "HAL/Platform.h"

#if PLATFORM_MAC | PLATFORM_LINUX

#include "IPingThread.h"

class MacLinuxPingThread : public IPingThread
{
private:
    bool RunBashCommand(FString command, int timeoutSeconds, int32* outReturnCode, FString* outStdOut, FString* outStdErr) const;
    FString WhichPing() const;
    
public:
	MacLinuxPingThread(FString const & host, UPingIP * pingOp) : IPingThread(host, pingOp) {}

	// FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

	virtual FRunnableThread* StartThread();
};

#endif