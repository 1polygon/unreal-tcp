#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TcpClient.generated.h"

class FTcpClientThread;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConnectClientDelegate, bool, Success);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDisconnectClientDelegate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDataClientDelegate, const TArray<uint8>&, Data);

UCLASS(Blueprintable)
class TCPSOCKETS_API UTcpClient : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Tcp Client")
	bool Connected;

	UPROPERTY(BlueprintAssignable, Category = "Tcp Client")
	FConnectClientDelegate EventConnect;

	UPROPERTY(BlueprintAssignable, Category = "Tcp Client")
	FDisconnectClientDelegate EventDisconnect;

	UPROPERTY(BlueprintAssignable, Category = "Tcp Client")
	FDataClientDelegate EventData;

private:
	FSocket* ClientSocket;
	FTcpClientThread* ClientThread;

public:
	UFUNCTION(BlueprintCallable, Category = "Tcp Client")
	void Connect(FString Ip = "127.0.0.1", int32 Port = 25565);
	UFUNCTION(BlueprintCallable, Category = "Tcp Client")
	void Close();
	UFUNCTION(BlueprintCallable, Category = "Tcp Client")
	void Send(TArray<uint8> Data);
	UFUNCTION(BlueprintCallable, Category = "Tcp Client")
	void SendFile(FString FilePath);
	UFUNCTION(BlueprintCallable, Category = "Tcp Client", Meta=(DefaultToSelf="Outer"))
	static UTcpClient* CreateTcpClient(UObject* Outer);
	virtual void BeginDestroy() override;

	void OnConnected(bool Success);
	void OnData(TArray<uint8> Data) const;
	void OnClose();

private:
};
