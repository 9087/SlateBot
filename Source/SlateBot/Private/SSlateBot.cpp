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
