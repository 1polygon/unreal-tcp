#include "Server/TcpServer.h"
#include "Server/TcpServerThread.h"

void UTcpServer::Listen(const FString Ip, const int32 Port)
{
	if (Listening)
	{
		return;
	}
	ServerThread = new FTcpServerThread(Ip, Port, this);
	Listening = true;
}

void UTcpServer::Close()
{
	if (Listening && ServerThread)
	{
		ServerThread->Exit();
	}
}

void UTcpServer::Send(int32 SocketId, TArray<uint8> Data)
{
	if (Listening && ServerThread)
	{
		TArray<uint8> DataWithHeader;
		FMemoryWriter Writer(DataWithHeader);

		// Header
		int32 Type = 0;
		int32 Size = Data.Num();
		Writer << Type;
		Writer << Size;

		// Payload
		for (int i = 0; i < Data.Num(); i++)
		{
			Writer << Data[i];
		}
		ServerThread->Send(SocketId, DataWithHeader);
	}
}

void UTcpServer::SendFile(int32 SocketId, FString FilePath)
{
	if (ServerThread && Listening)
	{
		AsyncTask(ENamedThreads::NormalThreadPriority, [this, SocketId, FilePath]()
		{
			TArray<uint8> FileData;
			if (FFileHelper::LoadFileToArray(FileData, *FilePath))
			{
				UE_LOG(LogTemp, Warning, TEXT("Send file of %i bytes to %i"), FileData.Num(), SocketId);
				Send(SocketId, FileData);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to read file: %s"), *FilePath);
			}
		});
	}
}

void UTcpServer::Terminate(int32 SocketId)
{
	if (Listening && ServerThread)
	{
		ServerThread->Disconnect(SocketId);
	}
}

UTcpServer* UTcpServer::CreateTcpServer(UObject* Outer)
{
	return NewObject<UTcpServer>(Outer);
}

void UTcpServer::BeginDestroy()
{
	UObject::BeginDestroy();
	Close();
	EventConnect.Clear();
	EventData.Clear();
	EventDisconnect.Clear();
	EventClose.Clear();
}

void UTcpServer::OnConnect(const int32 SocketId) const
{
	if (EventConnect.IsBound())
	{
		EventConnect.Broadcast(SocketId);
	}
}

void UTcpServer::OnData(const int32 SocketId, const TArray<uint8> Data) const
{
	if (EventData.IsBound())
	{
		EventData.Broadcast(SocketId, Data);
	}
}

void UTcpServer::OnDisconnect(const int32 SocketId) const
{
	if (EventDisconnect.IsBound())
	{
		EventDisconnect.Broadcast(SocketId);
	}
}

void UTcpServer::OnClose()
{
	if (ServerThread)
	{
		delete ServerThread;
		ServerThread = nullptr;
	}
	Listening = false;
	if (EventClose.IsBound())
	{
		EventClose.Broadcast();
	}
}
