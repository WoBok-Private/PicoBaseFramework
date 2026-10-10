#include "PicoBaseStart.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

APicoBaseStart::APicoBaseStart()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("FloorOrigin")));
    FacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
    FacingArrow->SetupAttachment(RootComponent);
    FacingArrow->ArrowColor = FColor(40, 210, 230);
    FacingArrow->ArrowSize = 2.0f;
    FacingArrow->SetHiddenInGame(true);
    FacingArrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetActorEnableCollision(false);
}

FTransform APicoBaseStart::GetFloorSpawnTransform() const
{
    // Scale and tilt are editor conveniences, not modifications of tracking space.
    return FTransform(FRotator(0, GetActorRotation().Yaw, 0), GetActorLocation(), FVector::OneVector);
}
