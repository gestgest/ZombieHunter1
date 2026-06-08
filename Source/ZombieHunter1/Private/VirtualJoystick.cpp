// Fill out your copyright notice in the Description page of Project Settings.

#include "VirtualJoystick.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<SWidget> UVirtualJoystick::RebuildWidget()
{
	// 위젯 블루프린트 없이 C++에서 트리를 직접 구성 (RootWidget이 비어있을 때만)
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		//캔버스 생성
		UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
		WidgetTree->RootWidget = RootOverlay;

		//배경 만들기
		Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("JoystickBackground"));

		//중앙에 넣어라
		if (UOverlaySlot* BgSlot = RootOverlay->AddChildToOverlay(Background))
		{
			//layout
			BgSlot->SetHorizontalAlignment(HAlign_Center);
			BgSlot->SetVerticalAlignment(VAlign_Center);
		}

		//핸들링 만들기
		Handle = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("JoystickHandle"));

		//중앙에 넣어라
		if (UOverlaySlot* HandleSlot = RootOverlay->AddChildToOverlay(Handle))
		{
			//layout
			HandleSlot->SetHorizontalAlignment(HAlign_Center); 
			HandleSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	return Super::RebuildWidget();
}

void UVirtualJoystick::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯 전체가 터치/마우스를 받도록
	SetVisibility(ESlateVisibility::Visible);
}

float UVirtualJoystick::GetEffectiveRange() const
{
	if (HandleRange > 0.f)
	{
		return HandleRange;
	}
	// 자동: 베이스 반지름에서 손잡이 반지름을 뺀 값 (손잡이가 베이스 안에 머물도록)
	return FMath::Max(1.f, (BackgroundSize.X - HandleSize.X) * 0.5f);
}

FReply UVirtualJoystick::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return HandlePressed(InGeometry, InGestureEvent);
}

FReply UVirtualJoystick::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	// 내 손가락이 맞을 때만 반응 (멀티터치: 다른 조이스틱 손가락 무시)
	if (InGestureEvent.GetPointerIndex() == ActivePointerIndex)
	{
		HandleMoved(InGeometry, InGestureEvent);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply UVirtualJoystick::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	return HandleReleased(InGestureEvent);
}

FReply UVirtualJoystick::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return HandlePressed(InGeometry, InMouseEvent);
}

FReply UVirtualJoystick::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetPointerIndex() == ActivePointerIndex)
	{
		HandleMoved(InGeometry, InMouseEvent);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply UVirtualJoystick::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return HandleReleased(InMouseEvent);
}

//조이스틱을 놓친다면
void UVirtualJoystick::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);

	// 어떤 경로로든 입력 캡처가 끝나면 무조건 손잡이를 중앙으로 되돌리고 (0,0)을 보냄
	if (ActivePointerIndex != -1)
	{
		ActivePointerIndex = -1;
		ResetHandle();
	}
}

FReply UVirtualJoystick::HandlePressed(const FGeometry& InGeometry, const FPointerEvent& InEvent)
{
	// 이미 다른 손가락이 잡고 있으면 무시
	if (ActivePointerIndex != -1)
	{
		return FReply::Unhandled();
	}

	ActivePointerIndex = InEvent.GetPointerIndex();
	UpdateFromScreenPosition(InGeometry, InEvent.GetScreenSpacePosition());

	// 이 포인터의 이후 Move/Up 이벤트를 이 위젯이 받도록 캡처
	return FReply::Handled().CaptureMouse(TakeWidget());
}

void UVirtualJoystick::HandleMoved(const FGeometry& InGeometry, const FPointerEvent& InEvent)
{
	UpdateFromScreenPosition(InGeometry, InEvent.GetScreenSpacePosition());
}

FReply UVirtualJoystick::HandleReleased(const FPointerEvent& InEvent)
{
	if (InEvent.GetPointerIndex() != ActivePointerIndex)
	{
		return FReply::Unhandled();
	}

	ActivePointerIndex = -1;
	ResetHandle();
	return FReply::Handled().ReleaseMouseCapture();
}

void UVirtualJoystick::UpdateFromScreenPosition(const FGeometry& InGeometry, const FVector2D& ScreenPos)
{
	// 위젯 로컬 좌표로 변환 후 중심 기준 오프셋 계산
	const FVector2D LocalPos = InGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D Center = InGeometry.GetLocalSize() * 0.5f;
	FVector2D Offset = LocalPos - Center;

	// 손잡이가 벗어나지 않도록 반경 제한
	const float Range = GetEffectiveRange();
	if (Offset.Size() > Range)
	{
		Offset = Offset.GetSafeNormal() * Range;
	}

	// 손잡이 위치 갱신 (시각 피드백)
	if (Handle)
	{
		Handle->SetRenderTranslation(Offset);
	}

	// 정규화: 화면 Y는 아래가 +이므로 Y를 뒤집어 "위 = +" 로 맞춤 (게임패드 스틱과 동일)
	FVector2D Value(Offset.X / Range, -Offset.Y / Range);

	// 데드존 적용
	if (Value.Size() < DeadZone)
	{
		Value = FVector2D::ZeroVector;
	}

	OnJoystickMoved.Broadcast(Value);
}

void UVirtualJoystick::ResetHandle()
{
	if (Handle)
	{
		Handle->SetRenderTranslation(FVector2D::ZeroVector);
	}
	OnJoystickMoved.Broadcast(FVector2D::ZeroVector);
}
