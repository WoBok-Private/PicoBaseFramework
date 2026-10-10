#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PicoBaseGameMode.generated.h"

UCLASS(meta=(DisplayName="PicoBaseGameMode"))
class PICOBASEFRAMEWORK_API APicoBaseGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    APicoBaseGameMode();
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
    virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;
};

