#pragma once

#include "Networking.h"
#include "CoreMinimal.h"
#include "TcpMessageDefinitions.h"

class UTcpServer;
class FTcpServerSocketState;

class TCPSOCKETS_API FTcpServerThread : public FRunnable
{
public:
	FTcpServerThread(FString InIp, int32 InPort, UTcpServer* InTcpServer);
	~FTcpServerThread();
	FRunnableThread* Thread;
	UTcpServer* TcpServer;
	FIPv4Endpoint Endpoint;
	FSocket* ListenSocket;
	int32 Port;
	FThreadSafeBool Listening;
	TMap<int32, FTcpServerSocketState*> Sockets;

	TQueue<FTcpSendQueueItem> SendQueue;
	TQueue<int32> DisconnectQueue;

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;
	virtual void Stop() override;

	void Send(int32 SocketId, TArray<uint8> Data);
	void Disconnect(int32 SocketId);

private:
	int32 GetNewSocketId() const;
	void CloseSocket(int32 SocketId, bool SendCloseMessage, bool EmitEvent);
};
