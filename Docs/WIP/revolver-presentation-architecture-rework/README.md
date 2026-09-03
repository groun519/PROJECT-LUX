# Phase 01 Revolver Presentation Architecture Rework

> Status: **Implementation-ready architecture correction**  
> Reviewed implementation: `main@f5e7beee675f2aeb0518824e969e51accaafa64e`  
> Scope: Phase 01-E / 01-F / Presentation Corrections  
> Preserve: Phase 01-B Chamber model, 01-C Server Fire / Death authority, 01-D production reload API  
> Goal: 현재 동작을 유지하면서 Gameplay 객체와 FP / TP Presentation의 책임 경계를 다시 세운다.

---

# 0. 결론

현재 구현은 기능적으로는 많이 보정되었다.

특히 `7d415bac` 이후 다음 문제는 이미 해결되었다.

- Owner Aim의 지연 복제 되감기
- R21 `"Aim"` 문자열 Reflection
- Fire local prediction의 cadence / confirmation
- Fire / Reload hot path의 반복 `LoadSynchronous()`
- Cylinder pose 및 Muzzle 위치 검증
- 1P / 3P presentation 기본 동기화

따라서 이전 01-E 버그 수정안을 다시 적용하지 않는다.

그러나 최신 `main@f5e7beee` 기준으로는 **구조 문제가 남아 있다.**

핵심 문제는 버그가 아니라 책임 소유권이다.

현재 `ALuxRevolver`가 동시에 다음을 소유한다.

```text
실제 Revolver gameplay truth
+ Server RPC / validation
+ Chamber / reload state
+ Local fire prediction
+ FP Weapon presentation
+ TP Weapon presentation
+ R21 asset paths
+ FP / TP montage selection
+ SFX
+ Niagara
+ Muzzle anchors
+ Bullet bone visibility
+ Cylinder pose timers
+ JIP cosmetic suppression
```

`LuxRevolver.cpp`는 현재 약 1,300 line이며, line count 자체가 문제가 아니라 **서로 다른 실제 기능 단위가 하나의 객체에 합쳐진 결과**라는 점이 문제다.

또 `ALuxCharacter`에도 다음 presentation 구현이 남아 있다.

```text
R21 FP Arms asset binding
FP montage playback
TP upper-body Dynamic Montage playback
FP FOV interpolation
typed FP AnimInstance driving
presentation stop
```

즉 현재는:

```text
Gameplay Character
       ↕
Gameplay Revolver
       ↕
FP Presentation details
       ↕
TP Presentation details
       ↕
R21 vendor details
```

가 서로 교차한다.

이는 현재 동작이 맞더라도 후속 기능이 추가될수록 구조가 다시 무너질 가능성이 높다.

---

# 1. Architecture 기준

이번 correction은 코드량을 줄이는 작업이 아니다.

우선순위는 다음이다.

## Decision - 정확한 객체화

실제로 서로 다른 기능은 서로 다른 객체가 소유한다.

현재 이미 실제 사용처가 두 개 존재한다.

```text
First Person Revolver Presentation
Third Person Revolver Presentation
```

따라서 두 Presentation을 별도 concrete 기능 단위로 분리하는 것은 미래를 위한 추상화가 아니다.

이미 존재하는 현재 요구를 정확하게 모델링하는 것이다.

## Decision - 작은 진입점

외부 gameplay 호출은 계속 다음 수준을 유지한다.

```text
Revolver->RequestFire()
Revolver->RequestOpenCylinder()
Revolver->RequestCloseCylinder()
Revolver->RequestCancelReload()
Revolver->BeginRoundInsertion(...)
```

호출자가 Montage, R21, Niagara, Socket, FP / TP 여부를 알지 않는다.

## Decision - Presentation 내부 완결

FP Presentation은 다음을 스스로 닫는다.

- Local Aim visual
- FOV
- FP Arms / Weapon animation
- Local Fire prediction cosmetic
- Server confirmation reconciliation
- FP Cylinder / Reload
- FP SFX / VFX
- FP bullet occupancy visual
- R21 FP-specific asset knowledge

TP Presentation은 다음을 스스로 닫는다.

- Remote Weapon visual
- Remote Body upper-body action
- Fire / Reload / Cylinder presentation
- TP SFX / VFX
- TP bullet occupancy visual
- R21 / Retargeted TP asset knowledge

## Decision - 확정된 미래

Revolver Actor를 독립 entity로 유지한다.

Drop / Transfer가 들어와도 Chamber truth와 Revolver entity가 Character 내부로 다시 들어가지 않게 한다.

반대로 아직 존재하지 않는 Rifle / Shotgun / generic Weapon Framework는 만들지 않는다.

---

# 2. 현재 구조에서 확인된 문제

## Finding A-01 - ALuxRevolver가 Gameplay와 Presentation을 동시에 소유

### Severity

**Critical Architecture**

### Fact

현재 `ALuxRevolver`에는 다음 gameplay 책임이 있다.

- Server-only `ChamberRoundTypes`
- `LoadedMask`
- `CurrentChamberIndex`
- Cylinder state
- Server Fire RPC
- Fire cadence validation
- Server trace
- Live / Blank / Rubber / Empty resolution
- Reload insertion validation
- Chamber consumption
- authoritative timers

동시에 다음 presentation 책임도 있다.

- `FirstPersonWeaponMesh`
- `ThirdPersonWeaponMesh`
- 1P / 3P Muzzle Anchor
- R21 mesh / AnimBP asset path
- all FP / TP fire montage references
- TP retargeted animation references
- Niagara
- Dry Fire / Cylinder sound
- all resolved presentation assets
- FP / TP muzzle scale / lifetime
- Cylinder Open pose
- bullet bone hide / unhide
- FP / TP Play / Stop functions

그리고 local prediction presentation까지 같은 class에 있다.

- `ELocalFirePrediction`
- `LocalFirePredictions`
- `LastLocalFirePresentationTimeSeconds`
- Owner presentation counters
- `PredictLocalFire()`
- `ConfirmLocalFire()`

### Problem

`ALuxRevolver`라는 객체가 더 이상 "리볼버의 상태와 행동"만 의미하지 않는다.

"이 리볼버를 어떤 화면에서 어떻게 보여주는가"까지 함께 의미한다.

이 상태에서 후속 presentation 변경은 gameplay actor의 수정으로 이어지고, 반대로 gameplay 변경도 presentation 구현에 쉽게 침투한다.

---

## Finding A-02 - Gameplay mutation 함수가 직접 Presentation을 실행

### Severity

**Critical Architecture**

### Fact

현재 다음 authoritative / state mutation 함수가 직접 presentation을 실행한다.

```text
ResolveServerFire()
→ PlayThirdPersonFirePresentation()

BeginRoundInsertion()
→ PlayReloadPresentation()
→ PlayThirdPersonReloadPresentation()

CommitPendingRoundInsertion()
→ PlayRoundInsertPresentation()
→ PlayThirdPersonRoundInsertPresentation()

TryOpenCylinder()
→ PlayCylinderPresentation()
→ PlayThirdPersonCylinderPresentation()

TryCloseCylinder()
→ presentation

TryCancelReload()
→ presentation
```

RepNotify도 직접 presentation을 실행한다.

```text
OnRep_FireSequence
OnRep_DryFireSequence
OnRep_ReloadSequence
OnRep_RoundInsertSequence
OnRep_CylinderOpen
```

### Problem

Server gameplay truth를 변경하는 함수가 "어떻게 보일 것인가"를 동시에 결정한다.

목표는:

```text
Gameplay state/event 확정
        ↓
Semantic notification
        ↓
Presentation unit가 표현
```

이다.

---

## Finding A-03 - Reload gameplay timing이 R21 Montage timing에 종속

### Severity

**Critical Architecture**

### Fact

`ALuxRevolver`가 다음 R21 presentation timing을 직접 가지고 있다.

```text
ReloadPresentationPlayRate
RoundInsertMontageStartSeconds = 2.2
RoundInsertMontageCommitSeconds = 2.88
RoundInsertSettleDelaySeconds
CylinderOpenPosePositionSeconds = 0.8
```

그리고 `BeginRoundInsertion()`의 authoritative commit delay가 다음 계산을 사용한다.

```text
(RoundInsertMontageCommitSeconds - RoundInsertMontageStartSeconds)
/
ReloadPresentationPlayRate
```

### Problem

Gameplay의 "한 발 삽입이 언제 확정되는가"가 Vendor animation asset의 특정 second marker를 알고 있다.

R21 montage를 교체하거나 retime하면 gameplay timing도 같이 변경된다.

### Decision

Gameplay은 presentation asset이 아니라 semantic duration을 소유한다.

예:

```text
RoundInsertionDurationSeconds
```

현재 체감을 유지하기 위해 초기값은 기존 계산 결과와 동일하게 맞춘다.

Presentation 쪽은:

```text
R21 Start Marker
R21 Insert Marker
Open Pose Marker
```

를 소유하고, Gameplay duration에 맞게 visual play rate / start position을 계산한다.

Gameplay timer는 R21 asset을 모른다.

---

## Finding A-04 - Character에 Revolver Presentation 구현이 노출

### Severity

**High Architecture**

### Fact

현재 `ALuxCharacter`가 다음 함수를 직접 제공한다.

```text
PlayFirstPersonMontage(...)
PlayThirdPersonUpperBodyAnimation(...)
StopFirstPersonMontages(...)
StopThirdPersonUpperBodyAnimation(...)
UpdateFirstPersonAimAnimation()
UpdateFirstPersonPresentation()
```

또 constructor가 직접 R21 FP Arms Mesh와 project FP AnimBP asset을 설정한다.

### Problem

Character가 다음을 알아야 한다.

- R21 Arms
- FP AnimBP
- Montage playback method
- TP slot animation method
- FOV animation
- Revolver presentation stop semantics

`ALuxCharacter`는 몸 / 입력 / 생존 / 장착 관계와 semantic Aim state를 소유하면 충분하다.

Revolver presentation 구현법은 알 필요가 없다.

---

## Finding A-05 - Vendor knowledge가 Core Gameplay class에 존재

### Severity

**High Architecture**

### Fact

현재 `ALuxRevolver.cpp`에는 다음 vendor-specific knowledge가 직접 존재한다.

```text
"38"
"hand_r"
"Slide_1"
"Bullet_1" ... "Bullet_6"
/Game/RevolverFPGM/...
R21 grip offsets
R21 muzzle bone offsets
R21 montage seconds
```

`ALuxCharacter.cpp`에도 R21 FP Arms asset path가 존재한다.

### Requirement

R21-specific path / bone / marker / pose knowledge는 concrete presentation unit 안으로 이동한다.

---

## Finding A-06 - TP AnimInstance가 "alive = revolver equipped"를 가정

### Severity

**High / Confirmed-future correctness**

### Fact

현재:

```cpp
const bool bUseRevolverUpperBody = Character && !Character->IsDead();
```

즉 살아 있는 Character는 Revolver를 실제로 들고 있는지와 무관하게 Revolver upper body weight가 1로 수렴한다.

### Requirement

최소 조건:

```text
Character valid
AND alive
AND EquippedRevolver exists
```

Presentation state가 실제 장착 관계를 따라야 한다.

실제 Drop system은 이번 작업에서 구현하지 않는다.

---

## Finding A-07 - Presentation lifecycle state가 Core Actor에 섞임

### Severity

**Medium Architecture**

현재 다음이 `ALuxRevolver`에 있다.

```text
bPresentationReplicationReady
CylinderOpenPoseTimer
OwnerFirePresentationCount
OwnerDryFirePresentationCount
```

### Decision

- JIP cosmetic baseline은 presentation component가 소유한다.
- Cylinder pose timer는 presentation component가 소유한다.
- presentation dev counters도 presentation component가 소유한다.
- Server gameplay fire / reload timer는 `ALuxRevolver`에 남는다.

---

# 3. Target object model

```text
ALuxCharacter
├─ body / movement / input
├─ death state
├─ semantic Aim state
├─ EquippedRevolver
├─ FirstPersonCamera      [view anchor]
└─ FirstPersonArms        [view anchor]

ALuxRevolver
├─ Chamber truth
├─ public replicated state
├─ Fire / Reload / Cylinder requests
├─ Server validation / trace / resolution
├─ Round insertion gameplay timing
├─ request / confirmation network transport
├─ ULuxRevolverFirstPersonPresentationComponent
└─ ULuxRevolverThirdPersonPresentationComponent

ULuxRevolverFirstPersonPresentationComponent
├─ local-only presentation
├─ R21 FP asset knowledge
├─ FP Weapon visual
├─ FOV / ADS visual
├─ typed ULuxFirstPersonAnimInstance driving
├─ local Fire prediction / reconciliation
├─ FP Reload / Cylinder
├─ FP SFX / Niagara
└─ FP bullet visual

ULuxRevolverThirdPersonPresentationComponent
├─ remote-only presentation
├─ R21 / retargeted TP asset knowledge
├─ TP Weapon visual
├─ Body upper-body Fire / Reload action
├─ TP Cylinder
├─ TP SFX / Niagara
└─ TP bullet visual

ULuxFirstPersonAnimInstance
= project-owned typed FP animation adapter

ULuxCharacterAnimInstance
= locomotion + persistent TP body pose / aim / head look
```

---

# 4. Ownership rules

## ALuxCharacter

### Own

- Character movement
- input
- `bIsDead`
- authoritative `bIsAiming`
- owner-local Aim intent
- camera / arms view anchors
- `EquippedRevolver` relationship

### Do not own

- R21 asset path
- Revolver montage selection
- Revolver Fire / Reload animation execution
- Revolver SFX / VFX
- Revolver bullet bone visual
- TP weapon mesh behavior
- R21 montage timing

Character는 presentation이 사용할 안정된 anchor만 제공할 수 있다.

필요하면 다음 getter는 허용한다.

```text
GetFirstPersonCamera()
GetFirstPersonArms()
GetMesh()
```

하지만 `PlayFirstPersonMontage` 같은 구현 helper는 제거한다.

---

## ALuxRevolver

### Own

- exact Chamber truth
- public Chamber occupancy
- current chamber
- cylinder gameplay state
- round insertion pending state
- request APIs
- RPC
- server validation
- hitscan
- lethal / non-lethal resolution
- chamber consume / advance
- replicated semantic sequences
- gameplay insertion duration
- local Fire request id transport

### Compose

```text
FirstPersonPresentation
ThirdPersonPresentation
```

Actor가 두 component의 lifetime과 wiring을 소유하는 것은 허용한다.

Actor가 R21 montage / sound / Niagara implementation을 소유하는 것은 허용하지 않는다.

### Stable external API

기존 API를 유지한다.

```text
RequestFire()
RequestOpenCylinder()
RequestCloseCylinder()
RequestCancelReload()
BeginRoundInsertion(...)
```

---

# 5. Presentation components

## 5.1 ULuxRevolverFirstPersonPresentationComponent

새 concrete component.

Generic Weapon Presentation base class를 만들지 않는다.

### Own

- owner-local relevance 판단
- FP R21 asset resolve
- FP Weapon mesh configuration
- FP muzzle anchor
- Aim FOV interpolation
- `ULuxFirstPersonAnimInstance` Aim update
- Arms / Weapon Fire montage
- Arms / Weapon Reload montage
- Cylinder open pose
- Dry Fire
- FP muzzle flash
- FP mechanical SFX
- LoadedMask -> FP bullet bone visual
- local Fire prediction state
- request id -> prediction map
- authoritative confirmation reconciliation
- local presentation dev counters
- local presentation timer

### Semantic entry points

권장 의미:

```text
BindOwner(...)
HandleLoadedMaskChanged(...)
HandleCylinderChanged(...)
PredictFire(...)
ConfirmFire(...)
HandleReloadStarted(...)
HandleRoundInserted(...)
HandleOwnerDeath()
Stop()
```

정확한 함수명은 프로젝트 naming에 맞춰 조정해도 된다.

---

## 5.2 ULuxRevolverThirdPersonPresentationComponent

새 concrete component.

### Own

- remote relevance 판단
- TP Revolver mesh configuration
- R21 weapon montage
- retargeted Casual01 Fire / Reload sequences
- Dynamic Montage body action
- Cylinder presentation
- remote Dry Fire / mechanical SFX
- TP muzzle flash
- LoadedMask -> TP bullet visual
- JIP initial sequence suppression / baseline
- TP presentation timers
- stop / death cleanup

### Semantic entry points

권장 의미:

```text
BindOwner(...)
HandleLoadedMaskChanged(...)
HandleCylinderChanged(...)
HandleFireOccurred(...)
HandleReloadStarted(...)
HandleRoundInserted(...)
HandleOwnerDeath()
MarkReplicationReady()
Stop()
```

---

# 6. Fire flow after rework

## Owner Client

```text
ALuxCharacter::Fire
        ↓
ALuxRevolver::RequestFire
        ↓
allocate RequestId
        ↓
FP Presentation.PredictFire(...)
        ↓
ServerFire(RequestId)
```

FP component는 public state만 사용한다.

- LoadedMask
- CurrentChamberIndex
- Cylinder
- local cadence
- alive / equipped relation

정확한 Live / Blank / Rubber를 알지 않는다.

## Server

```text
ServerFire
↓
ResolveServerFire
↓
authoritative state mutation
↓
FireSequence / DryFireSequence
↓
ForceNetUpdate
↓
ClientConfirmFire(RequestId, PublicResult)
```

`ResolveServerFire()` 안에서 Montage / Niagara / Sound를 호출하지 않는다.

Listen Server가 remote player의 Fire를 표시해야 하는 경우에도 동일한 semantic event를 TP component에 전달한다.

## Owner confirmation

현재 두 bool 대신 가능하면 concrete public result를 사용한다.

```text
ELuxRevolverPublicFireResult
- Rejected
- DryFire
- Fired
```

이 enum은 Live / Blank / Rubber를 구분하지 않는다.

FP component가:

```text
ConfirmFire(RequestId, PublicResult)
```

에서 prediction을 reconcile한다.

---

# 7. Replication event flow

`OnRep_*`는 presentation implementation을 직접 수행하지 않는다.

역할은 semantic dispatch다.

```text
OnRep_LoadedMask
→ FP.HandleLoadedMaskChanged
→ TP.HandleLoadedMaskChanged

OnRep_CylinderOpen
→ FP.HandleCylinderChanged
→ TP.HandleCylinderChanged

OnRep_FireSequence
→ TP.HandleFireOccurred(Fired)

OnRep_DryFireSequence
→ TP.HandleFireOccurred(DryFire)

OnRep_ReloadSequence
→ FP / TP HandleReloadStarted

OnRep_RoundInsertSequence
→ FP / TP HandleRoundInserted
```

각 concrete component가 자신이 현재 view에서 활성인지 판단한다.

`ALuxRevolver`에서 FP / TP presentation policy를 중앙 분기하지 않는다.

---

# 8. Reload timing separation

## Current

```text
Gameplay commit timing
= R21 montage second markers
```

## Target

```text
ALuxRevolver
RoundInsertionDurationSeconds
= gameplay fact
```

최초 migration에서는 현재 동작을 유지한다.

```text
(2.88 - 2.20) / 1.0
= 0.68 sec
```

FP / TP component가 각각 vendor marker를 소유한다.

```text
MontageStartSeconds = 2.20
MontageInsertMarkerSeconds = 2.88
OpenPoseSeconds = 0.80
SettleDelay
```

Presentation은 visual segment가 gameplay duration에 맞도록 play rate를 계산할 수 있다.

Server timer는 R21 marker를 알지 않는다.

---

# 9. Character cleanup

Rework 이후 `ALuxCharacter`에서 제거 대상:

```text
PlayFirstPersonMontage()
PlayThirdPersonUpperBodyAnimation()
StopFirstPersonMontages()
StopThirdPersonUpperBodyAnimation()
UpdateFirstPersonAimAnimation()
UpdateFirstPersonPresentation()
R21 FP Arms asset path
R21 FP AnimBP asset path
Revolver-specific FOV interpolation Tick
```

`FirstPersonCamera`와 `FirstPersonArms`는 view anchor로 남겨도 된다.

R21 mesh / AnimBP assignment와 Revolver-specific control은 FP presentation component가 담당한다.

Character 자체에 다른 Tick 이유가 없으면 `PrimaryActorTick.bCanEverTick = false`로 되돌리고, FOV interpolation은 local FP component tick으로 이동한다.

---

# 10. Third-person persistent pose

`ULuxCharacterAnimInstance` 자체는 유지한다.

Locomotion / persistent body pose를 처리하는 것은 적절하다.

단 다음 조건을 수정한다.

## Current

```text
alive
→ Revolver upper body
```

## Target

```text
alive
AND EquippedRevolver != null
→ Revolver upper body
```

Aim / Head Look은 Character semantic state를 읽는다.

Fire / Reload action은 TP presentation component가 body AnimInstance에 실행한다.

---

# 11. View mesh ownership

Unreal subobject 수명 때문에 구현을 억지로 복잡하게 만들지 않는다.

`ALuxRevolver` constructor가 다음 Scene subobject를 생성하는 것은 허용한다.

```text
FirstPersonWeaponMesh
ThirdPersonWeaponMesh
FirstPersonMuzzleAnchor
ThirdPersonMuzzleAnchor
```

이는 **composition / lifetime wiring**으로 취급한다.

단:

- gameplay method가 mesh를 animate하지 않는다.
- gameplay method가 bullet bone을 hide / unhide하지 않는다.
- gameplay class가 R21 asset path를 설정하지 않는다.
- gameplay class가 VFX를 spawn하지 않는다.
- concrete presentation component가 해당 view resource를 운영한다.

Codex가 Unreal 수명 안전성을 확인한 뒤 component 자체가 view resource를 안전하게 소유할 수 있다면 그 방식도 가능하다.

핵심은 행동 책임의 소유자다.

---

# 12. Do not create

이번 rework에서 다음은 만들지 않는다.

- generic `WeaponPresentationComponent` framework
- Weapon interface hierarchy
- rifle / shotgun extension point
- Presentation factory
- generic weapon Data Asset layer
- generic network prediction framework
- generic gameplay event bus
- animation service locator
- 모든 weapon presentation을 소유하는 Manager

현재 실제 두 기능만 만든다.

```text
ULuxRevolverFirstPersonPresentationComponent
ULuxRevolverThirdPersonPresentationComponent
```

두 concrete component를 만든 뒤 실제 중복이 확인된 경우에만 작은 private helper를 추출할 수 있다.

---

# 13. Migration order

## Step 1 - FP component extraction

- concrete FP component 추가
- FP asset resolve 이동
- FOV / typed FP AnimInstance control 이동
- local Fire prediction / reconciliation 이동
- FP Fire / Reload / Cylinder / VFX / SFX 이동
- FP bullet visual 이동
- 기존 01-E regression 확인

TP는 이 단계에서 기존 구현을 유지해도 된다.

## Step 2 - TP component extraction

- concrete TP component 추가
- remote Weapon / Body action 이동
- TP asset resolve 이동
- TP Fire / Reload / Cylinder / SFX / VFX 이동
- TP bullet visual 이동
- JIP cosmetic baseline 이동

## Step 3 - Gameplay actor cleanup

- `ALuxRevolver`의 모든 `Play*Presentation` 제거
- presentation asset refs 제거
- R21 constants 제거
- presentation timer / counters 제거
- mutation function의 direct presentation call 제거
- RepNotify는 semantic dispatch만 수행

## Step 4 - Reload timing decouple

- `RoundInsertionDurationSeconds` 도입
- Server commit timer를 semantic duration 기준으로 변경
- R21 marker는 presentation component로 이동
- 초기 behavior는 약 0.68 sec 유지

## Step 5 - Character cleanup

- presentation playback helper 제거
- R21 asset binding 제거
- FOV Tick 이동
- Character Tick 불필요 시 제거
- TP AnimInstance의 EquippedRevolver 조건 수정

## Step 6 - final regression

기존 01-E / 01-F correction scenario를 다시 실행한다.

---

# 14. 반드시 보존할 invariants

## Chamber secrecy

```text
Live / Blank / Rubber exact type
= Server only
```

FP prediction은 Loaded / Empty만 안다.

TP presentation도 exact type을 알지 않는다.

## Server authority

- Chamber consume = Server
- Hitscan = Server
- Death = Server
- insertion commit = Server
- Cylinder truth = Server

Presentation component가 authoritative gameplay state를 수정하지 않는다.

## Existing gameplay entrypoint

Character input은 기존 Revolver production API를 유지한다.

## Existing visual result

현재 통과한 다음 동작을 유지한다.

- ADS
- Hip / Aim Fire
- Cylinder Open
- one-round Insert
- Open pose hold
- bullet occupancy visual
- 1P / 3P muzzle flash
- SFX
- TP upper-body
- Head / Neck Look

---

# 15. Required QA

## Static architecture review

완료 후 검색한다.

### ALuxRevolver에 없어야 함

- `PlayFirePresentation`
- `PlayThirdPersonFirePresentation`
- `PlayReloadPresentation`
- `PlayThirdPersonReloadPresentation`
- `PlayCylinderPresentation`
- `SpawnMuzzleFlashFor`
- R21 montage / sound refs
- R21 bullet bone list
- local prediction map
- cylinder presentation timer

### ALuxCharacter에 없어야 함

- Revolver montage playback helper
- R21 asset paths
- Revolver-specific FOV Tick
- TP Revolver action playback helper

## Build

- UE 5.8 Development Editor
- Map Check
- UHT / Blueprint compile

## FP regression

- ADS / rapid ADS
- Hip Fire / Aim Fire
- Dry Fire
- Fire spam
- Cylinder Open / Close
- Insert / Cancel
- death during Aim
- death during Reload

## TP regression

- Host observes Client
- Client observes Host
- Client A observes Client B
- move + aim / fire / reload
- Cylinder Open / Close
- death cleanup
- Head Look

## Network

- 3 Player PIE
- 50ms / 100ms
- Chamber secrecy
- owner Fire exactly once
- JIP does not replay old events

## Confirmed-future sanity

Development validation:

```text
Character with EquippedRevolver == null
→ RevolverUpperBodyWeight converges to 0
```

실제 Drop system은 구현하지 않는다.

---

# 16. Completion report

Codex는 작업 후 다음 형식으로 보고하고 멈춘다.

1. Changed Files
2. Final Responsibility Map
3. Removed Responsibilities from ALuxRevolver
4. Removed Responsibilities from ALuxCharacter
5. FP Component Responsibilities
6. TP Component Responsibilities
7. Reload Gameplay Timing Before / After
8. R21 Vendor Knowledge Location
9. Tests Run
10. 50 / 100ms Result
11. Chamber Secrecy Regression
12. JIP Regression
13. Known Issues
14. Ready for Review

검수 전 01-G를 시작하지 않는다.
