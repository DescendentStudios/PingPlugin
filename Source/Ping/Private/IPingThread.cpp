#include "PingPrivatePCH.h"
#include "IPingThread.h"
#include "PingIP.h"
#include "CoreGlobals.h"

DECLARE_CYCLE_STAT(TEXT("PingPlugin - ProcessEcho"), STAT_PingPluginProcessEcho, STATGROUP_PingPlugin);

void IPingThread::ReturnResultToGameThread (bool bSuccess, int32 timeMS)
{
	if (IsInGameThread())
	{
		// Unlikely, but easy.
		PingOperation->ProcessEcho(timeMS, bSuccess);
	}
	else
	{
		// Local copies for lambda, because we don't want it to try to capture "this".
		TWeakObjectPtr<UPingIP> const	pingOp	(PingOperation);
		FString const					host	(Hostname);

		// Handle it next tick in the game thread.
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateLambda([bSuccess, timeMS, pingOp, host](){
				check(IsInGameThread());

				if (pingOp.IsValid())
				{
					pingOp->ProcessEcho(timeMS, bSuccess);
				}
				else
				{
					UE_LOG(LogPing, Verbose, TEXT("PingOperation to return results of ping to %s to %s"), *host, pingOp.IsStale() ? TEXT("is no longer valid") : TEXT("is invalid"));
				}
			}),
			GET_STATID(STAT_PingPluginProcessEcho),
			nullptr, // No prereqs
			ENamedThreads::GameThread);
	}
}