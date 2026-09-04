# Phase 01 Revolver Presentation Architecture Rework

> Status: **Implementation-ready architecture correction**  
> Reviewed implementation: `main@0275e299b9400488e7770f322f285f3b1e82792c`  
> Scope: Phase 01-E/F presentation, Third-Person Aim IK, Manual Chamber Reload  
> Preserve: Phase 01-B Chamber model, 01-C Server Fire/Death authority, 01-D production insertion concept  
> Goal: 현재 동작을 유지하면서 Gameplay truth, shared Revolver visual, FP presentation, TP presentation의 책임을 분리한다.

---

# 0. Latest review conclusion

이전 `main@f5e7beee` 검수 이후 다음 커밋이 추가되었다.

```text
7b2bf0a5  Revolver Foundation 10 - Third Person Aim IK
0d927970  Revolver Foundation 11 - Locomotion Blend Space Correction
0275e299  Revolver Foundation 12 - Manual Chamber Reload
```

기능 완성도는 올라갔다.

특히 다음은 좋은 방향이다.

- `ULuxRevolverAnimInstance` + `ABP_LuxRevolver`가 추가되어 Drum rotation / Bullet visibility를 프로젝트 소유 typed animation layer로 옮기기 시작했다.
- Manual Reload Position 1~6이 `CurrentChamberIndex` 기준으로 실제 Chamber에 매핑되는 gameplay 개념이 생겼다.
- Empty Dry Fire도 다음 Chamber로 진행해 실제 실린더 흐름과 공개 상태가 일치하도록 수정되었다.
- Third-Person Aim IK가 별도 presentation 계산으로 보강되었다.

하지만 구조 문제는 해결되지 않았고, **Manual Chamber Reload가 추가되면서 오히려 Gameplay와 R21 Presentation의 결합이 더 강해졌다.**

현재 리팩터를 01-G 이전에 수행하는 것이 적절하다.

---

# 1. Architecture principles

이번 작업의 판단 기준은 코드량이나 Ponytail 최소화가 아니다.

## Decision - 정확한 객체화

현재 실제로 존재하는 기능 단위는 최소 다음 네 가지다.

```text
Revolver Gameplay Truth
Shared Revolver Mechanical Visual
First-Person Revolver Presentation
Third-Person Character/Revolver Presentation
```

이들은 실제 책임이 다르므로 각각의 소유자를 둔다.

## Decision - 작은 Gameplay entry point

외부 호출은 계속 의미만 표현한다.

```text
RequestFire()
RequestOpenCylinder()
RequestCloseCylinder()
RequestCancelReload()
BeginRoundInsertion(ReloadPosition, RoundType)
```

호출자는 R21 montage second, bone name, Niagara, FP/TP mesh를 모른다.

## Decision - Vendor knowledge isolation

`R21`이라는 이름, asset path, bone/socket, montage marker, grip/muzzle offset은 Presentation implementation에만 존재한다.

Gameplay class에서 제거한다.

## Decision - 확정된 미래

Revolver는 독립 Actor로 유지한다.

Drop / Transfer가 구현될 때도 Chamber truth는 Revolver에 남는다.

반대로 Rifle / Shotgun / generic Weapon framework는 만들지 않는다.

---

# 2. Current responsibility problems

## A-01 - ALuxRevolver remains a mixed Gameplay + Presentation object

### Severity

**Critical Architecture**

현재 `ALuxRevolver`는 gameplay과 presentation을 동시에 소유한다.

### Gameplay responsibilities

- exact `ChamberRoundTypes`
- `LoadedMask`
- `CurrentChamberIndex`
- Reload Position -> Chamber mapping
- Cylinder state
- Fire / Reload RPC
- Server validation
- Hitscan
- Live / Blank / Rubber / Empty resolution
- Chamber advance
- insertion pending / commit

### Presentation responsibilities

- FP / TP Weapon Mesh
- FP / TP Muzzle Anchor
- R21 mesh / AnimBP path
- R21 socket / bone / grip / muzzle constants
- FP / TP fire montage
- TP retarget animation
- Dry Fire / Cylinder sound
- Niagara
- reload pose timer
- local prediction presentation state
- JIP presentation ready state
- bullet bone visibility helper

`ALuxRevolver`는 실제 총의 상태와 행동만 소유하고, 보이는 방법은 presentation unit가 소유해야 한다.

---

## A-02 - Gameplay mutation directly invokes Presentation

### Severity

**Critical Architecture**

현재 다음 gameplay 함수가 직접 presentation을 실행한다.

```text
ResolveServerFire()
BeginRoundInsertion()
CommitPendingRoundInsertion()
TryOpenCylinder()
TryCloseCylinder()
TryCancelReload()
```

그리고 RepNotify도 직접 FP/TP 실행 정책을 결정한다.

목표 흐름:

```text
Gameplay truth changes
        ↓
Semantic replicated state/event
        ↓
Concrete presentation units consume it
```

Gameplay mutation 함수에서 Montage / Sound / Niagara 호출을 제거한다.

---

# 3. New critical finding from Manual Chamber Reload

## A-03 - Production reload validity and commit timing depend on R21 animation windows

### Severity

**Critical Architecture / Immediate blocker**

최신 `0275e299`에서 `ALuxRevolver.cpp`에 다음 R21 전용 데이터가 추가되었다.

```text
R21RoundInsertStartSeconds[6]
R21RoundInsertCommitSeconds[6]
TryGetR21RoundInsertWindow(...)
```

그리고 production API:

```text
BeginRoundInsertion(ReloadPosition, RoundType)
```

가 `TryGetR21RoundInsertWindow()` 성공 여부를 validation 조건으로 사용한다.

즉 현재는:

```text
게임 규칙상 유효한 Chamber insertion
        ↓
R21 animation window가 존재해야만 허용
```

구조다.

이 의존은 제거한다.

## Decision - Gameplay reload truth

다음은 Gameplay domain이다.

```text
ReloadPosition 1..6
CurrentChamberIndex 기준 위치 매핑
Target Chamber가 Empty인지
RoundType이 유효한지
Cylinder가 Open인지
Insertion Pending인지
RoundInsertionDurationSeconds
```

`BeginRoundInsertion()`은 이 정보만으로 성공/실패를 결정한다.

R21 animation window 존재 여부는 production gameplay validation에 참여하지 않는다.

## Decision - Presentation mapping

다음은 Presentation domain이다.

```text
Position 1 visual segment
Position 2 visual segment
...
Position 6 visual segment
R21 montage start marker
R21 visual insert marker
Open pose marker
visual settle delay
```

FP / TP presentation이 `ActiveReloadPosition`을 보고 해당 R21 segment를 재생한다.

## Gameplay commit duration

각 R21 구간 길이가 서로 달라도 Server gameplay timing은 asset에 종속하지 않는다.

권장:

```text
ALuxRevolver::RoundInsertionDurationSeconds
```

하나의 semantic duration을 둔다.

최초 migration에서는 현재 체감과 최대한 같게 설정한다.

Presentation은 각 position의 source segment 길이에 맞추어 play rate를 계산해 semantic duration에 맞춘다.

---

# 4. Shared Revolver mechanical visual

## Positive finding

`ULuxRevolverAnimInstance`와 `ABP_LuxRevolver` 도입은 유지한다.

이 객체는 현재:

```text
CurrentChamberIndex
LoadedMask
        ↓
DrumRotationDegrees
Bullet1..6Scale
```

를 계산한다.

이는 FP와 TP 양쪽 Revolver Mesh가 공유하는 **실제 Revolver mechanical visual**이라는 정확한 기능 단위에 가깝다.

## Decision

`ULuxRevolverAnimInstance`는 다음을 소유한다.

- Drum visual rotation
- Chamber index -> drum angle
- LoadedMask -> bullet visual state
- Revolver mesh 내부 mechanical visual 계산

FP/TP presentation component가 각각 같은 로직을 복제하지 않는다.

## Cleanup required

현재 `ALuxRevolver`에 남아 있는:

```text
R21BulletBones
RefreshBulletVisuals()
HideBoneByName / UnHideBoneByName path
```

는 `ABP_LuxRevolver`가 실제 1P/3P 양쪽 Bullet visibility를 완전히 대체하는 것을 확인한 뒤 제거한다.

같은 visual truth를 Actor와 AnimInstance 두 곳에서 동시에 쓰지 않는다.

---

# 5. Third-Person Aim IK review

## Positive finding

`ULuxCharacterAnimInstance`가 다음 presentation-only 값을 소유하는 것은 적절하다.

- spine aim distribution
- neck/head look distribution
- right hand aim rotation
- minimum convergence distance
- maximum visual parallax
- smoothing

이는 body pose presentation 책임이다.

## A-04 - TP AnimInstance still assumes alive means revolver equipped

### Severity

**High / Confirmed-future correctness**

현재:

```cpp
const bool bUseRevolverUpperBody = Character && !Character->IsDead();
```

이다.

수정:

```text
Character valid
AND alive
AND EquippedRevolver != null
```

`RevolverAimWeight`도 실제 equipped revolver가 있을 때만 활성화한다.

Drop system 자체는 구현하지 않는다.

## A-05 - Third-person muzzle presentation query leaks through Gameplay Actor

### Severity

**Medium Architecture**

현재 `ULuxCharacterAnimInstance`는:

```text
Character
→ EquippedRevolver
→ ALuxRevolver::GetThirdPersonMuzzleTransform()
```

을 사용한다.

Muzzle Transform은 gameplay truth가 아니라 TP visual resource에서 나온다.

최종 구조에서 이 transform의 실제 계산/mesh knowledge는 TP Presentation unit가 소유한다.

`ALuxRevolver`가 작은 facade getter로 forwarding 하는 것은 허용할 수 있지만, Actor 자체가 TP mesh axis / muzzle anchor 계산을 구현하지 않는다.

---

# 6. Development manual reload driver leak

## A-06 - ALuxCharacter knows temporary debug ammo driver

### Severity

**Medium Architecture**

최신 Character는 직접:

```text
EKeys::One ... EKeys::Six
ReloadPosition1 ... ReloadPosition6
ReloadAtPosition()
ALuxPlayerController::LuxLoadRound("Live", Position)
```

를 알고 있다.

이 경로는 실제 gameplay input이 아니라 Inventory 이전의 non-Shipping driver다.

따라서 Character의 production responsibility가 아니다.

## Decision

- production `BeginRoundInsertion(ReloadPosition, RoundType)`는 유지한다.
- 개발용 Live round injection과 number-key driver는 `ALuxPlayerController` 또는 현재 Development driver 위치에 둔다.
- `ALuxCharacter`에서 `LuxPlayerController.h`, direct `EKeys::One..Six`, debug `ReloadAtPosition()`을 제거한다.
- 실제 Inventory/held-round input이 정해질 때 Character에는 실제 semantic input만 추가한다.
- 이를 위해 generic debug framework를 만들지 않는다.

---

# 7. Character presentation responsibility

## A-07 - ALuxCharacter still implements Revolver presentation details

### Severity

**High Architecture**

현재 Character가 다음을 소유한다.

```text
R21 FP Arms asset binding
FP Revolver-specific FOV interpolation
FP AnimInstance Aim driving
PlayFirstPersonMontage()
PlayThirdPersonUpperBodyAnimation()
StopFirstPersonMontages()
StopThirdPersonUpperBodyAnimation()
StopOwnerPresentation() 호출
```

Character의 최종 역할:

```text
body / movement / input
bIsDead
semantic aim state
owner-local aim intent
EquippedRevolver relation
FirstPersonCamera / FirstPersonArms anchor
```

Revolver-specific animation/VFX/SFX 실행은 분리한다.

---

# 8. Target object model

```text
ALuxCharacter
├─ body / movement / input
├─ death state
├─ semantic Aim state
├─ EquippedRevolver relationship
├─ FirstPersonCamera       [view anchor]
└─ FirstPersonArms         [view anchor]

ALuxRevolver
├─ exact Chamber truth
├─ public Chamber state
├─ CurrentChamberIndex
├─ ReloadPosition mapping
├─ Cylinder state
├─ Fire / Reload request APIs
├─ Server validation / trace / resolution
├─ semantic insertion duration
├─ replicated semantic state/events
├─ request-id transport
├─ ULuxRevolverFirstPersonPresentationComponent
└─ ULuxRevolverThirdPersonPresentationComponent

ULuxRevolverAnimInstance
= shared Revolver mechanical visual adapter
= Drum + bullet visual

ULuxRevolverFirstPersonPresentationComponent
├─ local presentation relevance
├─ R21 FP asset knowledge
├─ FP Weapon / Arms action animation
├─ ADS FOV / typed FP AnimInstance
├─ local Fire prediction + confirmation reconciliation
├─ FP Cylinder / Reload segment mapping
├─ FP SFX / Niagara
└─ local presentation timers / counters

ULuxRevolverThirdPersonPresentationComponent
├─ remote presentation relevance
├─ TP Weapon action animation
├─ retargeted body Fire / Reload action
├─ TP Cylinder / Reload segment mapping
├─ TP SFX / Niagara
├─ JIP cosmetic event baseline
└─ TP muzzle visual resource/query

ULuxCharacterAnimInstance
= locomotion + persistent TP body pose + procedural aim/look
```

---

# 9. ALuxRevolver ownership after rework

## Keep

- `ChamberRoundTypes`
- `LoadedMask`
- `CurrentChamberIndex`
- `ActiveReloadPosition`
- `GetChamberIndexForReloadPosition()`
- `IsReloadPositionLoaded()`
- `bCylinderOpen`
- `bRoundInsertionPending`
- Fire / DryFire / Reload / RoundInsert semantic sequences
- Server RPCs
- Server validation
- Hitscan / lethal / non-lethal resolution
- Chamber advance
- pending round type / target chamber
- authoritative insertion timer
- `RoundInsertionDurationSeconds`
- Fire request id network transport

## Remove from gameplay implementation

- R21 asset paths
- R21 socket / bone / grip / muzzle constants
- R21 insert start/commit arrays
- `TryGetR21RoundInsertWindow()`
- FP/TP montage refs
- TP retarget animation refs
- Niagara refs
- Sound refs
- visual play rate / pose seconds
- presentation asset cache
- FP/TP `Play*Presentation()` functions
- `RefreshBulletVisuals()`
- local presentation prediction map
- presentation timers/counters
- `bPresentationReplicationReady`
- actual TP muzzle transform calculation

---

# 10. Semantic event dispatch

RepNotify와 Server mutation은 presentation implementation을 직접 수행하지 않는다.

권장 흐름:

```text
OnRep_LoadedMask
→ shared Revolver AnimInstance는 property를 자체 read
→ 필요한 component에 semantic notification only

OnRep_CylinderOpen
→ FP.HandleCylinderChanged
→ TP.HandleCylinderChanged

OnRep_FireSequence
→ TP.HandleFireOccurred(Fired)

OnRep_DryFireSequence
→ TP.HandleFireOccurred(DryFire)

OnRep_ReloadSequence
→ FP.HandleReloadStarted(ActiveReloadPosition)
→ TP.HandleReloadStarted(ActiveReloadPosition)

OnRep_RoundInsertSequence
→ FP.HandleRoundInserted
→ TP.HandleRoundInserted
```

Listen Server도 동일한 semantic dispatch 경로를 사용한다.

Gameplay 함수 내부에서 view를 직접 판별하지 않는다.

---

# 11. Public Fire result cleanup

현재 owner confirmation:

```text
ClientConfirmFire(RequestId, bool bAccepted, bool bDryFire)
```

는 의미가 분산되어 있다.

가능하면 다음 concrete public enum으로 정리한다.

```text
ELuxRevolverPublicFireResult
- Rejected
- DryFire
- Fired
```

Live / Blank / Rubber는 구분하지 않는다.

Chamber secrecy를 그대로 유지한다.

이는 generic result framework가 아니라 현재 Fire 결과의 정확한 모델이다.

---

# 12. Migration order

## Step 1 - Gameplay/R21 reload timing decouple

가장 먼저 한다.

- `R21RoundInsertStartSeconds[]` / `CommitSeconds[]`를 Gameplay Actor에서 제거
- production validation에서 R21 window 검사 제거
- `RoundInsertionDurationSeconds` 도입
- 현재 `ReloadPosition -> Chamber` gameplay mapping은 보존
- R21 per-position segment data를 presentation 쪽으로 이동

이 단계에서 visual behavior를 바꾸지 않는다.

## Step 2 - Shared Revolver visual cleanup

- `ULuxRevolverAnimInstance` / `ABP_LuxRevolver` 유지
- FP/TP drum rotation 확인
- FP/TP bullet visibility 확인
- Actor의 `RefreshBulletVisuals()`와 `R21BulletBones` 제거

## Step 3 - FP presentation extraction

- concrete FP component 추가
- FP asset resolve 이동
- FP R21 knowledge 이동
- FOV/Aim visual 이동
- local Fire prediction/reconciliation 이동
- FP Fire/Reload/Cylinder/SFX/VFX 이동

## Step 4 - TP presentation extraction

- concrete TP component 추가
- TP Weapon action 이동
- TP Body Fire/Reload action orchestration 이동
- TP SFX/VFX/Cylinder 이동
- JIP presentation baseline 이동
- TP muzzle visual transform implementation 이동

`ULuxCharacterAnimInstance`의 persistent pose/Aim IK는 유지한다.

## Step 5 - Character cleanup

- R21 asset binding 제거
- FP/TP montage helper 제거
- Revolver FOV tick 제거
- debug number-key reload driver 제거
- TP upper-body activation에 actual EquippedRevolver condition 추가

## Step 6 - ALuxRevolver cleanup

- 모든 direct Presentation call 제거
- presentation asset/timer/state 제거
- RepNotify를 semantic dispatch로 제한

## Step 7 - full regression

- 01-E/F
- Third-Person Aim IK
- Manual Chamber Reload
- 50/100ms
- JIP
- Chamber secrecy

통과 후 01-G로 이동한다.

---

# 13. Do not create

이번 rework에서 만들지 않는다.

- generic Weapon Presentation base framework
- generic Weapon interface hierarchy
- Rifle / Shotgun extension
- Presentation factory
- generic weapon DataAsset configuration layer
- generic network prediction framework
- generic event bus
- animation service locator
- presentation manager

현재 실제 객체만 만든다.

```text
ULuxRevolverFirstPersonPresentationComponent
ULuxRevolverThirdPersonPresentationComponent
ULuxRevolverAnimInstance  [existing, keep]
```

---

# 14. Required invariants

## Chamber secrecy

```text
Live / Blank / Rubber exact type
= Server only
```

FP prediction / TP presentation / shared AnimInstance는 exact type을 알지 않는다.

## Server authority

- Fire resolution = Server
- Chamber advance = Server
- Cylinder truth = Server
- Round insertion commit = Server
- Death = Server

Presentation은 authoritative state를 수정하지 않는다.

## Manual Reload semantics

- Reload Position 1은 항상 next-fire Chamber 기준
- Position 1~6 mapping 유지
- occupied target insertion reject 유지
- `ActiveReloadPosition` remote presentation 유지
- final gameplay validity가 R21 animation availability에 의존하지 않음

## Current visual behavior

다음을 유지한다.

- ADS
- Hip/Aim Fire
- 1P/3P muzzle flash
- Dry Fire
- Cylinder Open/Close
- Position 1~6 manual single insertion
- Open pose return
- Drum step on loaded/dry fire
- Bullet occupancy visual
- TP upper-body / Head Look / Aim IK
- SFX timing

---

# 15. Required QA

## Static architecture check

### `ALuxRevolver.cpp`에서 없어야 함

```text
/Game/RevolverFPGM
R21RoundInsertStartSeconds
R21RoundInsertCommitSeconds
TryGetR21RoundInsertWindow
R21BulletBones
PlayFirePresentation
PlayThirdPersonFirePresentation
PlayReloadPresentation
PlayThirdPersonReloadPresentation
PlayCylinderPresentation
SpawnMuzzleFlashFor
RefreshBulletVisuals
```

R21 이름/asset knowledge가 Gameplay class에 남지 않는다.

### `ALuxCharacter.cpp`에서 없어야 함

```text
/Game/RevolverFPGM
PlayFirstPersonMontage
PlayThirdPersonUpperBodyAnimation
Revolver-specific FOV presentation Tick
EKeys::One ... EKeys::Six debug round injection
LuxLoadRound("Live", ...)
```

## Build

- UE 5.8 Development Editor build
- Map Check
- project-owned AnimBP compile

## 2/3 Player regression

- Host/Client Fire
- Live / Blank / Rubber / Empty
- 1P / 3P presentation exactly once
- ADS rapid input
- Cylinder Open / Close
- Position 1~6 insertion
- occupied insertion reject
- Fire -> chamber/drum step
- Empty Dry Fire -> chamber/drum step
- Reload cancel
- death during aim/reload

## Third-Person Aim

- horizontal
- high/low pitch
- 1m near obstruction
- remote observer consistency
- no revolver equipped -> Revolver UpperBody/Aim weight returns to 0

## Network

- 50ms
- 100ms
- JIP old events not replayed
- Chamber secrecy preserved

---

# 16. Codex completion report

작업 후 다음을 보고하고 멈춘다.

1. Changed Files
2. Final Responsibility Map
3. ALuxRevolver Remaining Responsibilities
4. Removed R21 Knowledge from Gameplay
5. Manual Reload Gameplay Timing Before / After
6. `ULuxRevolverAnimInstance` Final Role
7. FP Component Role
8. TP Component Role
9. Character Cleanup
10. Tests Run
11. 50/100ms Result
12. Manual Position 1~6 Result
13. TP Aim IK Regression
14. JIP Result
15. Chamber Secrecy Result
16. Known Issues
17. Ready for Review

검수 전 01-G를 시작하지 않는다.
