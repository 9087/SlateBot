// Copyright 2025 Wu Zhiwei. All Rights Reserved.

#include "SlateBotFunctionLibrary.h"

#include "SlateBotInstanceRegistry.h"
#include "SSlateBot.h"
#include "USlateBot.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Framework/Application/SlateApplication.h"
#include "SlateBotModule.h"
#include "Math/UnrealMathUtility.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Containers/Ticker.h"
#include "Async/TaskGraphInterfaces.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Serialization/BufferArchive.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Misc/App.h"
#include "HAL/FileManager.h"
#include "Widgets/SWindow.h"

TMap<FName, USlateBotFunctionLibrary::FWidgetTreeSnapshot> USlateBotFunctionLibrary::WidgetTreeSnapshots;

// Single-flight guard for synthetic mouse input. An asynchronous SendDrag
// occupies the primary mouse pointer over several frames, so while it runs any
// further mouse input (click/move/wheel/drag) is rejected with
// ESlateBotErrorCode::InputInProgress instead of colliding on the same pointer.
static bool bMouseDragInProgress = false;

FModifierKeysState FSlateBotModifierKeys::ToSlate() const
{
	// The engine FModifierKeysState stores each modifier's left/right state
	// separately (no "any" slot). We derive left/right so that a legacy agnostic
	// bX field means "this modifier is down on either side" (both left+right set),
	// while the bLeftX/bRightX overrides express a single side.
	auto Left  = [](bool Any, bool L, bool R) { return L || (Any && !R); };
	auto Right = [](bool Any, bool L, bool R) { return R || (Any && !L); };

	return FModifierKeysState(
		Left (bShift,   bLeftShift,   bRightShift),
		Right(bShift,   bLeftShift,   bRightShift),
		Left (bControl, bLeftControl, bRightControl),
		Right(bControl, bLeftControl, bRightControl),
		Left (bAlt,     bLeftAlt,     bRightAlt),
		Right(bAlt,     bLeftAlt,     bRightAlt),
		Left (bCommand, bLeftCommand, bRightCommand),
		Right(bCommand, bLeftCommand, bRightCommand),
		false);
}

TArray<FSlateBotInstanceInfo> USlateBotFunctionLibrary::GetSlateBotInstances()
{
	TArray<FSlateBotInstanceInfo> Result;
	// Return an empty array if not on the game thread. This is safe: the caller
	// simply sees no instances, rather than being told a mutation succeeded.
	if (!IsInGameThread())
	{
		return Result;
	}

	const FSlateBotInstanceRegistry& Registry = FSlateBotInstanceRegistry::Get();
	const TArray<FName> InstanceNames = Registry.GetAllInstanceNames();
	Result.Reserve(InstanceNames.Num());

	for (const FName& InstanceName : InstanceNames)
	{
		TSharedPtr<SSlateBot> Instance = Registry.Find(InstanceName);
		if (!Instance.IsValid())
		{
			continue;
		}

		USlateBot* SlateBotObject = nullptr;
		const TSharedPtr<FReflectionMetaData> MetaData = Instance->GetMetaData<FReflectionMetaData>();
		if (MetaData.IsValid() && MetaData->SourceObject.IsValid())
		{
			SlateBotObject = Cast<USlateBot>(MetaData->SourceObject.Get());
		}

		if (SlateBotObject == nullptr)
		{
			// Only UMG-backed SlateBot instances are supported for now.
			continue;
		}

		FSlateBotInstanceInfo Info;
		Info.InstanceName = Instance->GetInstanceName();
		Info.SlateBot = SlateBotObject;
		Result.Add(Info);
	}

	return Result;
}

bool USlateBotFunctionLibrary::IsInputBusy()
{
	return bMouseDragInProgress;
}

FSlateBotOperationResult USlateBotFunctionLibrary::SendClick(UWidget* Widget, const FSlateBotSendClickOptions& Options)
{
	FSlateBotOperationResult Result;
	if (!IsInGameThread())
	{
		return Result.Failure(ESlateBotErrorCode::NotOnGameThread,
			TEXT("This function must be called on the game thread."));
	}

	if (bMouseDragInProgress)
	{
		return Result.Failure(ESlateBotErrorCode::InputInProgress,
			TEXT("A mouse input is already in progress. Wait for it to finish (see IsInputBusy) before sending more."));
	}

	if (!Widget)
	{
		Result.Failure(ESlateBotErrorCode::InvalidArgument, TEXT("Widget must not be null."));
		return Result;
	}
	TSharedPtr<SWidget> SlateWidget = Widget->GetCachedWidget();
	if (!SlateWidget.IsValid())
	{
		Result.Failure(
			ESlateBotErrorCode::WidgetNotReady,
			TEXT("Widget has no cached Slate widget. Ensure it is constructed and visible."));
		return Result;
	}

	const FGeometry& Geometry = SlateWidget->GetCachedGeometry();
	const FVector2D Clamped(FMath::Clamp(Options.RelativePosition.X, 0.f, 1.f),
	                        FMath::Clamp(Options.RelativePosition.Y, 0.f, 1.f));
	const FVector2D ClickPoint = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * Clamped;

	FModifierKeysState ModifierState = Options.ModifierKeys.ToSlate();

	TSharedPtr<FGenericWindow> NativeWindow;
	if (const TSharedPtr<SWindow> FoundWindow = FSlateApplication::Get().FindWidgetWindow(SlateWidget.ToSharedRef()))
	{
		FoundWindow->BringToFront(true);
		NativeWindow = FoundWindow->GetNativeWindow();
	}

	const TSet<FKey> NoButtons;
	FPointerEvent OffScreenMove(FInputDeviceId::CreateFromInternalId(0), 0,
		FVector2D(-1, -1), FVector2D(-1, -1), NoButtons, FKey(), 0.f, ModifierState);
	FSlateApplication::Get().ProcessMouseMoveEvent(OffScreenMove);

	FPointerEvent MoveEvent(FInputDeviceId::CreateFromInternalId(0), 0,
		ClickPoint, ClickPoint, NoButtons, FKey(), 0.f, ModifierState);
	FSlateApplication::Get().ProcessMouseMoveEvent(MoveEvent);

	const TSet<FKey> PressedButtons{ Options.Button };

	FPointerEvent MouseDownEvent(FInputDeviceId::CreateFromInternalId(0), 0,
		ClickPoint, ClickPoint, PressedButtons, Options.Button, 0.f, ModifierState);
	FPointerEvent MouseUpEvent(FInputDeviceId::CreateFromInternalId(0), 0,
		ClickPoint, ClickPoint, NoButtons, Options.Button, 0.f, ModifierState);

	FSlateApplication::Get().ProcessMouseButtonDownEvent(NativeWindow, MouseDownEvent);
	FSlateApplication::Get().ProcessMouseButtonUpEvent(MouseUpEvent);

	if (Options.ClickType == ESlateBotClickType::Double)
	{
		FPointerEvent MouseDoubleClickEvent(FInputDeviceId::CreateFromInternalId(0), 0,
			ClickPoint, ClickPoint, PressedButtons, Options.Button, 0.f, ModifierState);
		FSlateApplication::Get().ProcessMouseButtonDoubleClickEvent(NativeWindow, MouseDoubleClickEvent);
		FSlateApplication::Get().ProcessMouseButtonUpEvent(MouseUpEvent);
	}

	Result.bSuccess = true;
	return Result;
}

FSlateBotOperationResult USlateBotFunctionLibrary::Focus(UWidget* Widget)
{
	FSlateBotOperationResult Result;
	if (!IsInGameThread())
	{
		return Result.Failure(ESlateBotErrorCode::NotOnGameThread,
			TEXT("This function must be called on the game thread."));
	}

	if (!Widget)
	{
		return Result.Failure(ESlateBotErrorCode::InvalidArgument, TEXT("Widget must not be null."));
	}
	TSharedPtr<SWidget> SlateWidget = Widget->GetCachedWidget();
	if (!SlateWidget.IsValid())
	{
		return Result.Failure(
			ESlateBotErrorCode::WidgetNotReady,
			TEXT("Widget has no cached Slate widget. Ensure it is constructed and visible."));
	}

	// Bring the owning window to the front so it is the active window;
	// otherwise Slate will not route keyboard input to it.
	if (const TSharedPtr<SWindow> FoundWindow = FSlateApplication::Get().FindWidgetWindow(SlateWidget.ToSharedRef()))
	{
		FoundWindow->BringToFront(true);
	}

	// Move keyboard focus to the widget so subsequent SendKey / SendText land
	// on it. With the window brought to front this is the gameplay target for
	// typed input, exactly like a user clicking into a text box first.
	FSlateApplication::Get().SetKeyboardFocus(SlateWidget, EFocusCause::SetDirectly);

	Result.bSuccess = true;
	return Result;
}

FSlateBotOperationResult USlateBotFunctionLibrary::SendKey(const FKey& Key, const FSlateBotModifierKeys& Modifiers)
{
	FSlateBotOperationResult Result;
	if (!IsInGameThread())
	{
		return Result.Failure(ESlateBotErrorCode::NotOnGameThread,
			TEXT("This function must be called on the game thread."));
	}

	const FModifierKeysState ModifierState = Modifiers.ToSlate();

	FKeyEvent KeyDownEvent(Key, ModifierState, 0, false, 0, 0);
	FKeyEvent KeyUpEvent(Key, ModifierState, 0, false, 0, 0);

	FSlateApplication::Get().ProcessKeyDownEvent(KeyDownEvent);
	FSlateApplication::Get().ProcessKeyUpEvent(KeyUpEvent);

	Result.bSuccess = true;
	return Result;
}

FSlateBotOperationResult USlateBotFunctionLibrary::SendText(const FString& Text)
{
	FSlateBotOperationResult Result;
	if (!IsInGameThread())
	{
		return Result.Failure(ESlateBotErrorCode::NotOnGameThread,
			TEXT("This function must be called on the game thread."));
	}

	for (const TCHAR Char : Text)
	{
		FCharacterEvent CharEvent(Char, FModifierKeysState(), 0, false);
		FSlateApplication::Get().ProcessKeyCharEvent(CharEvent);
	}

	Result.bSuccess = true;
	return Result;
}


FSlateBotOperationResult USlateBotFunctionLibrary::SendMouseMove(
	UWidget* Widget, FVector2D RelativePosition, const FSlateBotModifierKeys& ModifierKeys)
{
	FSlateBotOperationResult Result;
	if (!IsInGameThread())
	{
		return Result.Failure(ESlateBotErrorCode::NotOnGameThread,
			TEXT("This function must be called on the game thread."));
	}

	if (bMouseDragInProgress)
	{
		return Result.Failure(ESlateBotErrorCode::InputInProgress,
			TEXT("A mouse input is already in progress. Wait for it to finish (see IsInputBusy) before sending more."));
	}

	if (!Widget)
	{
		Result.Failure(ESlateBotErrorCode::InvalidArgument, TEXT("Widget must not be null."));
		return Result;
	}
	TSharedPtr<SWidget> SlateWidget = Widget->GetCachedWidget();
	if (!SlateWidget.IsValid())
	{
		Result.Failure(
			ESlateBotErrorCode::WidgetNotReady,
			TEXT("Widget has no cached Slate widget. Ensure it is constructed and visible."));
		return Result;
	}

	const FGeometry& Geometry = SlateWidget->GetCachedGeometry();
	const FVector2D Clamped(FMath::Clamp(RelativePosition.X, 0.f, 1.f),
	                        FMath::Clamp(RelativePosition.Y, 0.f, 1.f));
	const FVector2D ClickPoint = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * Clamped;

	FModifierKeysState ModifierState = ModifierKeys.ToSlate();

	if (const TSharedPtr<SWindow> FoundWindow = FSlateApplication::Get().FindWidgetWindow(SlateWidget.ToSharedRef()))
	{
		FoundWindow->BringToFront(true);
	}

	const TSet<FKey> NoButtons;
	FPointerEvent OffScreenMove(FInputDeviceId::CreateFromInternalId(0), 0,
		FVector2D(-1, -1), FVector2D(-1, -1), NoButtons, FKey(), 0.f, ModifierState);
	FSlateApplication::Get().ProcessMouseMoveEvent(OffScreenMove);

	FPointerEvent MoveEvent(FInputDeviceId::CreateFromInternalId(0), 0,
		ClickPoint, ClickPoint, NoButtons, FKey(), 0.f, ModifierState);
	FSlateApplication::Get().ProcessMouseMoveEvent(MoveEvent);

	Result.bSuccess = true;
	return Result;
}

FSlateBotOperationResult USlateBotFunctionLibrary::SendMouseWheel(UWidget* Widget, float Delta)
{
	FSlateBotOperationResult Result;
	if (!IsInGameThread())
	{
		return Result.Failure(ESlateBotErrorCode::NotOnGameThread,
			TEXT("This function must be called on the game thread."));
	}

	if (bMouseDragInProgress)
	{
		return Result.Failure(ESlateBotErrorCode::InputInProgress,
			TEXT("A mouse input is already in progress. Wait for it to finish (see IsInputBusy) before sending more."));
	}

	if (!Widget)
	{
		Result.Failure(ESlateBotErrorCode::InvalidArgument, TEXT("Widget must not be null."));
		return Result;
	}
	TSharedPtr<SWidget> SlateWidget = Widget->GetCachedWidget();
	if (!SlateWidget.IsValid())
	{
		Result.Failure(
			ESlateBotErrorCode::WidgetNotReady,
			TEXT("Widget has no cached Slate widget. Ensure it is constructed and visible."));
		return Result;
	}

	if (const TSharedPtr<SWindow> FoundWindow = FSlateApplication::Get().FindWidgetWindow(SlateWidget.ToSharedRef()))
	{
		FoundWindow->BringToFront(true);
	}

	const TSet<FKey> NoButtons;
	FPointerEvent WheelEvent(
		FInputDeviceId::CreateFromInternalId(0), 0,
		FSlateApplication::Get().GetCursorPos(), FSlateApplication::Get().GetCursorPos(),
		NoButtons, EKeys::Invalid, Delta, FModifierKeysState());
	FSlateApplication::Get().ProcessMouseWheelOrGestureEvent(WheelEvent, nullptr);

	Result.bSuccess = true;
	return Result;
}

FSlateBotOperationResult USlateBotFunctionLibrary::SendDrag(
	UWidget* Widget, const FSlateBotSendDragOptions& Options)
{
	FSlateBotOperationResult Result;
	if (!IsInGameThread())
	{
		return Result.Failure(ESlateBotErrorCode::NotOnGameThread,
			TEXT("This function must be called on the game thread."));
	}

	if (bMouseDragInProgress)
	{
		return Result.Failure(ESlateBotErrorCode::InputInProgress,
			TEXT("A mouse input is already in progress. Wait for it to finish (see IsInputBusy) before sending more."));
	}

	if (!Widget)
	{
		Result.Failure(ESlateBotErrorCode::InvalidArgument, TEXT("Widget must not be null."));
		return Result;
	}
	TSharedPtr<SWidget> SlateWidget = Widget->GetCachedWidget();
	if (!SlateWidget.IsValid())
	{
		Result.Failure(
			ESlateBotErrorCode::WidgetNotReady,
			TEXT("Widget has no cached Slate widget. Ensure it is constructed and visible."));
		return Result;
	}

	const FGeometry& Geometry = SlateWidget->GetCachedGeometry();
	const FVector2D ClampFrom(FMath::Clamp(Options.FromRelativePosition.X, 0.f, 1.f),
	                          FMath::Clamp(Options.FromRelativePosition.Y, 0.f, 1.f));
	const FVector2D ClampTo(FMath::Clamp(Options.ToRelativePosition.X, 0.f, 1.f),
	                        FMath::Clamp(Options.ToRelativePosition.Y, 0.f, 1.f));
	const FVector2D FromPoint = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * ClampFrom;
	const FVector2D ToPoint   = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * ClampTo;

	const FModifierKeysState ModifierState = Options.ModifierKeys.ToSlate();

	TSharedPtr<FGenericWindow> NativeWindow;
	if (const TSharedPtr<SWindow> FoundWindow = FSlateApplication::Get().FindWidgetWindow(SlateWidget.ToSharedRef()))
	{
		FoundWindow->BringToFront(true);
		NativeWindow = FoundWindow->GetNativeWindow();
	}

	// Position the cursor before the press (reset hover, then move to the drag
	// origin) synchronously. The down → move* → up sequence is then driven
	// frame-by-frame by a ticker so the game thread is never blocked by a long
	// synchronous sleep (ISSUE-007).
	const TSet<FKey> NoButtons;
	FPointerEvent OffScreenMove(FInputDeviceId::CreateFromInternalId(0), 0,
		FVector2D(-1, -1), FVector2D(-1, -1), NoButtons, FKey(), 0.f, ModifierState);
	FSlateApplication::Get().ProcessMouseMoveEvent(OffScreenMove);

	FPointerEvent MoveToFrom(FInputDeviceId::CreateFromInternalId(0), 0,
		FromPoint, FromPoint, NoButtons, FKey(), 0.f, ModifierState);
	FSlateApplication::Get().ProcessMouseMoveEvent(MoveToFrom);

	// Bound the drag so a runaway request can't stall the frame loop too long.
	const int32 Steps = FMath::Clamp(Options.Steps, 1, 200);
	const float StepMs = FMath::Max(1.0f, FMath::Clamp(Options.DurationMs, 1.f, 10000.f)) / Steps;
	const double Interval = FMath::Max(0.001, static_cast<double>(StepMs) * 0.001);

	// Shared state carried by the ticker lambda from frame to frame.
	struct FDragState
	{
		TWeakPtr<SWidget> WeakWidget;
		TSharedPtr<FGenericWindow> NativeWindow;
		FModifierKeysState ModifierState;
		FKey Button;
		FVector2D FromPoint;
		FVector2D ToPoint;
		int32 Steps = 0;
		int32 CurrentStep = 0; // 0 = down, 1..Steps = moves, Steps+1 = up
		bool bDone = false;
	};

	const TSharedPtr<FDragState> State = MakeShared<FDragState>();
	State->WeakWidget      = SlateWidget;
	State->NativeWindow    = NativeWindow;
	State->ModifierState   = ModifierState;
	State->Button          = Options.Button;
	State->FromPoint       = FromPoint;
	State->ToPoint         = ToPoint;
	State->Steps           = Steps;

	// Mark a mouse input as in progress so other mouse inputs are rejected
	// (InputInProgress) while the drag occupies the primary pointer across
	// several frames.
	bMouseDragInProgress = true;

	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda(
			[State](float) -> bool
			{
				if (State->bDone)
				{
					bMouseDragInProgress = false;
					return false;
				}

				// Abort if the source widget disappeared mid-drag.
				if (!State->WeakWidget.Pin().IsValid())
				{
					State->bDone = true;
					bMouseDragInProgress = false;
					return false;
				}

				const TSet<FKey> PressedButtons{ State->Button };
				const TSet<FKey> NoButtons;

				if (State->CurrentStep == 0)
				{
					FPointerEvent DownEvent(FInputDeviceId::CreateFromInternalId(0), 0,
						State->FromPoint, State->FromPoint, PressedButtons, State->Button, 0.f, State->ModifierState);
					FSlateApplication::Get().ProcessMouseButtonDownEvent(State->NativeWindow, DownEvent);
					State->CurrentStep = 1;
					return true;
				}

				if (State->CurrentStep <= State->Steps)
				{
					const float Alpha = static_cast<float>(State->CurrentStep) / State->Steps;
					const FVector2D InterpPoint = FMath::Lerp(State->FromPoint, State->ToPoint, Alpha);
					FPointerEvent MoveEvent(FInputDeviceId::CreateFromInternalId(0), 0,
						InterpPoint, InterpPoint, PressedButtons, State->Button, 0.f, State->ModifierState);
					FSlateApplication::Get().ProcessMouseMoveEvent(MoveEvent);
					++State->CurrentStep;
					return true;
				}

				// Final: release at the target position.
				FPointerEvent UpEvent(FInputDeviceId::CreateFromInternalId(0), 0,
					State->ToPoint, State->ToPoint, NoButtons, State->Button, 0.f, State->ModifierState);
				FSlateApplication::Get().ProcessMouseButtonUpEvent(UpEvent);
				State->bDone = true;
				bMouseDragInProgress = false;
				return false;
			}),
		Interval);

	// The drag is scheduled and runs asynchronously on the game thread. The
	// caller should wait (e.g. via WaitForWidgetTreeDiff) or sleep before
	// reading the resulting state.
	Result.bSuccess = true;
	return Result;
}


void USlateBotFunctionLibrary::ReadWidgetDelegates(UWidget* Widget, TArray<FSlateBotDelegateInfo>& OutDelegates)
{
	if (!Widget)
	{
		return;
	}

	// Iterate all properties on the widget class, keeping only multicast and
	// single-cast delegate properties.  Object/class/struct/primitive properties
	// are skipped -- only FMulticastDelegateProperty and FDelegateProperty carry
	// runtime binding status that SendClick can interact with.
	for (TFieldIterator<FProperty> It(Widget->GetClass()); It; ++It)
	{
		FProperty* Property = *It;

		const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(Property);
		const FDelegateProperty* SingleProp = CastField<FDelegateProperty>(Property);
		if (!MulticastProp && !SingleProp)
		{
			continue;
		}

		FSlateBotDelegateInfo Info;
		Info.TypeName = MulticastProp
			? MulticastProp->GetCPPType(nullptr, 0)
			: SingleProp->GetCPPType(nullptr, 0);

		if (MulticastProp)
		{
			const FMulticastScriptDelegate* Delegate = MulticastProp->ContainerPtrToValuePtr<FMulticastScriptDelegate>(Widget);
			Info.bHasBindings = Delegate ? Delegate->IsBound() : false;
		}
		else if (SingleProp)
		{
			FScriptDelegate* Delegate = SingleProp->ContainerPtrToValuePtr<FScriptDelegate>(Widget);
			Info.bHasBindings = Delegate ? Delegate->IsBound() : false;
		}

		OutDelegates.Add(MoveTemp(Info));
	}
}


TArray<FSlateBotTreeNodeInfo> USlateBotFunctionLibrary::GetWidgetTreeDiff(FName InstanceName)
{
	TArray<FSlateBotTreeNodeInfo> Result;

	// The registry (a lock-less TMap) and the UObject tree must only be touched
	// on the game thread. Return empty otherwise, mirroring GetSlateBotInstances.
	if (!IsInGameThread())
	{
		return Result;
	}

	// Resolve the instance before touching the snapshot cache so a destroyed or
	// unknown instance never creates a stale snapshot entry (ISSUE-003/006).
	UWidgetTree* WidgetTree = GetRootWidgetTree(InstanceName);
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		return Result;
	}

	// Only now may we create/read the cache entry for a confirmed-live instance.
	FWidgetTreeSnapshot& Cache = WidgetTreeSnapshots.FindOrAdd(InstanceName);
	const bool bIsFirstCall = Cache.Paths.IsEmpty();

	FWidgetTreeSnapshot Current;

	// Single-pass traversal: collect properties + diff against cache.
	TFunction<void(UWidget*, const FString&)> CollectAndDiff;
	CollectAndDiff = [&](UWidget* Widget, const FString& ParentPath)
	{
		if (!Widget)
		{
			return;
		}

		const FString Path = Widget->GetPathName();
		Current.Paths.Add(Path);

		TMap<FString, FWidgetPropertyValue> CurProperties;
		ReadWidgetProperties(Widget, CurProperties);
		if (!CurProperties.IsEmpty())
		{
			Current.Properties.Add(Path, CurProperties);
		}

		const TMap<FString, FWidgetPropertyValue>* CachedProps = Cache.Properties.Find(Path);
		const bool bIsNew = !Cache.Paths.Contains(Path);

		FSlateBotTreeNodeInfo Node;
		Node.WidgetPath = Path;
		Node.ParentPath = ParentPath;
		Node.WidgetClass = Widget->GetClass()->GetPathName();

		if (bIsFirstCall || bIsNew)
		{
			ReadWidgetDelegates(Widget, Node.Delegates);
			for (const auto& Pair : CurProperties)
			{
				FSlateBotPropertyInfo Change;
				Change.WidgetPath = Path;
				Change.PropertyName = Pair.Key;
				Change.PropertyType = Pair.Value.Type;
				Change.NewValue = Pair.Value.Value;
				Node.Properties.Add(MoveTemp(Change));
			}
		}
		else if (CachedProps)
		{
			for (const auto& Pair : CurProperties)
			{
				const FString& Name = Pair.Key;
				const FString& NewValue = Pair.Value.Value;
				const FString& Type = Pair.Value.Type;
				if (const FWidgetPropertyValue* OldProp = CachedProps->Find(Name))
				{
					if (!NewValue.Equals(OldProp->Value))
					{
						FSlateBotPropertyInfo Change;
						Change.WidgetPath = Path;
						Change.PropertyName = Name;
						Change.PropertyType = Type;
						Change.OldValue = OldProp->Value;
						Change.NewValue = NewValue;
						Node.Properties.Add(MoveTemp(Change));
					}
				}
			}
		}

		Result.Add(MoveTemp(Node));

		// Recurse children
		if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
		{
			if (UserWidget->WidgetTree && UserWidget->WidgetTree->RootWidget)
			{
				CollectAndDiff(UserWidget->WidgetTree->RootWidget, Path);
			}
		}
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			const int32 Count = Panel->GetChildrenCount();
			for (int32 i = 0; i < Count; ++i)
			{
				CollectAndDiff(Panel->GetChildAt(i), Path);
			}
		}
	};

	CollectAndDiff(WidgetTree->RootWidget, FString());

	// On subsequent calls, prune unchanged nodes (keep only nodes with
	// changes or nodes that are ancestors of changed nodes).
	if (!bIsFirstCall)
	{
		TMap<FString, int32> PathToIndex;
		for (int32 i = 0; i < Result.Num(); ++i)
		{
			PathToIndex.Add(Result[i].WidgetPath, i);
		}

		TSet<FString> KeepPaths;
		for (const FSlateBotTreeNodeInfo& Node : Result)
		{
			if (Node.Properties.Num() > 0)
			{
				FString P = Node.WidgetPath;
				while (!P.IsEmpty())
				{
					KeepPaths.Add(P);
					const int32* Index = PathToIndex.Find(P);
					P = Index ? Result[*Index].ParentPath : FString();
				}
			}
		}
		Result.RemoveAll([&](const FSlateBotTreeNodeInfo& N) {
			return !KeepPaths.Contains(N.WidgetPath);
		});
	}

	// Update cache
	Cache = MoveTemp(Current);

	return Result;
}

void USlateBotFunctionLibrary::ResetWidgetTreeCache(FName InstanceName)
{
	WidgetTreeSnapshots.Remove(InstanceName);
}

void USlateBotFunctionLibrary::CleanupInstanceSnapshot(FName InstanceName)
{
	WidgetTreeSnapshots.Remove(InstanceName);
}

FSlateBotWidgetTreeDiffResult USlateBotFunctionLibrary::WaitForWidgetTreeDiff(
	FName InstanceName, int32 TimeoutMs, int32 PollIntervalMs)
{
	FSlateBotWidgetTreeDiffResult Result;

	// Poll GetWidgetTreeDiff on the game thread until it returns non-empty
	// (i.e. the cache has a baseline and changes are detected), or timeout.
	// The caller is responsible for establishing the baseline before the
	// UI operation whose side-effects it wants to observe:
	//   1. ResetWidgetTreeCache("Minesweeper")
	//   2. GetWidgetTreeDiff("Minesweeper")   // baseline snapshot
	//   3. SendClick(...)                      // operation
	//   4. WaitForWidgetTreeDiff("Minesweeper") // wait for changes

	FEvent* CompletionEvent = FPlatformProcess::GetSynchEventFromPool();
	TSharedPtr<TArray<FSlateBotWidgetTreeChange>> SharedChanges =
		MakeShared<TArray<FSlateBotWidgetTreeChange>>();
	TSharedPtr<bool> SharedGotChanges = MakeShared<bool>(false);
	TSharedPtr<int32> SharedElapsedMs = MakeShared<int32>(0);
	TSharedPtr<FTSTicker::FDelegateHandle> TickerHandle =
		MakeShared<FTSTicker::FDelegateHandle>();

	const double StartTime = FPlatformTime::Seconds();
	const float PollIntervalSec = FMath::Max(0.001f, static_cast<float>(PollIntervalMs) * 0.001f);

	FFunctionGraphTask::CreateAndDispatchWhenReady(
		[InstanceName, CompletionEvent, SharedChanges, SharedGotChanges,
		 SharedElapsedMs, TickerHandle, PollIntervalSec, StartTime]()
		{
			*TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateLambda(
					[InstanceName, CompletionEvent, SharedChanges, SharedGotChanges,
					 SharedElapsedMs, StartTime](float DeltaTime) -> bool
					{
						TArray<FSlateBotTreeNodeInfo> Diff =
							USlateBotFunctionLibrary::GetWidgetTreeDiff(InstanceName);
						if (Diff.Num() > 0)
						{
							for (const FSlateBotTreeNodeInfo& Node : Diff)
							{
								for (const FSlateBotPropertyInfo& Prop : Node.Properties)
								{
									FSlateBotWidgetTreeChange Change;
									Change.WidgetPath = Prop.WidgetPath;
									Change.PropertyName = Prop.PropertyName;
									Change.OldValue = Prop.OldValue;
									Change.NewValue = Prop.NewValue;
									SharedChanges->Add(Change);
								}
							}
							*SharedGotChanges = true;
							*SharedElapsedMs = static_cast<int32>(
								(FPlatformTime::Seconds() - StartTime) * 1000.0);
							CompletionEvent->Trigger();
							return false; // Remove ticker
						}
						return true; // Keep polling
					}),
				PollIntervalSec);
		},
		TStatId(), nullptr, ENamedThreads::GameThread);

	const bool bTriggered = CompletionEvent->Wait(
		TimeoutMs > 0 ? static_cast<uint32>(TimeoutMs) : MAX_uint32);

	// Clean up the ticker on the game thread.
	FEvent* CleanupEvent = FPlatformProcess::GetSynchEventFromPool();
	FFunctionGraphTask::CreateAndDispatchWhenReady(
		[TickerHandle, CleanupEvent]()
		{
			FTSTicker::GetCoreTicker().RemoveTicker(*TickerHandle);
			CleanupEvent->Trigger();
		},
		TStatId(), nullptr, ENamedThreads::GameThread);
	CleanupEvent->Wait();
	FPlatformProcess::ReturnSynchEventToPool(CleanupEvent);

	Result.bTimedOut = !bTriggered;
	Result.TimeWaitedMs = bTriggered ? *SharedElapsedMs : TimeoutMs;
	if (bTriggered)
	{
		Result.Changes = *SharedChanges;
		// Re-baseline so a subsequent wait doesn't see the same changes again.
		ResetWidgetTreeCache(InstanceName);
	}

	FPlatformProcess::ReturnSynchEventToPool(CompletionEvent);
	return Result;
}

FSlateBotOperationResult USlateBotFunctionLibrary::CloseSlateBotWindow(FName InstanceName)
{
	FSlateBotOperationResult Result;
	if (!IsInGameThread())
	{
		return Result.Failure(ESlateBotErrorCode::NotOnGameThread,
			TEXT("This function must be called on the game thread."));
	}

	const FSlateBotInstanceRegistry& Registry = FSlateBotInstanceRegistry::Get();
	const TSharedPtr<SSlateBot> Instance = Registry.Find(InstanceName);
	if (!Instance.IsValid())
	{
		return Result.Failure(ESlateBotErrorCode::InstanceNotFound,
			FString::Printf(TEXT("No SlateBot instance named '%s'."), *InstanceName.ToString()));
	}

	TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(Instance.ToSharedRef());
	if (!Window.IsValid())
	{
		return Result.Failure(ESlateBotErrorCode::WindowNotFound,
			TEXT("The SlateBot instance is not hosted in a top-level window."));
	}

	Window->RequestDestroyWindow();
	Result.bSuccess = true;
	return Result;
}


FSlateBotCaptureScreenshotResult USlateBotFunctionLibrary::CaptureSlateBotScreenshot(
	FName InstanceName, const FString& OutputPath, int32 Width, int32 Height)
{
	FSlateBotCaptureScreenshotResult Result;
	if (!IsInGameThread())
	{
		return Result.Failure(ESlateBotErrorCode::NotOnGameThread,
			TEXT("CaptureSlateBotScreenshot must be called on the game thread."));
	}

	const FSlateBotInstanceRegistry& Registry = FSlateBotInstanceRegistry::Get();
	const TSharedPtr<SSlateBot> Instance = Registry.Find(InstanceName);
	if (!Instance.IsValid())
	{
		return Result.Failure(ESlateBotErrorCode::InstanceNotFound,
			FString::Printf(TEXT("No SlateBot instance named '%s'."), *InstanceName.ToString()));
	}

	if (!FApp::CanEverRender())
	{
		return Result.Failure(ESlateBotErrorCode::RenderUnavailable,
			TEXT("Rendering is unavailable in this build (headless/server). Cannot capture a screenshot."));
	}

	// Resolve the instance to a Slate widget to render off-screen.
	const TSharedRef<SWidget> SlateWidget = StaticCastSharedRef<SWidget>(Instance.ToSharedRef());

	// Determine the render size: explicit override, else the widget's current
	// on-screen size, else its desired size, else a sane default.
	FVector2D DrawSize;
	{
		const FGeometry& CachedGeometry = SlateWidget->GetCachedGeometry();
		const FVector2D CachedSize = CachedGeometry.GetAbsoluteSize();
		const FVector2D DesiredSize = SlateWidget->GetDesiredSize();

		DrawSize = (Width > 0 && Height > 0)
			? FVector2D(static_cast<float>(Width), static_cast<float>(Height))
			: (CachedSize.X >= 1.f && CachedSize.Y >= 1.f ? CachedSize : DesiredSize);

		if (DrawSize.X < 1.f || DrawSize.Y < 1.f)
		{
			DrawSize = FVector2D(1200.f, 800.f);
		}
		// Bound the output to avoid pathological allocations for huge widgets.
		DrawSize.X = FMath::Clamp(DrawSize.X, 64.f, 4096.f);
		DrawSize.Y = FMath::Clamp(DrawSize.Y, 64.f, 4096.f);
	}

	// Render the widget into a fresh off-screen render target and flush the
	// draw synchronously (bDeferRenderTargetUpdate = false) so we can read the
	// pixels back in the same call.
	TSharedRef<FWidgetRenderer> WidgetRenderer = MakeShared<FWidgetRenderer>(true, true);
	UTextureRenderTarget2D* RenderTarget = WidgetRenderer->DrawWidget(SlateWidget, DrawSize);
	if (!RenderTarget)
	{
		return Result.Failure(ESlateBotErrorCode::RenderFailed,
			TEXT("The widget renderer failed to create a render target."));
	}

	// Resolve the output file path.
	FString FullPath = OutputPath.TrimStartAndEnd();
	if (FullPath.IsEmpty())
	{
		const FString ScreenshotDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
		IFileManager::Get().MakeDirectory(*ScreenshotDir, true);
		const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
		FullPath = FPaths::Combine(ScreenshotDir,
			FString::Printf(TEXT("%s_%s.png"), *InstanceName.ToString(), *Stamp));
	}
	else if (FPaths::GetExtension(FullPath).IsEmpty())
	{
		// Caller supplied a path without an extension — default to PNG.
		FullPath += TEXT(".png");
	}

	// Encode the render target into a PNG buffer and write it to disk.
	FBufferArchive Buffer;
	if (!FImageUtils::ExportRenderTarget2DAsPNG(RenderTarget, Buffer))
	{
		return Result.Failure(ESlateBotErrorCode::EncodeFailed,
			TEXT("Failed to encode the rendered widget as PNG."));
	}

	if (FArchive* Ar = IFileManager::Get().CreateFileWriter(*FullPath))
	{
		Ar->Serialize(const_cast<uint8*>(Buffer.GetData()), Buffer.Num());
		delete Ar; // CreateFileWriter returns a new archive that we own.
	}
	else
	{
		return Result.Failure(ESlateBotErrorCode::FileWriteFailed,
			FString::Printf(TEXT("Failed to create file: '%s'."), *FullPath));
	}

	Result.bSuccess = true;
	Result.ScreenshotPath = FullPath;
	return Result;
}


UWidgetTree* USlateBotFunctionLibrary::GetRootWidgetTree(FName InstanceName)
{
	const FSlateBotInstanceRegistry& Registry = FSlateBotInstanceRegistry::Get();
	const TSharedPtr<SSlateBot> Instance = Registry.Find(InstanceName);
	if (!Instance.IsValid())
	{
		return nullptr;
	}

	const TSharedPtr<FReflectionMetaData> MetaData = Instance->GetMetaData<FReflectionMetaData>();
	if (!MetaData.IsValid() || !MetaData->SourceObject.IsValid())
	{
		return nullptr;
	}

	USlateBot* SlateBotObject = Cast<USlateBot>(MetaData->SourceObject.Get());
	if (!SlateBotObject)
	{
		return nullptr;
	}

	return Cast<UWidgetTree>(SlateBotObject->GetOuter());
}

void USlateBotFunctionLibrary::ReadWidgetProperties(UWidget* Widget, TMap<FString, FWidgetPropertyValue>& OutProperties)
{
	if (!Widget)
	{
		return;
	}

	for (TFieldIterator<FProperty> It(Widget->GetClass()); It; ++It)
	{
		FProperty* Property = *It;

		if (Property->IsA<FMulticastDelegateProperty>() || Property->IsA<FDelegateProperty>())
		{
			continue;
		}
		if (Property->IsA<FObjectProperty>() || Property->IsA<FClassProperty>() ||
			Property->IsA<FSoftObjectProperty>() || Property->IsA<FSoftClassProperty>())
		{
			continue;
		}
		if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_EditorOnly | CPF_Deprecated))
		{
			continue;
		}

		FString ValueString;
		Property->ExportTextItem_InContainer(ValueString, Widget, nullptr, Widget, PPF_None);

		FWidgetPropertyValue PropVal;
		PropVal.Type = Property->GetCPPType(nullptr, 0);
		PropVal.Value = MoveTemp(ValueString);
		OutProperties.Add(Property->GetName(), MoveTemp(PropVal));
	}
}


