// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyCanvas.h"
#include "MyPlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UAnimMontage;
class UNavigationInvokerComponent;
class UVirtualJoystick;
class UTexture2D;
class UJobComponent;

UCLASS()
class ZOMBIEHUNTER1_API AMyPlayer : public ACharacter
{
	GENERATED_BODY()
	UMyCanvas* CanvasWidget;
	APlayerController* controller;
	AActor* playerStart;

	/** 비스듬한 탑다운 카메라 암 (BP의 기존 CameraBoom과 이름 충돌 방지) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TopDown|Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* TopDownBoom;

	/** 탑다운 카메라 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TopDown|Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* TopDownCamera;

	/** NavMesh를 플레이어 주변에만 동적으로 생성시키는 인보커 (무한 맵용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TopDown|Navigation", meta = (AllowPrivateAccess = "true"))
	UNavigationInvokerComponent* NavInvoker;

	// 입력 누적값 (게임패드 / 터치를 분리 저장 후 Tick에서 합성)
	FVector2D GamepadMove = FVector2D::ZeroVector;
	FVector2D GamepadAim = FVector2D::ZeroVector;
	FVector2D TouchMove = FVector2D::ZeroVector;
	FVector2D TouchAim = FVector2D::ZeroVector;

	float TimeSinceLastAttack = 0.0f;

	// 게임패드 아날로그 축 콜백
	void OnMoveX(float Value);
	void OnMoveY(float Value);
	void OnAimX(float Value);
	void OnAimY(float Value);

	void UpdateMovement(float DeltaTime, const FVector2D& Move);
	void UpdateAimAndAttack(float DeltaTime, const FVector2D& Aim, const FVector2D& Move);

	// C++에서 생성하는 터치 조이스틱 인스턴스
	UPROPERTY()
	UVirtualJoystick* MoveJoystick;
	UPROPERTY()
	UVirtualJoystick* AimJoystick;

	void CreateTouchJoysticks();

	// 조이스틱 델리게이트 콜백 (→ SetMoveInput / SetAimInput 로 연결)
	UFUNCTION()
	void OnMoveJoystickMoved(FVector2D Value);
	UFUNCTION()
	void OnAimJoystickMoved(FVector2D Value);


public:
	// Sets default values for this character's properties
	AMyPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;





	// 블루프린트에서 읽고 쓸 수 있는 Money 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	int32 Money;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	int32 HP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	int32 Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* AttackSound; //MS


	// CheckDeath 함수가 리턴값이 없다면
	UFUNCTION(BlueprintImplementableEvent, Category = "Player")
	void CheckDeath(bool isDead);

	UFUNCTION() //몽타주의 delegate에 추가하려면 필수다.
	void OnNotifyBeginReceived(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);




	UFUNCTION(BlueprintCallable)
	void SetCanvasWidget(UMyCanvas* CW);

	//AddCoin
	UFUNCTION(BlueprintCallable)
	void AddMoney();
	

	UFUNCTION(BlueprintCallable)
	void AddHP(int add_hp);

	UFUNCTION(BlueprintCallable)
	void SetHP(int new_hp);

	UFUNCTION(BlueprintCallable)
	bool checkDead();


	void ReStart();
	void SetMoney(int Money);

	//////////////////////////////////////////////////////////////////////////
	// 탑다운 트윈스틱 설정 / 입력

	/** 카메라 내려보는 각도(피치). 음수일수록 위에서 내려봄 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Camera")
	float CameraPitch = -35.0f;

	/** 카메라 거리(암 길이) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Camera")
	float CameraDistance = 900.0f;

	/** 스틱 입력 데드존 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Input")
	float InputDeadzone = 0.25f;

	/** 캐릭터가 조준/이동 방향으로 회전하는 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Movement")
	float TurnInterpSpeed = 12.0f;

	/** 조준 중 자동 공격 간격(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Combat")
	float AttackInterval = 0.4f;

	/** 공격 시 재생할 몽타주. 노티파이가 현재 직업의 OnAttackNotify()를 호출 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Combat")
	UAnimMontage* AttackMontage = nullptr;

	//////////////////////////////////////////////////////////////////////////
	// 직업(Job) 시스템 — 시작 시 직업 1개를 생성해 부착한다.

	/** 시작 시 부착할 직업 클래스(검사/궁수/힐러/마법사). 비우면 검사로 기본 설정. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job")
	TSubclassOf<UJobComponent> DefaultJobClass;

	/** 현재 부착된 직업 컴포넌트 (런타임 생성) */
	UPROPERTY(BlueprintReadOnly, Category = "Job")
	UJobComponent* CurrentJob = nullptr;

	/** 키보드(WASD) Enhanced Input(IA_Move)에서 호출. 카메라가 고정된 월드축 기준으로 이동
	 *  (컨트롤러 회전을 타지 않아 탑다운 카메라와 항상 일치). Triggered 이벤트에 연결할 것. */
	UFUNCTION(BlueprintCallable, Category = "TopDown|Input")
	void MoveTopDown(FVector2D Value);

	/** 모바일 터치 가상 조이스틱(왼쪽: 이동)이 매 프레임 호출 */
	UFUNCTION(BlueprintCallable, Category = "TopDown|Input")
	void SetMoveInput(FVector2D Value);

	/** 모바일 터치 가상 조이스틱(오른쪽: 조준+공격)이 매 프레임 호출 */
	UFUNCTION(BlueprintCallable, Category = "TopDown|Input")
	void SetAimInput(FVector2D Value);

	//////////////////////////////////////////////////////////////////////////
	// 터치 조이스틱 (C++가 자동 생성해서 화면에 띄움 — 위젯 BP 불필요)

	/** 켜면 게임 시작 시 좌/우 가상 조이스틱을 자동 생성해 화면에 표시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Touch")
	bool bCreateTouchJoysticks = true;

	/** (선택) 조이스틱 베이스 텍스처. 비우면 반투명 박스로 표시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Touch")
	UTexture2D* JoystickBackgroundTexture = nullptr;

	/** (선택) 조이스틱 손잡이 텍스처. 비우면 반투명 박스로 표시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TopDown|Touch")
	UTexture2D* JoystickHandleTexture = nullptr;

	/** Returns TopDownBoom subobject */
	FORCEINLINE USpringArmComponent* GetTopDownBoom() const { return TopDownBoom; }
	/** Returns TopDownCamera subobject */
	FORCEINLINE UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }

};
