#include "Client/TcpClientThread.h"
#include "Networking.h"
#include "Client/TcpClient.h"

FTcpClientThread::FTcpClientThread(FString InIp, int32 InPort, UTcpClient* InTcpClient)
{
	Ip = InIp;
	Port = InPort;
	TcpClient = InTcpClient;
	Thread = FRunnableThread::Create(this, TEXT("TcpClientThread"));
}

FTcpClientThread::~FTcpClientThread()
{
	if (Thread)
	{
		Thread->Kill();
		delete Thread;
	}
}

bool FTcpClientThread::Init()
{
	return FRunnable::Init();
}

uint32 FTcpClientThread::Run()
{
	// Parse ip
	uint8 IpSegments[4];
	TArray<FString> IpSegmentStrings;
	Ip.ParseIntoArray(IpSegmentStrings, TEXT("."), true);
	for (int i = 0; i < IpSegmentStrings.Num(); i++)
	{
		IpSegments[i] = FCString::Atoi(*IpSegmentStrings[i]);
	}
	const auto Address = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	Address->SetIp(FIPv4Address(IpSegments[0], IpSegments[1], IpSegments[2], IpSegments[3]).Value);
	Address->SetPort(Port);

	// Connect
	Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);
	Connected = Socket->Connect(*Address);

	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		if (IsValid(TcpClient))
		{
			TcpClient->OnConnected(Connected);
		}
	});

	if (Connected)
	{
		UE_LOG(LogTemp, Log, TEXT("[TcpClient] connected"));
	}

	while (Connected)
	{
		bool ClosedByServer = false;

		// Receive
		uint32 Size;
		while (Socket && Socket->HasPendingData(Size))
		{
			TArray<uint8> Data;
			Data.SetNum(Size);
			int32 ReadOffset = 0;
			int32 Read;
			Socket->Recv(Data.GetData(), Size, Read);

			// Read messages
			while (Data.Num() - ReadOffset > 0)
			{
				// Begin message
				if (CurrentMessage.Type == None)
				{
					if (Read >= 8)
					{
						// Read header
						FMemoryReader Reader(Data, true);
						Reader.Seek(ReadOffset);
						CurrentMessage.Deserialize(Reader);
						ReadOffset += 8;

						UE_LOG(LogTemp, Verbose, TEXT("[TcpClient] Begin receiving total of %i bytes"),
						       CurrentMessage.Size);
					}
				}

				// Append data if the current message isn't complete yet
				if (ReceiveBuffer.Num() < CurrentMessage.Size)
				{
					if (Data.Num() - ReadOffset > 0)
					{
						// Only append data relevant for the current message
						const int NumRelevantBytes = FMath::Min(Data.Num() - ReadOffset, CurrentMessage.Size);
						for (int i = 0; i < NumRelevantBytes; i++)
						{
							ReceiveBuffer.Push(Data[ReadOffset + i]);
						}
						ReadOffset += NumRelevantBytes;
						UE_LOG(LogTemp, Verbose, TEXT("[TcpClient] Received segment of %i bytes"), NumRelevantBytes);
					}
				}

				// End message after all bytes have been received
				if (ReceiveBuffer.Num() == CurrentMessage.Size)
				{
					const auto Type = CurrentMessage.Type;
					const int32 ReceivedBytes = CurrentMessage.Size;

					switch (Type)
					{
					case ETcpMessageType::Close:
						ClosedByServer = true;
						break;
					case ETcpMessageType::Ping:
						// Todo
						break;
					case ETcpMessageType::Pong:
						// Todo
						break;
					case ETcpMessageType::Data:
						{
							// Copy received data to be used in the game thread
							TArray<uint8> DataCopy;
							DataCopy.SetNum(ReceiveBuffer.Num());
							FMemory::Memcpy(DataCopy.GetData(), ReceiveBuffer.GetData(),
							                ReceiveBuffer.Num());

							AsyncTask(ENamedThreads::GameThread, [this, DataCopy]()
							{
								if (IsValid(TcpClient))
								{
									TcpClient->OnData(DataCopy);
								}
							});
						}
						break;
					default:
						break;
					}

					// Reset type so a new message begins when receiving data again
					CurrentMessage.Type = None;
					ReceiveBuffer.Empty();

					UE_LOG(LogTemp, Verbose, TEXT("[TcpClient] Received total of %i bytes"), ReceivedBytes);
				}
			}
		}

		// Send
		if (Socket)
		{
			TArray<uint8> DataToSend;
			if (SendQueue.Dequeue(DataToSend))
			{
				int32 BytesSent;
				Socket->Send(DataToSend.GetData(), DataToSend.Num(), BytesSent);
				UE_LOG(LogTemp, Verbose, TEXT("[TcpClient] Sent %i bytes"), BytesSent);
			}
		}

		// Close socket when requested by client
		if (CloseRequested)
		{
			CloseSocket(true);
			break;
		}

		// Close socket when requested by server
		if (ClosedByServer)
		{
			CloseSocket(false);
			break;
		}

		if (Socket)
		{
			if (SendQueue.IsEmpty())
			{
				FPlatformProcess::Sleep(0.01f);
			}
		}
	}

	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		if (IsValid(TcpClient))
		{
			TcpClient->OnClose();
		}
	});

	return 0;
}

void FTcpClientThread::Exit()
{
	CloseRequested = true;
}

void FTcpClientThread::Stop()
{
	FRunnable::Stop();
}

void FTcpClientThread::Send(TArray<uint8> Data)
{
	SendQueue.Enqueue(Data);
}

void FTcpClientThread::CloseSocket(bool SendCloseMessage)
{
	if (Connected)
	{
		if (Socket)
		{
			if (SendCloseMessage)
			{
				int32 BytesSent;
				Socket->Send(FTcpMessageHeader::CreateCloseMessage().GetData(), 8, BytesSent);
			}
			Socket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
			Socket = nullptr;
		}
		Connected = false;
		UE_LOG(LogTemp, Log, TEXT("[TcpClient] disconnected"));
	}
}
