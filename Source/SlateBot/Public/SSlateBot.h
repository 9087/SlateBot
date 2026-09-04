// Copyright 2025 Wu Zhiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * SSlateBot — Access scope boundary marker widget.
 *
 * A compound Slate widget that wraps arbitrary child content.
 * Only widgets hosted under an SSlateBot are visible to the AI Agent.
 *
 * Lifecycle:
 * - Registers with FSlateBotInstanceRegistry on Construct().
 * - Unregisters on destruction; the registry holds a weak reference so
 *   widget/window teardown is never blocked.
 */
class SLATEBOT_API SSlateBot : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSlateBot)
		: _InstanceName(NAME_None)
	{}
		/** Globally unique name used by the AI Agent to locate this SlateBot instance. */
		SLATE_ARGUMENT(FName, InstanceName)
		/** Child content to be wrapped. */
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SSlateBot();

	/** Returns the globally unique name of this instance. */
	FName GetInstanceName() const { return InstanceName; }

	/**
	 * Walks up the parent widget chain to find the owning SWindow and returns its title.
	 * Returns an empty string if no window ancestor exists.
	 */
	FString GetWindowTitle() const;

	/** Replaces the child content widget. */
	void SetContent(TSharedRef<SWidget> InContent);

private:
	FName InstanceName;
};
