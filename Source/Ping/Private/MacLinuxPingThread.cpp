#include "PingPrivatePCH.h"
#include "HAL/Platform.h"

#if PLATFORM_MAC | PLATFORM_LINUX

#include "MacLinuxPingThread.h"
#include "Misc/ScopeExit.h"
#include <spawn.h> // see manpages-posix-dev
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <sys/wait.h>

bool MacLinuxPingThread::Init()
{
	return true;
}

FRunnableThread* MacLinuxPingThread::StartThread()
{
	return FRunnableThread::Create(this, TEXT("MacLinuxPingThread"), 0, TPri_Normal);
}

std::string which_ping()
{
	int32 exit_code;
	int32 cout_pipe[2];
	int32 cerr_pipe[2];
	posix_spawn_file_actions_t action;

	// establish pipe
	if (pipe(cout_pipe) || pipe(cerr_pipe))
	{
		UE_LOG(LogPing, Error, TEXT("'Which' Pipe() returned an error."));
		return "";
	}

	// prepare options for spawning 'which' process
	posix_spawn_file_actions_init(&action);
	posix_spawn_file_actions_addclose(&action, cout_pipe[0]);
	posix_spawn_file_actions_addclose(&action, cerr_pipe[0]);
	posix_spawn_file_actions_adddup2(&action, cout_pipe[1], 1);
	posix_spawn_file_actions_adddup2(&action, cerr_pipe[1], 2);

	posix_spawn_file_actions_addclose(&action, cout_pipe[1]);
	posix_spawn_file_actions_addclose(&action, cerr_pipe[1]);

	std::string command = "/usr/bin/which ping";
	std::string argsmem[] = { "bash","-l","-c" }; // allows non-const access to literals
	char * args[] = { &argsmem[0][0],&argsmem[1][0],&argsmem[2][0],&command[0],nullptr };

	pid_t pid;
	if (posix_spawnp(&pid, args[0], &action, NULL, &args[0], NULL) != 0)
	{
		UE_LOG(LogPing, Error, TEXT("posix_spawnp(which) failed with error: %d"), strerror(errno));
	}

	// close child-side of pipes
	close(cout_pipe[1]);
	close(cerr_pipe[1]);

	//std::cout << "Started 'which' process, PID is " << pid << std::endl;

	std::string buf(64, ' ');

	struct timespec timeout = { 5, 0 };

	// prepare to read from pipe
	fd_set read_set;
	memset(&read_set, 0, sizeof(read_set));
	FD_SET(cout_pipe[0], &read_set);
	FD_SET(cerr_pipe[0], &read_set);

	int32 larger_fd = (cout_pipe[0] > cerr_pipe[0]) ? cout_pipe[0] : cerr_pipe[0];

	int32 rc = pselect(larger_fd + 1, &read_set, NULL, NULL, &timeout, NULL);
	//thread blocks until either packet is received or the timeout goes through
	if (rc == 0)
	{
		UE_LOG(LogPing, Error, TEXT("'Which' timed out."));
		return "";
	}

	int bytes_read = read(cerr_pipe[0], &buf[0], buf.length());
	if (bytes_read > 0)
	{
		FString errorMessage(buf.substr(0, bytes_read).c_str());
		UE_LOG(LogPing, Error, TEXT("Got error message from 'which': %s"), *errorMessage);
		return "";
	}

	bytes_read = read(cout_pipe[0], &buf[0], buf.length());
	if (bytes_read > 0)
	{
		UE_LOG(LogPing, VeryVerbose, TEXT("Read in %d bytes from 'which' cout_pipe."), bytes_read);
	}
	else
	{
		UE_LOG(LogPing, Error, TEXT("Read nothing from 'which' cout_pipe."));
	}

	waitpid(pid, &exit_code, 0);

	// close our side of pipes
	close(cout_pipe[0]);
	close(cerr_pipe[0]);

	posix_spawn_file_actions_destroy(&action);

	return buf.substr(0, bytes_read - 1); //trim off the trailing newline
}

uint32 MacLinuxPingThread::Run()
{
	bool bSucceed = false;
	int32 PingTime = -1;

	// Ensures we tell the main thread what we found here.
	ON_SCOPE_EXIT
	{
		ReturnResultToGameThread(bSucceed, PingTime);
	};

	int32 exit_code;
	int32 cout_pipe[2];
	int32 cerr_pipe[2];
	posix_spawn_file_actions_t action;

	if (pipe(cout_pipe) || pipe(cerr_pipe))
	{
		UE_LOG(LogPing, Error, TEXT("'Ping' pipe returned an error."));
		return -1;
	}

	std::string ping_path = which_ping();

	if (ping_path.length() == 0)
	{
		UE_LOG(LogPing, Error, TEXT("Was unable to find ping executable."));
		return -1;
	}

	FString UPingPath(ping_path.c_str());
	UE_LOG(LogPing, VeryVerbose, TEXT("Ping path: %s"), *UPingPath);

	posix_spawn_file_actions_init(&action);
	posix_spawn_file_actions_addclose(&action, cout_pipe[0]);
	posix_spawn_file_actions_addclose(&action, cerr_pipe[0]);
	posix_spawn_file_actions_adddup2(&action, cout_pipe[1], 1);
	posix_spawn_file_actions_adddup2(&action, cerr_pipe[1], 2);

	posix_spawn_file_actions_addclose(&action, cout_pipe[1]);
	posix_spawn_file_actions_addclose(&action, cerr_pipe[1]);

	std::string ping_addr = TCHAR_TO_ANSI(*Hostname);
	std::string command = ping_path + " -c 1 " + ping_addr;
	//std::string command = "/bin/ping -c 1 8.8.4.4";
	std::string argsmem[] = { "bash","-l","-c" }; // allows non-const access to literals
	char * args[] = { &argsmem[0][0],&argsmem[1][0],&argsmem[2][0],&command[0],nullptr };

	//std::cout << "Ping command: " << command << std::endl;

	pid_t pid;
	if (posix_spawnp(&pid, args[0], &action, NULL, &args[0], NULL) != 0)
	{
		UE_LOG(LogPing, Error, TEXT("posix_spawnp(ping) failed with error: %d"), strerror(errno));
		exit(-2);
	}

	UE_LOG(LogPing, VeryVerbose, TEXT("Ping process ID: %d"), pid);

	close(cout_pipe[1]), close(cerr_pipe[1]); // close child-side of pipes

	// prepare buffer for 'ping' output
	std::string buf(512, ' ');
	std::stringstream bufstream;

	struct timespec timeout = { 5, 0 };

	// prepare for reading from pipe
	fd_set read_set;
	memset(&read_set, 0, sizeof(read_set));
	FD_SET(cout_pipe[0], &read_set);
	FD_SET(cerr_pipe[0], &read_set);

	int32 larger_fd = (cout_pipe[0] > cerr_pipe[0]) ? cout_pipe[0] : cerr_pipe[0];

	int32 rc = pselect(larger_fd + 1, &read_set, NULL, NULL, &timeout, NULL);
	//thread blocks until either packet is received or the timeout goes through
	if (rc == 0)
	{
		UE_LOG(LogPing, Error, TEXT("'Ping' timed out."));
		return -1;
	}

	int32 bytes_read = read(cerr_pipe[0], &buf[0], buf.length());
	if (bytes_read > 0)
	{
		FString errorMessage(buf.substr(0, bytes_read).c_str());
		UE_LOG(LogPing, Error, TEXT("Got error message from 'ping': %s"), *errorMessage);
		return -1;
	}

	bytes_read = read(cout_pipe[0], &buf[0], buf.length());
	if (bytes_read > 0)
	{
		UE_LOG(LogPing, VeryVerbose, TEXT("Read in %d bytes from 'ping' cout_pipe."), bytes_read);
	}
	else
	{
		UE_LOG(LogPing, Error, TEXT("Read nothing from 'ping' cout_pipe."));
		return -1;
	}

	UE_LOG(LogPing, VeryVerbose, TEXT("Done reading."));

	// dump output into FString for parsing
	FString pingOutput(buf.substr(0, bytes_read).c_str());

	// close our side of pipes
	close(cout_pipe[0]);
	close(cerr_pipe[0]);

	UE_LOG(LogPing, VeryVerbose, TEXT("Waiting for process to close."));
	waitpid(pid, &exit_code, 0);
	UE_LOG(LogPing, VeryVerbose, TEXT("Child closed."));

	posix_spawn_file_actions_destroy(&action);
	
	int32 timePos = pingOutput.Find("time=", ESearchCase::Type::IgnoreCase, ESearchDir::Type::FromEnd, pingOutput.Len() - 1);
	if (timePos != -1)
	{
		int32 msPos = pingOutput.Find("ms\n", ESearchCase::Type::IgnoreCase, ESearchDir::Type::FromStart, timePos);
		FString timeResult = pingOutput.Mid(timePos + 5, (msPos - 1) - (timePos + 5));
		ensure(timeResult.IsNumeric());
		PingTime = FCString::Atoi(*timeResult);
		bSucceed = true;
	}
	else
	{
		UE_LOG(LogPing, VeryVerbose, TEXT("No response from target host."));
	}
	
	return 0;
}

void MacLinuxPingThread::Stop()
{

}

#endif