#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "PicoBasePawn.generated.h"

UCLASS(Blueprintable, meta=(DisplayName="PicoBasePawn"))
class PICOBASEFRAMEWORK_API APicoBasePawn : public APawn
{
    GENERATED_BODY()
public:
    APicoBasePawn();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    void ConfigureFromStart(const class APicoBaseStart* Start);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pico Base Framework")
    TObjectPtr<USceneComponent> TrackingOrigin;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pico Base Framework")
    TObjectPtr<class UCameraComponent> VRCamera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pico Base Framework")
    TObjectPtr<class UMotionControllerComponent> LeftController;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pico Base Framework")
    TObjectPtr<class UMotionControllerComponent> RightController;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pico Base Framework")
    TObjectPtr<class UCapsuleComponent> MovementCapsule;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pico Base Framework", meta=(ClampMin="0", Units="cm/s"))
    float MoveSpeed = 300.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pico Base Framework", meta=(ClampMin="0", Units="cm"))
    float DesktopEyeHeight = 165.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pico Base Framework", meta=(ClampMin="0", ClampMax="0.95"))
    float RightStickDeadZone = 0.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pico Base Framework", meta=(ClampMin="0"))
    float MouseSensitivity = 1.0f;

    /** Block virtual locomotion; physical headset tracking remains independent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pico Base Framework", meta=(DisplayName="Enable Collision"))
    bool bEnableCollision = false;

private:
    friend class FPicoBaseFrameworkSpawnMovementTest;
    friend class FPicoBaseCollisionTest;
    void UpdateMovementCollision();
    void MoveHorizontally(const FVector& Delta);
    void DesktopMoveInput(const FInputActionValue& Value);
    void DesktopLookInput(const FInputActionValue& Value);
    void VRMoveInput(const FInputActionValue& Value);
    void UpdateViewMode(bool bVR);
    bool IsVRActive() const;
    void UpdateFloorTracking(float DeltaSeconds);
    UFUNCTION()
    void RecenterTracking();

    UPROPERTY()
    TObjectPtr<class UVRNotificationsComponent> Notifications;
    UPROPERTY(Transient)
    TObjectPtr<class UEnhancedInputLocalPlayerSubsystem> InputSubsystem;
    FVector2D DesktopInput = FVector2D::ZeroVector;
    FVector2D RightStickInput = FVector2D::ZeroVector;
    bool bVRMode = false;
    bool bAligned = false;
    bool bOriginRequested = false;
    bool bTriedLocalFloor = false;
    bool bWarnedFloor = false;
    int32 WarmupFrames = 0;
    float RetrySeconds = 0;
    FVector AlignmentWorldLocation = FVector::ZeroVector;
    float AlignmentWorldYaw = 0;
    FVector LastViewLocation = FVector::ZeroVector;
    float LastViewYaw = 0;
};

