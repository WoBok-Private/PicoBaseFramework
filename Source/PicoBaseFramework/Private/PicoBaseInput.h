#pragma once

class UInputAction;
class UInputMappingContext;

namespace PicoBaseInput
{
    UInputMappingContext* GetContext();
    const UInputAction* GetDesktopMove();
    const UInputAction* GetDesktopLook();
    const UInputAction* GetVRMove();
}
