// Copyright 2025 Wu Zhiwei. All Rights Reserved.

#include "USlateBot.h"
#include "SSlateBot.h"
#include "Widgets/SNullWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(USlateBot)

/////////////////////////////////////////////////////
// USlateBotSlot

void USlateBotSlot::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateBot.Reset();
}

void USlateBotSlot::BuildSlot(TSharedRef<SSlateBot> InSlateBot)
{
	SlateBot = InSlateBot;
	InSlateBot->SetContent(Content ? Content->TakeWidget() : SNullWidget::NullWidget);
}


/////////////////////////////////////////////////////
// USlateBot

USlateBot::USlateBot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USlateBot::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MySlateBot.Reset();
}

TSharedRef<SWidget> USlateBot::RebuildWidget()
{
	MySlateBot = SNew(SSlateBot)
		.InstanceName(InstanceName);

	if (GetChildrenCount() > 0)
	{
		Cast<USlateBotSlot>(GetContentSlot())->BuildSlot(MySlateBot.ToSharedRef());
	}

	return MySlateBot.ToSharedRef();
}

void USlateBot::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	// InstanceName is only meaningful at creation time — SSlateBot
	// uses it for registry registration in Construct().
}

UClass* USlateBot::GetSlotClass() const
{
	return USlateBotSlot::StaticClass();
}

void USlateBot::OnSlotAdded(UPanelSlot* InSlot)
{
	if (MySlateBot.IsValid())
	{
		Cast<USlateBotSlot>(InSlot)->BuildSlot(MySlateBot.ToSharedRef());
	}
}

void USlateBot::OnSlotRemoved(UPanelSlot* InSlot)
{
	if (MySlateBot.IsValid())
	{
		MySlateBot->SetContent(SNullWidget::NullWidget);
	}
}

#if WITH_EDITOR
const FText USlateBot::GetPaletteCategory()
{
	return NSLOCTEXT("SlateBot", "PaletteCategory", "SlateBot");
}
#endif
