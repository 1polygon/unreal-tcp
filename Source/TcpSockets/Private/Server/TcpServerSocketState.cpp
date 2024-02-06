#include "Server/TcpServerSocketState.h"
#include "TcpMessageDefinitions.h"
#include "Networking.h"

FTcpServerSocketState::FTcpServerSocketState(int32 InId, FSocket* InSocket)
{
	Id = InId;
	Socket = InSocket;
}

void FTcpServerSocketState::Close(bool SendCloseMessage)
{
	const auto SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if(Socket)
	{
		if(SendCloseMessage)
		{
			int32 BytesSent;
			Socket->Send(FTcpMessageHeader::CreateCloseMessage().GetData(), 8, BytesSent);
		}
		Socket->Close();
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
	}
}
