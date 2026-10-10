#include "PicoBaseInput.h"
#include "Modules/ModuleManager.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
    TStrongObjectPtr<UInputMappingContext> GPicoInputContext;
    UInputAction* DesktopMove = nullptr;
    UInputAction* DesktopLook = nullptr;
    UInputAction* VRMove = nullptr;

    UInputAction* MakeAction(const TCHAR* Name, EInputActionAccumulationBehavior Accumulation)
    {
        UInputAction* Action = NewObject<UInputAction>(GPicoInputContext.Get(), Name);
        Action->ValueType = EInputActionValueType::Axis2D;
        Action->ActionDescription = FText::FromString(Name);
        Action->AccumulationBehavior = Accumulation;
        return Action;
    }

    void MapAxis(UInputAction* Action, FKey Key, bool bToY = false, bool bNegate = false)
    {
        FEnhancedActionKeyMapping& Mapping = GPicoInputContext->MapKey(Action, Key);
        if (bNegate)
        {
            Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(GPicoInputContext.Get()));
        }
        if (bToY)
        {
            Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(GPicoInputContext.Get()));
        }
    }

    void EnsureInputContext()
    {
        if (GPicoInputContext.IsValid()) { return; }
        GPicoInputContext.Reset(NewObject<UInputMappingContext>(GetTransientPackage(), TEXT("PicoBaseFrameworkControls")));
        GPicoInputContext->ContextDescription = FText::FromString(TEXT("Pico Base Framework Controls"));
        DesktopMove = MakeAction(TEXT("PicoDesktopMove"), EInputActionAccumulationBehavior::Cumulative);
        DesktopLook = MakeAction(TEXT("PicoDesktopLook"), EInputActionAccumulationBehavior::Cumulative);
        VRMove = MakeAction(TEXT("PicoRightStickMove"), EInputActionAccumulationBehavior::Cumulative);
        MapAxis(DesktopMove, EKeys::W, true);
        MapAxis(DesktopMove, EKeys::S, true, true);
        MapAxis(DesktopMove, EKeys::D);
        MapAxis(DesktopMove, EKeys::A, false, true);
        MapAxis(DesktopLook, EKeys::MouseX);
        MapAxis(DesktopLook, EKeys::MouseY, true);
        // PICOXR 3.2.3 emits separate analog axes instead of the UE 5.8 OpenXR key.
        // Cumulative accumulation combines X and Y into a single diagonal vector.
        MapAxis(VRMove, FKey(TEXT("PICOTouch_Right_Thumbstick_X")));
        MapAxis(VRMove, FKey(TEXT("PICOTouch_Right_Thumbstick_Y")), true);
    }
}

UInputMappingContext* PicoBaseInput::GetContext() { EnsureInputContext(); return GPicoInputContext.Get(); }
const UInputAction* PicoBaseInput::GetDesktopMove() { EnsureInputContext(); return DesktopMove; }
const UInputAction* PicoBaseInput::GetDesktopLook() { EnsureInputContext(); return DesktopLook; }
const UInputAction* PicoBaseInput::GetVRMove() { EnsureInputContext(); return VRMove; }

class FPicoBaseFrameworkModule : public IModuleInterface
{
public:
    virtual void ShutdownModule() override
    {
        GPicoInputContext.Reset();
        DesktopMove = DesktopLook = VRMove = nullptr;
    }
};

IMPLEMENT_MODULE(FPicoBaseFrameworkModule, PicoBaseFramework)
