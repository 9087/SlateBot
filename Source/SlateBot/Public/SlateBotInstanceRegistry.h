// Copyright 2025 Wu Zhiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class SSlateBot;

/**
 * FSlateBotInstanceRegistry — Global singleton tracking all live SSlateBot instances.
 *
 * - Holds TWeakPtr references so SlateBot widget teardown is never blocked.
 * - SSlateBot registers on Construct() and unregisters on destruction.
 * - Later phases will expose instances via USlateBotFunctionLibrary::GetSlateBotInstances().
 */
class SLATEBOT_API FSlateBotInstanceRegistry
{
public:
	/** Returns the global singleton. */
	static FSlateBotInstanceRegistry& Get();

	/** Registers a SlateBot instance. A duplicate name overwrites the previous entry. */
	void Register(FName InstanceName, TWeakPtr<SSlateBot> Instance);

	/** Unregisters the SlateBot instance with the given name. */
	void Unregister(FName InstanceName);

	/** Looks up an instance by name. Returns the Pin()'d shared pointer, or nullptr if expired/unregistered. */
	TSharedPtr<SSlateBot> Find(FName InstanceName) const;

	/** Returns the current number of registered instances (for diagnostics). */
	int32 GetInstanceCount() const { return Instances.Num(); }

	/**
	 * Returns the names of all currently-registered instances in registration order.
	 * Stale (expired) entries are skipped.
	 */
	TArray<FName> GetAllInstanceNames() const;

private:
	FSlateBotInstanceRegistry() = default;

	/** InstanceName → weak reference map. */
	TMap<FName, TWeakPtr<SSlateBot>> Instances;

	/** Registration order (may contain stale names that will be skipped during enumeration). */
	TArray<FName> InstanceOrder;
};
