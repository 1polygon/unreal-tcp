// Fill out your copyright notice in the Description page of Project Settings.


#include "TcpUtils.h"


TArray<uint8> UTcpUtils::StringToBytes(FString String)
{
	TArray<uint8> ResultArray;
	for (int32 Index = 0; Index < String.Len(); ++Index)
	{
		uint8 CharAsUInt8 = static_cast<uint8>(String[Index]);
		ResultArray.Add(CharAsUInt8);
	}
	return ResultArray;
}

FString UTcpUtils::BytesToString(TArray<uint8> Bytes)
{
	FString ResultString;
	for (const uint8 Byte : Bytes)
	{
		ResultString.AppendChar(static_cast<TCHAR>(Byte));
	}
	return ResultString;
}
