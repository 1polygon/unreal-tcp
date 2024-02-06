#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TcpServer.generated.h"

class FTcpServerThread;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConnectDelegate, int32, SocketId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDisconnectDelegate, int32, SocketId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDataDelegate, int32, SocketId, const TArray<uint8>&, Data);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCloseDelegate);

UCLASS(Blueprintable)
class TCPSOCKETS_API UTcpServer : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Tcp Server")
	bool Listening;

	UPROPERTY(BlueprintAssignable, Category = "Tcp Server")
	FConnectDelegate EventConnect;

	UPROPERTY(BlueprintAssignable, Category = "Tcp Server")
	FDisconnectDelegate EventDisconnect;

	UPROPERTY(BlueprintAssignable, Category = "Tcp Server")
	FDataDelegate EventData;

	UPROPERTY(BlueprintAssignable, Category = "Tcp Server")
	FCloseDelegate EventClose;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Tcp Server")
	void Listen(FString Ip = "127.0.0.1", int32 Port = 25565);
	UFUNCTION(BlueprintCallable, Category = "Tcp Server")
	void Close();
	UFUNCTION(BlueprintCallable, Category = "Tcp Server")
	void Send(int32 SocketId, TArray<uint8> Data);
	UFUNCTION(BlueprintCallable, Category= "Tcp Server")
	void SendFile(int32 SocketId, FString FilePath);
	UFUNCTION(BlueprintCallable, Category = "Tcp Server")
	void Terminate(int32 SocketId);
	UFUNCTION(BlueprintCallable, Category = "Tcp Server", Meta=(DefaultToSelf="Outer"))
	static UTcpServer* CreateTcpServer(UObject* Outer);
	virtual void BeginDestroy() override;

	void OnConnect(int32 SocketId) const;
	void OnData(int32 SocketId, TArray<uint8> Data) const;
	void OnDisconnect(int32 SocketId) const;
	void OnClose();
private:
	FTcpServerThread* ServerThread;
};
