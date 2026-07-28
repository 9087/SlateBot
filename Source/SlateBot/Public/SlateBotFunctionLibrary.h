// Copyright 2025 Wu Zhiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "InputCoreTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SlateBotFunctionLibrary.generated.h"

class USlateBot;
class UWidget;
class UWidgetTree;

USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotInstanceInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FName InstanceName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	TObjectPtr<USlateBot> SlateBot = nullptr;
};

USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotOperationResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bSuccess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FName ErrorCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString ErrorMessage;

	FSlateBotOperationResult& Failure(const FName InErrorCode, const FString& InErrorMessage)
	{
		bSuccess = false;
		ErrorCode = InErrorCode;
		ErrorMessage = InErrorMessage;
		return *this;
	}
};

UENUM(BlueprintType)
enum class ESlateBotClickType : uint8
{
	Single,
	Double,
};

USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotModifierKeys
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bControl = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bAlt = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bShift = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bCommand = false;
};

USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotSendClickOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FKey Button = EKeys::LeftMouseButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	ESlateBotClickType ClickType = ESlateBotClickType::Single;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FSlateBotModifierKeys ModifierKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FVector2D RelativePosition = FVector2D(0.5f, 0.5f);
};

USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotSendDragOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FVector2D FromRelativePosition = FVector2D(0.0f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FVector2D ToRelativePosition = FVector2D(1.0f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	int32 Steps = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	float DurationMs = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FKey Button = EKeys::LeftMouseButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FSlateBotModifierKeys ModifierKeys;
};

/**
 * Describes a readable property on a widget (name, type, current value,
 * and optionally the previous value when diffing).
 */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotPropertyInfo
{
	GENERATED_BODY()

	/** Object path of the widget owning this property. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString WidgetPath;

	/** Name of the property. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString PropertyName;

	/** C++ type (e.g. "FLinearColor", "bool", "FText"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString PropertyType;

	/** Previous value (from the cache). Empty on first call. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString OldValue;

	/** Current value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString NewValue;
};

/**
 * Describes a delegate property on a widget (type and binding status).
 */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotDelegateInfo
{
	GENERATED_BODY()

	/** True if at least one function is bound at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bHasBindings = false;

	/** C++ type name (e.g. "FOnPointerEvent"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString TypeName;
};

/**
 * A node in the widget tree diff returned by GetWidgetTreeDiff.
 *
 * On the first call the full tree is returned with every node's readable
 * properties (OldValue empty, NewValue = current).  On subsequent calls
 * only changed nodes (and their ancestors, to preserve the tree structure
 * from the root) are included; each changed node carries only the
 * properties that differ from the cache.
 */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotTreeNodeInfo
{
	GENERATED_BODY()

	/** Object path of the widget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString WidgetPath;

	/** Object path of the parent widget (empty for root). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString ParentPath;

	/** UClass path of the widget (e.g. "/Script/UMG.Border"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString WidgetClass;

	/**
	 * Changed properties for this node.
	 * First call -- all readable properties (OldValue empty).
	 * Subsequent -- only changed properties (OldValue populated).
	 * Unchanged ancestors included for tree structure have an empty array.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	TArray<FSlateBotPropertyInfo> Properties;

	/**
	 * Delegate properties on this widget and whether they have runtime
	 * bindings.  Included on every call for nodes that appear in the diff.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	TArray<FSlateBotDelegateInfo> Delegates;
};

/** A single property change detected by WaitForWidgetTreeDiff. */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotWidgetTreeChange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString WidgetPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString PropertyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString OldValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString NewValue;
};

/** Result returned by WaitForWidgetTreeDiff. */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotWidgetTreeDiffResult
{
	GENERATED_BODY()

	/** True if the wait timed out before any changes were detected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bTimedOut = false;

	/** Milliseconds spent waiting before returning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	int32 TimeWaitedMs = 0;

	/** Properties that changed (empty if timed out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	TArray<FSlateBotWidgetTreeChange> Changes;
};

UCLASS()
class SLATEBOT_API USlateBotFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Returns all active UMG-backed SlateBot instances.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static TArray<FSlateBotInstanceInfo> GetSlateBotInstances();

	/**
	 * Simulates a mouse click on the specified UMG widget.
	 *
	 * WebRemoteControl call payload structure:
	 * {
	 *   "ObjectPath": "/Script/SlateBot.Default__SlateBotFunctionLibrary",
	 *   "FunctionName": "SendClick",
	 *   "Parameters": {
	 *     "Widget": "/Game/MyWidgetBlueprint.MyWidgetBlueprint_C:WidgetTree.Button_123",
	 *     "Options": {
	 *       "Button": { "KeyName": "RightMouseButton" },
	 *       "ClickType": "Single",
	 *       "ModifierKeys": {
	 *         "bControl": false,
	 *         "bAlt": false,
	 *         "bShift": false,
	 *         "bCommand": false
	 *       },
	 *       "RelativePosition": { "X": 0.5, "Y": 0.5 }
	 *     }
	 *   }
	 * }
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static FSlateBotOperationResult SendClick(UWidget* Widget, const FSlateBotSendClickOptions& Options);

	// ── Keyboard input ────────────────────────────────────────────────

	/**
	 * Simulates a key press (down + up) on the currently focused widget.
	 *
	 * @param Key        The key to press (e.g. EKeys::Enter, EKeys::Tab).
	 * @param Modifiers  Modifier key state (bControl, bAlt, bShift, bCommand).
	 * @return Operation result (bSuccess = true on success).
	 *
	 * WebRemoteControl call payload structure:
	 * {
	 *   "ObjectPath": "/Script/SlateBot.Default__SlateBotFunctionLibrary",
	 *   "FunctionName": "SendKey",
	 *   "Parameters": {
	 *     "Key": { "KeyName": "Enter" },
	 *     "Modifiers": { "bControl": false, "bAlt": false, "bShift": false, "bCommand": false }
	 *   }
	 * }
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static FSlateBotOperationResult SendKey(const FKey& Key, const FSlateBotModifierKeys& Modifiers);

	/**
	 * Simulates typing a string as a sequence of character events on the
	 * currently focused widget.
	 *
	 * @param Text  The text to type.
	 * @return Operation result (bSuccess = true on success).
	 *
	 * WebRemoteControl call payload structure:
	 * {
	 *   "ObjectPath": "/Script/SlateBot.Default__SlateBotFunctionLibrary",
	 *   "FunctionName": "SendText",
	 *   "Parameters": { "Text": "Hello" }
	 * }
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static FSlateBotOperationResult SendText(const FString& Text);

	// ── Pointer input ─────────────────────────────────────────────────

	/**
	 * Moves the mouse cursor to the specified position on a widget
	 * without clicking.  Useful for triggering hover / tooltip.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static FSlateBotOperationResult SendMouseMove(UWidget* Widget, FVector2D RelativePosition, const FSlateBotModifierKeys& ModifierKeys);

	/**
	 * Simulates a mouse wheel scroll event on a widget.
	 * Positive delta = scroll up / zoom in; negative = scroll down / zoom out.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static FSlateBotOperationResult SendMouseWheel(UWidget* Widget, float Delta);

	/**
	 * Simulates a drag gesture: press at FromPosition, move to ToPosition
	 * in Steps increments over DurationMs, then release.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static FSlateBotOperationResult SendDrag(UWidget* Widget, const FSlateBotSendDragOptions& Options);

	// ── Widget‑tree diff API ──────────────────────────────────────────
	//
	// The library maintains an internal cache (per instance) of widget paths
	// and their readable property values.  Calling GetWidgetTreeDiff reads
	// the current tree, diffs it against the cache, and updates the cache.
	//
	//   GetWidgetTreeDiff -- returns the changed portion of the widget
	//                        tree.  First call returns the full tree with
	//                        all properties.  Subsequent calls return only
	//                        changed nodes + ancestors, with property diffs.
	//                        Cache is updated automatically.

	/**
	 * Returns the changed portion of the widget tree.
	 *
	 * First call: full tree with all readable properties.
	 * Subsequent calls: only changed nodes + ancestor chain.
	 *
	 * @param InstanceName  The SlateBot instance name.
	 * @return Changed tree nodes (full tree on first call).
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot|Diff")
	static TArray<FSlateBotTreeNodeInfo> GetWidgetTreeDiff(FName InstanceName);

	/**
	 * Clears the internal cache for the given instance.  The next call to
	 * GetWidgetTreeDiff will return the full tree as if it were the first
	 * call.
	 *
	 * @param InstanceName  The SlateBot instance name.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot|Diff")
	static void ResetWidgetTreeCache(FName InstanceName);

	/**
	 * Blocks the calling thread until any widget property change is detected
	 * via GetWidgetTreeDiff, or until the timeout expires.
	 *
	 * Designed for RemoteControl (HTTP worker thread) usage.  The caller MUST
	 * establish a baseline snapshot before the UI operation whose side-effects
	 * it wants to observe:
	 *
	 *   1. ResetWidgetTreeCache("Minesweeper")
	 *   2. GetWidgetTreeDiff("Minesweeper")   // baseline snapshot
	 *   3. SendClick("...Cell_0_0")            // UI operation
	 *   4. WaitForWidgetTreeDiff("Minesweeper", 2000)  // wait for changes
	 *
	 * @param InstanceName   The SlateBot instance name.
	 * @param TimeoutMs      Maximum time to wait in ms (0 = wait forever).
	 * @param PollIntervalMs Interval between diff checks in ms (default 16 ≈ 60fps).
	 * @return Diff result with changes and timing info.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot|Diff")
	static FSlateBotWidgetTreeDiffResult WaitForWidgetTreeDiff(
		FName InstanceName, int32 TimeoutMs = 2000, int32 PollIntervalMs = 16);

	/**
	 * Closes the window hosting the named SlateBot instance.
	 * Used before Live Coding so the DLL can be hot‑reloaded.
	 *
	 * @param InstanceName  The SlateBot instance name.
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static FSlateBotOperationResult CloseSlateBotWindow(FName InstanceName);

private:
	struct FWidgetPropertyValue
	{
		FString Type;
		FString Value;
	};

	struct FWidgetTreeSnapshot
	{
		TSet<FString> Paths;
		TMap<FString, TMap<FString, FWidgetPropertyValue>> Properties; // Path -> PropName -> (Type, Value)
	};

	static TMap<FName, FWidgetTreeSnapshot> WidgetTreeSnapshots;

	/** Reads all readable UPROPERTY values from a widget into a map. */
	static void ReadWidgetProperties(UWidget* Widget, TMap<FString, FWidgetPropertyValue>& OutProperties);

	/** Reads all delegate properties from a widget with their binding status. */
	static void ReadWidgetDelegates(UWidget* Widget, TArray<FSlateBotDelegateInfo>& OutDelegates);

	/** Resolves a SlateBot instance name to its root UWidgetTree. */
	static UWidgetTree* GetRootWidgetTree(FName InstanceName);
};