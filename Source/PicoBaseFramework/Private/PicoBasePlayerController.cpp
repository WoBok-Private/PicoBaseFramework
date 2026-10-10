#include "PicoBasePlayerController.h"
#include "EnhancedPlayerInput.h"

APicoBasePlayerController::APicoBasePlayerController()
{
    OverridePlayerInputClass = UEnhancedPlayerInput::StaticClass();
    bShowMouseCursor = false;
}

void APicoBasePlayerController::AddPitchInput(float Value)
{
    // Enhanced Input already applies the axis sign and sensitivity; legacy project scales
    // (including the default negative pitch scale) must not invert them a second time.
    if (!IsLookInputIgnored()) { RotationInput.Pitch += Value; }
}

void APicoBasePlayerController::AddYawInput(float Value)
{
    if (!IsLookInputIgnored()) { RotationInput.Yaw += Value; }
}
