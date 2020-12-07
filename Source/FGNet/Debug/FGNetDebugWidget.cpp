#include "FGNetDebugWidget.h"

#include "Engine/World.h"
#include "Engine/NetDriver.h"
#include "GameFrameWork/PlayerController.h"
#include "GameFrameWork/PlayerState.h"
#include "Misc/DefaultValueHelper.h"


void UFGNetDebugWidget::UpdateNetworkSimulationSettings(const FFGBlueprintNetworksSimulationSettings& InSettings)
{
	if (UWorld* World = GetWorld())
	{
		if (World->GetNetDriver() != nullptr)
		{
			FPacketSimulationSettings PacketSimulation;
			PacketSimulation.PktLagMin = InSettings.MinLatency;
			PacketSimulation.PktLagMax = InSettings.MaxLatency;
			PacketSimulation.PktLoss = InSettings.PacketLossPercentage;
			PacketSimulation.PktIncomingLagMin = InSettings.MinLatency;
			PacketSimulation.PktIncomingLagMin = InSettings.MaxLatency;
			PacketSimulation.PktIncomingLoss = InSettings.PacketLossPercentage;
			World->GetNetDriver()->SetPacketSimulationSettings(PacketSimulation);

			FFGBlueprintNetworksSimulationSettingsText SettingsText;
			SettingsText.MaxLatency = FText::FromString(FString::FromInt(InSettings.MaxLatency));
			SettingsText.MinLatency = FText::FromString(FString::FromInt(InSettings.MinLatency));
			SettingsText.PacketLossPercentage = FText::FromString(FString::FromInt(InSettings.PacketLossPercentage));

			BP_OnUpdateNetworkSimulationSettings(SettingsText);
		}
	}
}

void UFGNetDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (APlayerController * PC = GetOwningPlayer())
	{
		if (APlayerState* PlayerState = PC->GetPlayerState<APlayerState>())
		{
			BP_UpdatePing(static_cast<int32>(PlayerState->GetPing()));
		}
	}
}

