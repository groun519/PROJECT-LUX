# PROJECT LUX - Phase 02 Round & Game Rule Foundation

> 문서 상태: Draft / Design Review Pending  
> 기준 엔진: Unreal Engine 5.8  
> 선행 조건: Phase 01 Revolver FPS Foundation 완료  
> 목적: 이미 구현된 서버 권한 사망을 실제 게임 규칙으로 연결하고, 이후 Parasite / Host / Escape 규칙이 같은 경계를 재사용할 수 있는 Round / Game Rule 기반을 만든다.

---

# 0. Phase 목표

Phase 02의 목표는 단순한 테스트용 Deathmatch를 추가하는 것이 아니다.

현재 리볼버 즉사 시스템을 이용해 실제로 한 판이 끝나는 최소 게임 루프를 만들되, 이후 PROJECT LUX의 최종 승패 규칙에서도 유지되는 구조를 먼저 확정한다.

최종 흐름:

```text
Server-authoritative lethal hit
        ↓
ALuxCharacter death confirmed
        ↓
OnDeathConfirmed broadcast
        ↓
ALuxGameMode receives event
        ↓
ULuxGameRuleManagerComponent
        ↓
EvaluateEndConditions()
        ↓
End condition satisfied
        ↓
ALuxGameMode::RequestGameEnd(...)
        ↓
ALuxGameMode commits authoritative result
        ↓
ALuxGameState replicates round state/result
        ↓
Clients observe confirmed state
```

Phase 02 완료 시 최소 2명 이상의 플레이어가 Round를 시작하고, 사망이 발생할 때마다 서버가 종료 조건을 재평가하며, 마지막 생존자 또는 생존자 없음 상태에서 게임 종료를 확정할 수 있어야 한다.

---

# 1. 설계 원칙

## 1.1 Character는 상위 게임 규칙을 모른다

`ALuxCharacter`는 자신의 죽음만 소유한다.

Character가 직접 다음 객체를 찾거나 호출하지 않는다.

- ALuxGameMode
- ALuxGameState
- ULuxGameRuleManagerComponent
- Game End Condition

Character의 역할:

```text
Die()
→ death state commit
→ OnDeathConfirmed.Broadcast(this)
→ 끝
```

즉 Character는 "게임이 끝나야 하는가"를 판단하지 않는다.

## 1.2 죽음은 Request와 Event를 구분한다

죽기 전:

```text
Weapon / gameplay source
→ Character death request / Die()
```

죽음 확정 후:

```text
Character
→ OnDeathConfirmed
```

이미 확정된 죽음은 다시 Request가 아니라 발생한 사실이므로 Event로 전달한다.

`OnRep_IsDead`에서는 게임 규칙용 Death Event를 다시 발생시키지 않는다.

## 1.3 GameMode는 authoritative commit boundary다

GameMode는 세부 승패 규칙을 직접 소유하지 않는다.

GameMode의 역할:

- Character death event binding
- 게임 규칙 Manager로 이벤트 전달
- Round Start / End request 진입점
- 유효한 요청인지 확인
- 확정된 Round State / Result를 GameState에 기록
- 서버 내부 상태 변경 delegate broadcast

Manager가 직접 GameState에 결과를 쓰지 않는다.

## 1.4 GameRuleManager는 게임 규칙을 소유한다

`ULuxGameRuleManagerComponent`는 `ALuxGameMode`가 소유하는 server-authoritative manager다.

역할:

- 현재 Round 참가자 관리
- 현재 생존 참가자 관리
- 게임 규칙에 영향을 주는 사건 수신
- Game End Condition 평가
- 종료 조건 충족 시 GameMode에 `RequestGameEnd`

GameRuleManager가 Character의 죽음 처리 자체를 수행하지 않는다.

## 1.5 GameState는 확정 상태의 replicated representation이다

`ALuxGameState`는 게임 종료 여부를 판단하지 않는다.

역할:

- 현재 Round State 복제
- 확정된 Game Result 복제
- OnRep / local delegate로 Client 표현 계층에 알림

즉:

```text
Manager
→ 판단

GameMode
→ 확정

GameState
→ 복제
```

---

# 2. 확정 클래스 경계

## 2.1 ALuxCharacter

추가:

```text
OnDeathConfirmed
```

C++ multicast delegate로 시작한다.

이벤트 발생 조건:

- Server authority
- 기존 `bIsDead == false`
- `Die()`가 성공적으로 death state를 확정한 직후
- 같은 Character에서 한 번만 발생

이벤트 payload의 최소값:

```text
ALuxCharacter*
```

Death Cause, Killer, Weapon, Damage Context는 Phase 02에서 필요하지 않으므로 추가하지 않는다.

## 2.2 ALuxGameMode

추가 책임:

```text
Character death delegate binding
        ↓
HandleCharacterDeath(...)
        ↓
GameRuleManager.HandleParticipantDeath(...)
```

또한 다음 authoritative entry point를 제공한다.

```text
RequestStartRound()
RequestGameEnd(Result)
```

GameMode 내부에서만 GameState의 Round State / Result를 확정한다.

## 2.3 ULuxGameRuleManagerComponent

`ALuxGameMode`의 기본 Component로 둔다.

권장 책임:

```text
RegisteredParticipants
ActiveRoundParticipants
AliveParticipants
EndConditions
```

정확한 Player identity는 Character pointer가 아니라 `ALuxPlayerState`를 기준으로 관리한다.

이유:

- PlayerState는 Pawn보다 Round participant identity에 적합하다.
- 이후 Spectator 전환으로 Pawn이 변경되어도 participant identity가 유지된다.
- Parasite / Host / Citizen의 게임 규칙 상태도 PlayerState 계층과 결합할 가능성이 높다.
- Character death와 Round participant elimination을 분리할 수 있다.

Character의 `bIsDead`는 현재 Body 상태와 입력/표현 차단을 위해 유지한다.

## 2.4 ULuxGameEndCondition

PROJECT LUX에는 이후 복수의 종료 조건이 확정적으로 필요하다.

예:

- 현재 Prototype Last Survivor
- Parasite elimination
- Citizen elimination
- Escape resolution

따라서 GameRuleManager 안에 모든 조건을 중앙 분기문으로 계속 추가하지 않고, End Condition을 독립 기능 단위로 분리한다.

최소 추상화:

```text
ULuxGameEndCondition
└─ Evaluate(...)
```

Phase 02 첫 concrete condition:

```text
ULuxLastSurvivorEndCondition
```

현재는 한 종류만 구현한다.

Room Setting, Data Asset 기반 Rule Set, Blueprint configurable rule framework는 만들지 않는다.

## 2.5 ALuxGameState

Round 공용 상태를 복제한다.

권장 상태:

```text
ELuxRoundState
- Waiting
- Playing
- Ended
```

게임 결과:

```text
FLuxGameResult
- EndReason
- Winners[]
```

`Winners`는 단일 Player가 아니라 배열로 둔다.

이유:

최종 PROJECT LUX의 Escape 결과는 여러 플레이어가 동시에 성공할 수 있으므로, Single Winner 전용 구조를 현재 기반으로 만들지 않는다.

Phase 02 EndReason 최소값:

```text
None
LastSurvivor
```

후속 Phase에서 실제 확정된 이유를 추가한다.

Round State와 Result는 Client가 종료 상태를 관찰할 때 서로 모순되지 않도록 같은 replicated state representation 안에서 일관되게 갱신하는 방향을 우선한다.

---

# 3. Participant 생명주기

## 3.1 연결

Player가 로그인하면 GameMode가 해당 `ALuxPlayerState`를 GameRuleManager에 등록한다.

```text
PostLogin
→ RegisterParticipant(PlayerState)
```

등록은 idempotent해야 한다.

단순 접속만으로 현재 Playing Round의 생존자로 자동 편입시키지는 않는다.

## 3.2 Character Spawn / Possess

GameMode가 새 playable Character의 생성/Restart 경로에서:

1. 기존 binding 중복 여부 확인
2. `OnDeathConfirmed` binding
3. Character와 PlayerState 관계 확인

을 수행한다.

Character가 GameMode를 찾아 자신을 등록하는 구조는 사용하지 않는다.

정확한 Unreal override 지점은 구현 전 현재 Spawn / Restart 호출 흐름을 확인해 결정한다.

후보:

- RestartPlayer
- HandleStartingNewPlayer 이후

중복 delegate binding이 발생하지 않아야 한다.

## 3.3 Round Start

Round Start 시 GameRuleManager는 현재 등록된 참가자를 기준으로 Active Round participant snapshot을 만든다.

기본 조건:

- Server authority
- 현재 Round State == Waiting
- 생존 가능한 참가자 2명 이상

```text
RegisteredParticipants
        ↓
StartRound()
        ↓
ActiveRoundParticipants snapshot
        ↓
AliveParticipants initialized
```

Round 시작 후 새로 접속한 Player를 현재 Round에 자동 참가시키는 정책은 Phase 02에서 확정하지 않는다.

즉 Late Join 최종 정책은 TBD이며, Phase 02에서는 Active snapshot 밖의 Player가 기존 Round 종료 조건에 영향을 주지 않게 한다.

## 3.4 Death

Server에서 Character death가 확정되면:

```text
ALuxCharacter::Die()
→ bIsDead = true
→ ApplyDeathState()
→ OnDeathConfirmed.Broadcast(this)
```

GameMode:

```text
HandleCharacterDeath(Character)
→ resolve ALuxPlayerState
→ GameRuleManager.HandleParticipantDeath(PlayerState)
```

GameRuleManager:

```text
Active participant?
    ↓ yes
AliveParticipants에서 제거
    ↓
EvaluateEndConditions()
```

이미 제거된 participant의 중복 death notification은 무시한다.

## 3.5 Disconnect

연결 종료 시 stale participant가 생존자로 남아 종료를 막지 않도록 등록 상태를 정리한다.

Phase 02 prototype에서는 Active Round participant가 disconnect하면 현재 생존 집계에서 제거할 수 있다.

다만 "disconnect를 최종 게임에서 사망/패배로 취급하는가"에 대한 최종 정책은 별도 TBD다.

---

# 4. End Condition 평가

## 4.1 평가 진입점

Phase 02에서 게임 종료 평가는 최소 다음 시점에 실행한다.

```text
Participant Death
→ EvaluateEndConditions()
```

후속 Phase에서는 동일 진입점을 다음 사건에서도 재사용할 수 있다.

```text
Escape resolved
Parasite eliminated
Objective state changed
기타 실제 게임 규칙 변화
```

범용 Event Bus는 만들지 않는다.

각 실제 기능은 의미가 분명한 Manager 진입점을 통해 평가를 요청한다.

## 4.2 ULuxLastSurvivorEndCondition

현재 Prototype 규칙:

```text
AliveParticipants > 1
→ Continue

AliveParticipants == 1
→ End
→ Winner = remaining participant

AliveParticipants == 0
→ End
→ Winners = empty
```

이 조건은 Round State가 Playing일 때만 평가한다.

Round Start 직전이나 Lobby / Waiting 상태에서 Player 수가 1명이라는 이유로 게임을 종료하지 않는다.

## 4.3 Manager → GameMode Request

End Condition이 결과를 만들면 GameRuleManager는 직접 GameState를 수정하지 않는다.

```text
GameRuleManager
→ GameMode.RequestGameEnd(Result)
```

GameMode는 다음을 검증한다.

- Server authority
- 현재 Round State == Playing
- 아직 종료 확정되지 않음
- Result 유효

유효하면 GameMode가 Round를 Ended로 확정한다.

---

# 5. GameMode / GameState 상태 흐름

기본 상태:

```text
Waiting
   ↓ RequestStartRound
Playing
   ↓ RequestGameEnd
Ended
```

Phase 02에서는 자동 Round Reset을 만들지 않는다.

재시작/다음 판 UX는 Lobby / Result / Session 흐름과 함께 이후 결정한다.

GameMode가 상태를 확정한 뒤 서버 내부 시스템에 알릴 수 있도록 Round State Changed delegate를 둘 수 있다.

```text
GameMode authoritative state commit
        ├─ GameState replicated state update
        └─ OnRoundStateChanged server delegate
```

후속 Task / Blackout / Escape 등 서버 Manager는 필요 시 이 delegate를 구독할 수 있다.

Client는 GameMode에 접근하지 않고 GameState의 replicated state와 local delegate를 사용한다.

---

# 6. 개발용 Round Start Driver

최종 Lobby의 게임 시작 장치/상호작용은 아직 TBD다.

따라서 Phase 02에서는 Production API와 테스트 Driver를 분리한다.

Production API:

```text
ALuxGameMode::RequestStartRound()
```

Development-only driver 예:

```text
LuxRoundStart
LuxRoundStatus
```

원칙:

- Shipping gameplay UI가 아님
- Host/server 개발 검증용
- 실제 StartRound 로직을 우회하지 않음
- 이후 Lobby start interaction은 동일 Production API를 사용

---

# 7. Checkpoint 계획

## 02-A - Death Event Boundary

### 목적

현재 Phase 01의 직접 사망 상태를 상위 게임 규칙이 안전하게 관찰할 수 있는 Event boundary로 확장한다.

### 범위

- `ALuxCharacter::OnDeathConfirmed`
- Server-only death broadcast
- duplicate death 방지 유지
- `OnRep_IsDead`와 gameplay event 분리
- GameMode의 Character death binding 기반

### 작업 순서

1. 현재 `ALuxCharacter::Die()` 호출 경로를 확인한다.
2. server authority / duplicate guard를 유지한다.
3. death state commit 이후 `OnDeathConfirmed`를 한 번 broadcast한다.
4. Client `OnRep_IsDead`에서는 broadcast하지 않는다.
5. GameMode가 Character를 직접 찾게 하지 않고 GameMode가 lifecycle에서 delegate를 bind한다.
6. 2 Player에서 Host/Client 사망 모두 Server handler가 정확히 한 번 호출되는지 확인한다.

### 검수 기준

- Character가 GM / GS / RuleManager를 참조하지 않음
- 실탄 사망 기존 동작 유지
- Server에서 death event 정확히 1회
- Client replication으로 동일 event가 중복되지 않음

---

## 02-B - Game Rule Manager & Participant Tracking

### 목적

Pawn 생명주기와 분리된 Round participant / alive truth를 만든다.

### 범위

- `ULuxGameRuleManagerComponent`
- PlayerState 기반 participant registry
- Active Round participant snapshot
- Alive participant tracking
- Round Start API
- non-Shipping Round Start driver

### 작업 순서

1. GameMode 기본 Component로 GameRuleManager를 생성한다.
2. PostLogin / Logout을 participant registry와 연결한다.
3. Round Start 시 현재 참가자를 snapshot한다.
4. 최소 2명 validation을 둔다.
5. Active participant와 Alive participant를 구분한다.
6. Character death를 PlayerState death event로 전달한다.
7. 중복 death와 non-active death가 alive count를 손상시키지 않는지 확인한다.

### 검수 기준

- PlayerState 기준 participant identity
- Round 시작 전 death/end condition 오작동 없음
- 최소 2명 미만 Round Start 거부
- Late Join이 현재 Active snapshot을 임의로 변경하지 않음
- death 시 Alive count 정확히 1 감소
- duplicate notification 안전

---

## 02-C - End Condition & Authoritative Game End

### 목적

게임 종료 조건과 authoritative commit / replication 경계를 완성한다.

### 범위

- `ULuxGameEndCondition`
- `ULuxLastSurvivorEndCondition`
- `ELuxRoundState`
- `FLuxGameResult`
- GameMode `RequestGameEnd`
- GameState replicated Round State / Result

### 작업 순서

1. End Condition의 최소 공통 contract를 만든다.
2. Last Survivor condition을 구현한다.
3. GameRuleManager가 active End Conditions를 평가하도록 한다.
4. condition 만족 시 GameMode에 Result를 요청한다.
5. GameMode가 중복/무효 End request를 거부한다.
6. GameMode만 GameState의 Round state/result를 commit한다.
7. GameState에서 Client용 OnRep / delegate를 제공한다.
8. Manager가 GameState를 직접 수정하는 경로가 없는지 검수한다.

### 검수 기준

- Alive > 1이면 Playing 유지
- Alive == 1이면 LastSurvivor 종료
- Alive == 0이면 Winner 없이 종료
- End request 중복 안전
- GameState가 판단 로직을 가지지 않음
- Manager → GameState 직접 write 없음
- Client가 replicated Ended / Result를 동일하게 관찰

---

## 02-D - Elimination Multiplayer QA & Phase Close

### 목적

현재 Revolver 시스템과 Round Rule 시스템을 결합해 실제 섬멸전 한 판이 끝나는지 검증한다.

### 범위

- 2 Player regression
- 3+ Player elimination
- 가능하면 6 Player PIE
- Host / Client kill combinations
- simultaneous lethal
- disconnect stale state
- Round state/result replication
- Phase 결과 기록

### 테스트

1. 2 Player Round Start.
2. Host가 Client를 실탄으로 사살.
3. Remaining Host가 Winner로 확정되는지 확인.
4. Client가 Host를 사살하는 반대 조합 확인.
5. 3 Player 이상에서 첫 사망 후 Round가 계속되는지 확인.
6. 마지막 2명 중 한 명 사망 시 Ended 확인.
7. 가능한 동일 server frame / 근접 타이밍의 상호 사망에서 Alive == 0 결과 확인.
8. dead Character의 재사망 notification이 결과를 재확정하지 않는지 확인.
9. Active participant disconnect 시 stale alive count가 남지 않는지 확인.
10. 모든 Client에서 같은 Round State / Result를 확인.
11. 기존 Revolver chamber secrecy / fire / reload 회귀 테스트를 수행한다.

### 검수 기준

- Death → GM → RuleManager → EndCondition → GM → GS 흐름이 Server authority에서 닫힘
- Character가 상위 규칙 객체를 모름
- 2명 이상 섬멸전이 실제로 종료됨
- 첫 사망으로 무조건 종료되지 않음
- 마지막 생존자 결과 일관
- 동시 전멸 결과 안전
- 중복 End commit 없음
- 기존 리볼버 기능 회귀 없음

---

# 8. Phase 02에서 하지 않는 것

- Parasite / Host role
- Citizen / Parasite faction victory rule
- Escape victory rule
- Spectator 전환
- Dead player voice information restriction
- Result UI
- Gameplay HUD
- Round restart / rematch UX
- Lobby final start device
- Final Late Join policy
- Final disconnect win/loss policy
- Team system
- Voting
- Role reveal
- Generic gameplay event bus
- Data-driven rule framework
- Blueprint rule editor
- Room Frame / Map generation
- Placement Point / procedural prop placement

Room Frame / Map architecture는 별도 설계 논의 후 계획한다.

---

# 9. 최종 구조 요약

```text
ALuxRevolver
    │
    │ lethal hit
    ▼
ALuxCharacter
    │
    │ Die() commits body death
    │ OnDeathConfirmed
    ▼
ALuxGameMode
    │
    │ HandleCharacterDeath
    ▼
ULuxGameRuleManagerComponent
    │
    │ participant state update
    │ EvaluateEndConditions
    ▼
ULuxGameEndCondition
    │
    │ matched result
    ▼
ULuxGameRuleManagerComponent
    │
    │ RequestGameEnd(Result)
    ▼
ALuxGameMode
    │
    │ authoritative commit
    ▼
ALuxGameState
    │
    │ replication
    ▼
Clients
```

책임 요약:

```text
Character
= 몸의 죽음

GameRuleManager
= 참가자 상태와 게임 규칙 평가

EndCondition
= 하나의 종료 규칙

GameMode
= authoritative request/commit boundary

GameState
= 확정된 공유 상태 복제
```

---

# 10. 구현 전 최종 확인 항목

이 문서는 Round/Game Rule 구조 초안이다.

구현 전 다음만 다시 확인한다.

- End Condition을 독립 UObject로 두는 현재 경계가 적절한가
- GameResult의 최소 필드가 충분한가
- Round Start를 Development command로 먼저 구동하는 방식이 적절한가
- Disconnect를 prototype alive set에서 제거하는 정책을 그대로 사용할지
- Character death delegate binding의 정확한 Unreal lifecycle override

Room Frame / Map 구조는 이 Phase와 분리해 별도로 논의한다.
