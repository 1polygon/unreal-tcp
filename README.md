# unreal-tcp (Work in progress)
TCP client/server for unreal. Can be used in Blueprints and C++. \
Uses [FSockets](https://docs.unrealengine.com/5.3/en-US/API/Runtime/Sockets/FSocket) in separate threads.
#### Blueprint Example
![](https://i.imgur.com/yglzmUB.png)
#### C++ Example
##### Server
```c++
auto Server = UTcpServer::CreateTcpServer(this);
Server->EventConnect.AddDynamic(this, &UExample::OnConnect);
Server->EventData.AddDynamic(this, &UExample::OnData);
Server->EventDisconnect.AddDynamic(this, &UExample::OnDisconnect);
Server->Listen("127.0.0.1", 25565);
```
```c++
void UExample::OnConnect(int32 SocketId)
{
	UE_LOG(LogTemp, Display, TEXT("Socket %i connected"), SocketId);
}

void UExample::OnData(int32 SocketId, const TArray<uint8>& Data)
{
	const FString Message = UTcpUtils::BytesToString(Data);
	UE_LOG(LogTemp, Display, TEXT("Socket %i sent: %s"), SocketId, *Message);
	if (Message == "Hello")
	{
		Server->Send(SocketId, UTcpUtils::StringToBytes("World"));
	}
}

void UExample::OnDisconnect(int32 SocketId)
{
	UE_LOG(LogTemp, Display, TEXT("Socket %i disconnected"), SocketId);
}
```
##### Client
```c++
auto Client = UTcpClient::CreateTcpClient(this);
Client->EventConnect.AddDynamic(this, &UExample::OnConnect);
Client->EventData.AddDynamic(this, &UExample::OnData);
Client->EventDisconnect.AddDynamic(this, &UExample::OnDisconnect);
Client->Connect("127.0.0.1", 25565);
```
```c++
void UExample::OnConnect(bool Success)
{
	if (Success)
	{
		UE_LOG(LogTemp, Display, TEXT("Connected"));
		Client->Send(UTcpUtils::StringToBytes("Hello"));
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to connect"));
	}
}

void UExample::OnData(const TArray<uint8>& Data)
{
	const FString Message = UTcpUtils::BytesToString(Data);
	UE_LOG(LogTemp, Display, TEXT("Received: %s"), *Message);
}

void UExample::OnDisconnect()
{
	UE_LOG(LogTemp, Display, TEXT("Disconnected"));
}
```