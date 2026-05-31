// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "VirtualJoystick.generated.h"

class UImage;

/** 조이스틱 값이 바뀔 때마다(드래그 중 매 입력) 브로드캐스트. Value는 정규화된 (-1~1), Y는 위가 + */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoystickMoved, FVector2D, Value);

/**
 * 모바일 터치 가상 조이스틱 위젯.
 * 위젯 트리(배경 + 손잡이 이미지)를 C++에서 직접 생성하므로 별도의 위젯 블루프린트가 필요 없다.
 * CreateWidget<UVirtualJoystick>(...) 로 바로 만들어 AddToViewport 하면 된다.
 * 드래그 입력은 OnJoystickMoved 델리게이트로 내보낸다.
 */
UCLASS()
class ZOMBIEHUNTER1_API UVirtualJoystick : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 드래그할 때마다 정규화 벡터(-1~1, Y 위가 +)를 전달. 손가락을 떼면 (0,0)을 한 번 보냄 */
	UPROPERTY(BlueprintAssignable, Category = "Joystick")
	FOnJoystickMoved OnJoystickMoved;

	/** 조이스틱 베이스(바깥 원) 텍스처. 비우면 반투명 박스로 표시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	TObjectPtr<UTexture2D> BackgroundTexture;

	/** 손잡이(안쪽 원) 텍스처. 비우면 반투명 박스로 표시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	TObjectPtr<UTexture2D> HandleTexture;

	/** 베이스 크기(px) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	FVector2D BackgroundSize = FVector2D(220.f, 220.f);

	/** 손잡이 크기(px) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	FVector2D HandleSize = FVector2D(100.f, 100.f);

	/** 손잡이가 중심에서 움직일 수 있는 최대 반경(px). 0 이하면 베이스 크기에서 자동 계산 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	float HandleRange = 0.f;

	/** 데드존(정규화 기준). 이보다 작은 입력은 (0,0)으로 무시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
	float DeadZone = 0.1f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	// 데스크탑(마우스)에서도 테스트할 수 있도록 마우스 입력도 동일하게 처리
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	/** 위젯 BP에서 같은 이름의 Image를 두면 자동 바인딩됨. 없으면 C++가 직접 생성 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> JoystickBackground;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> JoystickHandle;

	/** 현재 이 조이스틱을 잡고 있는 포인터(손가락) 번호. -1이면 비활성 */
	int32 ActivePointerIndex = -1;

	/** 텍스처/색/크기를 이미지에 적용 */
	void ApplyImageBrushes();
	/** 텍스처가 있으면 이미지, 없으면 단색 원형 브러시를 생성 */
	FSlateBrush MakeBrush(UTexture2D* Texture, const FVector2D& Size, const FLinearColor& FallbackTint) const;
	/** 실제 사용할 손잡이 반경 */
	float GetEffectiveRange() const;

	FReply HandlePressed(const FGeometry& InGeometry, const FPointerEvent& InEvent);
	void HandleMoved(const FGeometry& InGeometry, const FPointerEvent& InEvent);
	FReply HandleReleased(const FPointerEvent& InEvent);

	void UpdateFromScreenPosition(const FGeometry& InGeometry, const FVector2D& ScreenPos);
	void ResetHandle();
};
