#pragma once

#include "CoreMinimal.h"
#include "TcpMessageDefinitions.h"

class UTcpClient;

class TCPSOCKETS_API FTcpClientThread : public FRunnable
{
public:
	FTcpClientThread(FString InIp, int32 InPort, UTcpClient* InTcpClient);
	~FTcpClientThread();

private:
	FRunnableThread* Thread;
	UTcpClient* TcpClient;

	FSocket* Socket;
	TQueue<TArray<uint8>> SendQueue;
	bool Connected;
	FThreadSafeBool CloseRequested;
	FString Ip;
	int Port;

	TArray<uint8> ReceiveBuffer;
	FTcpMessageHeader CurrentMessage;

public:
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;
	virtual void Stop() override;

	void Send(TArray<uint8> Data);

private:
	void CloseSocket(bool SendCloseMessage);
};
