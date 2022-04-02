
#include "IPingThread.h"
#include "PingPrivatePCH.h"
#include "PingIP.h"
#include "UObject/UObjectThreadContext.h"
#include "CoreGlobals.h"

DECLARE_CYCLE_STAT(TEXT("PingPlugin - ProcessEcho"),		STAT_PingPluginProcessEcho,			STATGROUP_PingPlugin);
DECLARE_CYCLE_STAT(TEXT("PingPlugin - ProcessEcho Delay"),	STAT_PingPluginProcessEchoDelay,	STATGROUP_PingPlugin);

// Turns out that there are PostLoad situations where the game thread will run task
// graph entries, even though it will refuse to run dynamic delegates.  Since there isn't
// a readily available TimeManager to use to delay via, this is basically a task that 
// delays for a short period of time to give the PostLoad situation time to clear up.
//
// This is a rare situation, the ping has to come back during a level load or the
// game thread task has to end up getting scheduled while a streaming object is
// being finalized.
//
// NOTE: I'm unsure if sleeping in a Task is bad form.  I didn't do a full FRunnable
//       because I don't have a way to auto-clean up the thread handle, and felt
//       like I'd have to leak it.
class FDelayPingResultTask
{
public:
	explicit FDelayPingResultTask (FSimpleDelegate const & cb = FSimpleDelegate(),
								   float delaySeconds = 0.25f)
		: m_cb (cb)
		, m_delaySeconds (FMath::Max(delaySeconds, 0.0f))
	{
	}

	void DoWork ()
	{
		// Delay for time period
		FPlatformProcess::Sleep(m_delaySeconds);
		// Execute callback, which will try again.
		m_cb.ExecuteIfBound();
	}

	bool CanAbandon ()
	{
		return true;
	}

	void Abandon ()
	{
		// If abandoned, just run right now without the delay, at
		// worst, we'll end up scheduling another task for later.
		m_cb.ExecuteIfBound();
	}

	FORCEINLINE TStatId GetStatId() const
	{
		return GET_STATID(STAT_PingPluginProcessEchoDelay);
	}

protected:
	FSimpleDelegate	m_cb;
	float m_delaySeconds;
};

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
		TWeakObjectPtr<UPingIP> const	pingOp		(PingOperation);
		FString const					host		(Hostname);

		// Indirections that allow us to reference the lambdas before their declared.
		TSharedPtr<FSimpleDelegate>		pSafetyCb			(new FSimpleDelegate);
		TSharedPtr<FSimpleDelegate>		pRunInMainThreadCb	(new FSimpleDelegate);

		// Does the actual work, could just be embedded in safetyCb directly, but
		// this makes it a little more clear what the various lambdas are for.
		auto const						workCb		= [bSuccess, timeMS, pingOp, host](){
				check(IsInGameThread());

				if (pingOp.IsValid())
				{
					pingOp->ProcessEcho(timeMS, bSuccess);
				}
				else
				{
					UE_LOG(LogPing, Verbose, TEXT("PingOperation to return results of ping to %s to %s"), *host, pingOp.IsStale() ? TEXT("is no longer valid") : TEXT("is invalid"));
				}
			};
		// Used to force the update onto the game thread.  Called in place at the end of
		// this function, but also used via an indirection used by FDelayPingResultTask.
		auto const						runInGameThread = [pSafetyCb]() {
			// Handle it next tick in the game thread.
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateLambda([pSafetyCb](){ 
					pSafetyCb->ExecuteIfBound(); 
				}),
				GET_STATID(STAT_PingPluginProcessEcho),
				nullptr, // No prereqs
				ENamedThreads::GameThread);
			};
		// Does the safety check that all this indirection is about.  If IsRoutingPostLoad
		// is set, we can't call dynamic delegates that ProcessEcho() uses in workCb, so
		// if that's the (unusual) case, use FDelayPingResultTask to delay for a 
		// short period and then try again back on the main thread.  I don't just
		// immediately reschedule because we're already on the game thread, and that
		// will just cause a lot of spinning.
		auto const						safetyCb	= [workCb, pRunInMainThreadCb]() {
				check(IsInGameThread());

				if (!FUObjectThreadContext::Get().IsRoutingPostLoad)
				{
					workCb();
				}
				else if (ensure(pRunInMainThreadCb.IsValid()))
				{
					(new FAutoDeleteAsyncTask<FDelayPingResultTask>(*pRunInMainThreadCb.Get()))->StartBackgroundTask();
				}
			};

		// Now that the lambdas are declared, bind the indirections used in them
		// to the actual functions.
		pSafetyCb->BindLambda(safetyCb);
		pRunInMainThreadCb->BindLambda(runInGameThread);

		// Ask to be run in the game thread next tick.
		runInGameThread();
	}
}