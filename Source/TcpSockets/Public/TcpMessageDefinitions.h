#pragma once

#include "CoreMinimal.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

struct TCPSOCKETS_API FTcpSendQueueItem
{
	int32 SocketId;
	TArray<uint8> Data;

	FTcpSendQueueItem()
	{
		SocketId = -1;
	}

	FTcpSendQueueItem(int32 InSocketId, TArray<uint8> InData)
	{
		SocketId = InSocketId;
		Data = InData;
	}
};

enum TCPSOCKETS_API ETcpMessageType : int32
{
	None = -1,
	Data = 0,
	Close = 1,
	Ping = 2,
	Pong = 3
};

struct TCPSOCKETS_API FTcpMessageHeader
{
	FTcpMessageHeader()
	{
	};

	FTcpMessageHeader(FMemoryReader& Reader)
	{
		Deserialize(Reader);
	}

	ETcpMessageType Type = None;
	int32 Size = 0;

	void Serialize(FMemoryWriter& Writer)
	{
		int32 TypeInt = Type;
		Writer << TypeInt;
		Writer << Size;
	}

	void Deserialize(FMemoryReader& Reader)
	{
		int32 TypeInt = None;
		Reader << TypeInt;
		Reader << Size;
		Type = static_cast<ETcpMessageType>(TypeInt);
	}

	static TArray<uint8> CreateCloseMessage()
	{
		TArray<uint8> Data;
		FMemoryWriter Writer(Data, true);
		FTcpMessageHeader Header;
		Header.Type = Close;
		Header.Serialize(Writer);
		return Data;
	}
};
