# 코드 리뷰 개선 목록 (2026-07-03)

> Source/ 전체 C++ 리뷰에서 나온 개선 항목.
> ~~1번(플레이어를 베이스 죽음 시스템에 태우기)은 완료~~ — `AMyPlayer::SetHP`가 `Super::SetHP` 호출,
> `checkDead()` 부작용 제거(OnDeath/OnRevive로 이동), `AEnemy::TrackingPlayer`는 `IsDead` 조회 + 널 체크.

---

## 🔴 2. 널 체크 누락 — 실제 크래시 나는 지점들

- [x] `Enemy.cpp` `TrackingPlayer()` — `Cast<AMyPlayer>(GetPlayerCharacter(...))` 결과 널 체크 (1번 작업 때 같이 처리됨)
- [ ] `ZombieSlayerGameMode.cpp` `initEnemy()` — `newEnemy->SetID(index)`를 **`if (newEnemy)` 체크보다 먼저** 호출.
      `EnemyClass`를 BP에서 안 꽂으면 SpawnActor가 null → 즉사 크래시. 체크를 위로 올릴 것.
- [ ] `ZombieSlayerGameMode.cpp` `initCoin()` — 동일 문제 (`newCoin->setID`가 널 체크보다 먼저).
- [ ] `ZombieSlayerGameMode.cpp` `SpawnEnemy()` / `spawnCoin()` — `NavSys`, `myPlayer` 둘 다 널 체크 없이 사용.
      두 함수가 거의 복붙이라 같은 문제 두 벌.
- [ ] `MyCanvas.cpp` `RestartGame()` — `myPlayer` 널 체크 없이 `->ReStart()` 호출.

**규칙**: `Cast<>`와 `SpawnActor<>`의 반환값은 항상 그 줄 바로 다음에 검사한다.

---

## 🟡 3. GC 안전성 — UPROPERTY 없는 UObject 포인터

- [ ] `MyPlayer.h` — `CanvasWidget`, `controller`, `playerStart`가 UPROPERTY 아님.
      GC가 참조를 못 봐서 위젯 파괴/레벨 스트리밍 시 **댕글링 포인터** 위험. `UPROPERTY()` 부착.
- [ ] `Companion.h` — `AICon`, `Leader`, `CurrentJob`도 UPROPERTY인지 확인.
- 참고: `Enemy.h`의 `aiController`엔 이미 UPROPERTY + 이유 주석까지 있음. 같은 기준을 전체에 적용.

---

## 🟡 4. 스탯 하드코딩이 에디터 값을 덮어씀

베이스에 `MaxHP`, `Damage`를 `EditAnywhere`로 만들어놓고 코드가 덮어쓰는 중:

- [ ] `Enemy.cpp` `BeginPlay()` — `SetHP(5); Damage = 1;` → `SetHP(MaxHP)`로, `Damage = 1` 삭제(CDO 기본값이 이미 1)
- [ ] `MyPlayer.cpp` `BeginPlay()` — `Damage = 1;` 삭제
- [ ] `MyPlayer.cpp` `ReStart()` — `SetHP(5)` → `SetHP(MaxHP)`
- [ ] `ZombieSlayerGameMode.cpp` `SpawnEnemy()` — `enemy->SetHP(5)` → `enemy->SetHP(enemy->MaxHP)`

**원칙**: 숫자의 출처는 한 곳(에디터에서 튜닝 가능한 UPROPERTY).

### 힐러 클램프 버그 (같은 맥락)
- [ ] `HealerJob.cpp` `HealCharacter()` — **직업의 `MaxHP`로 대상을 클램프** 중. 대상마다 최대체력이 다르면 틀림 → `Target->MaxHP` 사용.
- [ ] 더 좋은 방법: 베이스 `ACombatCharacter::SetHP`에서 `HP = FMath::Clamp(new_hp, 0, MaxHP)`로 클램프
      → 힐러는 `Target->AddHP(HealAmount)` 한 줄이면 끝. (HP가 음수로 내려가는 것도 같이 막힘)

---

## 🟡 5. 풀에 있는 "숨은" 적들이 실제로는 살아있음

`SetActorHiddenInGame(true)`는 **렌더링만** 끔. 초기 풀의 적 전원이 (0,0,90)에
콜리전 켜진 채 겹쳐 스폰돼 서로 밀치고, Tick 돌고, 스윕 공격에도 맞음.

- [ ] `AEnemy`에 풀 반납/대여 함수 추가 후 GameMode에서 사용:

```cpp
void SetPooled(bool bPooled)
{
    SetActorHiddenInGame(bPooled);
    SetActorEnableCollision(!bPooled);
    SetActorTickEnabled(!bPooled);
}
```

---

## 🟢 기타 (덜 급하지만 알아두면 좋은 것)

- [ ] `Enemy.cpp` `hit()` — `DrawDebugLine`이 디버그 토글 없이 **항상** 그려짐.
      적이 공격할 때마다 빨간 선 출력 → 베이스의 `bDebugCombat`으로 감쌀 것.
- [ ] **근접 스윕 코드 중복** — `AEnemy::hit()`과 `USwordsmanJob::OnAttackNotify()`가 거의 동일.
      "전방 구체 스윕 + 피격자 필터 + 넉백"을 `ACombatCharacter` 헬퍼 함수로 추출.
- [ ] **피아 식별이 `!IsA(AEnemy::StaticClass())`** (`Enemy.cpp`, `HealerJob.cpp`) —
      적 종류가 늘거나 진영 변경이 생기면 무너짐. `ACombatCharacter`에 `ETeam Team` enum +
      `IsHostileTo(Other)` 판정 함수를 다음 리팩토링 후보로.
- [ ] `Companion.cpp` — `FindNearestEnemy()`가 매 Tick `GetAllActorsOfClass` 호출 +
      사거리 밖이면 `MoveToActor`도 매 프레임 재요청(경로 재계산이라 비쌈).
      타겟 재탐색은 0.2~0.3초 간격 타이머로 충분.
- [ ] **네이밍 컨벤션 통일** — `checkDead`/`hit`/`spawn`/`init`(camelCase) vs `SpawnEnemy`(PascalCase),
      `enemy_size`/`add_hp`(snake_case), bool `b` 접두사 혼재(`IsDead` vs `bArmed`).
      UE 표준: 함수/변수 PascalCase, bool은 `b` 접두사. BP 핀 호환은 `meta = (DisplayName)`이나
      Core Redirects로 해결 가능 — 새 코드부터라도 통일.
- [ ] `Enemy.h` — `AIController.h` 통 include + 전방선언 중복. 멤버가 포인터뿐이니
      헤더는 전방선언만 남기고 include는 cpp로 이동(컴파일 시간 개선).
