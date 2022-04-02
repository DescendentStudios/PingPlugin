
#if PLATFORM_WINDOWS
#include "WinPingThread.h"
#include "PingPrivatePCH.h"
#include "HAL/Platform.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <IntSafe.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#include "Misc/ScopeExit.h"

//DEFINE_LOG_CATEGORY(LogPing);

union sockaddr_both
{
	sockaddr_in6 addr6;
	sockaddr_in addr4;
};

bool WinPingThread::Init()
{
	return true;
}

FRunnableThread* WinPingThread::StartThread()
{
	return FRunnableThread::Create(this, TEXT("WinPingThread"), 0, TPri_Normal);
}

//function that resolves hostname
bool hostnameResolve(FString host, union sockaddr_both& ipAddress, bool& ipv6)
{
	//initialize socket for hostname resolution
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		UE_LOG(LogPing, Error, TEXT("Failed to create WinSocket for hostname resolution: %d"), iResult);
		return false;
	}

	UE_LOG(LogPing, VeryVerbose, TEXT("WinSocket established successfully."));

	//set up networking info for resolving the hostname
	struct addrinfo netInfo;
	ZeroMemory(&netInfo, sizeof(netInfo));
	netInfo.ai_flags = AI_ALL;
	//netInfo.ai_family = AF_UNSPEC;
	netInfo.ai_family = AF_INET;
	netInfo.ai_socktype = SOCK_STREAM;
	netInfo.ai_protocol = IPPROTO_TCP;

	//designate info struct to receive IP address from getaddrinfo() call
	struct addrinfo *result = NULL;

	//prepare sockaddrs for IPv4 and IPv6
	struct sockaddr_in *sockaddr_ipv4 = NULL;
	//struct sockaddr_in6 *sockaddr_ipv6 = NULL;

	//resolve hostname
	int hostRetVal = getaddrinfo(TCHAR_TO_ANSI(*host), NULL, &netInfo, &result);
	if (hostRetVal != 0) 
	{
		UE_LOG(LogPing, Error, TEXT("Hostname resolution failed: %d for %s"), hostRetVal, *host);
		return false;
	}

	//create char buffer for 

	//loop through results
	struct addrinfo *ptr = NULL;
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		switch (ptr->ai_family)
		{
		case AF_INET:
			//if (!sockaddr_ipv6)
			{
				sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
			}
			break;
			/*
		case AF_INET6:
			sockaddr_ipv6 = (struct sockaddr_in6 *) ptr->ai_addr;
			break;
			*/
		default:
			continue;
		}
	}

	//if (!sockaddr_ipv6 && !sockaddr_ipv4)
	if (!sockaddr_ipv4)
	{
		UE_LOG(LogPing, Error, TEXT("No valid addresses found."));
		return false;
	}
	/*
	else if (sockaddr_ipv6)
	{
		ipAddress.addr6 = *sockaddr_ipv6;
		ipv6 = true;
		return true;
	}
	*/
	else
	{
		ipAddress.addr4 = *sockaddr_ipv4;
		ipv6 = false;
		return true;
	}
}

/*

//function that pings via IPv6
bool pingIPv6(struct sockaddr_in6 ipAddress)
{
	char ipAddrBuf[46];
	size_t ipAddrBufLen = 46;
	inet_ntop(AF_INET6, &ipAddress, ipAddrBuf, ipAddrBufLen);
	UE_LOG(LogPing, VeryVerbose, TEXT("Successfully resolved hostname into %s"), ANSI_TO_TCHAR(ipAddrBuf));

	HANDLE hIcmpFile;
	hIcmpFile = Icmp6CreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE)
	{
		UE_LOG(LogPing, Error, TEXT("Error in creating ICMP6 handle."));
		return false;
	}


}

*/

//function that pings via IPv4
bool pingIPv4(struct sockaddr_in ipAddress, int32& pingTime)
{
	char ipAddrBuf[16];
	size_t ipAddrBufLen = 16;
	inet_ntop(AF_INET, &ipAddress, ipAddrBuf, ipAddrBufLen);
	UE_LOG(LogPing, VeryVerbose, TEXT("Successfully resolved hostname into %s."), ANSI_TO_TCHAR(ipAddrBuf));

	HANDLE hIcmpFile;
	unsigned long dwRetVal = 0;
	char SendData[32] = "Data Buffer";
	LPVOID ReplyBuffer = NULL;
	unsigned long ReplySize = 0;

	hIcmpFile = IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE)
	{
		UE_LOG(LogPing, Error, TEXT("Error in creating ICMP handle."));
		return false;
	}

	ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
	ReplyBuffer = (VOID*)malloc(ReplySize);
	if (ReplyBuffer == NULL)
	{
		UE_LOG(LogPing, Error, TEXT("Error in allocating memory for ICMP echo reply."));
		return false;
	}

	dwRetVal = IcmpSendEcho(hIcmpFile, ipAddress.sin_addr.S_un.S_addr, SendData, sizeof(SendData),
		NULL, ReplyBuffer, ReplySize, 3000);
	if (dwRetVal != 0) {
		PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
		pingTime = pEchoReply->RoundTripTime;
		return true;
	}
	return false;
}

uint32 WinPingThread::Run()
{
	bool bSucceed = false;
	int32 PingTime = -1;

	// Ensures we tell the main thread what we found here.
	ON_SCOPE_EXIT
	{
		ReturnResultToGameThread(bSucceed, PingTime);
	};


	union sockaddr_both ipAddress;
	bool ipv6;

	UE_LOG(LogPing, VeryVerbose, TEXT("Hostname is %s."), *Hostname);

	if (!hostnameResolve(Hostname, ipAddress, ipv6))
	{
		return -1;
	}

	if (ipv6)
	{
		//not currently implemented, since game joins are over IPv4, and Icmp6SendEcho2 is a pain in the ass

		/*
		if (pingIPv6(ipAddress.addr6))
		{

		}
		*/
		UE_LOG(LogPing, Error, TEXT("Managed to get to IPv6 flow."));
	}
	else
	{
		if (pingIPv4(ipAddress.addr4, PingTime))
		{
			UE_LOG(LogPing, VeryVerbose, TEXT("Ping to %s complete.  Ping time was %d ms."), *Hostname, PingTime);
			bSucceed = true;
		}
		else
		{
			UE_LOG(LogPing, Error, TEXT("Failed to reach host: %s"), *Hostname);
		}
	}

	return 1;
}

void WinPingThread::Stop()
{

}

#endif