#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PicoBaseStart.generated.h"

/** Place the actor origin ON the floor. The arrow defines the initial viewing direction. */
UCLASS(Blueprintable, meta=(DisplayName="PicoBaseStart"))
class PICOBASEFRAMEWORK_API APicoBaseStart : public AActor
{
    GENERATED_BODY()
public:
    APicoBaseStart();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pico Base Framework", meta=(ClampMin="0", ToolTip="Movement speed in centimeters per second."))
    float MoveSpeed = 300.0f;

    /** Only for desktop preview; never added to tracked VR eye height. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pico Base Framework", meta=(ClampMin="0", Units="cm"))
    float DesktopEyeHeight = 165.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pico Base Framework", meta=(ClampMin="0", ClampMax="0.95"))
    float RightStickDeadZone = 0.2f;

    /** Block virtual locomotion against the player's capsule collision profile. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pico Base Framework", meta=(DisplayName="Enable Collision"))
    bool bEnableCollision = false;

    /** Higher priority wins if multiple start actors exist. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pico Base Framework")
    int32 Priority = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pico Base Framework")
    TObjectPtr<class UArrowComponent> FacingArrow;

    FTransform GetFloorSpawnTransform() const;
};

