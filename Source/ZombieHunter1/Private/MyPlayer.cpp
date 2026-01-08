// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayer.h"
#include "Enemy.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h" //getCharacter, sound


#include "GameFramework/GameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h" //GetCharacterMovement



// Sets default values
AMyPlayer::AMyPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMyPlayer::BeginPlay()
{
	Super::BeginPlay();
    SetMoney(0);
    Damage = 1;
    
    controller = Cast<APlayerController>(GetController());

    if (AGameModeBase* gameMode = GetWorld()->GetAuthGameMode())
    {
         playerStart = gameMode->FindPlayerStart(controller);
    }

    UAnimInstance* animInstance = Cast<UAnimInstance>(GetMesh()->GetAnimInstance());

    if (animInstance)
    {
        animInstance->OnPlayMontageNotifyBegin.AddDynamic(
            this, &AMyPlayer::OnNotifyBeginReceived
        ); //신호
    }

}

// Called every frame
void AMyPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

void AMyPlayer::SetCanvasWidget(UMyCanvas* CW)
{
    CanvasWidget = CW;
}

// Called to bind functionality to input
void AMyPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AMyPlayer::AddMoney()
{
    //GetTargetLocation();
    SetMoney(Money + 1);
}

void AMyPlayer::SetMoney(int value)
{
    //GetTargetLocation();
    this->Money = value;

    if (CanvasWidget)
    {
        CanvasWidget->UpdateCoinText(this->Money);
    }

}

void AMyPlayer::AddHP(int add_hp)
{
    SetHP(this->HP + add_hp);
}

void AMyPlayer::SetHP(int new_hp)
{
    this->HP = new_hp;

    FVector2D size(new_hp * 100, 50);
    if (CanvasWidget)
    {
        CanvasWidget->SetProgressUISize(size);
    }
    CheckDeath(checkDead());
}

bool AMyPlayer::checkDead()
{
    bool isDead = this->HP <= 0;
    if (isDead)
    {
        GetCharacterMovement()->DisableMovement();
        CanvasWidget->SetVisRestartButton(true); //버튼 on

        if (controller)
        {
            controller->bShowMouseCursor = true;
            controller->SetInputMode(FInputModeUIOnly());
        }
    }
    else
    {
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        CanvasWidget->SetVisRestartButton(false); //버튼 off
        
        if (controller)
        {
            //3인칭
            controller->bShowMouseCursor = false;
            controller->SetInputMode(FInputModeGameOnly());
        }

    }
    return isDead;
}

void AMyPlayer::ReStart()
{
    SetHP(5);
    SetMoney(0);
    //리스폰
    if (playerStart)
    {
        SetActorLocation(playerStart->GetActorLocation());
    }
}

void AMyPlayer::OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{

    if (hit())
    {
        //사운드
        if (AttackSound)
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,  // WorldContextObject
                AttackSound,
                GetActorLocation(),
                1.0f,  // VolumeMultiplier
                1.0f   // PitchMultiplier
            );
        }
    }
}

bool AMyPlayer::hit()
{
    bool isAttack = false;

    //여기에 함수 죄다 넣어야 함
    TArray<FHitResult> hitResults;
    FVector start = GetActorLocation();
    FVector end = start + (GetActorForwardVector() * 150.0f);
    float radius = 25.0f;

    FCollisionShape Sphere = FCollisionShape::MakeSphere(radius);
    FCollisionQueryParams queryParams;
    queryParams.AddIgnoredActor(this);

    //SweepSingleByChanne
    bool bHit = GetWorld()->SweepMultiByChannel(
        hitResults,
        start,
        end,
        FQuat::Identity,
        ECC_Pawn, //TraceChannel
        FCollisionShape::MakeSphere(radius),
        queryParams
    );

    for (const FHitResult& hit : hitResults)
    {
        AEnemy* hitEnemy = Cast<AEnemy>(hit.GetActor());
        if (hitEnemy)
        {
            UE_LOG(LogTemp, Log, TEXT("Hit Enemy!"));
            hitEnemy->AddHP(-Damage);
            FVector force = GetActorForwardVector() * 500 + FVector(0, 0, 100);
            hitEnemy->LaunchCharacter(force, false, false);
            isAttack = true;
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("oh no!"));
        }
    }
    return isAttack;
}
