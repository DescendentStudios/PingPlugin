#pragma once
#if PLATFORM_MAC | PLATFORM_LINUX
#include "Object.h"
#include "Engine.h"
#include "IPingThread.h"

class MacLinuxPingThread : public IPingThread
{
private:
    bool RunBashCommand(FString command, int timeoutSeconds, int32* outReturnCode, FString* outStdOut, FString* outStdErr) const;
    FString WhichPing() const;
    
public:
	MacLinuxPingThread(volatile int32* completeFlag, volatile int32* time, FString host) : IPingThread(completeFlag, time, host) {};

	// FRunnable interface
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();

	virtual FRunnableThread* StartThread();
};

#endif