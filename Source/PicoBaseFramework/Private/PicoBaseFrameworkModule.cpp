#include "PicoBaseInput.h"
#include "Modules/ModuleManager.h"
#include "Features/IModularFeatures.h"
#include "IMotionController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"
#include "IOpenXRExtensionPlugin.h"
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
        VRMove = MakeAction(TEXT("PicoRightStickMove"), EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
        MapAxis(DesktopMove, EKeys::W, true);
        MapAxis(DesktopMove, EKeys::S, true, true);
        MapAxis(DesktopMove, EKeys::D);
        MapAxis(DesktopMove, EKeys::A, false, true);
        MapAxis(DesktopLook, EKeys::MouseX);
        MapAxis(DesktopLook, EKeys::MouseY, true);
        GPicoInputContext->MapKey(VRMove, FKey(TEXT("PICO_Right_Thumbstick_2D")));
        GPicoInputContext->MapKey(VRMove, EKeys::OculusTouch_Right_Thumbstick_2D);
    }
}

UInputMappingContext* PicoBaseInput::GetContext() { EnsureInputContext(); return GPicoInputContext.Get(); }
const UInputAction* PicoBaseInput::GetDesktopMove() { EnsureInputContext(); return DesktopMove; }
const UInputAction* PicoBaseInput::GetDesktopLook() { EnsureInputContext(); return DesktopLook; }
const UInputAction* PicoBaseInput::GetVRMove() { EnsureInputContext(); return VRMove; }

class FPicoBaseFrameworkModule : public IModuleInterface, public IOpenXRExtensionPlugin
{
public:
    virtual void StartupModule() override
    {
        RegisterOpenXRExtensionModularFeature();
    }

    virtual FString GetDisplayName() override { return TEXT("PicoBaseFramework"); }

    virtual const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override
    {
        // Register once for each new session, before OpenXR builds its action sets.
        // Session destruction clears the engine's context map, so register again on recreation.
        TSet<TObjectPtr<UInputMappingContext>> Contexts;
        Contexts.Add(PicoBaseInput::GetContext());
        for (IMotionController* Device : IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>(IMotionController::GetModularFeatureName()))
        {
            if (Device->GetMotionControllerDeviceTypeName() == FName(TEXT("OpenXR")))
            {
                Device->AttachInputMappingContexts(Contexts);
            }
        }
        return InNext;
    }

    virtual void ShutdownModule() override
    {
        UnregisterOpenXRExtensionModularFeature();
        GPicoInputContext.Reset();
        DesktopMove = DesktopLook = VRMove = nullptr;
    }
};

IMPLEMENT_MODULE(FPicoBaseFrameworkModule, PicoBaseFramework)
