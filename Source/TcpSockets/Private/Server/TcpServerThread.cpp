#include "Server/TcpServerThread.h"
#include "Server/TcpServerSocketState.h"
#include "TcpMessageDefinitions.h"
#include "TcpUtils.h"
#include "Server/TcpServer.h"

FTcpServerThread::FTcpServerThread(const FString InIp, const int32 InPort, UTcpServer* InTcpServer)
{
	// Parse ip
	uint8 IpSegments[4];
	TArray<FString> IpSegmentStrings;
	InIp.ParseIntoArray(IpSegmentStrings, TEXT("."), true);
	for (int i = 0; i < IpSegmentStrings.Num(); i++)
	{
		IpSegments[i] = FCString::Atoi(*IpSegmentStrings[i]);
	}
	Port = InPort;
	Endpoint = FIPv4Endpoint(FIPv4Address(IpSegments[0], IpSegments[1], IpSegments[2], IpSegments[3]), Port);

	TcpServer = InTcpServer;

	// Create thread
	Thread = FRunnableThread::Create(this, TEXT("TcpServerThread"));
}

FTcpServerThread::~FTcpServerThread()
{
	if (Thread)
	{
		Thread->Kill();
		delete Thread;
	}
}

bool FTcpServerThread::Init()
{
	return true;
}

uint32 FTcpServerThread::Run()
{
	// Start listening
	const auto SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	ListenSocket = FTcpSocketBuilder(*FString("ServerSocket"))
	               .AsReusable()
	               .BoundToEndpoint(Endpoint)
	               .Listening(8);
	Listening = true;
	UE_LOG(LogTemp, Log, TEXT("[TcpServer] Listening on port %i"), Port);

	while (Listening)
	{
		// Accept connections
		bool Pending;
		ListenSocket->HasPendingConnection(Pending);
		if (Pending)
		{
			TSharedRef<FInternetAddr> RemoteAddress = SocketSubsystem->CreateInternetAddr();
			FSocket* ClientSocket = ListenSocket->Accept(*RemoteAddress, TEXT("TcpSocket"));
			int32 SocketId = GetNewSocketId();
			Sockets.Add(SocketId, new FTcpServerSocketState(SocketId, ClientSocket));
			AsyncTask(ENamedThreads::GameThread, [this, SocketId]()
			{
				if (IsValid(TcpServer))
				{
					TcpServer->OnConnect(SocketId);
				}
			});
			UE_LOG(LogTemp, Verbose, TEXT("[TcpServer] Socket connected"));
		}

		// Process connected sockets
		TArray<int32> RemoveQueue;
		bool HasPendingData = false;
		for (auto& Pair : Sockets)
		{
			FTcpServerSocketState* State = Pair.Value;
			if (!State)
			{
				continue;
			}

			FSocket* Socket = State->Socket;
			int32 SocketId = State->Id;

			// Receive data
			uint32 Size;
			if (Socket && Socket->HasPendingData(Size))
			{
				HasPendingData = true;
				TArray<uint8> Data;
				Data.SetNum(Size);
				int32 Read;
				Socket->Recv(Data.GetData(), Size, Read);
				UE_LOG(LogTemp, Verbose, TEXT("[TcpServer] Received %i bytes"), Read);

				// Read messages
				while (Data.Num() > 0)
				{
					// Begin message 
					if (State->ReceiveHeader.Type == None)
					{
						// Read header
						FMemoryReader Reader(Data);
						State->ReceiveHeader.Deserialize(Reader);

						// Remove header
						constexpr int32 HeaderSize = 8;
						Data.RemoveAt(0, HeaderSize);

						UE_LOG(LogTemp, Verbose, TEXT("[TcpServer] Begin receiving total of %i bytes"),
						       State->ReceiveHeader.Size);
					}

					// Append data if the current message isn't complete yet
					if (State->ReceiveBuffer.Num() < State->ReceiveHeader.Size)
					{
						if (Data.Num() > 0)
						{
							// Only append data relevant for the current message
							const int NumRelevantBytes = FMath::Min(Data.Num(), State->ReceiveHeader.Size);
							for (int i = 0; i < NumRelevantBytes; i++)
							{
								State->ReceiveBuffer.Push(Data[i]);
							}
							Data.RemoveAt(0, NumRelevantBytes);
							UE_LOG(LogTemp, Verbose, TEXT("[TcpServer] Received segment of %i bytes"), NumRelevantBytes);
						}
					}
					
					// End message after all bytes have been received
					if (State->ReceiveBuffer.Num() == State->ReceiveHeader.Size)
					{
						const auto Type = State->ReceiveHeader.Type;
						const int32 ReceivedBytes = State->ReceiveHeader.Size;

						switch (Type)
						{
						case ETcpMessageType::Close:
							RemoveQueue.Add(SocketId);
							break;
						case ETcpMessageType::Ping:
							{
								// Todo
							}
							break;
						case ETcpMessageType::Pong:
							{
								// Todo
							}
							break;
						case ETcpMessageType::Data:
							{
								// Copy received data to be used in the game thread
								TArray<uint8> DataCopy;
								DataCopy.SetNum(State->ReceiveBuffer.Num());
								FMemory::Memcpy(DataCopy.GetData(), State->ReceiveBuffer.GetData(),
								                State->ReceiveBuffer.Num());

								AsyncTask(ENamedThreads::GameThread, [this, SocketId, DataCopy]()
								{
									if (IsValid(TcpServer))
									{
										TcpServer->OnData(SocketId, DataCopy);
									}
								});
							}
							break;
						default:
							break;
						}
						
						// Reset state so a new message begins when receiving data again
						State->ReceiveHeader.Type = None;
						State->ReceiveBuffer.Empty();

						UE_LOG(LogTemp, Verbose, TEXT("[TcpServer] Received total of %i bytes"), ReceivedBytes);
					}
				}
			}
		}

		// Remove closed sockets (Close initiated by client)
		for (int32 SocketId : RemoveQueue)
		{
			CloseSocket(SocketId, false, true);
		}

		// Disconnect sockets (Close initiated by server)
		if (int32 SocketId; DisconnectQueue.Dequeue(SocketId))
		{
			CloseSocket(SocketId, true, true);
		}

		// Send queued data
		FTcpSendQueueItem DataToSend;
		if (SendQueue.Dequeue(DataToSend))
		{
			auto State = Sockets.FindRef(DataToSend.SocketId);
			if (State && State->Socket)
			{
				int32 BytesSent;
				State->Socket->Send(DataToSend.Data.GetData(), DataToSend.Data.Num(), BytesSent);
				UE_LOG(LogTemp, Verbose, TEXT("[TcpServer] Sent %i bytes to %i"), BytesSent, DataToSend.SocketId);
			}
		}

		if (ListenSocket)
		{
			if (SendQueue.IsEmpty() && !HasPendingData)
			{
				FPlatformProcess::Sleep(0.01f);
			}
		}
	}

	// Close connected sockets and stop listening
	for (const auto& Pair : Sockets)
	{
		FTcpServerSocketState* State = Pair.Value;
		State->Close(true);
		delete Pair.Value;
	}
	Sockets.Empty();
	if (ListenSocket)
	{
		ListenSocket->Close();
		SocketSubsystem->DestroySocket(ListenSocket);
	}
	ListenSocket = nullptr;
	UE_LOG(LogTemp, Log, TEXT("[TcpServer] Closed"));

	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		if (IsValid(TcpServer))
		{
			TcpServer->OnClose();
		}
	});

	return 0;
}

void FTcpServerThread::Exit()
{
	Listening = false;
}

void FTcpServerThread::Stop()
{
	FRunnable::Stop();
}

void FTcpServerThread::Send(int32 SocketId, TArray<uint8> Data)
{
	SendQueue.Enqueue(FTcpSendQueueItem(SocketId, Data));
}

void FTcpServerThread::Disconnect(int32 SocketId)
{
	DisconnectQueue.Enqueue(SocketId);
}

int32 FTcpServerThread::GetNewSocketId() const
{
	int Id = Sockets.Num();
	while (Sockets.Contains(Id))
	{
		Id++;
	}
	return Id;
}

void FTcpServerThread::CloseSocket(int32 SocketId, bool SendCloseMessage, bool EmitEvent)
{
	FTcpServerSocketState* State = Sockets.FindRef(SocketId);
	State->Close(SendCloseMessage);
	Sockets.Remove(SocketId);
	delete State;
	State = nullptr;
	if (EmitEvent)
	{
		AsyncTask(ENamedThreads::GameThread, [this, SocketId]()
		{
			if (IsValid(TcpServer))
			{
				TcpServer->OnDisconnect(SocketId);
			}
		});
	}
}
