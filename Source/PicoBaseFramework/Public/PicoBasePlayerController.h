#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PicoBasePlayerController.generated.h"

UCLASS(meta=(DisplayName="PicoBasePlayerController"))
class PICOBASEFRAMEWORK_API APicoBasePlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    APicoBasePlayerController();
    virtual void AddPitchInput(float Value) override;
    virtual void AddYawInput(float Value) override;
};
