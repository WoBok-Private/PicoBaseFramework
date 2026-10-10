#include "PicoBasePawn.h"
#include "PicoBaseStart.h"
#include "PicoBaseInput.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "CollisionQueryParams.h"
#include "MotionControllerComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "VRNotificationsComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"
#include "GameFramework/PlayerController.h"

APicoBasePawn::APicoBasePawn()
{
    PrimaryActorTick.bCanEverTick = true;
    OverrideInputComponentClass = UEnhancedInputComponent::StaticClass();
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("FloorRoot")));
    MovementCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("MovementCapsule"));
    MovementCapsule->SetupAttachment(RootComponent);
    MovementCapsule->InitCapsuleSize(25.0f, 90.0f);
    MovementCapsule->SetRelativeLocation(FVector(0, 0, 92.0f));
    MovementCapsule->SetCollisionProfileName(TEXT("Pawn"));
    MovementCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MovementCapsule->SetGenerateOverlapEvents(false);
    TrackingOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("TrackingOrigin"));
    TrackingOrigin->SetupAttachment(RootComponent);
    VRCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VRCamera"));
    VRCamera->SetupAttachment(TrackingOrigin);
    LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
    LeftController->SetupAttachment(TrackingOrigin);
    LeftController->SetTrackingMotionSource(TEXT("Left"));
    RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
    RightController->SetupAttachment(TrackingOrigin);
    RightController->SetTrackingMotionSource(TEXT("Right"));
    Notifications = CreateDefaultSubobject<UVRNotificationsComponent>(TEXT("XRNotifications"));
    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SetActorEnableCollision(false);
    bUseControllerRotationYaw = false;
}

void APicoBasePawn::ConfigureFromStart(const APicoBaseStart* Start)
{
    if (Start)
    {
        MoveSpeed = Start->MoveSpeed;
        DesktopEyeHeight = Start->DesktopEyeHeight;
        RightStickDeadZone = Start->RightStickDeadZone;
        bEnableCollision = Start->bEnableCollision;
        SetActorTransform(Start->GetFloorSpawnTransform(), false, nullptr, ETeleportType::TeleportPhysics);
    }
    AlignmentWorldLocation = GetActorLocation();
    AlignmentWorldYaw = GetActorRotation().Yaw;
    UpdateMovementCollision();
    if (HasActorBegunPlay()) { UpdateViewMode(bVRMode); }
}

void APicoBasePawn::BeginPlay()
{
    Super::BeginPlay();
    UpdateMovementCollision();
    AlignmentWorldLocation = GetActorLocation();
    AlignmentWorldYaw = GetActorRotation().Yaw;
    LastViewLocation = GetActorLocation();
    LastViewYaw = GetActorRotation().Yaw;
    Notifications->HMDRecenteredDelegate.AddDynamic(this, &APicoBasePawn::RecenterTracking);
    Notifications->HMDReconnectedDelegate.AddDynamic(this, &APicoBasePawn::RecenterTracking);
    UpdateViewMode(IsVRActive());
}

bool APicoBasePawn::IsVRActive() const
{
    // Connected/enabled hardware alone does not make ordinary editor Play a VR session.
    return GEngine && GetWorld() && GEngine->XRSystem.IsValid() &&
        GEngine->IsStereoscopic3D() && GEngine->XRSystem->IsHeadTrackingAllowedForWorld(*GetWorld()) &&
        UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
}

void APicoBasePawn::UpdateViewMode(bool bVR)
{
    bVRMode = bVR;
    VRCamera->bLockToHmd = bVR;
    VRCamera->bUsePawnControlRotation = !bVR;
    VRCamera->SetRelativeLocation(bVR ? FVector::ZeroVector : FVector(0, 0, DesktopEyeHeight));
    TrackingOrigin->SetRelativeTransform(FTransform::Identity);
    bAligned = bOriginRequested = bTriedLocalFloor = bWarnedFloor = false;
    RetrySeconds = 0;
    DesktopInput = RightStickInput = FVector2D::ZeroVector;
    AlignmentWorldLocation = GetActorLocation();
    AlignmentWorldYaw = GetActorRotation().Yaw;
}

void APicoBasePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    const auto Bind = [this, Input](const UInputAction* Action, auto Handler)
    {
        Input->BindAction(Action, ETriggerEvent::Triggered, this, Handler);
        Input->BindAction(Action, ETriggerEvent::Completed, this, Handler);
        Input->BindAction(Action, ETriggerEvent::Canceled, this, Handler);
    };
    Bind(PicoBaseInput::GetDesktopMove(), &APicoBasePawn::DesktopMoveInput);
    Bind(PicoBaseInput::GetDesktopLook(), &APicoBasePawn::DesktopLookInput);
    Bind(PicoBaseInput::GetVRMove(), &APicoBasePawn::VRMoveInput);
    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
        {
            InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
            if (InputSubsystem) { InputSubsystem->AddMappingContext(PicoBaseInput::GetContext(), 0); }
        }
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
}

void APicoBasePawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (InputSubsystem) { InputSubsystem->RemoveMappingContext(PicoBaseInput::GetContext()); }
    Super::EndPlay(EndPlayReason);
}

void APicoBasePawn::DesktopMoveInput(const FInputActionValue& Value)
{
    DesktopInput = Value.Get<FVector2D>();
}

void APicoBasePawn::DesktopLookInput(const FInputActionValue& Value)
{
    if (!bVRMode)
    {
        const FVector2D Look = Value.Get<FVector2D>();
        AddControllerYawInput(Look.X * MouseSensitivity);
        AddControllerPitchInput(Look.Y * MouseSensitivity);
    }
}

void APicoBasePawn::VRMoveInput(const FInputActionValue& Value)
{
    RightStickInput = Value.Get<FVector2D>();
}

void APicoBasePawn::RecenterTracking()
{
    if (!bVRMode) { return; }
    // Keep the current virtual position/direction instead of jumping back to the initial actor.
    AlignmentWorldLocation = FVector(LastViewLocation.X, LastViewLocation.Y, GetActorLocation().Z);
    AlignmentWorldYaw = LastViewYaw;
    bAligned = bOriginRequested = bTriedLocalFloor = false;
    RetrySeconds = 0;
}

void APicoBasePawn::UpdateFloorTracking(float DeltaSeconds)
{
    if (bAligned) { return; }
    RetrySeconds -= DeltaSeconds;
    if (!bOriginRequested)
    {
        if (RetrySeconds > 0) { return; }
        UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Stage);
        bOriginRequested = true;
        bTriedLocalFloor = false;
        WarmupFrames = 2;
        return;
    }
    if (WarmupFrames-- > 0 || !UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition()) { return; }
    const auto Origin = UHeadMountedDisplayFunctionLibrary::GetTrackingOrigin();
    if (Origin != EHMDTrackingOrigin::Stage && Origin != EHMDTrackingOrigin::LocalFloor)
    {
        if (!bTriedLocalFloor)
        {
            UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);
            bTriedLocalFloor = true;
            WarmupFrames = 2;
            return;
        }
        if (!bWarnedFloor)
        {
            UE_LOG(LogTemp, Error, TEXT("PicoBaseFramework: no floor tracking. Calibrate the headset floor; locomotion is waiting for Stage/LocalFloor."));
            bWarnedFloor = true;
        }
        bOriginRequested = false;
        RetrySeconds = 1.0f;
        return;
    }
    FRotator HeadRotation;
    FVector HeadPosition;
    UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HeadRotation, HeadPosition);
    const FRotator Alignment(0, AlignmentWorldYaw - GetActorRotation().Yaw - HeadRotation.Yaw, 0);
    TrackingOrigin->SetRelativeRotation(Alignment);
    const FVector TargetLocal = GetActorTransform().InverseTransformPosition(AlignmentWorldLocation);
    TrackingOrigin->SetRelativeLocation(FVector(TargetLocal.X, TargetLocal.Y, 0) -
        Alignment.RotateVector(FVector(HeadPosition.X, HeadPosition.Y, 0)));
    bAligned = true;
    UE_LOG(LogTemp, Display, TEXT("PicoBaseFramework: floor Z=%.2f cm, tracked eye height=%.2f cm."), GetActorLocation().Z, HeadPosition.Z);
}

void APicoBasePawn::UpdateMovementCollision()
{
    SetActorEnableCollision(bEnableCollision);
    MovementCapsule->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void APicoBasePawn::MoveHorizontally(const FVector& Delta)
{
    UpdateMovementCollision();
    if (!bEnableCollision || Delta.IsNearlyZero())
    {
        AddActorWorldOffset(Delta, false);
        return;
    }

    // Keep the actor origin on the floor. Sweep the attached capsule explicitly,
    // since UE only sweeps the root component during AddActorWorldOffset.
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PicoBaseMovement), false, this);
    const FCollisionResponseParams Responses(MovementCapsule->GetCollisionResponseToChannels());
    const FCollisionShape Shape = MovementCapsule->GetCollisionShape();
    FVector Remaining = Delta;
    for (int32 Attempt = 0; Attempt < 2 && !Remaining.IsNearlyZero(); ++Attempt)
    {
        const FVector From = MovementCapsule->GetComponentLocation();
        FHitResult Hit;
        const bool bBlocked = GetWorld()->SweepSingleByChannel(Hit, From, From + Remaining,
            MovementCapsule->GetComponentQuat(), MovementCapsule->GetCollisionObjectType(), Shape, Params, Responses);
        if (!bBlocked)
        {
            AddActorWorldOffset(Remaining, false);
            break;
        }
        if (Hit.bStartPenetrating) { break; }
        const float SafeTime = FMath::Max(0.0f, Hit.Time - 0.1f / Remaining.Size());
        AddActorWorldOffset(Remaining * SafeTime, false);
        const FVector HorizontalNormal = FVector(Hit.Normal.X, Hit.Normal.Y, 0).GetSafeNormal();
        if (HorizontalNormal.IsNearlyZero()) { break; }
        Remaining = FVector::VectorPlaneProject(Remaining * (1.0f - SafeTime), HorizontalNormal);
        Remaining.Z = 0;
    }
}

void APicoBasePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const bool bVR = IsVRActive();
    if (bVR != bVRMode) { UpdateViewMode(bVR); }
    if (bVRMode) { UpdateFloorTracking(DeltaSeconds); }
    FVector2D MovementInput = DesktopInput;
    if (bVRMode)
    {
        MovementInput = FVector2D::ZeroVector;
        if (bAligned && UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition())
        {
            const float Magnitude = RightStickInput.Size();
            const float DeadZone = FMath::Clamp(RightStickDeadZone, 0.0f, 0.95f);
            if (Magnitude > DeadZone)
            {
                MovementInput = RightStickInput / Magnitude *
                    FMath::Clamp((Magnitude - DeadZone) / (1.0f - DeadZone), 0.0f, 1.0f);
            }
        }
    }
    const float Yaw = bVRMode ? VRCamera->GetComponentRotation().Yaw : GetControlRotation().Yaw;
    const FVector LocalMovement(MovementInput.Y, MovementInput.X, 0);
    const FVector WorldMovement = FRotator(0, Yaw, 0).RotateVector(LocalMovement.GetClampedToMaxSize(1));
    MoveHorizontally(WorldMovement * FMath::Max(0.0f, MoveSpeed) * DeltaSeconds);
    LastViewLocation = VRCamera->GetComponentLocation();
    LastViewYaw = VRCamera->GetComponentRotation().Yaw;
}
