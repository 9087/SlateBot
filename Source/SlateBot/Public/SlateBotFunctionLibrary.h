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
class UUserWidget;
class FModifierKeysState;

USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotInstanceInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FName InstanceName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	TObjectPtr<USlateBot> SlateBot = nullptr;
};

/**
 * Error code reported by SlateBot operations.
 *
 * A type-safe replacement for the ad-hoc FName/string literals that were
 * previously used. When serialized (e.g. via RemoteControl) a UENUM exports
 * as its name string, so existing scripts that match on the textual code
 * (e.g. "NotOnGameThread") continue to work.
 */
UENUM(BlueprintType)
enum class ESlateBotErrorCode : uint8
{
	// DisplayName is set to the exact pre-enum string literal so that
	// serialization (RemoteControl/etc.) keeps emitting e.g. "InstanceNotFound"
	// rather than UE's auto-humanized "Instance Not Found", preserving
	// backward compatibility for scripts that match on the textual code.
	None            UMETA(DisplayName = "None"),
	NotOnGameThread UMETA(DisplayName = "NotOnGameThread"),
	InvalidArgument UMETA(DisplayName = "InvalidArgument"),
	WidgetNotReady  UMETA(DisplayName = "WidgetNotReady"),
	InstanceNotFound UMETA(DisplayName = "InstanceNotFound"),
	WindowNotFound  UMETA(DisplayName = "WindowNotFound"),
	RenderUnavailable UMETA(DisplayName = "RenderUnavailable"),
	RenderFailed    UMETA(DisplayName = "RenderFailed"),
	EncodeFailed    UMETA(DisplayName = "EncodeFailed"),
	FileWriteFailed UMETA(DisplayName = "FileWriteFailed"),
	// A mouse input is already in progress (e.g. an async SendDrag is running),
	// so this new mouse input was rejected instead of colliding with it.
	InputInProgress UMETA(DisplayName = "InputInProgress"),
};

USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotOperationResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bSuccess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	ESlateBotErrorCode ErrorCode = ESlateBotErrorCode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString ErrorMessage;

	FSlateBotOperationResult& Failure(const ESlateBotErrorCode InErrorCode, const FString& InErrorMessage)
	{
		bSuccess = false;
		ErrorCode = InErrorCode;
		ErrorMessage = InErrorMessage;
		return *this;
	}
};

/**
 * Result returned by CaptureSlateBotScreenshot.
 *
 * Dedicated type so the screenshot-specific output (the saved file path)
 * does not pollute the generic FSlateBotOperationResult used by the input
 * simulation functions.
 */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotCaptureScreenshotResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bSuccess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	ESlateBotErrorCode ErrorCode = ESlateBotErrorCode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString ErrorMessage;

	/** Full path of the saved PNG on success (empty on failure). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString ScreenshotPath;

	FSlateBotCaptureScreenshotResult& Failure(const ESlateBotErrorCode InErrorCode, const FString& InErrorMessage)
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

/**
 * Modifier key state for input simulation.
 *
 * The four `bControl`/`bAlt`/`bShift`/`bCommand` fields are the agnostic
 * "is this key held" switches and are fully backward compatible.
 * The `bLeft*`/`bRight*` fields are optional *overrides* that let scripts
 * distinguish the left vs right modifier (e.g. right-Control only). When all
 * of the new fields are false (the default), behavior is identical to before.
 */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotModifierKeys
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bControl = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bLeftControl = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bRightControl = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bAlt = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bLeftAlt = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bRightAlt = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bShift = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bLeftShift = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bRightShift = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bCommand = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bLeftCommand = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bRightCommand = false;

	/**
	 * Converts this Blueprint/RemoteControl-facing state into the Slate-internal
	 * FModifierKeysState (which stores left/right separately and is not
	 * reflectable). Mapping:
	 *   left  = bLeftX  || (bX && !bRightX)
	 *   right = bRightX || (bX && !bLeftX)
	 * Legacy "agnostic" bX fields therefore mean "this modifier is down on
	 * either side", matching the original behavior; the bLeftX/bRightX
	 * overrides let scripts express a single side.
	 */
	FModifierKeysState ToSlate() const;
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
 * Where a widget sits on screen, as reported by GetWidgetGeometry.
 *
 * Position and size are absolute screen pixels (already multiplied by
 * LayoutScale) - the same space a click point or a screenshot pixel lives in.
 */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotWidgetGeometry
{
	GENERATED_BODY()

	/**
	 * False when the widget has no live Slate widget yet (not constructed /
	 * not laid out), so there is no geometry to report.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bSuccess = true;

	/** Top-left corner in absolute (screen) space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FVector2D AbsolutePosition = FVector2D::ZeroVector;

	/** Size in absolute (screen) space, i.e. already multiplied by LayoutScale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FVector2D AbsoluteSize = FVector2D::ZeroVector;

	/** Size in the widget's own (unscaled) space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FVector2D LocalSize = FVector2D::ZeroVector;

	/** Accumulated layout (DPI) scale applied to this widget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	float LayoutScale = 1.0f;
};

/**
 * One item held by a UMG UListView: the data object stored in ListItems, not its
 * row widget. A row only exists while the item is on screen - see
 * FSlateBotListEntryInfo / GetListEntryInfo.
 */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotListItemInfo
{
	GENERATED_BODY()

	/** Item index in the list. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	int32 Index = 0;

	/** The item object held in ListItems. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	TObjectPtr<UObject> Item = nullptr;

	/**
	 * The item object's class. Redundant with Item->GetClass() in C++, but a
	 * caller driving this over RemoteControl only receives Item as a path string,
	 * so this is what carries the type.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	TObjectPtr<UClass> ItemClass = nullptr;
};

/**
 * The row widget of one list item, as reported by GetListEntryInfo.
 *
 * This is UE's "entry widget": a UUserWidget implementing IUserListEntry, of the
 * class given by UListView::EntryWidgetClass. Only items that are on screen have
 * one, and the list recycles them, so do not hold on to the pointer across
 * scrolls.
 */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotListEntryInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bSuccess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	ESlateBotErrorCode ErrorCode = ESlateBotErrorCode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString ErrorMessage;

	/** Item index this row belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	int32 Index = 0;

	/** The item's row widget, or null when the item has no row yet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	TObjectPtr<UUserWidget> EntryWidget = nullptr;

	/**
	 * The row widget's actual class. Not necessarily UListView::EntryWidgetClass,
	 * because a list may pick a class per item (OnGetEntryClassForItem).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	TObjectPtr<UClass> EntryWidgetClass = nullptr;

	FSlateBotListEntryInfo& Failure(const ESlateBotErrorCode InErrorCode, const FString& InErrorMessage)
	{
		bSuccess = false;
		ErrorCode = InErrorCode;
		ErrorMessage = InErrorMessage;
		return *this;
	}
};

/** Result of GetListViewInfo. */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotListViewInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	bool bSuccess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	ESlateBotErrorCode ErrorCode = ESlateBotErrorCode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString ErrorMessage;

	/** Number of items in the list. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	int32 ItemCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	TArray<FSlateBotListItemInfo> Items;

	FSlateBotListViewInfo& Failure(const ESlateBotErrorCode InErrorCode, const FString& InErrorMessage)
	{
		bSuccess = false;
		ErrorCode = InErrorCode;
		ErrorMessage = InErrorMessage;
		return *this;
	}
};

/**
 * Describes a readable property on a widget (name, type, current value,
 * and optionally the previous value when diffing).
 */
USTRUCT(BlueprintType)
struct SLATEBOT_API FSlateBotPropertyInfo
{
	GENERATED_BODY()

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
 * What happened to a node since the previous GetWidgetTreeDiff call. Applying
 * this delta lets a consumer keep a local model of the tree.
 *
 *   Add    - the node did not exist in the previous read
 *   Change - the node existed and at least one readable property differs
 *   Delete - the node existed and is gone now; only the ROOT of a removed
 *            subtree is reported, since its descendants went with it
 *   None   - the node is unchanged and present only to keep the tree structure
 *            (its property array is empty)
 *
 * The first call after a reset has no previous read, so it reports the whole
 * tree as Add.
 */
UENUM(BlueprintType)
enum class ESlateBotNodeChangeType : uint8
{
	None   UMETA(DisplayName = "None"),
	Add    UMETA(DisplayName = "Add"),
	Change UMETA(DisplayName = "Change"),
	Delete UMETA(DisplayName = "Delete"),
};

/**
 * A node in the widget tree diff returned by GetWidgetTreeDiff.
 *
 * On the first call the full tree is returned with every node's readable
 * properties (OldValue empty, NewValue = current), each reported as Add.  On
 * subsequent calls only touched nodes (and their ancestors, to preserve the
 * tree structure from the root) are included; each changed node carries only
 * the properties that differ from the cache.  See ESlateBotNodeChangeType.
 *
 * The diff compares the current tree against the previous read, so a node that
 * appears and disappears between two calls is not reported at all.
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

	/** UClass path of the widget (e.g. "/Script/UMG.Border"). Empty for Delete nodes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FString WidgetClass;

	/** What happened to this node since the previous call. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	ESlateBotNodeChangeType ChangeType = ESlateBotNodeChangeType::None;

	/**
	 * Changed properties for this node.
	 * First call -- all readable properties (OldValue empty).
	 * Subsequent -- only changed properties (OldValue populated).
	 * None (ancestor) and Delete nodes have an empty array.
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

	// ── Keyboard/focus input ──────────────────────────────────────────

	/**
	 * Moves keyboard focus to the specified widget and brings its window to
	 * the front. Subsequent SendKey / SendText calls will be routed to this
	 * widget instead of whatever had focus before.
	 *
	 * This unblocks multi-field form automation: call Focus once per
	 * field, then SendText to type into it. (SendClick alone does not
	 * guarantee the click target gains keyboard focus.)
	 *
	 * @param Widget  The UMG widget to focus.
	 *
	 * WebRemoteControl call payload structure:
	 * {
	 *   "ObjectPath": "/Script/SlateBot.Default__SlateBotFunctionLibrary",
	 *   "FunctionName": "Focus",
	 *   "Parameters": {
	 *     "Widget": "/Game/MyWidgetBlueprint.MyWidgetBlueprint_C:WidgetTree.EditBox_123",
	 *   }
	 * }
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static FSlateBotOperationResult Focus(UWidget* Widget);

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
	 *
	 * The gesture is driven frame-by-frame on the game thread via a ticker,
	 * so it never blocks the frame loop with a synchronous sleep. This call
	 * returns immediately once the drag has been *scheduled* (bSuccess = true
	 * means scheduled, not completed). To observe the drag's effect, poll
	 * IsMouseInputPending and re-read the widget tree until it settles.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static FSlateBotOperationResult SendDrag(UWidget* Widget, const FSlateBotSendDragOptions& Options);

	/**
	 * Returns true if a mouse input is currently pending (i.e. an asynchronous
	 * SendDrag has been scheduled and has not finished yet).
	 *
	 * While this returns true, any new mouse input (SendClick, SendMouseMove,
	 * SendMouseWheel, SendDrag) will fail with ESlateBotErrorCode::InputInProgress.
	 * Callers can poll this to wait for the pending input to finish before
	 * sending the next one.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static bool IsMouseInputPending();

	// ── Geometry ──────────────────────────────────────────────

	/**
	 * Returns where a widget sits on screen.
	 *
	 * Position and size are absolute screen pixels (already multiplied by
	 * LayoutScale) - the same space a click point or a screenshot pixel lives in.
	 * bSuccess is false when the widget has no live Slate widget yet, in which
	 * case the numbers are zero.
	 *
	 * @param Widget  The UMG widget to measure.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot|Geometry")
	static FSlateBotWidgetGeometry GetWidgetGeometry(UWidget* Widget);

	// ── ListView (UMG UListView) ──────────────────────────────────────

	/**
	 * Introspects a UMG UListView: how many items it holds and what each item is.
	 *
	 * This reports items only (the data objects), not their row widgets: a
	 * UListView virtualizes, so only the items on screen have a row. Use
	 * GetListEntryInfo to get the row widget of one item.
	 *
	 * @param Widget      The UListView to introspect.
	 * @param MaxItems  0 = describe every item; otherwise only the first N.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot|ListView")
	static FSlateBotListViewInfo GetListViewInfo(UWidget* Widget, int32 MaxItems = 0);

	/**
	 * Returns the row widget of one list item, as it is right now.
	 *
	 * Read-only: a UListView virtualizes, so an item that is off screen has no
	 * row and this reports ESlateBotErrorCode::WidgetNotReady. Use
	 * ScrollToListEntry to act on a row that is not on screen yet.
	 *
	 * @param Widget  The UListView.
	 * @param Index   Item index (0-based).
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot|ListView")
	static FSlateBotListEntryInfo GetListEntryInfo(UWidget* Widget, int32 Index);

	/**
	 * Scrolls the list to one item and returns its row.
	 *
	 * If the item has no row yet (it is virtualized out of view) this puts the
	 * requested line at the top of the viewport and drives the list's tick on the
	 * spot - a RemoteControl call occupies the game thread, so the engine would
	 * not tick by itself - then re-reads the row. An item whose row is already
	 * there is left where it is.
	 *
	 * @param Widget  The UListView.
	 * @param Index   Item index (0-based).
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot|ListView")
	static FSlateBotListEntryInfo ScrollToListEntry(UWidget* Widget, int32 Index);

	/**
	 * Scrolls the list to one item, then clicks its row.
	 *
	 * @param Widget   The UListView.
	 * @param Index    Item index (0-based).
	 * @param Options  Which button, and where inside the row, to click.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot|ListView")
	static FSlateBotOperationResult ScrollToListEntryAndSendClick(
		UWidget* Widget, int32 Index, const FSlateBotSendClickOptions& Options);

	// ── Widget‑tree diff API ──────────────────────────────────────────
	//
	// The library maintains an internal cache (per instance) of widget paths
	// and their readable property values.  Calling GetWidgetTreeDiff reads
	// the current tree, diffs it against the cache, and updates the cache.
	//
	//   GetWidgetTreeDiff -- returns the touched portion of the widget tree,
	//                        each node tagged with an ESlateBotNodeChangeType.
	//                        First call returns the full tree (all Add).
	//                        Subsequent calls return only touched nodes plus
	//                        the ancestors that keep the tree structure.
	//                        Cache is updated automatically.

	/**
	 * Returns the changed portion of the widget tree.
	 *
	 * First call: the full tree with all readable properties, every node marked
	 * Add.  Subsequent calls: only touched nodes plus the ancestors that keep the
	 * tree structure; each node carries an ESlateBotNodeChangeType
	 * (Add/Change/Delete) and only the properties that differ.
	 *
	 * @param InstanceName  The SlateBot instance name.
	 * @return Touched tree nodes (full tree on first call).
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
	 * Closes the window hosting the named SlateBot instance.
	 * Used before Live Coding so the DLL can be hot‑reloaded.
	 *
	 * @param InstanceName  The SlateBot instance name.
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static FSlateBotOperationResult CloseSlateBotWindow(FName InstanceName);

	// ── Capture ──────────────────────────────────────────────────────

	/**
	 * Renders the SlateBot instance (its SSlateBot and all wrapped content)
	 * off-screen and writes it to a PNG file on disk.
	 *
	 * @param InstanceName  The SlateBot instance name.
	 * @param OutputPath    Full file path (including .png extension). If
	 *                      empty, a path is generated under the project
	 *                      Saved/Screenshots directory with a timestamp.
	 * @param Width         Optional output width in pixels (0 = use the
	 *                      widget's current on-screen size, clamped).
	 * @param Height        Optional output height in pixels (0 = use the
	 *                      widget's current on-screen size, clamped).
	 * @return Capture result. On success, ScreenshotPath holds the saved file.
	 *
	 * WebRemoteControl call payload structure:
	 * {
	 *   "ObjectPath": "/Script/SlateBot.Default__SlateBotFunctionLibrary",
	 *   "FunctionName": "CaptureSlateBotScreenshot",
	 *   "Parameters": {
	 *     "InstanceName": "PuzzleApp",
	 *     "OutputPath": "",
	 *     "Width": 0,
	 *     "Height": 0
	 *   }
	 * }
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot")
	static FSlateBotCaptureScreenshotResult CaptureSlateBotScreenshot(
		FName InstanceName,
		const FString& OutputPath = TEXT(""),
		int32 Width = 0,
		int32 Height = 0);

	/**
	 * Drops all per-instance bookkeeping (the widget-tree diff snapshot and any
	 * in-flight wait) for a given instance.
	 *
	 * Called by FSlateBotInstanceRegistry::Unregister() so the state is cleaned up
	 * when an instance is destroyed - this keeps the global table from
	 * accumulating stale entries and from poisoning a same-name instance that
	 * reopens later.
	 */
	UFUNCTION(BlueprintCallable, Category = "SlateBot|Diff")
	static void CleanupInstanceInfo(FName InstanceName);

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

		// Key   = a widget's full UMG object path (UWidget::GetPathName()).
		// Value = that widget's parent widget's full object path; empty for the
		//         root widget.
		// Kept so a node that disappears can still report where it used to hang,
		// and so the root of a removed subtree can be told apart from the
		// descendants that went with it.
		TMap<FString, FString> Parents;
	};

	/**
	 * Per-instance bookkeeping kept by the library. Currently just the
	 * widget-tree snapshot the diff is computed against; keeping it as a named
	 * struct means per-instance state has a single home to inspect, reset and
	 * clean up as more of it appears.
	 */
	struct FInstanceInfo
	{
		FWidgetTreeSnapshot Snapshot;
	};

	static TMap<FName, FInstanceInfo> InstanceInfos;

	/**
	 * Single-flight guard for synthetic mouse input. True while an asynchronous
	 * SendDrag occupies the primary mouse pointer; while true other mouse inputs
	 * are rejected with ESlateBotErrorCode::InputInProgress (see IsMouseInputPending).
	 */
	static bool bMouseDragInProgress;

	/** Reads all readable UPROPERTY values from a widget into a map. */
	static void ReadWidgetProperties(UWidget* Widget, TMap<FString, FWidgetPropertyValue>& OutProperties);

	/** Reads all delegate properties from a widget with their binding status. */
	static void ReadWidgetDelegates(UWidget* Widget, TArray<FSlateBotDelegateInfo>& OutDelegates);

	/** Resolves a SlateBot instance name to its root UWidgetTree. */
	static UWidgetTree* GetRootWidgetTree(FName InstanceName);
};