# UCombatAnimInstance 도입 — 작업 기록 & 남은 일 (2026-07-03)

## 무엇을 왜 했나

ABP 이벤트 그래프의 "캐스트 → 계산 → Set" 배선을 없애기 위해 **공용 AnimInstance C++ 베이스**를 도입.
데이터 수집은 C++이, ABP는 변수 읽기 + 블렌드만 담당한다 (Lyra 스타일, 현업 표준 패턴).

- 이벤트 그래프가 비면 애님 그래프가 FastPath(워커 스레드 업데이트)를 탈 수 있음 → 좀비 물량전에 유리
- 플레이어/동료/적 ABP가 스켈레톤이 달라도 같은 C++ 부모를 공유 → 배선 코드가 한 곳에

### 추가/변경된 파일

| 파일 | 내용 |
|---|---|
| `Public/Animation/CombatAnimInstance.h` | 새 클래스. ABP가 읽을 변수 정의 |
| `Private/Animation/CombatAnimInstance.cpp` | `NativeUpdateAnimation`에서 매 프레임 캐릭터 상태 수집 |
| `Public/Characters/MyPlayer.h` | `GetLegYawOffset()` public 게터 추가 (LegYawOffset이 private이라) |
| `Public/Characters/CombatCharacter.h` | bArmed 주석을 새 구조에 맞게 갱신 |

### ABP에서 읽을 수 있는 변수 (전부 BlueprintReadOnly)

| 변수 | 내용 |
|---|---|
| `GroundSpeed` | 수평 속도(cm/s) — 블렌드스페이스 입력 |
| `bIsFalling` | 공중 여부 (BP 표시명 "Is Falling") |
| `bArmed` | 무장 상태 — 캐릭터 bArmed 미러 (BP 표시명 "Armed") |
| `bIsDead` | 죽음 상태 — 캐릭터 IsDead 미러 (BP 표시명 "Is Dead") |
| `LegYawOffset` | 하체 yaw 오프셋(도) — 플레이어만 값 있음, 동료/적은 0 |
| `OwnerCombatCharacter` | 캐시된 소유 캐릭터 — 추가 정보 필요 시 캐스트 없이 사용 |

---

## ⚠️ 빌드 (사용자)

- [ ] **VS2022에서 풀 리빌드** — 새 C++ 클래스라 Live Coding(Ctrl+Alt+F11) 쓰면 크래시 재발함
- 참고: DebugGame만 빌드하면 BP가 부모 클래스를 잃는 문제 있음 → Development도 같이 빌드할 것

## 🎨 에디터 작업 (사용자) — ABP 3개(플레이어/동료/적) 각각

- [ ] **1. 부모 변경**: AnimBP 열기 → Class Settings → Details의 Parent Class → `CombatAnimInstance`
- [ ] **2. 겹치는 BP 변수 삭제**: 기존 BP 변수(GroundSpeed, IsFalling, Armed, Dead, LegYaw 등
      이름이 겹치거나 역할이 같은 것)를 My Blueprint 패널에서 삭제.
      안 지우면 상속 변수와 이름 충돌 경고 발생.
- [ ] **3. 깨진 노드 교체**: 변수 삭제로 깨진 Get 노드들을 상속된 변수
      (`GroundSpeed` / `Is Falling` / `Armed` / `Is Dead` / `LegYawOffset`)의 Get 노드로 교체
- [ ] **4. 이벤트 그래프 비우기**: `Event Blueprint Update Animation`의
      캐스트 노드 + Set 노드 전부 삭제. **아무것도 안 남는 게 정상.**
- [ ] **5. 컴파일 & 확인**: 컴파일 후 경고 0인지, 프리뷰/PIE에서 idle↔walk 전환,
      무장 블렌드, 죽음 상태, (플레이어) 다리 방향이 전과 동일하게 도는지 확인

### 에디터 팁
- 애님 그래프에서 변수 노드에 **번개 아이콘**이 뜨면 FastPath를 타는 것 (성공 지표)
- 프리뷰에서는 폰이 없어 변수가 기본값(0/false)일 수 있음 — 정상. PIE에서 확인할 것.

---

## 🤖 다음에 AI가 할 일

- [ ] 에디터 작업 완료 후: PIE에서 이상 있으면 원인 추적 (변수명 매칭, FastPath 여부 등)
- [ ] `Docs/code-review-todo.md`의 남은 항목 진행 (2번 널 체크부터 권장 — 30분 분량)
- [ ] (선택) `bShouldMove` 같은 파생 변수 추가 — ABP 전이 조건에서 "GroundSpeed > 3" 비교를
      반복 중이면 C++로 내리기
- [ ] (선택) 조준/공격 관련 애님 변수가 ABP에 더 남아 있으면 같은 방식으로 C++로 이관
- [ ] (장기) 죽음 처리도 상태머신 전이 대신 몽타주/애님 시퀀스 정리 검토

## 📝 그 밖의 메모

- `LegYawOffset` 계산 자체는 여전히 `AMyPlayer::UpdateLegYawOffset()`(Tick)이 담당.
  AnimInstance는 **읽기만** 한다. 동료도 하체 분리가 필요해지면 계산을 베이스로 올리는 것 검토.
- `bIsDead`는 이번에 고친 죽음 시스템(베이스 `SetDead` 전환)을 그대로 미러링 —
  플레이어 죽음이 ABP에 반영 안 되면 죽음 리팩토링 쪽을 먼저 의심할 것.
- ABP 이름이 겹치는 변수 삭제(2단계) 전에 **어떤 노드가 그 변수를 쓰는지**
  우클릭 → Find References로 확인하고 지우면 교체가 수월함.
