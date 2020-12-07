#pragma once

#include "Blueprint/UserWidget.h"
#include "FGNetDebugWidget.generated.h"

USTRUCT(BlueprintType)
struct FFGBlueprintNetworksSimulationSettings
{
GENERATED_BODY()
public:
	// Minimum latency to add to packets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings", meta = (DisplayName = "Minimum Latency", ClampMin = "0", ClampMax = "5000"))
	int32 MinLatency = 0;

	// Maximum latency to add to packets. We use a random value between the minimum and maximum (when 0 = always the minimum value)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings", meta = (DisplayName = "Maximum Latency", ClampMin = "0", ClampMax = "5000"))
	int32 MaxLatency = 0;

	// Ratio of packets to randomly drop (0 = none, 100 = all)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings", meta = (ClampMin = "0", ClampMax = "100"))
	int32 PacketLossPercentage = 0;
};

USTRUCT(BlueprintType)
struct FFGBlueprintNetworksSimulationSettingsText
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings", meta = (DisplayName = "Minimum Latency"))
	FText MinLatency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings", meta = (DisplayName = "Maximum Latency"))
	FText MaxLatency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings")
	FText PacketLossPercentage;
};

UCLASS()
class FGNET_API UFGNetDebugWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = Widget)
	void UpdateNetworkSimulationSettings(const FFGBlueprintNetworksSimulationSettings& InSettings);

	UFUNCTION(BlueprintImplementableEvent, Category = Widget, Meta = (DisplayName = "On Update Networks Simulation Settings"))
	void BP_OnUpdateNetworkSimulationSettings(const FFGBlueprintNetworksSimulationSettingsText& Settings);

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = Widget, meta = (DisplayName = "On Update Ping"))
	void BP_UpdatePing(int32 Ping);

	UFUNCTION(BlueprintImplementableEvent, Category = Widget, meta = (DisplayName = "On Show Widget"))
	void BP_OnShowWidget();

	UFUNCTION(BlueprintImplementableEvent, Category = Widget, meta = (DisplayName = "On Hide Widget"))
	void BP_OnHideWidget();
};