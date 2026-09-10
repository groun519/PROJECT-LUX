# Third-Person View / Body / Look / Aim High-End Architecture Plan

> Status: **Implementation-ready after root/body axis contract is proven**  
> Stable baseline: `main@0275e299b9400488e7770f322f285f3b1e82792c`  
> Local experiment basis: `LocalChangesReview_92935637.md`  
> Companion diagnosis: `../third-person-view-body-rotation-review/README.md`  
> Scope: idle free-look, head-only side look, neck/head/spine coordination, visual-body follow, Turn-in-Place, ADS/hip-fire weapon aim, locomotion integration, remote presentation  
> Goal: 카메라와 몸을 단순히 같은 방향으로 돌리는 시스템이 아니라, **View -> Head/Neck -> Spine -> Visual Body -> Weapon**이 서로 다른 응답 속도와 해부학적 한계를 가지며 협력하는 하이엔드 third-person presentation을 만든다.

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
View moves immediately
    ↓
Eyes / Head intention changes first
    ↓
Neck follows with a slightly slower response
    ↓
Upper spine joins only when the residual grows
    ↓
Visual body follows only when the look is large, sustained, moving, or ADS-constrained
    ↓
Large idle turns use an authored Turn-in-Place while preserving the look target
    ↓
Weapon shoulder/arm solver aligns the muzzle only when weapon-aim participation is required
```

## Requirement - Head-only side look

**이 기능은 최종 품질의 핵심 요구사항이다.**

정지한 플레이어가 마우스로 옆을 볼 때, 작은/중간 각도에서는 골반과 발을 돌리지 않고 **고개와 목만 먼저 돌려 옆을 볼 수 있어야 한다.**

의도:

```text
Quick side glance
View moves
→ Head immediately follows
→ Neck follows
→ Pelvis stays almost fixed

Sustained side look
View stays off-center
→ Head / Neck hold target
→ Upper spine gradually joins
→ Only then does visual body catch up

Body catch-up
→ Head does NOT snap back to center
→ residual look angle shrinks naturally as body turns
```

즉 큰 시선 변화에서도 body가 즉시 camera를 쫓지 않는다. **짧은 shoulder-check / side glance는 실제로 head/neck 중심으로 끝날 수 있어야 한다.**

현재 scope에서는 별도 Free-Look 키를 만들지 않는다. 일반 Look 입력 자체가 idle에서 view/body separation을 만든다. 향후 별도 Free-Look 입력이 필요해져도 같은 semantic state를 재사용할 수 있어야 한다.

## Requirement - Relaxed look

Relaxed 상태는 angle 하나만으로 body follow를 결정하지 않는다.

- 작은 yaw: head / neck only
- 중간 yaw: head / neck + upper spine
- 큰 yaw라도 매우 짧은 glance: body response를 지연
- 큰 yaw가 일정 시간 유지됨: body soft follow 시작
- 매우 큰 yaw 또는 장시간 유지: Turn-in-Place로 재정렬

초기 tuning target은 아래 정도에서 시작하되 실제 skeleton preview로 조정한다.

```text
Idle head/neck comfort band       approximately 0 - 35 deg
Idle spine participation band     approximately 35 - 65 deg
Idle body soft-follow band        approximately 55 - 80 deg
Idle hard turn request            approximately 80 - 95 deg
Post-turn comfortable residual    approximately 20 - 35 deg
```

수치는 규칙이 아니라 tuning 시작점이다.

## Requirement - ADS

ADS에서는 head-only 자유도가 줄고 chest / shoulder / body가 더 일찍 view에 참여한다.

```text
Relaxed
Head / Neck lead strongly
Spine joins later
Body follows late

ADS
Head stays near sight line
Chest / Shoulder participate immediately
Body follows at a smaller residual
Weapon aim alpha stays high
```

ADS에서도 head/neck가 완전히 rigid하게 고정되지는 않는다. 작은 자연스러운 residual은 남긴다.

## Requirement - Moving look

이동 시작이 head lead를 즉시 0으로 만들면 안 된다.

- moving에서는 relaxed idle보다 free-look band를 줄인다.
- head / neck가 약간의 look residual을 계속 가질 수 있다.
- visual body는 view에 더 적극적으로 정렬한다.
- cardinal strafe / backward locomotion은 유지한다.

초기 tuning target:

```text
Idle relaxed head lead     approximately 30 - 40 deg
Moving head lead           approximately 10 - 20 deg
ADS head lead              approximately 5 - 10 deg
```

## Requirement - Weapon aim is not the same thing as Look

다음 세 방향을 같은 값으로 취급하지 않는다.

```text
View Direction
= 카메라 / 플레이어가 보고 있는 방향

Visual Body Direction
= 골반 / 발 / 몸 전체가 시각적으로 향하는 방향

Weapon Aim Direction
= 총구가 실제로 표현상 정렬해야 하는 방향
```

Relaxed idle에서 옆을 보는 동안:

```text
View     → side
Head     → side
Body     → mostly forward
Weapon   → mostly body-oriented
```

ADS에서는:

```text
View     → target
Head     → near target
Chest    → target
Weapon   → target
Body     → follows earlier
```

Hip Fire는 서버 ballistic trace를 바꾸지 않는다. 발사 순간에만 짧은 `WeaponAimAlpha`를 올려 remote presentation의 shoulder / arm / muzzle가 shot direction에 수렴하게 하고, ADS가 아니면 다시 relaxed pose로 돌아간다.

## Requirement - Weapon alignment

총구 정렬은 손목 하나가 전체 오차를 해결하지 않는다.

```text
Visual body orientation
+ chest / clavicle participation
+ upper arm / lower arm
+ final wrist residual
= muzzle presentation alignment
```

기존 `HandToMuzzle` 역산은 target 생성에 사용한다. 최종 hand bone을 C++에서 replace하지 않는다.

## Requirement - Locomotion

- Forward / Backward / Left / Right Blend Space를 유지한다.
- 움직인다고 몸을 camera yaw로 snap하지 않는다.
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

그 뒤 Control Rig의 weapon pass가 clavicle / arm chain에 correction을 분배한다.

손목은 마지막 residual만 담당한다.

---

## Finding H-09 - One smoothed rotation makes every upper-body segment react at the same time

현재 local diff는 하나의 `SmoothedLookRotation`을 만든 뒤 spine / neck / head에 나눠 쓴다.

이 방식은 각 bone의 최종 양만 달라지고 **반응 시간은 모두 동일**하다.

사람의 실제 side glance에서는 head가 먼저 움직이고 neck, upper spine, body가 순차적으로 따라온다.

### Decision

최종 시스템은 적어도 다음 response layer를 분리한다.

```text
Head response        fastest
Neck response        fast
Upper spine response medium
Visual body response slowest
```

단순한 서로 다른 `FInterpTo`를 여러 곳에 흩뿌리지 않는다.

AnimInstance / Control Rig 내부에서 angular critically-damped spring 또는 동등한 안정된 response model을 사용하고, 각 layer는 같은 semantic residual을 다른 response time으로 추종한다.

overshoot는 기본적으로 허용하지 않는다.

---

## Finding H-10 - RootYawOffset should not be the canonical body state

현재 experiment는 `LocalRootYawOffset`을 누적하고 Actor yaw delta를 반대로 빼는 방식이다.

이것은 AnimGraph 구현값을 feature truth처럼 다루기 쉽다.

### Decision

canonical state는 **VisualBodyYaw**다.

```text
ViewYaw
VisualBodyYaw
DesiredVisualBodyYaw
```

를 직접 유지하고:

```text
RootYawOffset
= Delta(ActorYaw, VisualBodyYaw)
```

는 animation adapter에서 파생한다.

내부 body yaw는 +-180 wrap에서 튀지 않도록 연속 각도로 적분하고, 외부 노출 / replication 경계에서만 normalize한다.

---

## Finding H-11 - Look and weapon aim currently share one conceptual direction

현재 구현은 presentation view를 head / spine / right-hand aim 모두의 공통 target으로 사용한다.

그러면 relaxed side glance에서도 weapon chain이 view를 적극적으로 따라가게 되어 **고개만 옆을 보는 표현**과 충돌한다.

### Decision

semantic presentation state에 다음을 구분한다.

```text
Look Target
Body Target
Weapon Aim Target
Weapon Aim Alpha
```

Look target은 보통 View와 같다.

Weapon Aim은 문맥에 따라 다르다.

```text
Relaxed idle  → low / zero weapon aim alpha
Moving armed  → partial weapon aim alpha
Hip Fire      → short high-alpha pulse
ADS           → sustained high alpha
```

server fire direction / hit result는 절대 이 presentation alpha에 의존하지 않는다.

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
├─ canonical View / VisualBody relationship
├─ relaxed / moving / ADS body-follow policy
├─ quick-glance / sustained-look timing
├─ canonical VisualBodyYaw
├─ DesiredVisualBodyYaw
├─ ViewBodyDeltaYaw
├─ BodyTurnIntent
└─ minimal network state

ULuxCharacterAnimInstance
├─ locomotion parameters
├─ derived RootYawOffset
├─ Turn-in-Place selection / phase
├─ per-layer look response state
├─ turn curves / foot lock state
└─ typed Control Rig inputs

CR_LuxUpperBodyPresentation
├─ Pass A: Look Distribution
│  ├─ head / neck priority
│  ├─ progressive upper-spine participation
│  ├─ anatomical yaw/pitch limits
│  └─ per-layer response timing
└─ Pass B: Revolver Weapon Aim
   ├─ chest / clavicle participation
   ├─ right arm effector
   ├─ elbow preference
   └─ final wrist residual

ULuxRevolverThirdPersonPresentationComponent
├─ presentation-only world aim target
├─ muzzle convergence / parallax clamp
├─ DesiredHandEffectorTransform
├─ WeaponAimAlpha
└─ Fire / ADS context → typed rig input

ABP_LuxCharacter
├─ Locomotion / Turn-in-Place base
├─ optional moving orientation correction
├─ Revolver authored upper-body layer
├─ Inertialization
├─ verified whole-body visual yaw correction
├─ CR_LuxUpperBodyPresentation
├─ optional foot stabilization
└─ Output Pose
```

## Responsibility rule

`ULuxViewBodyRotationComponent`는 skeleton bone 이름과 Revolver를 모른다.

`ULuxCharacterAnimInstance` / Control Rig는 gameplay rule을 모른다.

Revolver TP presentation은 weapon target만 계산하고 body-follow policy를 결정하지 않는다.

Look과 Weapon Aim이 동일한 Control Rig asset 안에 있더라도 **두 solve pass와 두 alpha를 독립적으로 유지한다.**

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

canonical 계산은 `VisualBodyYaw`를 직접 목표로 한다.

```text
ViewYaw
    ↓
LookYaw = Delta(ViewYaw, VisualBodyYaw)
    ↓
Angle curve
+ View angular velocity
+ Sustained-look dwell
+ Movement / ADS context
    ↓
BodyFollowGain
    ↓
DesiredVisualBodyYaw
    ↓
critically-damped angular response
    ↓
VisualBodyYaw
```

### Comfortable residual

body가 항상 ViewYaw까지 100% 따라가지는 않는다.

Relaxed에서는 turn 후에도 head / neck가 자연스러운 residual을 유지하도록:

```text
DesiredVisualBodyYaw
= ViewYaw - SignedComfortableResidual
```

개념을 사용한다.

ADS에서는 comfortable residual을 작게, relaxed에서는 크게 둔다.

### Hysteresis

예:

```text
Start soft follow at larger residual
Stop follow only after smaller residual
```

로 threshold chatter를 막는다.

### Angular spring

`FInterpTo`를 여러 곳에서 중첩하지 않는다.

wrap-safe angular spring 하나로 canonical VisualBodyYaw를 이동시키고, normalize는 API / replication 경계에서만 한다.

Relaxed / Moving / ADS는 서로 다른 response parameter를 사용할 수 있다.

필요한 tuning curve는 프로젝트 소유 `UCurveFloat` 또는 작은 UPROPERTY set으로 둔다. generic profile framework는 만들지 않는다.

---

# 5. High-end upper-body solver

## Decision - staged Control Rig, not one monolithic FBIK solve

최종 목표에서는 sequential `Transform (Modify) Bone`의 고정 비율 additive chain을 중심 구조로 사용하지 않는다.

하지만 head와 hand를 하나의 Full Body IK solve에 동시에 넣어 서로 같은 spine 자유도를 두고 경쟁시키는 방식도 기본안으로 사용하지 않는다.

**Look과 Weapon Aim을 두 pass로 분리한다.**

```text
Pass A - Look Distribution
View/body residual
→ head / neck first
→ upper spine only as needed

Pass B - Weapon Aim
presentation target
→ chest / clavicle support
→ upperarm / lowerarm
→ hand residual
```

### Pass A - Look Distribution

Control Rig에서 head / neck / spine chain에 Aim / quaternion 기반 rotation distribution을 사용한다.

목표:

- small residual: spine contribution near zero
- medium residual: upper spine gradually joins
- body turning: residual automatically unwinds
- no Euler additive 160% accumulation
- no roll contamination

C++에서 `Spine01 = 0.15 * Rotator` 식으로 최종 bone rotation을 직접 만든다.

### Pass B - Weapon Aim

weapon pass는 필요할 때만 활성화한다.

권장 시작 chain:

```text
spine_03 / chest
clavicle_r
upperarm_r
lowerarm_r
hand_r
```

이 pass에는 Full Body IK 또는 제한된 arm-chain solver를 사용할 수 있다.

필수 조건:

- pelvis / legs는 weapon solve가 움직이지 않는다.
- stretch는 기본적으로 비활성
- elbow preferred angle 지정
- shoulder / clavicle가 일부 correction을 받음
- hand는 마지막 residual만 해결
- exact ballistic result에는 영향 없음

## Anatomical limits

yaw / pitch를 각각 독립 clamp만 하는 것으로 끝내지 않는다.

극단적인 diagonal look에서:

```text
large yaw + large pitch
```

가 동시에 최대치에 도달하지 않도록 combined anatomical envelope를 둔다.

개념:

```text
(yaw / MaxYaw)^2 + (pitch / MaxPitch)^2 <= 1
```

정확한 envelope와 비대칭 up/down limit은 skeleton preview에서 조정한다.

## Response timing

같은 target이라도 response time이 다르다.

```text
Head       fastest
Neck       slightly slower
Chest      medium
VisualBody slowest
```

critically-damped response를 우선해 overshoot 없이 cascade를 만든다.

## Relaxed mode

- Look pass high
- Weapon pass low / zero
- small side look is head / neck only
- spine starts only after comfort range

## Moving mode

- Look pass remains active
- free-look range narrows
- body follow gain rises
- weapon pass may stay partial

## ADS mode

- Weapon pass high
- chest / shoulder participate early
- head remains close to sight line
- body follow threshold decreases

## Hip Fire pulse

발사 순간:

```text
WeaponAimAlpha
→ quickly rises
→ muzzle/arm presentation converges to shot direction
→ decays back to relaxed value
```

feet / pelvis를 발사 때문에 snap시키지 않는다.

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

# 7. Head / neck / side-glance behavior

## Semantic residual

```text
LookYaw   = Delta(ViewYaw, VisualBodyYaw)
LookPitch = ViewPitch relative to visual body reference
```

head / neck는 이 residual을 소비한다.

## Head-only comfort region

작은 look에서는 spine alpha가 0에 가까워야 한다.

```text
0 → head starts immediately
small residual → head + neck
comfort edge → neck approaches preferred limit
beyond comfort → upper spine begins joining
```

"head-only"는 head bone 하나만 돌린다는 뜻이 아니다.

실제 목표는:

```text
pelvis / lower torso almost fixed
upper spine almost fixed
neck + head perform the glance
```

이다.

## Quick glance vs sustained look

body follow는 angle뿐 아니라 **view angular velocity와 hold duration**도 고려한다.

```text
Fast 60-degree glance
→ head / neck react
→ body intentionally lags

Same 60-degree look held
→ after a short dwell, soft body follow grows
```

초기 dwell tuning은 약 `0.15 - 0.30 sec` 범위에서 시작한다.

ADS / movement에서는 dwell을 줄이거나 생략할 수 있다.

## Preserve gaze during body turn

Turn-in-Place나 soft follow가 시작되어도 head target을 center로 강제 reset하지 않는다.

```text
View stays at 90
VisualBody 0 → 30 → 55
Look residual 90 → 60 → 35
Head / neck naturally unwind
```

이 흐름이 반드시 연속적이어야 한다.

## Return glance

플레이어가 다시 정면을 볼 때:

- head가 먼저 돌아온다.
- neck가 뒤따른다.
- 이미 body가 따라온 상태라면 body는 별도 settle policy로 천천히 정렬한다.
- body가 필요 이상으로 반대 방향까지 chasing하지 않는다.

## Pitch

pitch도 yaw와 같은 고정 비율을 사용하지 않는다.

- look up / down은 head / neck 우선
- 극단 pitch에서 upper chest가 조금 참여 가능
- down과 up limit은 비대칭 가능
- yaw + pitch combined anatomical envelope 적용

Control Rig target은 한 번만 coordinate conversion하고 solver 내부에서 일관된 Rig Global / Component contract를 사용한다.

C++에서 mesh `-90 degree` relative yaw를 여러 함수에서 각각 보정하지 않는다.

---

# 8. Turn-in-Place final quality target

하이엔드 최종 결과에는 큰 idle yaw에서 실제 Turn-in-Place가 필요하다.

Turn-in-Place는 **head-only look을 대체하는 기능이 아니다.** head / neck comfort range를 넘는 시선이 충분히 크고 오래 유지될 때만 body를 재정렬한다.

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

network는 skeleton 결과나 animation state가 아니라 최소 canonical orientation만 전달한다.

Owner의 head-only side glance가 즉시 반응하려면 local view 입력은 절대 server round-trip을 기다리지 않는다.

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

현재 `bUseControllerRotationYaw = true` 조건에서 simulated proxy가 Actor/BaseAim state만으로 ViewYaw를 충분히 재구성할 수 있는지 먼저 측정한다.

충분하면 `ReplicatedViewYaw`는 제거한다.

부족한 경우에만:

```text
PresentationViewYaw
```

하나를 추가한다.

다음은 replicate하지 않는다.

```text
RootYawOffset
RootYawOffsetMode
TurnDirection
TurnState
BodyFollowAlpha
LookYaw / LookPitch
Spine rotations
Head / Neck rotations
WeaponAimAlpha
Hand effector transform
FBIK outputs
```

이 값들은 canonical View / VisualBody state와 현재 local presentation context에서 derive한다.

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
Control Rig: CR_LuxUpperBodyPresentation
  Pass A - Look Distribution
  Pass B - Weapon Aim
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

## VB-02 - Free-Look / Natural Body Follow Policy

### Work

- canonical `VisualBodyYaw`를 source of truth로 전환
- idle head-only comfort band 정의
- relaxed / moving / ADS body follow behavior 분리
- angle + view angular velocity + dwell 기반 soft follow
- comfortable residual target
- hysteresis
- wrap-safe critically-damped body response
- movement start / stop integration
- component 자체 update ownership 검토

### Gate

- idle +-30 degree: pelvis 거의 고정, head-only look 가능
- quick +-60 degree glance: body가 즉시 추종하지 않음
- same +-60 degree held: body가 지연 후 부드럽게 따라옴
- 90-degree sustained look: head target 유지한 채 body가 catch-up
- body catch-up 중 head가 center로 snap하지 않음
- threshold 부근에서 state chatter 없음

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

## VB-04 - Control Rig Look Distribution

### Work

- `CR_LuxUpperBodyPresentation` Pass A
- head / neck priority solve
- upper-spine progressive participation
- separate response timing
- combined yaw/pitch anatomical envelope
- gaze preservation during body catch-up
- current fixed `Spine01/02/03AimRotation`, `NeckLookRotation`, `HeadLookRotation` 제거 또는 debug-only 전환

### Gate

- idle +-30 degree: upper spine/pelvis 거의 고정
- head reacts before spine
- +-60 degree glance: neck/head lead, spine only residual support
- diagonal look에서 neck twist 없음
- no 160% over-rotation
- no roll contamination
- hold 상태 jitter 없음
- body turn 중 head gaze가 연속적으로 unwind

---

## VB-05 - Revolver Weapon Aim Pass

### Work

- `CR_LuxUpperBodyPresentation` Pass B
- existing convergence / parallax target math 유지
- desired hand transform을 weapon effector로 전달
- direct hand replacement 제거
- chest / clavicle / arm participation
- elbow preferred angle
- `WeaponAimAlpha` 분리
- ADS sustained alpha
- Hip Fire short aim pulse
- muzzle debug error 유지

### Gate

- relaxed side glance: weapon가 head와 같이 과도하게 따라가지 않음
- ADS: distant / up-down target alignment
- 1m close target
- maximum parallax clamp
- hip fire 순간 muzzle가 shot direction에 표현상 수렴
- shoulder / elbow / wrist가 correction을 나눔
- pelvis / feet는 weapon aim 때문에 snap하지 않음

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

### Idle 30-degree side look

- camera/view turns immediately
- head leads
- neck follows
- upper spine contribution is minimal
- pelvis / feet stay almost still

### Quick 60-degree shoulder check

- head / neck can perform the glance
- body does not instantly chase the view
- returning the camera lets head return first

### Sustained 60-90-degree side look

- upper spine gradually joins
- after dwell, visual body begins catch-up
- body does not necessarily remove all residual
- head stays on the same gaze target while the body turns
- no sudden 60/90-degree snap

### Turn-in-Place

- actual authored turn pose
- planted foot remains believable
- visual body yaw moves continuously
- residual neck/spine twist releases during the turn
- no gaze reset

### ADS

- chest / shoulder align earlier
- head remains close to sight line
- weapon stays on presentation target
- wrist is not the only joint moving

### Hip Fire from side look

- ballistic direction remains server truth
- remote presentation briefly engages shoulder/arm toward shot direction
- feet do not snap to shot direction
- after the shot, pose returns to relaxed look naturally

### Locomotion

- body follows movement context
- strafe / backward animations remain
- a small moving head lead remains possible
- head and gun do not oscillate when starting movement

### Remote

- owner has immediate head response
- observer sees the same visual-body intention
- no packet-step neck twitch
- body/head/gun do not contradict each other

---

# 16. Do not do

- 현재 broken root graph 위에 Control Rig / FBIK를 바로 추가하지 않는다.
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
