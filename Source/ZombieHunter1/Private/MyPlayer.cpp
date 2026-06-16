// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayer.h"
#include "Enemy.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/InputComponent.h" //BindAxisKey
#include "InputCoreTypes.h"            //EKeys
#include "Kismet/GameplayStatics.h" //getCharacter, sound
#include "Animation/AnimInstance.h" //SetRootMotionMode, ERootMotionMode


#include "GameFramework/GameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h" //GetCharacterMovement
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "NavigationInvokerComponent.h"
#include "VirtualJoystick.h"
#include "Engine/Engine.h" //GEngine 화면 디버그
#include "Jobs/JobComponent.h"
#include "Jobs/SwordsmanJob.h"



// Sets default values
AMyPlayer::AMyPlayer()
{
 	// Tick() 업데이트 키는 변수
	PrimaryActorTick.bCanEverTick = true;

	// 컨트롤러 회전이 캐릭터를 돌리지 않게 함 (조준 방향으로 직접 회전시킴)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 이동 방향 자동 회전 끄기 — 오른쪽 스틱(조준) 방향으로 수동 회전
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);

	// 비스듬한 탑다운 카메라 암
	TopDownBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("TopDownBoom"));
	TopDownBoom->SetupAttachment(RootComponent);
	TopDownBoom->SetUsingAbsoluteRotation(true); // 캐릭터가 회전해도 카메라는 고정
	TopDownBoom->TargetArmLength = CameraDistance;
	TopDownBoom->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
	TopDownBoom->bDoCollisionTest = false; // 탑다운: 벽에 의해 줌인되지 않게

	// 탑다운 카메라
	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(TopDownBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	// NavMesh 동적 생성 : 플레이어 주변에만 NavMesh를 깔고, 멀어지면 제거.
	// 무한 청크 맵의 시야 범위(약 ViewRadius*ChunkSize)를 덮도록 반경 설정.
	NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
	NavInvoker->SetGenerationRadii(7000.0f, 9000.0f);

	// 기본 직업: 검사. 에디터(BP)에서 DefaultJobClass를 바꾸면 다른 직업으로 시작한다.
	DefaultJobClass = USwordsmanJob::StaticClass();
}

// Called when the game starts or when spawned
void AMyPlayer::BeginPlay()
{
	Super::BeginPlay();
    SetMoney(0);
    Damage = 1;

    // 에디터에서 수정한 카메라 값 적용
    if (TopDownBoom)
    {
        TopDownBoom->TargetArmLength = CameraDistance;
        TopDownBoom->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
    }

    // BP에 남아있는 옛 카메라(3인칭 등)를 끄고, 탑다운 카메라만 활성화
    // → 블루프린트를 안 건드려도 탑다운 시점이 화면 카메라로 잡힘
    if (TopDownCamera)
    {
        TArray<UCameraComponent*> Cameras;
        GetComponents<UCameraComponent>(Cameras);
        for (UCameraComponent* Cam : Cameras)
        {
            Cam->SetActive(Cam == TopDownCamera);
        }

        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            PC->SetViewTargetWithBlend(this, 0.0f);
        }
    }

    // 무한 맵 생성기는 레벨에 직접 배치해 사용한다(폰에서 스폰하지 않음).
    // → 월드 생성 책임을 레벨/게임모드 쪽에 두고, Details 패널에서 설정·디버그.

    controller = Cast<APlayerController>(GetController());

    // 탑다운: 컨트롤러 yaw를 0으로 고정하고 마우스 Look 입력을 무시한다.
    // → BP의 "컨트롤 회전 기준 이동"이 월드축(+X/+Y) 고정 이동과 동일해지고,
    //   마우스 때문에 직진이 휘는 yaw 드리프트도 사라진다. (카메라는 절대회전이라 무관)
    if (controller)
    {
        controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
        controller->SetIgnoreLookInput(true);

        // 마우스 커서 표시 + 게임/UI 동시 입력 (데스크탑에서 마우스로 조이스틱 조작)
        controller->bShowMouseCursor = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        controller->SetInputMode(InputMode);
    }

    // 터치 조이스틱 자동 생성 (위젯 BP 없이 C++가 만들어 화면에 띄움)
    CreateTouchJoysticks();

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

        // 공격 몽타주의 루트 모션이 캐릭터 이동을 덮어써서 공격 중 못 움직이는 문제 해결.
        // IgnoreRootMotion: 루트 모션을 추출해 메시는 제자리에 고정하되 이동에는 적용하지 않음.
        // → 공격 중에도 AddMovementInput(이동 입력)이 그대로 캐릭터를 움직임.
        animInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
    }

    // 직업(Job) 컴포넌트 생성 — 시작 시 1개 고정.
    if (!DefaultJobClass)
    {
        DefaultJobClass = USwordsmanJob::StaticClass();
    }
    CurrentJob = NewObject<UJobComponent>(this, DefaultJobClass);
    if (CurrentJob)
    {
        CurrentJob->RegisterComponent();
        CurrentJob->InitializeForOwner(this);

        // 기존 BP_MyPlayer에 설정해 둔 값을 직업이 비어 있으면 승계 (검사 동작 보존).
        // → 직업 BP/C++에 별도 값을 넣으면 그쪽이 우선.
        if (!CurrentJob->AttackMontage)
        {
            CurrentJob->AttackMontage = AttackMontage;
        }
        if (!CurrentJob->AttackSound)
        {
            CurrentJob->AttackSound = AttackSound;
        }
        CurrentJob->Damage = Damage;
    }
}

// Called every frame
void AMyPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 죽으면 이동/조준/공격 입력 처리를 멈춤 (죽은 뒤 공격 몽타주가 또 재생되는 것 방지)
    if (HP <= 0)
    {
        return;
    }

    // 마우스(로스트아크식)를 기존 스틱 포맷으로 변환:
    // 커서 방향 (dx,dy) → FVector2D(Y=dx, X=dy). UpdateMovement/UpdateAimAndAttack의 축 매핑과 일치.
    FVector2D MouseMove = FVector2D::ZeroVector;
    FVector2D MouseAim = FVector2D::ZeroVector;
    if (bRightMouseHeld || bLeftMouseHeld)
    {
        FVector Cursor;
        if (GetCursorGroundLocation(Cursor))
        {
            FVector ToCursor = Cursor - GetActorLocation();
            ToCursor.Z = 0.0f;
            if (ToCursor.Normalize()) // 커서가 캐릭터 위에 겹치면 방향 없음 → 입력 무시
            {
                const FVector2D Dir(ToCursor.Y, ToCursor.X);
                if (bRightMouseHeld) { MouseMove = Dir; }
                if (bLeftMouseHeld)  { MouseAim = Dir; }
            }
        }
    }

    // 게임패드 / 터치 / 마우스 중 가장 크게 입력된 쪽을 사용 (전부 지원)
    auto Largest = [](const FVector2D& A, const FVector2D& B, const FVector2D& C) -> FVector2D
    {
        const FVector2D& AB = (A.SizeSquared() >= B.SizeSquared()) ? A : B;
        return (AB.SizeSquared() >= C.SizeSquared()) ? AB : C;
    };
    const FVector2D Move = Largest(TouchMove, GamepadMove, MouseMove);
    const FVector2D Aim = Largest(TouchAim, GamepadAim, MouseAim);

    UpdateMovement(DeltaTime, Move);
    UpdateAimAndAttack(DeltaTime, Aim, Move);

    // 직업 패시브(힐러 자가 회복 등) — 살아있을 때만 (위에서 HP<=0이면 return)
    if (CurrentJob)
    {
        CurrentJob->TickJob(DeltaTime);
    }
}

void AMyPlayer::UpdateMovement(float DeltaTime, const FVector2D& Move)
{
    if (Move.SizeSquared() <= InputDeadzone * InputDeadzone)
    {
        return;
    }

    // 카메라가 고정(yaw 0)이므로 월드축 기준: 스틱 위(+Y) = 화면 위(+X), 스틱 오른쪽(+X) = +Y
    AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Move.Y);
    AddMovementInput(FVector(0.0f, 1.0f, 0.0f), Move.X);
}

void AMyPlayer::UpdateAimAndAttack(float DeltaTime, const FVector2D& Aim, const FVector2D& Move)
{
    const bool bAiming = Aim.SizeSquared() > InputDeadzone * InputDeadzone;

    // 조준 중이면 조준 방향, 아니면 이동 방향을 바라봄
    const FVector2D Face = bAiming ? Aim : Move;

    if (Face.SizeSquared() > InputDeadzone * InputDeadzone)
    {
        const FVector FaceDir(Face.Y, Face.X, 0.0f); // 이동과 동일한 축 매핑
        const FRotator TargetRot(0.0f, FaceDir.Rotation().Yaw, 0.0f);
        if (bAiming)
        {
            // 조준(좌클릭/오른쪽 스틱) 중에는 즉시 그 방향을 바라본다.
            // 부드러운 보간을 쓰면 회전이 끝나기 전에 발사돼 화살이 중간 방향으로 나가므로,
            // 발사 프레임에 정면 = 커서 방향이 되도록 스냅한다. (발사체는 액터 정면으로 나감)
            SetActorRotation(TargetRot);
        }
        else
        {
            // 단순 이동 방향 바라보기는 부드럽게 보간
            const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, TurnInterpSpeed);
            SetActorRotation(NewRot);
        }
    }

    // 조준 중에는 일정 간격으로 자동 공격 (간격은 현재 직업이 결정)
    const float Interval = CurrentJob ? CurrentJob->AttackInterval : AttackInterval;
    TimeSinceLastAttack += DeltaTime;
    if (bAiming && TimeSinceLastAttack >= Interval)
    {
        TimeSinceLastAttack = 0.0f;
        if (CurrentJob)
        {
            // 직업이 공격을 수행(보통 몽타주 재생). 몽타주 노티파이가
            // OnNotifyBeginReceived -> CurrentJob->OnAttackNotify() 로 실제 타격 판정.
            CurrentJob->Attack();
        }
    }
}


bool AMyPlayer::GetCursorGroundLocation(FVector& OutLocation) const
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return false;
    }

    // 화면의 마우스 위치를 월드 광선(원점+방향)으로 역투영
    FVector WorldOrigin, WorldDirection;
    if (!PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
    {
        return false; // 커서가 화면 밖이거나 마우스 위치 없음
    }

    // 플레이어 발 높이의 수평면(z = 플레이어 Z)과 카메라 광선의 교점을 구함.
    // 지면 콜리전에 의존하지 않아 어디를 가리켜도 안정적인 목표점이 나온다.
    if (FMath::IsNearlyZero(WorldDirection.Z))
    {
        return false; // 광선이 수평이면 평면과 안 만남
    }
    const float T = (GetActorLocation().Z - WorldOrigin.Z) / WorldDirection.Z;
    if (T < 0.0f)
    {
        return false; // 평면이 카메라 뒤쪽
    }
    OutLocation = WorldOrigin + WorldDirection * T;
    return true;
}

//모바일용
void AMyPlayer::MoveTopDown(FVector2D Value)
{
    // 카메라가 yaw 0으로 고정(월드 +X를 바라봄)이므로 컨트롤러 회전을 무시하고 월드축으로 이동.
    // UpdateMovement(게임패드)와 동일한 매핑: 스틱/키 위(+Y) = 화면 위(+X), 오른쪽(+X) = +Y
    AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value.Y);
    AddMovementInput(FVector(0.0f, 1.0f, 0.0f), Value.X);
}

void AMyPlayer::SetMoveInput(FVector2D Value) { TouchMove = Value; }
void AMyPlayer::SetAimInput(FVector2D Value) { TouchAim = Value; }

void AMyPlayer::OnMoveJoystickMoved(FVector2D Value) { SetMoveInput(Value); }
void AMyPlayer::OnAimJoystickMoved(FVector2D Value) { SetAimInput(Value); }

void AMyPlayer::CreateTouchJoysticks()
{
    if (!bCreateTouchJoysticks)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("[Joystick] PlayerController 없음 - 생성 실패"));
        }
        return;
    }

    //조이스틱
    //OnMoveJoystickMoved
    //OnAimJoystickMoved
}

void AMyPlayer::SetCanvasWidget(UMyCanvas* CW)
{
    CanvasWidget = CW;

    // 캔버스에 배치된 조이스틱의 입력을 내 콜백에 바인딩.
    // (플레이어가 캔버스를 들고 있으므로 캔버스가 거꾸로 플레이어를 찾을 필요 없음)
    if (CanvasWidget)
    {
        if (CanvasWidget->MoveJoystick)
        {
            CanvasWidget->MoveJoystick->OnJoystickMoved.AddUniqueDynamic(this, &AMyPlayer::OnMoveJoystickMoved);
        }
        if (CanvasWidget->AimJoystick)
        {
            CanvasWidget->AimJoystick->OnJoystickMoved.AddUniqueDynamic(this, &AMyPlayer::OnAimJoystickMoved);
        }
    }
}

// Input
void AMyPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // 게임패드: 별도 에셋 없이 아날로그 스틱 키에 직접 바인딩
    // (왼쪽 스틱 = 이동, 오른쪽 스틱 = 조준+공격)
    PlayerInputComponent->BindAxisKey(EKeys::Gamepad_LeftX, this, &AMyPlayer::OnMoveX);
    PlayerInputComponent->BindAxisKey(EKeys::Gamepad_LeftY, this, &AMyPlayer::OnMoveY);
    PlayerInputComponent->BindAxisKey(EKeys::Gamepad_RightX, this, &AMyPlayer::OnAimX);
    PlayerInputComponent->BindAxisKey(EKeys::Gamepad_RightY, this, &AMyPlayer::OnAimY);

    // 마우스(로스트아크식): 좌클릭 = 공격, 우클릭 = 이동. 누르는 동안 Tick에서 처리.
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AMyPlayer::OnLeftMousePressed);
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AMyPlayer::OnLeftMouseReleased);
    PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AMyPlayer::OnRightMousePressed);
    PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AMyPlayer::OnRightMouseReleased);
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
        // 진행 중인 공격 몽타주를 즉시 끊어 슬롯을 비움 → ABP의 죽음 상태가 바로 재생됨
        StopAnimMontage();

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
            // 마우스 커서를 보이게 하고, 게임+UI 입력 모드로 둠
            // → 데스크탑에서 마우스로 조이스틱을 조작할 수 있고 커서도 보임
            controller->bShowMouseCursor = true;

            FInputModeGameAndUI InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            InputMode.SetHideCursorDuringCapture(false);
            controller->SetInputMode(InputMode);
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
    // 실제 타격 판정/사운드는 현재 직업이 담당한다 (검사: 근접 스윕, 궁수: 발사체 등).
    if (CurrentJob)
    {
        CurrentJob->OnAttackNotify(NotifyName);
    }
}

void AMyPlayer::OnMoveX(float Value) { GamepadMove.X = Value; }
void AMyPlayer::OnMoveY(float Value) { GamepadMove.Y = Value; }
void AMyPlayer::OnAimX(float Value) { GamepadAim.X = Value; }
void AMyPlayer::OnAimY(float Value) { GamepadAim.Y = Value; }

// 마우스 버튼: 누르고 있는 동안만 작동(로스트아크식 조작)
void AMyPlayer::OnLeftMousePressed() { bLeftMouseHeld = true; }
void AMyPlayer::OnLeftMouseReleased() { bLeftMouseHeld = false; }
void AMyPlayer::OnRightMousePressed() { bRightMouseHeld = true; }
void AMyPlayer::OnRightMouseReleased() { bRightMouseHeld = false; }
