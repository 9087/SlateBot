// Copyright 2025 Wu Zhiwei. All Rights Reserved.

#include "SlateBotModule.h"

DEFINE_LOG_CATEGORY(LogSlateBot);

FSlateBotModule& FSlateBotModule::Get()
{
	return FModuleManager::LoadModuleChecked<FSlateBotModule>("SlateBot");
}

void FSlateBotModule::StartupModule()
{
	UE_LOG(LogSlateBot, Log, TEXT("SlateBot module started"));
}

void FSlateBotModule::ShutdownModule()
{
	UE_LOG(LogSlateBot, Log, TEXT("SlateBot module shut down"));
}

IMPLEMENT_MODULE(FSlateBotModule, SlateBot)
