#include "PicoBaseInput.h"
#include "Modules/ModuleManager.h"
#include "Features/IModularFeatures.h"
#include "IMotionController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"
#include "EnhancedInputDeveloperSettings.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/CoreRedirects.h"

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

    void AttachToControllers()
    {
        TSet<TObjectPtr<UInputMappingContext>> Contexts;
        Contexts.Add(GPicoInputContext.Get());
        for (IMotionController* Device : IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>(IMotionController::GetModularFeatureName()))
        {
            Device->AttachInputMappingContexts(Contexts);
        }
    }
}

UInputMappingContext* PicoBaseInput::GetContext() { return GPicoInputContext.Get(); }
const UInputAction* PicoBaseInput::GetDesktopMove() { return DesktopMove; }
const UInputAction* PicoBaseInput::GetDesktopLook() { return DesktopLook; }
const UInputAction* PicoBaseInput::GetVRMove() { return VRMove; }

class FPicoBaseFrameworkModule : public IModuleInterface
{
    FDelegateHandle FeatureHandle;
    FDelegateHandle WorldHandle;
public:
    virtual void StartupModule() override
    {
        // Preserve references in levels/Blueprints saved with the initial plugin name.
        const FCoreRedirect Redirects[] = {
            FCoreRedirect(ECoreRedirectFlags::Type_Package, TEXT("/Script/PicoUniversal"), TEXT("/Script/PicoBaseFramework")),
            FCoreRedirect(ECoreRedirectFlags::Type_Class, TEXT("PicoUniversalGameMode"), TEXT("/Script/PicoBaseFramework.PicoBaseGameMode")),
            FCoreRedirect(ECoreRedirectFlags::Type_Class, TEXT("PicoUniversalPawn"), TEXT("/Script/PicoBaseFramework.PicoBasePawn")),
            FCoreRedirect(ECoreRedirectFlags::Type_Class, TEXT("PicoUniversalStart"), TEXT("/Script/PicoBaseFramework.PicoBaseStart")),
            FCoreRedirect(ECoreRedirectFlags::Type_Class, TEXT("PicoUniversalPlayerController"), TEXT("/Script/PicoBaseFramework.PicoBasePlayerController"))
        };
        FCoreRedirects::AddRedirectList(Redirects, TEXT("PicoBaseFrameworkRename"));
        // Create and register the plugin's native Enhanced Input mappings.
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
        GPicoInputContext->MapKey(VRMove, EKeys::Gamepad_Right2D);

        // OpenXR needs mappings before session creation, not just when the pawn binds input.
        FDefaultContextSetting Setting;
        Setting.InputMappingContext = GPicoInputContext.Get();
        Setting.bAddImmediately = false;
        GetMutableDefault<UEnhancedInputDeveloperSettings>()->DefaultMappingContexts.Add(Setting);
        FeatureHandle = IModularFeatures::Get().OnModularFeatureRegistered().AddLambda(
            [](const FName& Name, IModularFeature* Feature)
            {
                if (Name == IMotionController::GetModularFeatureName())
                {
                    TSet<TObjectPtr<UInputMappingContext>> Contexts;
                    Contexts.Add(GPicoInputContext.Get());
                    static_cast<IMotionController*>(Feature)->AttachInputMappingContexts(Contexts);
                }
            });
        // OpenXR discards attached contexts on session shutdown. Register again for the next world.
        WorldHandle = FWorldDelegates::OnPostWorldInitialization.AddLambda(
            [](UWorld* World, const UWorld::InitializationValues)
            {
                if (World->IsGameWorld()) { AttachToControllers(); }
            });
        AttachToControllers();
    }

    virtual void ShutdownModule() override
    {
        IModularFeatures::Get().OnModularFeatureRegistered().Remove(FeatureHandle);
        FWorldDelegates::OnPostWorldInitialization.Remove(WorldHandle);
        if (UObjectInitialized())
        {
            GetMutableDefault<UEnhancedInputDeveloperSettings>()->DefaultMappingContexts.RemoveAll(
                [](const FDefaultContextSetting& Entry) { return Entry.InputMappingContext.Get() == GPicoInputContext.Get(); });
        }
        GPicoInputContext.Reset();
        DesktopMove = DesktopLook = VRMove = nullptr;
    }
};

IMPLEMENT_MODULE(FPicoBaseFrameworkModule, PicoBaseFramework)
