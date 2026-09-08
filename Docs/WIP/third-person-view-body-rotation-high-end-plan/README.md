# Third-Person View / Body Rotation High-End Recovery Plan

> Status: **Implementation-ready after axis contract is proven**  
> Stable baseline: `main@0275e299b9400488e7770f322f285f3b1e82792c`  
> Local experiment basis: `LocalChangesReview_92935637.md`  
> Companion diagnosis: `../third-person-view-body-rotation-review/README.md`  
> Scope: idle view/body separation, natural neck/head/spine response, body catch-up, Turn-in-Place, revolver muzzle alignment, remote presentation  
> Goal: 현재 가능한 범위에서 기계적인 bone rotation이 아니라 실제 사람이 시선을 먼저 돌리고 몸이 문맥에 따라 따라오는 수준의 하이엔드 third-person view/body presentation을 만든다.

---

# 0. Reading rule

이 문서는 기존 failure review를 대체하지 않는다.

- failure review는 현재 broken experiment의 원인 경계와 복구 gate를 정의한다.
- 이 문서는 그 gate가 통과된 뒤 도달할 **최종 구조와 구현 순서**를 정의한다.
- root axis / coordinate-space 계약이 증명되기 전에는 뒤 단계의 smoothing, FBIK, Turn-in-Place를 production graph에 한꺼번에 넣지 않는다.
- 현재 local diff는 안정 구현이 아니다.
- `Config/DefaultEditor.ini`는 이번 기능 범위가 아니므로 stage / commit하지 않는다.

---

# 1. Target player-facing result

최종 결과는 다음 순서로 보여야 한다.

```text
View moves first
    ↓
Head reacts immediately
    ↓
Neck shares the remaining angle
    ↓
Upper spine progressively joins
    ↓
When the angle grows, the visual body begins to catch up
    ↓
At large idle turns, an actual Turn-in-Place pose rotates the body with planted feet
    ↓
The revolver arm solver corrects only the remaining muzzle error
```

## Requirement - Relaxed look

정지 상태에서 작은 좌우 시선 변화는 몸 전체가 즉시 따라오지 않는다.

- 작은 yaw: head / neck가 우선 반응
- 중간 yaw: upper spine이 점진적으로 참여
- 큰 yaw: visual body가 자연스럽게 따라오기 시작
- 매우 큰 yaw: Turn-in-Place로 몸을 재정렬

## Requirement - ADS

ADS에서는 relaxed look보다 몸통과 어깨가 더 적극적으로 view에 정렬된다.

```text
Relaxed
Head / Neck lead
Spine secondary
Body follows late

ADS
Chest / Shoulder participate earlier
Head / Neck keep anatomical remainder
Body follows at a smaller view-body gap
```

## Requirement - Weapon alignment

총구 정렬은 손목 하나가 전체 오차를 해결하지 않는다.

```text
Body orientation
+ Spine / clavicle distribution
+ Arm chain
+ Final wrist correction
= Muzzle target alignment
```

기존 `HandToMuzzle` 역산 아이디어는 유지할 수 있지만, 최종 hand rotation을 바로 덮어쓰는 것이 아니라 upper-body solver의 hand target으로 사용한다.

## Requirement - Locomotion

- Forward / Backward / Left / Right Blend Space를 유지한다.
- 움직인다고 몸을 즉시 camera yaw로 snap하지 않는다.
- strafe / backward pose를 살린다.
- 이동 시작 시 body alignment는 더 적극적으로 진행하되 짧은 관성과 transition을 유지한다.
- 큰 방향 변화에서도 발이 미끄러지거나 골반이 순간 회전하지 않아야 한다.

---

# 2. Current local diff findings that must be corrected

## Finding H-01 - Fixed additive distribution can exceed the intended view angle

현재 local code는 다음과 같이 분배한다.

```text
Spine01 = 15%
Spine02 = 20%
Spine03 = 25%
Head + Neck = 100% when not aiming
```

non-ADS에서 parent-child additive rotation이 실제로 모두 누적되면 총합이 최대 약 160%가 될 수 있다.

### Decision

최종 구현에서는 fixed percentage additive chain을 중심 구조로 사용하지 않는다.

- residual view/body delta를 하나의 semantic target으로 계산한다.
- Control Rig / FBIK가 per-bone stiffness와 limits를 이용해 chain 전체에 자연스럽게 분배한다.
- head / hand target의 solver alpha만 relaxed / ADS 문맥에 따라 조절한다.
- 개별 spine bone의 최종 각도를 C++에서 직접 합산하지 않는다.

---

## Finding H-02 - TurnRequested exists but body catch-up is not actually completed

현재 component는 idle offset이 threshold를 넘으면 `TurnRequested`를 만들지만, 그 상태 자체가 visual body를 목표 방향으로 실제 회전시키지는 않는다.

### Decision

`TurnRequested`는 단순 label이 아니라 실제 body target / turn action으로 이어져야 한다.

최종 flow:

```text
Free look range
→ Body stays

Soft follow range
→ Body target begins following view slowly

Turn threshold exceeded
→ Turn-in-Place request
→ Visual body rotates toward target using authored turn pose
→ Root yaw residual is consumed
→ Settle range reached
```

threshold에는 hysteresis를 둔다.

예:

```text
Request threshold: larger angle
Settle threshold: smaller angle
```

같은 경계에서 request / cancel이 반복되지 않게 한다.

정확한 수치는 Editor tuning으로 결정한다.

---

## Finding H-03 - Animation implementation state is mixed into the reusable rotation policy

현재 `ULuxViewBodyRotationComponent`는 다음 animation-specific state까지 소유한다.

```text
RootYawOffset
RootYawOffsetMode
TurnState
TurnDirection
```

특히 `RootYawOffsetMode`는 AnimGraph 구현 방식에 가까운 개념이다.

### Decision

`ULuxViewBodyRotationComponent`는 **view / visual body relationship policy**만 소유한다.

권장 semantic output:

```text
PresentationViewYaw
VisualBodyYaw
ViewBodyDeltaYaw
DesiredBodyYaw
BodyTurnIntent
BodyTurnDirection
BodyFollowAlpha
```

AnimInstance는 다음을 파생한다.

```text
RootYawOffset
Turn animation selection
Turn animation phase
Foot lock
Bone solver alpha
```

즉:

```text
ViewBodyRotationComponent
= what direction should the body face?

AnimInstance / Control Rig
= how does the skeleton reach that direction?
```

---

## Finding H-04 - Too many derived presentation values are replicated

현재 experiment는 다음을 각각 replicate한다.

```text
ReplicatedViewYaw
ReplicatedVisualBodyYaw
ReplicatedRootYawOffsetMode
ReplicatedTurnState
ReplicatedTurnDirection
```

### Decision

network는 가능한 한 canonical state만 전달한다.

우선 검증 순서:

1. simulated proxy에서 기존 `GetBaseAimRotation()`으로 필요한 view yaw를 얻을 수 있는지 측정한다.
2. 충분하면 custom view yaw replication을 만들지 않는다.
3. 부족하면 custom `PresentationViewYaw` 하나만 추가한다.
4. body presentation에 필요한 canonical state는 `VisualBodyYaw` 하나를 우선 사용한다.
5. `RootYawOffset`, mode, turn direction, turn state는 remote에서 derive한다.

Actual Turn-in-Place animation의 phase까지 여러 client에서 맞춰야 할 필요가 확인된 경우에만 작은 semantic event를 추가한다.

예:

```text
TurnSequence
SignedTurnAngle
```

bone transform이나 solver output은 replicate하지 않는다.

---

## Finding H-05 - Character manually drives a feature component every Tick

현재:

```text
ALuxCharacter::Tick
→ UpdateViewBodyRotation
→ ViewBodyRotationComponent::UpdateRotation
```

### Decision

`ULuxViewBodyRotationComponent`는 자신의 update cycle을 스스로 소유하는 방향을 우선한다.

- owner / authority에서만 필요한 tick을 활성화
- CharacterMovement 이후, animation evaluation 이전의 안정된 순서를 확보
- death / disable 같은 gameplay gate는 작은 semantic entrypoint로 전달

권장 외부 API 의미:

```text
SetRotationEnabled(bool)
SetAiming(bool)
GetPresentationState()
```

Character가 component 내부 policy를 매 frame 구현하지 않는다.

단, 실제 Unreal tick prerequisite가 오히려 구조를 복잡하게 만들면 Character tick orchestration을 유지해도 된다. 이 경우에도 Character는 raw algorithm을 소유하지 않고 component 호출만 수행한다.

---

## Finding H-06 - `GetPresentationAimRotation` naming is too weapon-oriented

이 값은 ADS 전용 Aim이 아니라 일반 view/body presentation에 사용된다.

### Decision

의미가 유지된다면 다음과 같은 이름을 우선 검토한다.

```text
GetPresentationViewRotation()
```

또는 component가 직접 presentation view rotation을 제공한다.

함수명만 읽고 weapon aim 전용 값으로 오해되지 않아야 한다.

---

## Finding H-07 - Revolver upper-body pose ignores actual equipment relationship

현재:

```text
alive
→ RevolverUpperBodyWeight = 1
```

### Requirement

최소 조건:

```text
Character valid
AND alive
AND EquippedRevolver exists
```

Drop / Transfer를 지금 구현하지는 않는다.

하지만 이미 Revolver를 독립 entity로 둔 현재 기반과 모순되는 `alive = revolver equipped` 가정은 제거한다.

---

## Finding H-08 - Current direct hand replacement is too rigid for final high-end solve

현재 `UpdateRightHandAim()`은 muzzle target에서 원하는 hand component-space rotation을 역산한다.

이 target 계산은 유용하다.

문제는 최종 회전을 hand 하나가 직접 해결하는 방식이다.

### Decision

기존 계산은 다음 역할로 축소한다.

```text
View target
→ Desired muzzle transform
→ Desired hand effector transform
```

그 뒤 Control Rig / FBIK가 다음 chain에 분배한다.

```text
spine
clavicle
upper arm
lower arm
hand
```

손목은 마지막 residual만 담당한다.

---

# 3. Target architecture

```text
ALuxCharacter
├─ movement / input
├─ death
├─ aiming semantic state
├─ EquippedRevolver
├─ camera
└─ ULuxViewBodyRotationComponent

ULuxViewBodyRotationComponent
├─ view/body separation policy
├─ relaxed vs aiming body-follow policy
├─ canonical VisualBodyYaw
├─ view-body delta
├─ turn request decision
└─ minimal network state

ULuxCharacterAnimInstance
├─ locomotion parameters
├─ body-turn animation state
├─ RootYawOffset derived from canonical state
├─ Turn-in-Place selection / phase
├─ turn curves / foot lock state
└─ Control Rig input state

CR_LuxUpperBodyAim
├─ head orientation target
├─ right-hand effector target
├─ upper-body FBIK
├─ per-bone stiffness / limits
└─ final procedural correction

ABP_LuxCharacter
├─ Locomotion / Turn-in-Place base
├─ optional moving orientation correction
├─ Revolver upper-body animation layer
├─ Inertialization
├─ verified root/body correction
├─ CR_LuxUpperBodyAim
├─ foot stabilization if required
└─ Output Pose
```

---

# 4. ULuxViewBodyRotationComponent final responsibility

## Own

- current presentation view yaw
- current visual body yaw
- desired visual body yaw
- view-body yaw delta
- relaxed / aiming follow policy
- idle free-look range
- body catch-up threshold
- body catch-up target
- movement body-follow behavior
- turn request / direction semantic
- canonical replicated presentation yaw if required

## Do not own

- root bone name
- pelvis bone name
- spine bone names
- R21
- revolver mesh
- montage
- Control Rig
- FBIK settings
- foot lock curves
- turn animation asset paths
- `RootYawOffsetMode`

## Body follow behavior

하드 threshold 하나로 body가 갑자기 바뀌지 않는다.

초기 tuning concept:

```text
Small delta
  body follow = 0

Medium delta
  body follow gradually rises

Large delta
  body follows strongly

Extreme delta
  explicit Turn-in-Place request
```

Relaxed와 ADS는 다른 curve / thresholds를 사용할 수 있다.

이것은 generic config framework가 아니라 실제 body-follow tuning data다.

필요한 경우 프로젝트 소유 `UCurveFloat` 1~2개로 표현한다.

---

# 5. High-end upper-body solver

## Decision - Control Rig + Full Body IK for the upper-body correction pass

최종 목표에서는 sequential `Transform (Modify) Bone`만으로 head/spine/hand를 수동 배분하지 않는다.

UE 5.8의 Control Rig Full Body IK는 multi-effector, per-bone stiffness, preferred angle, bone limit을 지원하므로 이 문제에 적합하다.

단, solver root는 full character pelvis가 아니라 **upper-body chain**으로 제한한다.

권장 시작점:

```text
FBIK Root
  spine_01

Effectors
  head      rotation-focused
  hand_r    position + rotation

Excluded / locked
  pelvis and legs
```

## Per-bone intent

정확한 값은 skeleton preview에서 조정한다.

의도:

```text
spine_01
  stiffest upper-body root

spine_02
  moderate contribution

spine_03 / chest
  larger contribution

clavicle_r
  participates in weapon alignment

upperarm_r / lowerarm_r
  preserve natural elbow bend

hand_r
  final muzzle alignment only

neck / head
  anatomical look limits
```

- stretch는 기본적으로 허용하지 않는다.
- elbow preferred angle을 지정해 inversion을 막는다.
- wrist twist / bend limit을 둔다.
- solver alpha는 animation pose를 완전히 덮지 않고 correction layer로 사용한다.

## Relaxed mode

- head effector alpha 높음
- hand effector alpha 낮거나 0
- spine contribution은 residual angle이 커질수록 증가

## ADS mode

- hand effector alpha 높음
- head target도 view와 정렬
- chest / clavicle가 더 적극적으로 참여
- wrist correction은 제한

---

# 6. Muzzle target pipeline

현재 `UpdateRightHandAim()`의 좋은 부분은 유지한다.

```text
Pawn view location
+ presentation view direction
→ world target
→ minimum convergence distance
→ maximum parallax clamp
→ desired muzzle transform
→ hand-to-muzzle inverse
→ desired hand effector transform
```

변경점은 마지막이다.

## Current

```text
Desired hand rotation
→ RightHandAimRotation
→ hand bone replacement
```

## Target

```text
Desired hand transform
→ CR_LuxUpperBodyAim Hand Effector
→ FBIK distributes correction across upper body / arm chain
```

### Requirement

- exact ballistic trace remains gameplay/server responsibility.
- this target is presentation alignment only.
- close-range convergence clamp remains presentation-only.
- presentation solver must never change server hit result.

---

# 7. Head / neck behavior

head / neck는 단순히 view yaw의 고정 비율이 아니다.

## Semantic residual

```text
ResidualViewYaw = ViewYaw - VisualBodyYaw
ResidualPitch   = ViewPitch relative to body pose
```

## Target behavior

- 작은 residual에서 head가 먼저 움직인다.
- neck는 head limit에 가까워질수록 더 참여한다.
- upper spine은 residual이 커질수록 참여한다.
- body catch-up이 진행되면 head / spine residual은 자연스럽게 감소한다.
- yaw와 pitch limit은 별도로 둔다.
- up / down limit도 필요하면 비대칭으로 조정한다.

Control Rig target은 한 번만 coordinate conversion하고 solver 내부에서 일관된 rig/global space를 사용한다.

C++에서 mesh `-90 degree` relative yaw를 여러 함수에서 각각 보정하지 않는다.

---

# 8. Turn-in-Place final quality target

하이엔드 최종 결과에는 큰 idle yaw에서 실제 Turn-in-Place가 필요하다.

## Preferred path

현재 보유 asset에서 먼저 다음을 탐색한다.

```text
Left 90
Right 90
Left 180
Right 180
```

사용 가능한 clip이 있으면 project-owned retargeted asset으로 가져온다.

없으면 Marketplace 원본을 수정하지 않고 project-owned turn clip을 별도로 만든다.

## Turn decision

대략적인 의미:

```text
moderate excess angle
→ 90 turn

very large excess angle
→ 180 turn
```

정확한 cutoff는 tuning한다.

## Curve-driven yaw consumption

Turn animation에는 project-owned yaw curve를 둔다.

```text
TurnYaw
```

animation이 실제로 body를 회전하는 진행량만큼 RootYawOffset residual을 소비한다.

목표:

- 발은 authored turn pose를 따른다.
- mesh가 한 프레임에 snap하지 않는다.
- turn animation과 root compensation이 서로 싸우지 않는다.

## Foot lock

Turn clip이 충분하지 않으면 다음 curve를 추가할 수 있다.

```text
FootLock_L
FootLock_R
```

필요할 때만 IK foot goal을 사용한다.

foot lock system을 generic locomotion framework로 확장하지 않는다.

## Transition

Turn-in-Place 진입 / 종료에는 Inertialization을 우선 검토한다.

목표는 짧은 transition에서 pose pop 없이 기존 locomotion으로 복귀하는 것이다.

---

# 9. Moving locomotion integration

## Requirement

움직임 시작은 idle body turn과 별도의 문맥이다.

```text
Idle
  view can lead body

Move
  body follows more aggressively
  directional Blend Space still expresses strafe/backpedal
```

## Candidate - Orientation Warping

UE 5.8 Pose Warping의 Orientation Warping은 이동 방향 차이를 pose에 보정하고 여러 spine bone에 twist를 분배할 수 있다.

하지만 현재 `BS_Lux_Locomotion` asset이 이 node의 graph-mode prerequisite와 잘 맞는지는 먼저 확인해야 한다.

따라서:

1. current Blend Space를 제거하지 않는다.
2. Orientation Warping을 별도 test graph에서 시험한다.
3. 실제 foot direction과 torso twist가 개선될 때만 production에 넣는다.
4. Control Rig upper-body solve와 spine twist가 중복되지 않도록 order / bone influence를 조정한다.

적합하지 않으면 기존 directional Blend Space + Turn-in-Place + foot stabilization을 유지한다.

Motion Matching으로 전체 locomotion을 갈아엎는 것은 이번 기능의 요구가 아니다.

---

# 10. Network model

## Principle

network는 skeleton 결과가 아니라 최소 semantic orientation만 전달한다.

## Owner

```text
Local view input
→ local ViewBodyRotationComponent predicts immediately
→ local presentation has no network input latency
```

## Authority

server는 동일한 body-follow policy 또는 authoritative view state를 기준으로 canonical body orientation을 계산한다.

presentation 결과는 gameplay hit detection에 영향을 주지 않는다.

## Remote

```text
Replicated canonical orientation
→ remote ViewBodyRotationComponent state
→ local AnimInstance derives RootYawOffset / turn state
→ local Control Rig solves bones
```

## Replication target

우선 목표:

```text
VisualBodyYaw
```

추가 view yaw는 기존 Unreal aim replication으로 충분하지 않다는 측정이 있을 때만 추가한다.

다음은 replicate하지 않는다.

```text
RootYawOffsetMode
TurnDirection
TurnState
Spine rotations
Head rotation
Hand rotation
FBIK effector transforms
```

## Remote smoothing

packet update 사이의 visual yaw는 하나의 canonical value에서만 smoothing한다.

여러 derived property에 별도 interpolation을 넣지 않는다.

필요하면 critically-damped spring을 사용해:

- quick response
- no overshoot
- stable settle

를 목표로 한다.

smoothing은 local root-axis 오류를 숨기는 용도로 사용하지 않는다.

## JIP

late join은 현재 VisualBodyYaw에서 바로 initialize한다.

과거 Turn-in-Place event를 새 동작처럼 replay하지 않는다.

---

# 11. AnimGraph target order

정확한 Root Yaw axis / space는 failure review Stage 1에서 먼저 증명한다.

그 뒤 목표 graph는 다음 순서를 우선 검토한다.

```text
Locomotion / Turn-in-Place base
        ↓
Optional moving orientation correction
        ↓
Revolver upper-body animation layer
        ↓
Inertialization
        ↓
Verified whole-body Root Yaw correction
        ↓
Control Rig: CR_LuxUpperBodyAim
        ↓
Optional foot stabilization / Leg IK
        ↓
Output Pose
```

### Rule

- Root/body separation은 whole-body orientation layer다.
- head/spine/hand procedural solve는 actual visual body orientation 이후에 계산한다.
- Control Rig target을 actor-space 가정과 mesh-space 가정 사이에서 반복 변환하지 않는다.
- root correction 실패 상태에서 Control Rig tuning을 시작하지 않는다.

---

# 12. Current source cleanup target

## ALuxCharacter

최종적으로 view/body solver의 내부 정책을 소유하지 않는다.

유지 가능:

```text
ViewBodyRotationComponent subobject
Aiming semantic state
Camera
EquippedRevolver
```

정리 대상:

```text
UpdateViewBodyRotation algorithm orchestration
weapon-specific naming in general view API
```

`Config/DefaultEditor.ini`는 stage하지 않는다.

## ULuxCharacterAnimInstance

현재 다음 fixed outputs는 Control Rig 도입 후 제거 또는 debug-only로 축소한다.

```text
Spine01AimRotation
Spine02AimRotation
Spine03AimRotation
RightHandAimRotation
NeckLookRotation
HeadLookRotation
```

대신 semantic inputs를 제공한다.

예:

```text
PresentationViewRotation
VisualBodyYaw
ViewBodyDeltaYaw
AimAlpha
DesiredHeadTarget
DesiredHandTarget
```

실제 bone solve는 Control Rig가 한다.

## ULuxViewBodyRotationComponent

현재 replicate되는 derived fields를 정리한다.

최종 목표는 `VisualBodyYaw` 중심의 최소 상태다.

---

# 13. Implementation checkpoints

기존 failure review Stage 0~2를 먼저 통과한다.

그 뒤 아래 순서로 진행한다.

## VB-01 - Semantic View / Body State Cleanup

### Work

- axis-proven root/body separation 유지
- component에서 animation-specific RootYaw mode 제거
- canonical `VisualBodyYaw` 중심으로 state 정리
- `GetPresentationAimRotation` naming 정리
- Revolver upper-body equipment condition 수정
- unnecessary replicated derived fields 제거

### Gate

- idle `+-45 degrees` view lead 안정
- pelvis measured yaw와 calculated visual body yaw 일치
- no neck/spine procedural correction 상태에서도 body separation 정상

---

## VB-02 - Natural Body Follow Policy

### Work

- relaxed / ADS body follow behavior 분리
- hard threshold 대신 soft follow curve 도입
- hysteresis 추가
- movement start / stop body follow 통합
- component 자체 update ownership 검토

### Gate

- slow mouse sweep에서 body angular velocity가 discontinuity 없이 변함
- threshold 부근에서 state chatter 없음
- 90-degree look에서 body가 갑자기 snap하지 않음

---

## VB-03 - Turn-in-Place

### Work

- existing turn asset inventory
- project-owned 90 / 180 turn assets 확정
- TurnYaw curve 작성
- RootYaw residual consumption
- Inertialization
- 필요 시 foot-lock curve

### Gate

- +-90 / +-180 turn
- pelvis rotates in intended direction
- planted foot visible slide 허용 범위 내
- turn 종료 후 residual yaw 안정

---

## VB-04 - Control Rig Upper-Body FBIK

### Work

- `CR_LuxUpperBodyAim`
- root `spine_01`
- head + hand_r effectors
- per-bone stiffness / limits
- preferred elbow angle
- relaxed / ADS solver alpha

### Gate

- small look: head leads
- large look: spine joins naturally
- no 160% over-rotation
- no roll contamination
- no elbow inversion
- hold 상태 jitter 없음

---

## VB-05 - Revolver Muzzle Integration

### Work

- existing convergence / parallax target math 유지
- desired hand transform을 FBIK effector로 전달
- direct hand replacement 제거
- muzzle debug error 유지

### Gate

- distant target alignment
- up/down aim
- 1m close target
- maximum parallax clamp
- shoulder/elbow/wrist가 함께 correction을 나눔

---

## VB-06 - Moving Locomotion Polish

### Work

- Forward / Backward / Strafe regression
- Orientation Warping isolated test
- 필요 시 foot stabilization
- turn-to-move / move-to-idle transition polish

### Gate

- cardinal locomotion 보존
- no body snap
- no obvious foot skate
- no double spine twist

---

## VB-07 - Multiplayer Presentation QA

### Work

- minimal network state 확정
- 2 / 3 player PIE
- 50ms / 100ms
- JIP
- remote smoothing only if measured necessary

### Gate

- owner immediate response
- observer sees same body intent
- remote head/body/weapon stable during slow and fast look
- no stale Turn-in-Place replay

---

# 14. Debug measurements

각 단계에서 화면 느낌만 보지 않고 다음 값을 같이 측정한다.

```text
ViewYaw
ActorYaw
VisualBodyYaw
MeasuredPelvisWorldYaw
ViewBodyDeltaYaw
TurnRequested
TurnTargetYaw
HeadWorldForward vs ViewForward error
MuzzleForward vs PresentationTarget error
Hand effector correction angle
Left / Right foot world displacement during Turn-in-Place
```

## Important

`CalculatedVisualBodyYaw`가 맞다는 이유만으로 성공 판정하지 않는다.

반드시 실제 pelvis / head / muzzle bone world transform을 함께 측정한다.

---

# 15. Acceptance quality bar

최종 사용자가 관찰했을 때 다음이 보여야 한다.

### Idle small look

- head first
- neck natural
- pelvis almost still

### Idle large look

- torso gradually joins
- body begins catch-up before anatomical limit
- no sudden 60/90-degree snap

### Turn-in-Place

- actual turn pose
- feet remain believable
- residual twist releases during animation

### ADS

- chest / shoulder align earlier
- weapon stays on view target
- wrist is not the only joint moving

### Locomotion

- body follows movement context
- strafe / backward animations remain
- head and gun do not oscillate when starting movement

### Remote

- no visible packet-step neck twitch
- body/head/gun tell the same facing intention

---

# 16. Do not do

- 현재 broken root graph 위에 FBIK를 바로 추가하지 않는다.
- 160% additive 문제를 단순 숫자 감소로만 숨기지 않는다.
- `TurnRequested` 상태만 만들고 actual body movement를 생략하지 않는다.
- 5개의 derived presentation property를 계속 network truth로 두지 않는다.
- bone transform을 replicate하지 않는다.
- R21 원본 asset을 수정하지 않는다.
- generic `CharacterIKFramework`를 만들지 않는다.
- Motion Matching 전체 전환을 이번 문제 해결책으로 사용하지 않는다.
- `DefaultEditor.ini`를 stage하지 않는다.
- user visual gate 없이 자동 테스트만으로 high-end animation quality를 완료 처리하지 않는다.

---

# 17. Unreal Engine 5.8 references

Implementation 시 확인할 공식 기능:

- Control Rig Full Body IK  
  https://dev.epicgames.com/documentation/en-us/unreal-engine/control-rig-full-body-ik-in-unreal-engine

- IK Rig Full Body IK solver  
  https://dev.epicgames.com/documentation/unreal-engine/ik-rig-solvers-in-unreal-engine?lang=en-US

- Pose Warping / Orientation Warping  
  https://dev.epicgames.com/documentation/en-us/unreal-engine/pose-warping-in-unreal-engine

- Inertialization API / animation transition support  
  https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FAnimNode_Inertialization/RequestInertialization

---

# 18. Codex completion rule

각 checkpoint는 다음 순서를 따른다.

```text
Implement current checkpoint only
→ Build
→ automated transform measurements
→ visual capture / user gate
→ report
→ stop
```

사용자 검수 전에 다음 checkpoint로 자동 진행하지 않는다.

보고 형식:

1. Changed Files
2. Responsibility Changes
3. Coordinate Space Contract
4. Measured Bone Results
5. Visual Result
6. Network Result if applicable
7. Known Issues
8. Ready for Review
