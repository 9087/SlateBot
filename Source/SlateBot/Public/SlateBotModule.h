// Copyright 2025 Wu Zhiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ISlateBotModule.h"

SLATEBOT_API DECLARE_LOG_CATEGORY_EXTERN(LogSlateBot, Log, All);

class SLATEBOT_API FSlateBotModule : public ISlateBotModule
{
public:
	static FSlateBotModule& Get();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
