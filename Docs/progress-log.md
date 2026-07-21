# 작업 진행 로그

> 터미널이 끊기거나 새 세션으로 다시 시작할 때, 이 파일 맨 위 "지금 하던 것"만 보면
> 어디서 멈췄는지 바로 알 수 있게 적어두는 파일. 완료된 기능은 아래로 내려가고,
> 진행 중/막힌 것은 항상 맨 위에 남긴다.

---

## 🚧 지금 하던 것

### Necropolis 애셋을 실제 바닥/장애물로 교체 + 사수모드 디버깅 3건 (2026-07-21)
- Necropolis(디아블로풍 폐허/묘지 마켓플레이스 팩)를 절차적 맵의 실제 바닥/장애물 애셋으로 붙이는 중
- `.gitignore`의 `Content/Necropolis/*` 판단은 확인 완료(2.2GB, 마켓플레이스 팩이라 gitignore 맞음) — 더 볼 필요 없음
- 바닥 재질 후보: `MI_floor_tiles_01`(크립트용, 어두움) 복제해서 `Landscape/Textures/T_rocky_moss_floor_*`로 텍스처 교체 → 어두운 톤 유지하면서 실외 느낌 내는 방향으로 진행 중
- `Content/StarterContent`(585MB) 전체 삭제 검토 중 — `Content/Sound/MS_Attack.uasset`이 `StarterContent/Audio/Explosion02`를 참조하고 있어서, 그 사운드만 딴 걸로 교체하고 나서 폴더 통째로 지우면 됨
- **사수모드로 자가진단 중인 버그 3건** (Claude가 원인 조사 후 메모리에 정답 기록해둠 — 사용자가 힌트 받으면서 직접 찾는 중, 아직 다 안 풀림):
  1. `ObstacleMeshes`에 Necropolis 나무(Foliage 원본 메시)를 넣으니 공중에 뜸 → `InfiniteMapGenerator.cpp:328` 근처 `HalfHeight` Z오프셋 계산 확인 중
  2. 마을 파란 바닥(`VillageFloorMaterial`) 범위가 POI 디버그 박스(`DrawDebugBox`, `InfiniteMapGenerator.cpp:298~308`)보다 오른쪽/뒤로 밀려 보임 → `FloorMesh` 배치 좌표와 메시 피벗 위치 비교 중
  3. `Docs/test.png`: 절차적 바닥이 자연스럽게 안 이어지고 청크 단위 사각형 경계로 뚜렷하게 보임 → `SpawnMeshActor`의 Mobility 설정, 바닥이 청크마다 분리된 개별 액터라는 구조 쪽에서 원인 확인 중
- 다음 세션에서 이어갈 것: 위 3건 힌트 이어받아서 원인 스스로 확정 → 수정은 에디터/코드에서 직접

### POI 스켈레톤 (2026-07-07) — C++ 완료, 빌드+인게임 확인 대기
- `AInfiniteMapGenerator`에 리전 기반 POI(마을/좀비마을) 배치 로직 추가 (`Docs/poi-event-design.md` 기획의 1단계)
- 확인 방법: 빌드 → 플레이 → 걸어다니면 **파란 바닥 청크 = 마을, 어두운 바닥 = 좀비마을** (장애물 없음, 초록/빨강 디버그 박스 + `[POI]` 로그)
- 시작 리전은 마을 보장(시작점에서 2~7청크 거리), 떠났다 돌아오면 같은 자리에 재생성되는지 확인
- 다음 단계: POI 상태 영속(FPOIState) → 마을 v1(힐존+발판)

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
# 해야할 일
당연히 다 하지말고 하나씩 하자. 순서는 알아서
- [ ] 무한맵에 걸맞는 이벤트 출시 (개빡셀듯)
	- [ ] 마을
		- [ ] npc => enemy에게 공격당할 수 있음
			- [ ] 퀘스트를 주는 → 설계 메모는 아래 "퀘스트 설계" 참고
			- [ ] 공격하는 npc
		- [ ] 무기 업그레이드
		- [ ] 회복하는 곳 => 돈을 내야할까?
		- [ ] 가끔 좀비들이 침공할 수도 있음
		- [ ] 무기고 업그레이드도?
		- [ ] 무기 상점
		- [ ] 스폰존 (여기서 랜덤으로 직업) => 1회성 (이건 아마 매개변수 있을거임)
	- [ ] 좀비마을?
		- [ ] 무기 (1% 확률로)
		- [ ] 돈


## 퀘스트 설계 메모 (2026-07-13) — 쇼츠 템포 유지가 핵심
RPG식(대화창+수락 버튼+로그 UI)으로 하면 루즈해짐. 원칙: **"받는 행위"를 없애고, 하던 플레이가 곧 진행이 되게** (뱀서/아처로의 미션 방식).

- **대화창 없음**: 주민(AVillager) 근처를 지나가면 머리 위 말풍선(아이콘+한 줄)이 뜨고 범위에 들어가면 자동 수락.
  기존 패턴 조합으로 구현 — 적 HP바(WidgetComponent) + 발판(PadZone) 오버랩
- **동시 1개만**: 퀘스트 로그 UI를 아예 안 만든다. HUD에 한 줄만 (`🧟 12/20`, `📦 →방향화살표`).
  새 주민 만나면 교체 여부만 결정
- **완료도 자동, 보상 즉시**: 달성 순간 그 자리에서 돈/경험치 + 토스트 한 줄. 주민에게 돌아가 보고하는 단계 금지
  (무한맵에서 되돌아가기는 고통 + 템포 사망). 배달만 예외 — 도착 자체가 완료라 자연스러움

### 퀘스트 3종 (구현 비용 순)
1. **처치** "좀비 20마리" — 거의 공짜. `AEnemy::OnDeath`가 이미 AddExp 호출하니 거기에 카운트 한 줄
2. **배달** "물건을 옆 마을로" — 아이템 액터 1개 + HUD 가장자리 방향 화살표. 무한맵 달리기와 궁합 최고
3. **방어** "30초간 마을 방어" — 스폰 링 중심을 마을로 바꾸면 됨. "좀비 침공" 이벤트와 겸용 가능

### 구현 골격
```
FQuestData (struct): 타입, 목표 수, 보상 돈/경험치, 한 줄 문구
플레이어(또는 GameMode)에 CurrentQuest 1슬롯
 ├─ AVillager: 오버랩 → 제안/자동 수락 (말풍선 위젯)
 ├─ AEnemy::OnDeath → 처치 카운트 통지
 ├─ 배달: AQuestItem 액터 + 목표 마을 청크 좌표 (InfiniteMapGenerator가 이웃 마을 위치 앎)
 └─ UMyCanvas: QuestText 한 줄 (BindWidgetOptional, 죽음 패널과 같은 방식)
```

**착수 순서**: 처치 1종만으로 파이프라인(주민→수락→HUD→카운트→보상 토스트)을 끝까지 뚫고, 템포 확인 후 배달 추가.

---

## 참고 문서
- `Docs/poi-event-design.md` — 무한맵 이벤트(마을/좀비마을) 기획 문서 (2026-07-07)
- `Docs/devlog_2026-06-29_0705.md` — 이번 주 개발로그(담백 버전)
- `Docs/code-review-todo.md` — 코드 리뷰에서 나온 미해결 개선 항목 목록
- `Docs/combat-anim-instance-setup.md` — CombatAnimInstance 에디터 적용 방법
