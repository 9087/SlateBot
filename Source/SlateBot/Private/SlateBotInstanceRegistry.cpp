// Copyright 2025 Wu Zhiwei. All Rights Reserved.

#include "SlateBotInstanceRegistry.h"
#include "SSlateBot.h"

FSlateBotInstanceRegistry& FSlateBotInstanceRegistry::Get()
{
	static FSlateBotInstanceRegistry Registry;
	return Registry;
}

void FSlateBotInstanceRegistry::Register(FName InstanceName, TWeakPtr<SSlateBot> Instance)
{
	if (!Instances.Contains(InstanceName))
	{
		InstanceOrder.Add(InstanceName);
	}
	Instances.Add(InstanceName, Instance);
}

void FSlateBotInstanceRegistry::Unregister(FName InstanceName)
{
	Instances.Remove(InstanceName);
	InstanceOrder.Remove(InstanceName);
}

TSharedPtr<SSlateBot> FSlateBotInstanceRegistry::Find(FName InstanceName) const
{
	const TWeakPtr<SSlateBot>* Found = Instances.Find(InstanceName);
	if (Found)
	{
		return Found->Pin();
	}
	return nullptr;
}

TArray<FName> FSlateBotInstanceRegistry::GetAllInstanceNames() const
{
	TArray<FName> ValidNames;
	ValidNames.Reserve(InstanceOrder.Num());
	for (const FName& Name : InstanceOrder)
	{
		const TWeakPtr<SSlateBot>* Found = Instances.Find(Name);
		if (Found && Found->IsValid())
		{
			ValidNames.Add(Name);
		}
	}
	return ValidNames;
}
