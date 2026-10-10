#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "PicoBaseGameMode.h"
#include "PicoBasePawn.h"
#include "PicoBasePlayerController.h"
#include "PicoBaseStart.h"
#include "PicoBaseInput.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "EnhancedPlayerInput.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPicoBaseFrameworkSpawnMovementTest, "PicoBaseFramework.SpawnAndHorizontalMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPicoBaseFrameworkSpawnMovementTest::RunTest(const FString&)
{
    UWorld::InitializationValues IV;
    IV.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::SM5, &IV);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    APicoBaseStart* Start = World->SpawnActor<APicoBaseStart>();
    Start->SetActorLocation(FVector(100, 200, -40));
    Start->SetActorRotation(FRotator(30, 90, 20));
    Start->SetActorScale3D(FVector(3));
    Start->MoveSpeed = 200;
    Start->DesktopEyeHeight = 172;
    APicoBaseGameMode* Mode = World->SpawnActor<APicoBaseGameMode>();
    TestTrue(TEXT("Floor actor selected without a PlayerStart"), Mode->ChoosePlayerStart(nullptr) == Start);
    APicoBasePawn* Pawn = Cast<APicoBasePawn>(Mode->SpawnDefaultPawnAtTransform(nullptr, FTransform(FVector(900, 900, 900))));
    if (!TestNotNull(TEXT("Pawn spawned"), Pawn)) { return false; }
    TestTrue(TEXT("Explicit start overrides editor spawn transform"), Pawn->GetActorLocation().Equals(Start->GetActorLocation()));
    TestTrue(TEXT("Actor scale is not applied to tracking"), Pawn->GetActorScale3D().Equals(FVector::OneVector));
    TestTrue(TEXT("Spawn ignores tilt"), Pawn->GetActorRotation().Equals(FRotator(0, 90, 0)));
    TestEqual(TEXT("Start speed copied"), Pawn->MoveSpeed, 200.0f);
    Pawn->DispatchBeginPlay();
    TestEqual(TEXT("Desktop eye height above floor"), Pawn->VRCamera->GetRelativeLocation().Z, 172.0);
    APicoBasePlayerController* PC = World->SpawnActor<APicoBasePlayerController>();
    PC->SetPawn(Pawn);
    Mode->FinishRestartPlayer(PC, FRotator(30, 45, 20));
    TestTrue(TEXT("Possession uses marker yaw rather than editor spawn rotation/tilt"), PC->GetControlRotation().Equals(FRotator(0, 90, 0)));
    PC->RotationInput = FRotator::ZeroRotator;
    // This isolated world has no LocalPlayer; APawn intentionally filters look input
    // for such controllers. Exercise the receiving controller directly instead.
    PC->AddYawInput(1);
    PC->AddPitchInput(-1);
    TestTrue(TEXT("Mouse look preserves Enhanced Input sign/sensitivity regardless of legacy project scales"),
        PC->RotationInput.Equals(FRotator(-1, 1, 0)));
    PC->SetControlRotation(FRotator(80, 90, 0));
    // A blocking volume occupies the route, including the destination.
    AActor* Blocker = World->SpawnActor<AActor>();
    UBoxComponent* Box = NewObject<UBoxComponent>(Blocker);
    Blocker->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(500));
    Box->SetCollisionProfileName(TEXT("BlockAll"));
    Box->RegisterComponent();
    Blocker->SetActorLocation(Start->GetActorLocation());
    Pawn->DesktopMoveInput(FInputActionValue(FVector2D(0, 1)));
    Pawn->Tick(1);
    TestTrue(TEXT("Forward follows yaw despite looking upward and blocking geometry"),
        Pawn->GetActorLocation().Equals(FVector(100, 400, -40), 0.01));
    Pawn->DesktopMoveInput(FInputActionValue(FVector2D(1, 1)));
    const FVector Previous = Pawn->GetActorLocation();
    Pawn->Tick(1);
    TestTrue(TEXT("Diagonal speed equals straight speed"), FMath::IsNearlyEqual((Pawn->GetActorLocation() - Previous).Size(), 200.0, 0.01));
    TestEqual(TEXT("Z stays at floor reference"), Pawn->GetActorLocation().Z, -40.0);
    Pawn->DesktopMoveInput(FInputActionValue(FVector2D::ZeroVector));
    const FVector Released = Pawn->GetActorLocation();
    Pawn->Tick(1);
    TestTrue(TEXT("Released input stops movement"), Pawn->GetActorLocation().Equals(Released));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPicoBaseInputTest, "PicoBaseFramework.PortableInputMappings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPicoBaseInputTest::RunTest(const FString&)
{
    const UInputMappingContext* Context = PicoBaseInput::GetContext();
    if (!TestNotNull(TEXT("Native mapping context exists"), Context)) { return false; }
    UEnhancedPlayerInput* PlayerInput = NewObject<UEnhancedPlayerInput>();
    UWorld::InitializationValues IV;
    IV.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* LookWorld = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::SM5, &IV);
    ON_SCOPE_EXIT { LookWorld->DestroyWorld(false); };
    APicoBasePlayerController* LookController = LookWorld->SpawnActor<APicoBasePlayerController>();
    if (!TestNotNull(TEXT("Mouse look controller spawned"), LookController)) { return false; }
    int32 MouseAxisCount = 0;
    for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
    {
        if (Mapping.Action != PicoBaseInput::GetDesktopLook()) { continue; }
        ++MouseAxisCount;
        for (const float Direction : { -1.0f, 1.0f })
        {
            FInputActionValue Value(FVector2D(Direction, 0));
            for (UInputModifier* Modifier : Mapping.Modifiers)
            {
                Value = Modifier->ModifyRaw(PlayerInput, Value, 1.0f / 60.0f);
            }
            const FVector2D Look = Value.Get<FVector2D>();
            LookController->RotationInput = FRotator::ZeroRotator;
            LookController->AddYawInput(Look.X);
            LookController->AddPitchInput(Look.Y);
            if (Mapping.Key == EKeys::MouseY)
            {
                TestTrue(TEXT("Mouse up/down produces matching pitch and view elevation"),
                    FMath::IsNearlyEqual(LookController->RotationInput.Pitch, Direction) &&
                    LookController->RotationInput.Vector().Z * Direction > 0);
            }
            else if (Mapping.Key == EKeys::MouseX)
            {
                TestTrue(TEXT("Mouse right/left produces matching yaw"),
                    FMath::IsNearlyEqual(LookController->RotationInput.Yaw, Direction));
            }
        }
    }
    TestEqual(TEXT("Both mouse axes are bound"), MouseAxisCount, 2);
    FVector Combined = FVector::ZeroVector;
    for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
    {
        if (Mapping.Action == PicoBaseInput::GetDesktopMove())
        {
            FInputActionValue Value(FVector2D(1, 0));
            for (UInputModifier* Modifier : Mapping.Modifiers)
            {
                Value = Modifier->ModifyRaw(PlayerInput, Value, 1.0f / 60.0f);
            }
            const FVector2D Axis = Value.Get<FVector2D>();
            const FVector2D Expected = Mapping.Key == EKeys::W ? FVector2D(0, 1) :
                Mapping.Key == EKeys::S ? FVector2D(0, -1) : Mapping.Key == EKeys::A ? FVector2D(-1, 0) : FVector2D(1, 0);
            TestTrue(*FString::Printf(TEXT("%s moves on the expected axis"), *Mapping.Key.ToString()), Axis.Equals(Expected));
            Combined += FVector(Axis.X, Axis.Y, 0);
        }
    }
    TestTrue(TEXT("Opposing keys cancel"), Combined.IsNearlyZero());
    bool bPicoStick = false;
    bool bStreamingStick = false;
    for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
    {
        if (Mapping.Action == PicoBaseInput::GetVRMove())
        {
            bPicoStick |= Mapping.Key == FKey(TEXT("PICO_Right_Thumbstick_2D"));
            bStreamingStick |= Mapping.Key == EKeys::OculusTouch_Right_Thumbstick_2D;
        }
    }
    TestTrue(TEXT("PICO right stick bound"), bPicoStick);
    TestTrue(TEXT("OpenXR Touch profile right stick bound"), bStreamingStick);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPicoBaseCollisionTest, "PicoBaseFramework.OptionalMovementCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPicoBaseCollisionTest::RunTest(const FString&)
{
    UWorld::InitializationValues IV;
    IV.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::SM5, &IV);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    APicoBaseStart* Start = World->SpawnActor<APicoBaseStart>();
    APicoBasePawn* Pawn = World->SpawnActor<APicoBasePawn>();
    TestFalse(TEXT("Start collision defaults to off"), Start->bEnableCollision);
    TestFalse(TEXT("Pawn collision defaults to off"), Pawn->bEnableCollision);
    AActor* Wall = World->SpawnActor<AActor>();
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(10, 500, 100));
    Box->SetCollisionProfileName(TEXT("BlockAll"));
    Wall->SetActorLocation(FVector(100, 0, 100));
    Box->RegisterComponent();

    Pawn->MoveHorizontally(FVector(200, 0, 0));
    TestTrue(TEXT("Disabled collision allows movement through the wall"), Pawn->GetActorLocation().Equals(FVector(200, 0, 0)));
    Pawn->SetActorLocation(FVector::ZeroVector);
    Start->bEnableCollision = true;
    Pawn->ConfigureFromStart(Start);
    TestTrue(TEXT("Start collision setting is copied to the pawn"), Pawn->bEnableCollision);
    TestTrue(TEXT("Collision capsule is enabled for queries"), Pawn->MovementCapsule->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
    Pawn->MoveHorizontally(FVector(200, 0, 0));
    TestTrue(TEXT("Enabled collision stops before the wall"), Pawn->GetActorLocation().X > 60 && Pawn->GetActorLocation().X < 65.1);
    TestEqual(TEXT("Collision keeps the floor origin Z"), Pawn->GetActorLocation().Z, 0.0);

    Pawn->SetActorLocation(FVector::ZeroVector);
    Pawn->MoveHorizontally(FVector(200, 200, 0));
    TestTrue(TEXT("Diagonal movement slides along the wall"), Pawn->GetActorLocation().X < 65.1 && Pawn->GetActorLocation().Y > 190);
    Pawn->bEnableCollision = false;
    Pawn->MoveHorizontally(FVector(200, 0, 0));
    TestTrue(TEXT("Collision can be turned off again"), Pawn->GetActorLocation().X > 200);
    return true;
}
#endif
