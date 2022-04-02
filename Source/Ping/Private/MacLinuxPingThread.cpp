

#if PLATFORM_MAC | PLATFORM_LINUX

#include "MacLinuxPingThread.h"
#include "PingPrivatePCH.h"
#include "HAL/Platform.h"
#include "Misc/ScopeExit.h"
#include <spawn.h> // see manpages-posix-dev
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <sys/wait.h>
#include <time.h>

bool MacLinuxPingThread::Init()
{
	return true;
}

FRunnableThread* MacLinuxPingThread::StartThread()
{
	return FRunnableThread::Create(this, TEXT("MacLinuxPingThread"), 0, TPri_Normal);
}

bool MacLinuxPingThread::RunBashCommand(FString command, int timeoutSeconds, int32* outReturnCode, FString* outStdOut, FString* outStdErr) const
{
    FString executableFileName = TEXT("/bin/bash");
    FString cmdLineParams = TEXT("-l -c \"") + command + TEXT("\"");
    int32 returnCode = -1;
    FString defaultError;
    if (!outStdErr)
    {
        outStdErr = &defaultError;
    }
    
    void* pipeRead = nullptr;
    void* pipeWrite = nullptr;
    verify(FPlatformProcess::CreatePipe(pipeRead, pipeWrite));
    
    bool wasInvoked = false;
    
    const bool launchDetached = true;
    const bool launchHidden = true;
    const bool launchReallyHidden = true;
    bool didTimeout = false;
    
    FProcHandle procHandle = FPlatformProcess::CreateProc(*executableFileName, *cmdLineParams, launchDetached, launchHidden, launchReallyHidden, NULL, 0, NULL, pipeWrite);
    if (procHandle.IsValid())
    {
        time_t startTime = std::time(nullptr);
        while (FPlatformProcess::IsProcRunning(procHandle))
        {
            if ((didTimeout = difftime(std::time(nullptr), startTime) >= timeoutSeconds))
            {
                UE_LOG(LogPing, Error, TEXT("'%s' timed out."), *command);
                break;
            }
            
            FString newLine = FPlatformProcess::ReadPipe(pipeRead);
            if (newLine.Len() > 0)
            {
                if (outStdOut != nullptr)
                {
                    *outStdOut += newLine;
                }
            }
            FPlatformProcess::Sleep(0.1);
        }
        
        // read the remainder
        if (!didTimeout)
        {
            for(;;)
            {
                FString newLine = FPlatformProcess::ReadPipe(pipeRead);
                if (newLine.Len() <= 0)
                {
                    break;
                }
                
                if (outStdOut != nullptr)
                {
                    *outStdOut += newLine;
                }
            }
        }
        
        FPlatformProcess::Sleep(0.5);
        
        wasInvoked = true;
        bool bGotReturnCode = FPlatformProcess::GetProcReturnCode(procHandle, &returnCode);
        if (bGotReturnCode)
            *outReturnCode = returnCode;
        
        FPlatformProcess::CloseProc(procHandle);
    }
    else
    {
        wasInvoked = false;
        *outReturnCode = -1;
        *outStdOut = "";
        UE_LOG(LogPing, Error, TEXT("Failed to launch tool. (%s)"), *executableFileName);
    }
    FPlatformProcess::ClosePipe(pipeRead, pipeWrite);
    return wasInvoked;
}

FString MacLinuxPingThread::WhichPing() const
{
    int32 returnCode;
    FString stdErr;
    FString stdOut;
    
    if (!RunBashCommand(TEXT("/usr/bin/which ping"), 5, &returnCode, &stdOut, &stdErr))
    {
        return TEXT("");
    }
    
    bool validPingPath = false;
    FString processedOutput;
    if (stdOut.Len() > 0)
	{
		UE_LOG(LogPing, VeryVerbose, TEXT("Read in %d characters from 'which' cout."), stdOut.Len());

        processedOutput = stdOut;
        processedOutput = processedOutput.TrimEnd();
        int32 lastNewline;
        if (processedOutput.FindLastChar(TEXT('\n'), lastNewline))
        {
            processedOutput = processedOutput.Mid(lastNewline + 1);
        }

        const FString pingSuffix = TEXT("/ping");
        validPingPath = processedOutput.Len() >= pingSuffix.Len() && processedOutput.EndsWith(pingSuffix, ESearchCase::IgnoreCase);
        if (!validPingPath)
        {
            UE_LOG(LogPing, Error, TEXT("Response from 'which' appears invalid: %s"), *stdOut);
        }
	}
	else
	{
		UE_LOG(LogPing, Error, TEXT("Read nothing from 'which' cout."));
	}
    
    if (stdErr.Len() > 0)
    {
        UE_LOG(LogPing, Warning, TEXT("Got error message from 'which': %s"), *stdErr);
    }

    return validPingPath ? processedOutput : TEXT("");
}

uint32 MacLinuxPingThread::Run()
{
    int32* PingTime = new int32(-1);
    int32* ThreadComplete = new int32;
    
//    FString pingPath = WhichPing();
//
//	if (pingPath.Len() == 0)
//	{
//		UE_LOG(LogPing, Error, TEXT("Was unable to find ping executable."));
//		FPlatformAtomics::InterlockedAdd(ThreadComplete, -1);
//		Stop();
//		return -1;
//	}
//    
//    UE_LOG(LogPing, VeryVerbose, TEXT("Ping path: %s"), *pingPath);

    int32 returnCode;
    FString stdErr;
    FString stdOut;
    
    if (!RunBashCommand(TEXT("ping -c1 ") + Hostname, 5, &returnCode, &stdOut, &stdErr))
	{
		exit(-2);
	}

	if (stdOut.Len() > 0)
	{
		UE_LOG(LogPing, VeryVerbose, TEXT("Read in %d characters from 'ping' cout."), stdOut.Len());
	}
	else
	{
		UE_LOG(LogPing, Error, TEXT("Read nothing from 'ping' cout."));
		FPlatformAtomics::InterlockedAdd(ThreadComplete, -1);
		Stop();
		return -1;
	}

    const FString timeString = TEXT("time=");
	int32 timePos = stdOut.Find(timeString, ESearchCase::Type::IgnoreCase, ESearchDir::Type::FromEnd, stdOut.Len() - 1);
    bool validResponse = false;
	if (timePos != -1)
	{
    const FString msString = TEXT("ms\n");
		int32 msPos = stdOut.Find(msString, ESearchCase::Type::IgnoreCase, ESearchDir::Type::FromStart, timePos);
    validResponse = msPos != -1 && msPos > timePos + 1;
		FString timeResult = stdOut.Mid(timePos + timeString.Len(), (msPos - 1) - (timePos + timeString.Len()));
		FPlatformAtomics::InterlockedAdd(PingTime, FCString::Atoi(*timeResult));
	}
	else
	{
		UE_LOG(LogPing, VeryVerbose, TEXT("No response from target host."));
	}
    
    if (stdErr.Len() > 0)
    {
        UE_LOG(LogPing, Warning, TEXT("Got error message from 'ping': %s"), *stdErr);
        FPlatformAtomics::InterlockedAdd(ThreadComplete, -1);
        Stop();
        if (!validResponse)
        {
            return -1;
        }
    }
	
	FPlatformAtomics::InterlockedIncrement(ThreadComplete);

	Stop();
  return validResponse ? 0 : -1;
}

void MacLinuxPingThread::Stop()
{

}

#endif