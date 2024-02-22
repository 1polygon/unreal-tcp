#include "Client/TcpClient.h"
#include "Client/TcpClientThread.h"

void UTcpClient::Connect(FString Ip, int32 Port)
{
	if (Connected || ClientThread)
	{
		return;
	}

	ClientThread = new FTcpClientThread(Ip, Port, this);
}

void UTcpClient::Close()
{
	if (Connected && ClientThread)
	{
		ClientThread->Exit();
	}
}

void UTcpClient::Send(TArray<uint8> Data)
{
	if (!ClientThread || !Connected)
	{
		return;
	}

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

	ClientThread->Send(DataWithHeader);
}

void UTcpClient::SendFile(FString FilePath)
{
	if (ClientThread && Connected)
	{
		AsyncTask(ENamedThreads::NormalThreadPriority, [this, FilePath]()
		{
			TArray<uint8> FileData;
			if (FFileHelper::LoadFileToArray(FileData, *FilePath))
			{
				Send(FileData);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to read file: %s"), *FilePath);
			}
		});
	}
}

UTcpClient* UTcpClient::CreateTcpClient(UObject* Outer)
{
	return NewObject<UTcpClient>(Outer);
}

void UTcpClient::BeginDestroy()
{
	UObject::BeginDestroy();
	Close();
	EventConnect.Clear();
	EventData.Clear();
	EventDisconnect.Clear();
}

void UTcpClient::OnConnected(const bool Success)
{
	Connected = Success;
	if (EventConnect.IsBound())
	{
		EventConnect.Broadcast(Success);
	}
}

void UTcpClient::OnData(const TArray<uint8> Data) const
{
	if (EventData.IsBound())
	{
		EventData.Broadcast(Data);
	}
}

void UTcpClient::OnClose()
{
	if (ClientThread)
	{
		delete ClientThread;
		ClientThread = nullptr;
	}
	Connected = false;
	if (EventDisconnect.IsBound())
	{
		EventDisconnect.Broadcast();
	}
}
