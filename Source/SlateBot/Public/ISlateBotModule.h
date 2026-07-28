// Copyright 2025 Wu Zhiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class SLATEBOT_API ISlateBotModule : public IModuleInterface
{
public:
	static inline ISlateBotModule& Get()
	{
		return FModuleManager::LoadModuleChecked<ISlateBotModule>("SlateBot");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("SlateBot");
	}

	virtual ~ISlateBotModule() = default;
};
