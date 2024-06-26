﻿#pragma once

#include "CoreMinimal.h"
#include "TcpMessageDefinitions.h"


class TCPSOCKETS_API FTcpServerSocketState
{
public:
	FTcpServerSocketState(int32 InId, FSocket* InSocket);

	int32 Id;
	FSocket* Socket;
	FTcpMessageHeader ReceiveHeader;
	TArray<uint8> ReceiveBuffer;
	int64 ReadOffset = 0;
	
private:

public:
	void Close(bool SendCloseMessage);
};
