# 작업 진행 로그

> 터미널이 끊기거나 새 세션으로 다시 시작할 때, 이 파일 맨 위 "지금 하던 것"만 보면
> 어디서 멈췄는지 바로 알 수 있게 적어두는 파일. 완료된 기능은 아래로 내려가고,
> 진행 중/막힌 것은 항상 맨 위에 남긴다.

---

## 🔴 지금 하던 것 — 돈이 안 늘어나는 버그

**증상**: 코인을 먹어도 소지금(Money)이 화면에서 증가하지 않음.

**아직 커밋 안 한 변경사항** (작업 중 상태 그대로 남겨둠):
- `MyPlayer.cpp` `SetMoney()` — `CanvasWidget`이 null이면 화면에 빨간 디버그 메시지(`"CanvasWidget is null"`)를 띄우도록 임시 로그 추가. → 이 메시지가 뜨는지가 다음 확인 포인트.
- `BP_Coin.uasset` — 에디터에서 뭔가 수정됨(내용 미확인, 아마 픽업 관련 손보던 흔적).
- `ZombieSlayerGameMode.cpp` — 수정 표시는 있으나 실제 텍스트 diff는 없음(줄바꿈 등 사소한 변경으로 추정).

**세운 가설 2개** (아직 둘 다 미검증):
1. **발판 드레인설**: `AMoneyPadZone`(동료 소환/무기 강화 발판) 트리거 박스 범위 안에 있으면 0.15초마다 돈이 자동으로 빠져나간다(`TrySpendMoney`). 발판 근처에서 코인을 먹으면 +1 되자마자 -1로 흡수돼 안 오르는 것처럼 보일 수 있음.
2. **CanvasWidget null설**: `SetCanvasWidget()`이 호출되기 전에 `AddMoney`/`SetMoney`가 먼저 불려서 `CanvasWidget`이 아직 null인 상태로 코인 카운트 UI 갱신을 건너뛰는 경우. 위 디버그 로그로 확인 중.

**다음에 할 일**:
- [ ] 게임 실행 후 코인 먹었을 때 화면에 `"CanvasWidget is null"` 빨간 메시지가 뜨는지 확인
- [ ] 뜨면 → `SetCanvasWidget()` 호출 시점(BP_Canvas의 언제 호출되는지) 확인, BeginPlay 순서 문제 의심
- [ ] 안 뜨면 → 발판에서 멀리 떨어진 곳에서 코인 먹어보고 정상 증가하는지 확인 (발판 드레인설 검증)
- [ ] 원인 확정되면 임시 디버그 로그(`GEngine->AddOnScreenDebugMessage`) 제거하고 정식 수정

---

## ✅ 완료된 작업 (최근 → 과거)

### 2026-07-05
- **적 타겟팅 버그 수정**: 적이 플레이어만 쫓던 것 → 플레이어/동료 중 가까운 쪽을 쫓도록 (`AEnemy::TrackingPlayer`)
- **HP바 표시**: 적/동료 머리 위 스크린 스페이스 HP바. `ACombatCharacter`에 opt-in 방식(`CreateHPBarComponent()`)으로 구현, 플레이어는 기존 HUD 유지
- **경험치 시스템**: 적 처치 시 `AddExp` 호출, 레벨업 시 필요 경험치 증가, `OnLevelUp` BP 훅
- **무기 강화 발판**: 동료 소환 발판(`ACompanionSpawnZone`)과 공통 로직을 `AMoneyPadZone` 베이스로 뽑고, `AWeaponUpgradeZone` 추가. 강화는 `CurrentJob->Damage` 상승
- **적 스폰 방식 변경**: 풀 크기 10 → 20, "1마리씩 5초 쿨타임" → "숨겨진 적 전부 한 번에 배치 스폰"

### 2026-07-04
- 화살 풀링(`UProjectilePoolSubsystem`)
- 적 리쉬 회수(멀어지거나 낙하하면 시야 밖 스폰 링으로 재배치)
- 동료 AI 연산 주기화(`UpdateDecision`, 매 프레임 → 간격 호출)
- 적 스폰을 시야 밖 링(`FindReachablePointInRing`)에서 하도록 변경

### 2026-07-01 ~ 2026-07-03
- Layered Blend Per Bone(상하체 분리 애니메이션)
- 전투 AnimInstance 공용화(`UCombatAnimInstance`) — 플레이어/동료/적 ABP 공통 부모
- 동료 애니메이션 적용
- MyPlayer 헤더 리팩토링

> 이 이전 작업들은 `Docs/devlog_*.md` 주간 로그에 더 자세히 정리돼 있음.

---

## 참고 문서
- `Docs/devlog_2026-06-29_0705.md` — 이번 주 개발로그(담백 버전)
- `Docs/code-review-todo.md` — 코드 리뷰에서 나온 미해결 개선 항목 목록
- `Docs/combat-anim-instance-setup.md` — CombatAnimInstance 에디터 적용 방법
