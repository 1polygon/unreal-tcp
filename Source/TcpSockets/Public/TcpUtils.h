// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TcpUtils.generated.h"

/**
 * 
 */
UCLASS()
class TCPSOCKETS_API UTcpUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="TcpSockets")
	static TArray<uint8> StringToByteArray(FString String);

	UFUNCTION(BlueprintCallable, Category="TcpSockets")
	static FString ByteArrayToString(TArray<uint8> Bytes);
};
