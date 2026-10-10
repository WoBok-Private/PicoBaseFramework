#include "PicoBaseGameMode.h"
#include "PicoBasePawn.h"
#include "PicoBaseStart.h"
#include "PicoBasePlayerController.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"

APicoBaseGameMode::APicoBaseGameMode()
{
    DefaultPawnClass = APicoBasePawn::StaticClass();
    PlayerControllerClass = APicoBasePlayerController::StaticClass();
}

AActor* APicoBaseGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    APicoBaseStart* Selected = nullptr;
    int32 Count = 0;
    for (TActorIterator<APicoBaseStart> It(GetWorld()); It; ++It)
    {
        ++Count;
        if (!Selected || It->Priority > Selected->Priority ||
            (It->Priority == Selected->Priority && It->GetPathName() < Selected->GetPathName()))
        {
            Selected = *It;
        }
    }
    if (Count > 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("PicoBaseFramework: multiple floor starts; selected %s by priority/name."), *Selected->GetName());
    }
    if (Selected)
    {
        return Selected;
    }
    UE_LOG(LogTemp, Warning, TEXT("PicoBaseFramework: place a PicoBaseStart on the floor. Falling back to Player Start, whose origin will be treated as floor height."));
    return Super::ChoosePlayerStart_Implementation(Player);
}

APawn* APicoBaseGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
    // Always honor the explicit floor start, including editor 'Play From Here'.
    APicoBaseStart* Start = Cast<APicoBaseStart>(ChoosePlayerStart(NewPlayer));
    const FTransform FloorTransform = Start ? Start->GetFloorSpawnTransform() :
        FTransform(FRotator(0, SpawnTransform.Rotator().Yaw, 0), SpawnTransform.GetLocation(), FVector::OneVector);
    FActorSpawnParameters Params;
    Params.Instigator = GetInstigator();
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    APawn* Pawn = GetWorld()->SpawnActor<APawn>(GetDefaultPawnClassForController(NewPlayer), FloorTransform, Params);
    if (APicoBasePawn* PicoPawn = Cast<APicoBasePawn>(Pawn))
    {
        PicoPawn->ConfigureFromStart(Start);
    }
    return Pawn;
}

void APicoBaseGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
    // The engine may pass the editor camera / tilted start rotation here after spawning.
    const float Yaw = NewPlayer && NewPlayer->GetPawn() ? NewPlayer->GetPawn()->GetActorRotation().Yaw : StartRotation.Yaw;
    Super::FinishRestartPlayer(NewPlayer, FRotator(0, Yaw, 0));
}
