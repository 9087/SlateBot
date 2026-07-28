// Copyright 2025 Wu Zhiwei. All Rights Reserved.

#include "SSlateBot.h"
#include "SlateBotInstanceRegistry.h"
#include "Widgets/SWindow.h"

void SSlateBot::Construct(const FArguments& InArgs)
{
	InstanceName = InArgs._InstanceName;

	// Register with the global registry. The registry holds a weak reference
	// so it will never prevent this widget from being destroyed.
	FSlateBotInstanceRegistry::Get().Register(InstanceName, SharedThis(this));

	// Attach caller-provided child content.
	ChildSlot
	[
		InArgs._Content.Widget
	];
}

SSlateBot::~SSlateBot()
{
	// Automatically unregister from the global registry on destruction.
	FSlateBotInstanceRegistry::Get().Unregister(InstanceName);
}

FString SSlateBot::GetWindowTitle() const
{
	// Walk up the parent widget chain to locate the owning SWindow.
	TSharedPtr<SWidget> Current = GetParentWidget();
	while (Current.IsValid())
	{
		if (TSharedPtr<SWindow> Window = StaticCastSharedPtr<SWindow>(Current))
		{
			return Window->GetTitle().ToString();
		}
		Current = Current->GetParentWidget();
	}
	return TEXT("");
}

void SSlateBot::SetContent(TSharedRef<SWidget> InContent)
{
	ChildSlot
	[
		InContent
	];
}

FString SSlateBot::RegisterWidgetId(TSharedRef<SWidget> Widget)
{
	SWidget* RawPtr = &Widget.Get();
	const FString Id = FString::Printf(TEXT("0x%p"), RawPtr);

	if (!WidgetCache.Contains(Id))
	{
		WidgetCache.Add(Id, TWeakPtr<SWidget>(Widget));
	}
	return Id;
}

TSharedPtr<SWidget> SSlateBot::ResolveWidgetId(const FString& WidgetId)
{
	TWeakPtr<SWidget>* Found = WidgetCache.Find(WidgetId);
	if (!Found)
	{
		return nullptr;
	}

	TSharedPtr<SWidget> Pinned = Found->Pin();
	if (!Pinned.IsValid())
	{
		// Clean up stale entry so it is never reused.
		WidgetCache.Remove(WidgetId);
	}
	return Pinned;
}
