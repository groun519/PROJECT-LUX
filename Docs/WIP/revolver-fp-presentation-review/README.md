# Phase 01-E First-Person Presentation Review

> Status: **Correction Required**  
> Reviewed implementation: `main@49e3f7ee701d0247f91663a1229e9ea760812277`  
> Scope: Phase 01-E only  
> Reference: `Docs/Development/Phases/Phase01_RevolverFPSFoundation.md`  
> Purpose: 01-E 구현을 01-F 진행 전에 재검수하고, 기존 01-B~01-D 기반을 보존한 채 필요한 수정 범위를 명확히 한다.

---

# 1. Review conclusion

01-E는 기능적으로 FP Arms, ADS, Fire/Reload montage, SFX, Muzzle Flash를 연결했고 기존 Server Authority와 Chamber secrecy도 유지했다.

하지만 01-B~01-D와 비교했을 때 다음 문제가 새로 들어왔다.

1. Local Fire prediction이 Server validation과 동일하지 않아 허위/누락 Presentation이 발생할 수 있다.
2. `bIsAiming` 하나가 Local prediction state와 replicated authoritative state를 동시에 맡아 owner state가 latency에서 되감길 수 있다.
3. R21 AnimBP 내부 변수 `"Aim"`을 문자열 Reflection으로 매 Tick 직접 조작한다.
4. Fire/Reload 입력 hot path에서 required presentation asset을 `LoadSynchronous()`로 반복 resolve한다.
5. Muzzle socket을 어느 SkeletalMesh가 실제 소유하는지 코드에서 보장하지 않는다.
6. Cylinder visual presentation이 실제로 완성됐는지 코드만으로 확인되지 않는다.
7. Phase 문서 상단 상태가 아직 01-D로 남아 있다.

판정:

**현재 01-E는 "동작 확인된 prototype presentation" 수준이며, 01-F 진행 전 correction checkpoint를 두는 것이 적절하다.**

01-B~01-D의 Chamber / Fire Authority / Death / Reload truth 구조를 다시 설계하지 않는다.

---

# 2. 반드시 보존할 기존 구조

이번 correction에서 다음은 정상 기반으로 취급한다.

## Fact

- `ALuxRevolver`가 Revolver entity와 Chamber truth를 소유한다.
- 정확한 `ChamberRoundTypes`는 Server-only이며 복제하지 않는다.
- Client에는 `LoadedMask`, 공개 Chamber state 등 관찰 가능한 정보만 복제한다.
- `ServerFire`가 authoritative Fire validation / Trace / round resolution을 수행한다.
- Client가 HitResult 또는 exact RoundType을 Server에 보내지 않는다.
- Live / Blank / Rubber의 외부 Fire presentation은 가능한 한 동일해야 한다.
- `BeginRoundInsertion()`이 production insertion entry point다.
- 개발용 Load driver는 production insertion path를 우회하지 않는다.
- `ALuxCharacter::Die()`의 현재 Phase 01 death foundation을 01-E correction 때문에 확장하지 않는다.
- 범용 Weapon Framework를 만들지 않는다.

## Requirement

Correction은 위 경계를 유지하면서 Presentation 연결만 바로잡는다.

---

# 3. Finding E-01 - Local Fire prediction과 Server validation 불일치

## Severity

**High**

## Fact

현재 `ALuxRevolver::RequestFire()`는 Server 결과를 받기 전에 local presentation을 재생한다.

Local 조건:

```text
IsLocallyPresented
Character exists
Character alive
Cylinder locally closed
CurrentChamberIndex locally valid
```

그 뒤 Server는 `CanFire()`에서 추가로 다음을 검증한다.

```text
Authority
actual equipped revolver
alive
cylinder closed
valid chamber
MinimumFireIntervalSeconds
```

현재 local prediction에는 최소 발사 간격 검증이 없다.

또한 owner가 보는 `LoadedMask`, `CurrentChamberIndex`, `bCylinderOpen`은 replicated authoritative state이므로 latency 동안 Server보다 오래된 값일 수 있다.

`OnRep_FireSequence()`와 `OnRep_DryFireSequence()`는 현재 비어 있어 owner가 Server의 accepted result를 이용한 correction을 하지 않는다.

## Failure examples

### A. Fire cadence reject

```text
Local click
→ Fire presentation

0.25 sec 이전 재클릭
→ Local fire presentation again

Server
→ second request rejected by MinimumFireIntervalSeconds
```

결과:

- 실제 Chamber는 소비되지 않았는데 총성 / Muzzle Flash / Fire montage가 재생될 수 있다.

### B. Close cylinder 직후 fire under latency

```text
Client local state: Cylinder still Open
Server: Close request already accepted

Client presses Fire
→ local presentation skipped because stale bCylinderOpen == true
→ ServerFire sent

Server
→ cylinder is closed
→ shot accepted
```

현재 accepted `FireSequence`의 OnRep이 owner presentation을 재생하지 않으므로 실제 Server shot이 발생했는데 owner는 Fire cosmetic을 못 볼 수 있다.

### C. Chamber state stale

```text
Client public chamber state != latest Server public chamber state
→ local code chooses Fire vs DryFire from stale LoadedMask / CurrentChamberIndex
→ Server resolves a different public result
```

정확한 탄종 유출 문제는 아니지만 owner feedback이 authoritative result와 달라질 수 있다.

## Requirement

- Local responsiveness는 유지한다.
- Server Authority는 유지한다.
- exact Live / Blank / Rubber truth를 Client prediction state에 추가하지 않는다.
- Local prediction이 최소한 Server의 공개 검증 조건과 cadence를 일관되게 따라야 한다.
- accepted/rejected Server event와 local prediction 사이에 필요한 최소 correction 경계를 둔다.
- Server가 accepted한 Fire가 owner에게 완전히 누락되는 경로가 없어야 한다.
- 같은 Fire가 owner에게 두 번 재생되어서는 안 된다.
- 범용 네트워크 prediction framework를 새로 만들지 않는다.

## Acceptance

- 0.25초 이내 spam click에서 fake second Fire cosmetic이 발생하지 않는다.
- 50ms / 100ms latency에서 Cylinder close 직후 Fire가 accepted되면 owner presentation이 정확히 1회 발생한다.
- stale public Chamber state가 있어도 다음 authoritative update 이후 owner visual state가 수렴한다.
- Chamber secrecy가 유지된다.

---

# 4. Finding E-02 - `bIsAiming`이 Local intent와 replicated truth를 동시에 소유

## Severity

**High**

## Fact

현재 owner Client는 `SetAiming()`에서 `bIsAiming`을 즉시 변경한 뒤 `ServerSetAiming()` RPC를 보낸다.

동시에 `bIsAiming`은 일반 replicated property다.

```text
Owner local input
→ bIsAiming local write
→ ServerSetAiming
→ Server bIsAiming write
→ same bIsAiming replicated back to owner
```

Local FOV와 Fire montage 선택도 같은 `bIsAiming`을 읽는다.

## Risk

빠른 Aim press/release와 latency가 겹치면 owner가 이미 release한 뒤 Server의 이전 `true` state가 owner property에 도착해 Local presentation을 잠시 다시 Aim 상태로 만들 수 있다.

즉 하나의 bool이 다음 두 책임을 동시에 가진다.

```text
Local immediate presentation intent
Replicated authoritative remote state
```

## Requirement

- Local owner의 즉시 ADS 반응과 Remote player에게 필요한 authoritative Aim state를 구분한다.
- owner replication이 최신 Local input intent를 불필요하게 되감지 않아야 한다.
- Remote view를 위해 authoritative Aim state는 유지한다.
- 현재 요구에 맞는 가장 작은 구조를 사용한다.

가능한 방향은 구현 시 현재 Unreal replication 흐름을 확인해 선택한다.

예:

```text
local presentation intent + replicated server aim state
```

또는 owner에게 불필요한 authoritative echo를 제외하는 방식.

특정 방식은 이 문서가 강제하지 않는다.

## Acceptance

- 100ms latency에서 Aim press/release 반복 시 FOV가 이전 Server state 때문에 다시 Aim으로 튀지 않는다.
- Remote state는 Server 권한으로 유지된다.
- Dead Character가 Aim 상태로 되돌아가지 않는다.

---

# 5. Finding E-03 - R21 AnimBP 내부 변수에 문자열 Reflection으로 직접 결합

## Severity

**High**

## Fact

현재 `ALuxCharacter::UpdateFirstPersonPresentation()`은 매 Tick 다음 작업을 한다.

```cpp
FindFProperty<FBoolProperty>(
    AnimInstance->GetClass(),
    TEXT("Aim")
)
```

그리고 찾은 property를 직접 변경한다.

Character는 R21 vendor AnimBP에 `Aim`이라는 정확한 이름의 bool property가 있다는 사실을 알고 있다.

또한 property가 없거나 이름이 바뀌어도 compile-time 오류 없이 기능이 조용히 실패한다.

## Problem

현재 dependency:

```text
ALuxCharacter
→ vendor ABP implementation detail
→ string "Aim"
→ Reflection write
```

이는 Project gameplay object와 vendor asset implementation 사이의 명확한 진입점이 아니다.

또한 `FindFProperty`를 매 frame 수행할 이유가 없다.

## Requirement

- `ALuxCharacter`가 vendor AnimBP 내부 property 이름을 직접 알지 않게 한다.
- 문자열 Reflection을 정상 runtime state 전달 방식으로 사용하지 않는다.
- Project-owned typed boundary를 둔다.
- 새 범용 Animation Framework는 만들지 않는다.
- R21 원본 asset을 직접 뜯어 고치는 것보다 LUX 소유 adapter/AnimInstance/derived asset 등 유지 가능한 최소 경계를 우선 검토한다.
- 정확한 Unreal/R21 구현 방식은 Codex가 실제 local R21 asset 구조를 확인한 뒤 결정한다.

## Acceptance

- `UObject/UnrealType.h`와 Tick의 `FindFProperty("Aim")` 의존이 제거된다.
- Aim state 전달이 compile-time 또는 명시적 typed contract로 추적 가능하다.
- R21 내부 변수명 변경이 silent runtime failure로 이어지는 구조가 아니다.
- Aim animation이 기존과 동일하게 동작한다.

---

# 6. Finding E-04 - Presentation hot path의 synchronous asset loading

## Severity

**Medium**

## Fact

다음 runtime input/presentation 경로에서 `LoadSynchronous()`를 직접 호출한다.

- Fire montage
- Weapon Fire montage
- Reload montage
- Weapon Reload montage
- Muzzle Flash
- Fire Sound
- Dry Fire Sound
- Cylinder Sound
- Round Insert Sound

예:

```text
Fire input
→ PlayFirePresentation()
→ LoadSynchronous()
```

## Problem

이 asset들은 현재 Revolver FP presentation에서 항상 필요한 확정 자산이다.

첫 사용 순간 sync load가 발생하면 Fire / Reload 입력 시 hitch 가능성이 생긴다.

Soft reference 자체가 잘못이라는 의미는 아니다. 문제는 required runtime asset resolve가 input hot path에 있다는 점이다.

## Requirement

- required FP presentation asset을 실제 Fire/Reload 입력 전에 준비한다.
- hot path에서는 이미 준비된 object를 사용한다.
- 필요한 경우 soft path를 유지해도 되지만 resolve/cache 시점을 분리한다.
- 이 문제 해결을 위해 Asset Manager framework나 generic loader layer를 추가하지 않는다.

## Acceptance

- Fire/Reload/Mechanical presentation 함수 안에서 required asset을 매번 `LoadSynchronous()`하지 않는다.
- missing asset은 개발 중 명확히 확인 가능하다.
- 기존 asset 경로와 visual 결과는 유지된다.

---

# 7. Finding E-05 - Muzzle socket owner 확인 필요

## Severity

**Verification Required / Medium if incorrect**

## Fact

Revolver FP mesh는 다음과 같이 Arms의 `38` socket에 붙는다.

```text
FirstPersonWeaponMesh
→ FirstPersonArms socket "38"
```

하지만 Fire SFX와 Niagara Muzzle Flash의 attachment component는 `FirstPersonArms`이며 socket name은 `weapon_r_muzzle`이다.

```text
SpawnSoundAttached
→ FirstPersonArms / weapon_r_muzzle

SpawnSystemAttached
→ FirstPersonArms / weapon_r_muzzle
```

Source code만으로는 R21에서 `weapon_r_muzzle` socket이 Arms에 있는지 Revolver mesh에 있는지 확인할 수 없다.

## Requirement

Codex는 local R21 asset에서 실제 socket owner를 확인한다.

- Arms에 실제 socket이 있으면 현재 parent가 맞다는 근거를 기록한다.
- Weapon mesh socket이면 Fire SFX / Muzzle Flash를 해당 mesh에 붙인다.
- 주요 required socket은 initialization 시 한 번 검증 가능한 형태를 우선한다.
- socket 누락을 silent failure로 두지 않는다.

## Acceptance

- Muzzle Flash가 실제 barrel muzzle에서 발생한다.
- Fire audio attachment 위치가 의도한 component/socket과 일치한다.
- `38` 및 `weapon_r_muzzle`의 실제 owner가 작업 보고에 기록된다.

---

# 8. Finding E-06 - Cylinder visual presentation 완성 여부 확인 필요

## Severity

**Verification Required**

## Fact

현재 `PlayCylinderPresentation()`은 Cylinder Open/Close Sound만 재생한다.

Source에서 `bCylinderOpen` 상태가 FP weapon cylinder visual 또는 R21 cylinder animation으로 연결되는 명시적 코드 경로는 확인되지 않는다.

01-E 계획 범위에는 Reload / Cylinder presentation이 포함되어 있다.

## Requirement

Codex는 실제 R21 FP Weapon AnimBP / montage를 확인한다.

다음 중 무엇인지 보고한다.

1. 기존 reload montage/AnimBP가 이미 cylinder visual을 올바르게 처리한다.
2. Cylinder Open/Close visual state가 현재 빠져 있다.

2번이면 현재 checkpoint 범위 안에서 필요한 최소 visual 연결을 구현한다.

단, cylinder truth는 계속 `ALuxRevolver::bCylinderOpen`의 Server-authoritative state를 따른다.

## Acceptance

- Open/Close 상태를 실제 1인칭 weapon visual에서 확인할 수 있다.
- Sound만 바뀌고 cylinder geometry는 그대로인 상태가 아니다.
- Reload montage와 cylinder state가 서로 충돌하지 않는다.

---

# 9. Finding E-07 - Character Tick 범위

## Severity

**Low**

## Fact

01-E에서 `PrimaryActorTick.bCanEverTick`이 false에서 true로 변경됐다.

현재 Tick은 Local FOV interpolation과 R21 `Aim` Reflection을 위해 사용된다.

Remote Character와 Server-side non-local Character도 Actor Tick 호출 자체는 발생한 뒤 `IsLocallyControlled()`에서 빠진다.

## Requirement

E-03 수정 후 Tick의 남은 실제 책임을 다시 확인한다.

- FOV interpolation을 위해 Local Character Tick이 필요하면 유지할 수 있다.
- 모든 Character instance가 불필요하게 Actor Tick을 돌 필요는 없다.
- 01-F 요구를 추측해 새 Tick framework를 만들지 않는다.

이 항목은 correctness blocker가 아니다.

---

# 10. Finding E-08 - Phase 문서 상태 불일치

## Severity

**Low**

## Fact

현재 `Phase01_RevolverFPSFoundation.md` 상단은:

```text
상태: In Progress - 01-D Ready for Review
```

로 남아 있지만 01-E 실행 결과와 checklist는 완료 상태로 기록되어 있다.

또한 현재 review 결과상 01-E는 correction 전에는 최종 통과로 보기 어렵다.

## Requirement

Correction 완료 후 실제 상태에 맞게 문서 header / 실행 결과 / Known Issues를 갱신한다.

수정 전에는 01-F를 완료 상태로 진행하지 않는다.

---

# 11. Out-of-scope observation - 01-D부터 존재한 Cylinder input latency

이 항목은 01-E가 새로 만든 regression이 아니다.

현재 Character Reload input은 local replicated `bCylinderOpen`을 보고 Open / Close / Cancel request를 선택한다.

고 latency에서 첫 Open request의 replication이 돌아오기 전에 R을 다시 누르면 Client가 다시 Open request를 보낼 수 있다.

이는 향후 latency QA에서 확인할 수 있다.

이번 01-E correction이 Cylinder input state까지 자연스럽게 건드리지 않는 한, 이 문제를 이유로 01-D 구조 전체를 리팩토링하지 않는다.

---

# 12. Correction scope

## Must fix before 01-F

1. E-01 Fire prediction / authoritative confirmation 정합성
2. E-02 owner Aim prediction과 replicated Aim state 책임 분리
3. E-03 vendor AnimBP string Reflection 제거
4. E-04 input hot path synchronous load 제거 또는 사전 resolve/cache
5. E-05 muzzle socket owner 실제 asset 검증 및 필요 시 수정
6. E-06 Cylinder visual presentation 실제 상태 확인 및 누락 시 연결
7. E-08 Phase 문서 상태 정리

## May fix if naturally adjacent

- E-07 unnecessary Character Tick 범위 축소

## Do not do

- 01-F Third-Person implementation
- generic Weapon hierarchy
- generic Presentation framework
- generic prediction framework
- inventory/ammo pickup
- Drop/Transfer
- Parasite/Host
- Round/GameRule
- 01-B Chamber model 재설계
- 01-C Fire authority / death model 재설계
- 01-D insertion API 재설계
- R21 vendor folder 대규모 수정
- unrelated refactor

---

# 13. Required verification

Correction 후 최소 다음을 실행한다.

## Build

- UE 5.8 Development Editor build
- Map Check

## Local FP

- ADS press/release
- rapid ADS press/release
- Hip Fire
- Aim Fire
- Dry Fire
- Cylinder Open / Close
- Single Round Reload
- Reload Cancel
- death while aiming
- death while montage active

## Multiplayer

최소 3 Player PIE:

- Host fire
- Client fire
- Client → Host lethal regression
- Chamber secrecy regression
- owner-only FP Arms regression
- owner Fire presentation exactly once

## Latency

Packet Simulation 또는 동등한 환경:

- 50ms
- 100ms

검증:

1. rapid Fire spam에서 Server-rejected shot이 fake accepted shot처럼 보이지 않는가
2. Cylinder Close 직후 Fire에서 accepted Server shot presentation이 누락되지 않는가
3. Aim press/release가 stale replicated state로 되감기지 않는가

---

# 14. Codex completion report

Correction 작업 후 다음 형식으로 보고하고 멈춘다.

1. Changed Files
2. Root Cause per Finding
3. Implemented Fix per Finding
4. R21 Asset Facts
   - `38` socket owner
   - `weapon_r_muzzle` socket owner
   - Cylinder animation/state source
5. Tests Run
6. Latency Test Result
7. Chamber Secrecy Regression Result
8. Remaining Known Issues
9. Ready for Review

검수 전 01-F를 시작하지 않는다.
