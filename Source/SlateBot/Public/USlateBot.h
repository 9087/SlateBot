// Copyright 2025 Wu Zhiwei. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ContentWidget.h"
#include "Components/PanelSlot.h"
#include "USlateBot.generated.h"

class SSlateBot;

/**
 * Slot for USlateBot — single child container.
 */
UCLASS()
class SLATEBOT_API USlateBotSlot : public UPanelSlot
{
	GENERATED_BODY()

public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	/** Builds the slot content into the parent SSlateBot widget. */
	void BuildSlot(TSharedRef<SSlateBot> InSlateBot);

protected:
	TWeakPtr<SSlateBot> SlateBot;
};


/**
 * USlateBot — UMG wrapper for SSlateBot.
 *
 * Wraps child content so it is visible to the SlateBot AI Agent.
 */
UCLASS()
class SLATEBOT_API USlateBot : public UContentWidget
{
	GENERATED_BODY()

public:
	USlateBot(const FObjectInitializer& ObjectInitializer);

	/** Globally unique name used by the AI Agent to locate this instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlateBot")
	FName InstanceName;

public:
	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;
	//~ End UWidget Interface

	//~ Begin UVisual Interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~ End UVisual Interface

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	//~ Begin UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~ End UWidget Interface

	//~ Begin UPanelWidget Interface
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* InSlot) override;
	virtual void OnSlotRemoved(UPanelSlot* InSlot) override;
	//~ End UPanelWidget Interface

private:
	TSharedPtr<SSlateBot> MySlateBot;
};
