// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CompanionSpawnZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AMyPlayer;

/**
 * 동료 소환 구역 (쇼츠 게임식 "밟으면 게이지가 차는" 발판).
 *
 * 플레이어가 이 구역(박스 트리거) 안에 서 있으면 게이지(Progress)가 차오르고,
 * 밖으로 나가면 다시 줄어든다(긴장감). 게이지가 가득 차면 플레이어의
 * RecruitCompanion()을 호출해 동료를 소환한다 — C키 섭외와 동일한 로직 재사용.
 *
 * UI는 OnProgressChanged / OnZoneCompleted (BlueprintImplementableEvent)로 위젯에 연동하거나,
 * bShowDebugGauge로 발판 위에 기본 게이지 바를 띄워 BP 없이도 바로 확인할 수 있다.
 */
UCLASS()
class ZOMBIEHUNTER1_API ACompanionSpawnZone : public AActor
{
	GENERATED_BODY()

public:
	ACompanionSpawnZone();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	//////////////////////////////////////////////////////////////////////////
	// 컴포넌트

	/** 밟는 영역(트리거). 이 박스 안에 플레이어가 들어오면 게이지가 찬다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpawnZone")
	UBoxComponent* TriggerBox;

	/** 발판 바닥 메시(선택). BP에서 평평한 큐브/플레인 메시를 지정해 시각화. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpawnZone")
	UStaticMeshComponent* PadMesh;

	//////////////////////////////////////////////////////////////////////////
	// 튜닝 값

	/** 게이지를 0→100%까지 채우는 데 걸리는 시간(초). 플레이어가 계속 서 있을 때 기준. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone", meta = (ClampMin = "0.1"))
	float FillDuration = 2.0f;

	/** 비어 있을 때(플레이어가 없을 때) 게이지가 줄어드는 속도 배율. FillDuration 대비.
	 *  1.0 이면 채우는 속도와 같은 속도로 빠진다. 0 이면 줄지 않고 유지된다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone", meta = (ClampMin = "0.0"))
	float DrainRateScale = 0.75f;

	/** 한 번 완성되면 더 이상 작동하지 않게 할지(일회성 발판). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone")
	bool bOneShot = false;

	/** 완성 후 다시 채울 수 있게 되기까지의 쿨다운(초). bOneShot이 false일 때만 의미 있음. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone", meta = (ClampMin = "0.0"))
	float Cooldown = 1.0f;

	/** 켜면 발판 위에 기본 디버그 게이지 바를 그려 BP 위젯 없이도 진행도를 확인할 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnZone|Debug")
	bool bShowDebugGauge = true;

	//////////////////////////////////////////////////////////////////////////
	// 런타임 상태

	/** 현재 게이지(0.0 ~ 1.0). */
	UPROPERTY(BlueprintReadOnly, Category = "SpawnZone")
	float Progress = 0.0f;

	/** 현재 구역 안에 플레이어가 서 있는지. */
	UPROPERTY(BlueprintReadOnly, Category = "SpawnZone")
	bool bPlayerInside = false;

	//////////////////////////////////////////////////////////////////////////
	// 블루프린트 이벤트 (UI 연동용)

	/** 게이지가 바뀔 때마다 호출(NewProgress: 0~1). 진행 바 위젯 갱신에 사용. */
	UFUNCTION(BlueprintImplementableEvent, Category = "SpawnZone")
	void OnProgressChanged(float NewProgress);

	/** 게이지가 가득 차서 동료를 소환한 직후 호출. 이펙트/사운드 연출에 사용. */
	UFUNCTION(BlueprintImplementableEvent, Category = "SpawnZone")
	void OnZoneCompleted(AMyPlayer* Recruiter);

protected:
	/** 게이지가 가득 찼을 때 호출 — 안에 있는 플레이어의 RecruitCompanion()을 실행. */
	void CompleteZone();

	/** bShowDebugGauge가 켜져 있을 때 발판 위에 진행도 바를 그린다. */
	void DrawDebugGauge();

	/** 트리거 오버랩 콜백 */
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** 현재 구역 안에 있는 플레이어(여러 명 겹쳐도 첫 유효 플레이어 1명만 추적). */
	UPROPERTY()
	AMyPlayer* CurrentPlayer = nullptr;

	/** 완성 후 남은 쿨다운(초). 0이면 다시 채울 수 있음. */
	float CooldownRemaining = 0.0f;

	/** bOneShot 발판이 이미 한 번 작동했는지. */
	bool bConsumed = false;
};
