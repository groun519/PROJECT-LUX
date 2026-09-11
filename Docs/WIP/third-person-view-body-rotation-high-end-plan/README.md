# Third-Person View / Body / Intent Presentation Plan

> Status: **VB-01 policy rework required on top of the current local implementation**  
> Stable baseline: `main@0275e299b9400488e7770f322f285f3b1e82792c`  
> Local implementation basis: `LocalChangesReview_42591216.md`  
> Companion diagnosis: `../third-person-view-body-rotation-review/README.md`  
> Scope: view/body separation, movement-facing body intent, expressive head/upper-body look, continuous turning, Turn-in-Place, revolver aim presentation, locomotion polish, remote presentation  
> Product goal: **다른 플레이어가 캐릭터를 봤을 때 그 플레이어가 어디로 가고 무엇을 보고 있으며 무엇을 하려는지가 몸짓으로 읽히게 한다.**

---

# 0. Reading rule

이 문서는 단순히 '하이엔드 IK'를 만드는 계획이 아니다.

최종 품질 기준은 solver의 복잡도나 bone 수가 아니라 다음 사용자 경험이다.

```text
Movement tells others where I am going.
View tells others what I am paying attention to.
Head / upper body expose that difference.
Body turns only when there is a reason for the body itself to turn.
```

따라서 구현 선택은 항상 아래 순서로 판단한다.

1. 플레이어의 조작을 방해하지 않는가?
2. 관찰자에게 플레이어의 의도가 잘 읽히는가?
3. 작은 행동이 재미있는 몸짓으로 남는가?
4. 연속 회전이나 이동 같은 기본 조작에서 시스템이 방해물이 되지 않는가?
5. 그 뒤에 animation / IK 품질을 높인다.

현재 local diff에는 이미 `ULuxViewBodyRotationComponent`, canonical `VisualBodyYaw`, AnimInstance의 `RootYawOffset` 파생, 최소 replication의 기초가 들어가 있다.

따라서 기존 VB-01 / VB-02를 따로 구현하지 않는다.

**기존 VB-01과 VB-02를 하나의 새 VB-01로 통합한다.**

---

# 1. Target player-facing experience

## Requirement - Two directions are the core of the presentation

캐릭터 표현에는 서로 다른 두 방향이 존재한다.

```text
Movement Direction
= 캐릭터가 실제로 이동하고 있는 방향
= 하체 / 골반 / 몸 전체가 주로 표현해야 하는 방향

View Direction
= 카메라 / 플레이어가 바라보는 방향
= 머리 / 목 / 상체가 표현해야 하는 방향
```

이 둘의 차이가 곧 캐릭터의 비언어적 표현이다.

예:

```text
Body / movement  → forward
View             → right

Result
→ legs and pelvis keep moving forward
→ upper torso twists right
→ neck turns further
→ head clearly looks right
```

관찰자는 별도의 HUD 없이도 다음을 읽을 수 있어야 한다.

- 저 사람은 이쪽으로 가고 있다.
- 그런데 저쪽 사람을 보고 있다.
- 방금 옆을 힐끔 봤다.
- 계속 누군가를 쳐다보고 있다.
- 이제 실제로 몸까지 돌리려 한다.

## Requirement - Small glance should remain visible

정지한 플레이어가 옆을 살짝 볼 때 body가 view를 즉시 추종하면 안 된다.

```text
Small glance
View changes immediately
→ Head reacts first
→ Neck follows
→ Upper spine joins only as needed
→ Pelvis / feet remain almost unchanged
```

이 행동은 짧든 오래 유지되든 그 자체로 유효한 표현이다.

**단순히 일정 시간 유지했다는 이유만으로 body가 view 방향까지 따라가서는 안 된다.**

즉 기존 계획의:

```text
Sustained look
→ dwell
→ body automatically catches up
```

를 기본 규칙에서 제거한다.

## Requirement - A held side look can stay a side look

플레이어가 가만히 서서 오른쪽 사람을 계속 쳐다보는 경우:

```text
Movement = none
Visual Body = previous body direction
View = right

→ body can stay
→ head / neck / upper body keep the look
```

이 상태가 유지되어야 '계속 저 사람을 보고 있음'이라는 의도가 관찰자에게 남는다.

## Requirement - Continuous turning must never be blocked by IK

플레이어가 한 방향으로 계속 마우스를 돌리면 몇 바퀴든 자연스럽게 회전할 수 있어야 한다.

금지되는 결과:

```text
look right
→ head reaches limit
→ yaw wraps
→ head suddenly swings left
→ repeats
```

원하는 결과:

```text
view keeps rotating right
→ head / upper body lead
→ body detects continued same-direction turn intent
→ body starts following in the same direction
→ upper-body residual becomes available again
→ rotation can continue indefinitely
```

핵심 규칙:

**View leads. Body follows when the player is actually turning. Body never blocks View.**

## Requirement - Body follows movement

이동 중 body orientation의 1차 기준은 View가 아니라 **Movement Direction**이다.

```text
Start moving
→ desired visual body direction comes from horizontal movement
→ pelvis / legs / body align toward travel direction
→ View-Body difference remains available to head / upper body
```

정지하면 body는 마지막 유효 orientation을 유지한다.

이 요구는 기존 목업의 'moving에서도 view를 더 적극적으로 follow한다' 규칙을 대체한다.

이전의 cardinal strafe / backward pose 보존은 더 이상 상위 요구사항이 아니다.

현재 Blend Space를 당장 삭제하지는 않지만, 최종 이동 표현은 **실제 이동 방향을 몸이 표현한다**는 UX를 우선한다.

## Requirement - Anatomical limit is a safety boundary, not the main behavior

View와 Visual Body가 너무 크게 벌어지면 사람 몸처럼 보일 수 없다.

초기 tuning target:

```text
comfortable expressive range    approximately 0 - 90 deg
upper-body extreme range        approximately 90 - 110 deg
hard correction region          approximately 100 - 110 deg
```

정확한 수치는 skeleton preview와 플레이테스트로 조정한다.

약 100~110도 부근에서는:

```text
View continues farther
→ body rotates only enough to recover usable upper-body range
→ gaze remains on the same View target
```

이 correction은 '오래 쳐다봤기 때문'이 아니라 **해부학적으로 더 이상 표현할 수 없기 때문**에 발생한다.

## Requirement - Continuous turn intent can start body motion before the hard limit

한 방향으로 지속적으로 회전 중이라는 의도가 명확하면 hard limit까지 기다릴 필요는 없다.

Body follow는 다음 신호를 함께 사용할 수 있다.

```text
signed view angular velocity
same-direction rotation duration
current View-Body residual
proximity to anatomical limit
```

중요한 차이:

```text
hold 90 deg and stop moving the mouse
→ can remain a side look

pass through 90 deg while continuously rotating right
→ body should already be following right
```

## Requirement - ADS changes participation, not ownership

ADS에서는 relaxed보다 chest / shoulder / body가 View와 더 적극적으로 정렬될 수 있다.

하지만 ADS도 View 입력을 막지 않고, Movement Direction이라는 body의 기본 의미를 완전히 지우지 않는다.

```text
Relaxed
→ large expressive View-Body separation allowed

ADS
→ smaller preferred residual
→ torso and weapon participate earlier
→ body correction can begin earlier
```

정확한 ADS residual과 threshold는 weapon pass까지 연결한 뒤 tuning한다.

## Requirement - Weapon aim is not Look

다음 세 방향을 같은 값으로 취급하지 않는다.

```text
View Direction
Visual Body Direction
Weapon Aim Direction
```

Relaxed side glance:

```text
View     → side
Head     → side
Body     → movement / retained body direction
Weapon   → mostly body-oriented
```

ADS:

```text
View     → target
Head     → target
Chest    → participates strongly
Weapon   → target
Body     → participates according to ADS body policy
```

---

# 2. Canonical semantic contract

최종 시스템은 먼저 '어디를 향해야 하는가'를 결정하고, 그 다음 skeleton이 '어떻게 거기까지 가는가'를 해결한다.

## View / movement / body

```text
PresentationViewYaw
= player view / camera yaw used for third-person presentation

MovementDirectionYaw
= valid horizontal travel direction when moving

VisualBodyYaw
= pelvis / legs / whole-body presentation orientation

ViewBodyResidualYaw
= Delta(VisualBodyYaw, PresentationViewYaw)
```

`VisualBodyYaw`는 animation bone 결과가 아니다.

이 값은 **관찰자에게 몸 전체가 어느 방향을 향하고 있다고 보여야 하는가**에 대한 semantic truth다.

## RootYawOffset

`RootYawOffset`은 canonical state가 아니다.

```text
RootYawOffset
= Delta(ActorYaw, VisualBodyYaw)
```

AnimInstance / animation adapter에서 파생한다.

현재 local diff의 이 방향은 유지한다.

## Look residual

Head / neck / spine은 더 이상:

```text
BaseAimRotation - ActorRotation
```

만을 기준으로 사용하지 않는다.

최종 Look solve의 yaw source는:

```text
ViewBodyResidualYaw
= View relative to VisualBody
```

여야 한다.

Actor가 controller view를 따라 돌더라도 visual body separation이 head / upper-body 표현에서 사라지면 안 된다.

## Continuous yaw

내부 turn intent 계산에서는 normalized yaw 하나만으로 방향 의도를 판단하지 않는다.

권장 내부 모델:

```text
CurrentNormalizedViewYaw
PreviousNormalizedViewYaw
SignedFrameViewDelta = Delta(Previous, Current)
ContinuousViewYaw += SignedFrameViewDelta
```

필요하면 `VisualBodyYaw`도 내부에서는 continuous angle로 유지하고 API / replication 경계에서만 normalize한다.

목표:

- +179 / -179 경계에서 turn direction이 뒤집히지 않음
- 같은 방향 spin이 계속 같은 signed intent로 유지됨
- shortest-path helper가 player turn intent를 덮어쓰지 않음

---

# 3. What rotates the body?

Visual Body가 회전하는 이유를 명시적으로 제한한다.

## Cause A - Movement

가장 일반적인 body orientation source.

```text
horizontal speed above threshold
→ derive MovementDirectionYaw from velocity / resolved movement
→ DesiredVisualBodyYaw follows movement direction
```

몸은 순간 snap하지 않고 짧은 response를 가질 수 있다.

하지만 발과 이동 방향이 장시간 모순되면 안 된다.

## Cause B - Continuous turn intent

정지 상태에서도 player가 같은 방향으로 계속 camera를 회전하면 body가 뒤따른다.

```text
same-direction view rotation
+ sufficient angular velocity / continuity
+ meaningful residual
→ begin body follow in the same signed direction
```

player가 회전을 멈추면 body가 자동으로 View center까지 끝까지 추적하지 않는다.

현재 위치에서 안정적으로 settle하고 residual을 남길 수 있다.

## Cause C - Anatomical limit correction

View-Body residual이 허용 범위를 초과하려 할 때 강제 안전 correction.

```text
abs(residual) approaches MaxExpressiveYaw
→ move VisualBodyYaw just enough to keep residual inside envelope
```

이 correction은 continuous turn intent가 없어도 동작한다.

## Cause D - ADS / strong action context

ADS나 이후 명확한 gameplay action이 whole-body commitment를 요구할 경우 body participation을 높일 수 있다.

단:

- gameplay ballistic direction을 변경하지 않는다.
- View input을 제한하지 않는다.
- movement-facing 의미를 완전히 없애지 않는다.

## Non-cause - Dwell alone

다음은 더 이상 body rotation 원인이 아니다.

```text
player has looked 60 degrees to the right for N seconds
```

각도가 해부학적 한계 안이고 player가 더 회전하지 않는다면 body는 그대로 남을 수 있다.

---

# 4. Current local implementation findings

기준: `LocalChangesReview_42591216.md`

## Fact L-01 - Structural VB-01 foundation already exists

현재 local diff에는 다음이 존재한다.

```text
ULuxViewBodyRotationComponent
canonical VisualBodyYaw
component-owned Tick
VisualBodyYaw replication with COND_SkipOwner
AnimInstance-derived RootYawOffset
equipped-revolver check for upper-body weight
```

따라서 새 VB-01은 이 구조를 버리지 않는다.

기존 구현 위에서 body policy를 교체한다.

## Finding L-02 - Current body policy is still View-follow policy

현재 component는 `ActorYaw`를 canonical input처럼 사용하고 VisualBodyYaw가 ActorYaw를 늦게 따라간다.

현재 Character 설정:

```text
bUseControllerRotationYaw = true
```

즉 ActorYaw는 사실상 view/controller yaw와 강하게 연결된다.

현재 policy의 의미는:

```text
View leads
VisualBody follows View with relaxed / aiming / moving thresholds
```

이다.

새 요구는:

```text
Movement owns body while moving
View owns attention
Continuous turn / anatomical limit can rotate body while idle
```

이므로 policy 수정이 필요하다.

## Finding L-03 - Current moving policy still follows Actor/View

현재 moving에서는 `bFollowingView = true`에 가깝게 동작하며 `MovingFullFollowAngle`, `MovingMaximumFollowSpeed`로 ActorYaw를 추종한다.

이 규칙은 새 UX와 맞지 않는다.

Moving state는 View follow gain이 아니라 **MovementDirectionYaw를 body target으로 선택하는 문맥**이 되어야 한다.

## Finding L-04 - Current head/spine yaw source does not express VisualBody separation directly

현재 AnimInstance의 look delta:

```text
GetBaseAimRotation() - GetActorRotation()
```

새 구조에서는 yaw가:

```text
PresentationViewYaw - VisualBodyYaw
```

를 반영해야 한다.

그렇지 않으면 RootYawOffset으로 body를 분리해도 head / spine가 실제 View-Body 차이를 제대로 표현하지 못할 수 있다.

## Finding L-05 - Fixed additive look distribution can exceed the intended rotation

현재:

```text
Spine01 = 15%
Spine02 = 20%
Spine03 = 25%
Neck + Head = up to 100% outside ADS
```

parent-child additive 누적 시 목표보다 큰 총 회전이 발생할 수 있다.

이 구현은 VB-03에서 교체한다.

## Finding L-06 - All upper-body segments share one response time

현재 `SmoothedLookRotation` 하나를 모든 spine / neck / head가 공유한다.

따라서 최종 각도 비율만 다르고:

```text
Head first
Neck second
Spine later
```

라는 의도 표현이 나오지 않는다.

## Finding L-07 - Current normalized yaw policy does not preserve continuous turn intent as an explicit state

현재 body policy는 normalized ActorYaw / VisualBodyYaw와 shortest delta를 사용한다.

일반 orientation comparison에는 유효하지만:

```text
I am still turning right
```

라는 player intent를 별도로 보존하지 않는다.

새 VB-01에는 wrap-safe continuous view delta / turn intent가 필요하다.

## Finding L-08 - Current right-hand aim has useful target math but final solve is too hand-centric

유지할 것:

- camera trace
- minimum convergence distance
- parallax clamp
- desired muzzle rotation
- HandToMuzzle inverse target generation
- muzzle aim error debug

교체할 것:

- hand bone 하나가 전체 correction을 해결하는 최종 방식

## Finding L-09 - Root axis diagnostics are still local implementation evidence

현재 local bundle에는 Root Axis diagnostic assets가 있다.

VB-01 완료는 계산값만이 아니라 실제 pelvis / body world orientation을 확인한 뒤 판정한다.

`Config/DefaultEditor.ini`는 기능 범위가 아니므로 stage / commit하지 않는다.

---

# 5. Target architecture

```text
ALuxCharacter
├─ input / movement
├─ camera / controller view
├─ aiming semantic state
├─ EquippedRevolver
└─ ULuxViewBodyRotationComponent

ULuxViewBodyRotationComponent
├─ PresentationViewYaw
├─ continuous view delta / turn intent
├─ MovementDirectionYaw
├─ canonical VisualBodyYaw
├─ DesiredVisualBodyYaw
├─ ViewBodyResidualYaw
├─ movement-facing policy
├─ continuous-turn body follow
├─ anatomical-limit correction
├─ ADS body participation
└─ minimal replicated VisualBodyYaw

ULuxCharacterAnimInstance
├─ locomotion parameters
├─ derived RootYawOffset
├─ Look residual inputs
├─ Turn-in-Place selection / phase
├─ per-layer response state
└─ typed Control Rig inputs

CR_LuxUpperBodyPresentation
├─ Pass A: Look Distribution
│  ├─ head lead
│  ├─ neck follow
│  ├─ progressive upper-spine participation
│  ├─ anatomical yaw / pitch envelope
│  └─ different response times
└─ Pass B: Revolver Weapon Aim
   ├─ chest / clavicle participation
   ├─ arm effector
   ├─ elbow preference
   └─ final wrist residual

ULuxRevolverThirdPersonPresentationComponent
├─ presentation world aim target
├─ convergence / parallax clamp
├─ desired hand / muzzle effector target
├─ WeaponAimAlpha
└─ fire / ADS context
```

## Responsibility rule

`ULuxViewBodyRotationComponent`가 결정한다:

```text
What direction should the visual body face?
```

`AnimInstance / Control Rig`가 결정한다:

```text
How should the skeleton express View relative to that body?
```

`Revolver TP presentation`이 결정한다:

```text
How should the weapon converge on its presentation target?
```

세 책임을 섞지 않는다.

---

# 6. ULuxViewBodyRotationComponent target responsibility

## Own

- presentation view yaw
- previous view yaw / signed per-frame view delta
- optional continuous unwrapped view yaw
- horizontal movement direction
- canonical visual body yaw
- desired visual body yaw
- view-body residual
- movement body target
- continuous turn intent
- anatomical correction
- ADS body participation parameters
- canonical replicated VisualBodyYaw

## Do not own

- bone names
- spine distribution
- neck / head rotations
- Control Rig
- FBIK settings
- turn animation asset paths
- R21 animation timing
- muzzle ballistic truth
- foot-lock implementation details

## Suggested semantic outputs

외부에 모든 내부 값을 노출할 필요는 없다.

필요한 최소 의미:

```text
GetVisualBodyYaw()
GetPresentationViewYaw()
GetViewBodyResidualYaw()
GetMovementDirectionYaw() if required by animation/debug
IsContinuousTurnActive() if required by animation/debug
```

실제 API 이름은 구현 시 기존 호출부와 책임을 보고 최소화한다.

generic profile framework는 만들지 않는다.

---

# 7. VB-01 policy model

VB-01의 목표는 **몸이 어디를 향해야 하는가를 완전히 해결하는 것**이다.

## Step 1 - Resolve View

```text
PresentationViewYaw
→ current player presentation view
```

owner는 local input에 즉시 반응한다.

server round-trip을 기다리지 않는다.

## Step 2 - Accumulate signed view motion

```text
SignedViewDelta
ViewAngularVelocity
SameDirectionTurnTime
```

를 계산한다.

wrap crossing에서 sign이 바뀌지 않아야 한다.

## Step 3 - Resolve movement body target

horizontal movement가 유효하면:

```text
MovementDirectionYaw = atan2(Velocity.Y, Velocity.X)
DesiredVisualBodyYaw = MovementDirectionYaw
```

를 기본 target으로 사용한다.

movement 시작 직후 한 프레임 snap을 만들지 않고 안정된 angular response로 접근한다.

## Step 4 - Idle keeps its body

horizontal movement가 없고 continuous turn / anatomical correction / ADS correction도 없으면:

```text
DesiredVisualBodyYaw = current VisualBodyYaw
```

이다.

View가 몇 초 유지됐는지는 body target 변경 이유가 아니다.

## Step 5 - Continuous turn correction

정지 중 같은 방향으로 계속 회전한다면:

```text
turn intent strength
= function(
    signed angular velocity,
    same-direction continuity,
    residual magnitude,
    anatomical-limit proximity
)
```

를 사용해 body가 같은 signed direction으로 따라가기 시작한다.

이때 View center까지 추종하는 것이 목표가 아니다.

player가 회전을 멈추면 자연스러운 residual을 남기고 멈출 수 있다.

## Step 6 - Hard anatomical correction

```text
abs(ViewBodyResidualYaw) > MaxExpressiveYaw
```

가 되지 않도록 body target을 최소한으로 이동한다.

초기 target은 약 `100 - 110 deg` 범위.

hard clamp에서 갑자기 snap하지 않도록 pre-limit soft zone을 둘 수 있다.

## Step 7 - ADS adjustment

ADS에서는:

- preferred maximum residual을 줄일 수 있다.
- body response를 더 빠르게 할 수 있다.
- continuous turn activation을 조금 더 일찍 허용할 수 있다.

하지만 ADS가 별도의 body truth를 만들지는 않는다.

## Step 8 - Produce canonical body yaw

최종 DesiredVisualBodyYaw를 wrap-safe angular response로 VisualBodyYaw에 적용한다.

`FInterpTo`를 여러 derived property에 중복 적용하지 않는다.

가속 / 감속 또는 critically-damped angular response 중 실제 체감이 좋은 최소 구조를 사용한다.

---

# 8. Turn-in-Place

Turn-in-Place는 body orientation policy가 아니다.

VB-01이:

```text
VisualBodyYaw should rotate
```

라고 결정한 뒤, 정지한 skeleton이 그 회전을 어떻게 believable하게 보여줄지를 해결한다.

## Trigger contexts

- continuous turn intent로 idle body target이 이동함
- anatomical correction으로 idle body target이 이동함
- ADS가 idle body realignment를 요구함

## Non-trigger

- 단순히 옆을 오래 보고 있음

## Preferred behavior

가능하면 project-owned:

```text
Left 90
Right 90
Left 180
Right 180
```

turn asset / curve를 사용한다.

정확한 clip inventory는 구현 전에 확인한다.

Turn animation 동안에도 View target은 유지한다.

```text
View stays
Body turns
→ ViewBodyResidual shrinks continuously
→ head / spine naturally unwind
```

head를 center로 강제 reset하지 않는다.

---

# 9. Look Distribution

목표는 '많은 bone을 돌리는 것'이 아니라 **의도가 읽히는 순서**다.

```text
View changes
→ Head leads
→ Neck follows
→ Upper spine joins as residual grows
→ Body only changes through VB-01
```

## Small residual

```text
mostly head / neck
minimal upper spine
```

## Medium residual

```text
head / neck remain dominant
upper spine progressively joins
```

## Large residual

```text
upper torso visibly participates
head still stays on gaze target
VB-01 protects anatomical envelope
```

## Response timing

각 layer는 같은 smoothed rotator를 공유하지 않는다.

```text
Head       fastest
Neck       slightly slower
UpperSpine slower
VisualBody controlled separately by VB-01
```

## Solver decision

최종 구현에서 fixed additive percentage chain을 중심 구조로 사용하지 않는다.

Control Rig 기반 Look Distribution pass를 우선한다.

단, 목적은 FBIK 자체가 아니라:

- no 160% accumulation
- stable anatomical limits
- clean residual consumption
- separate response timing

을 만족하는 것이다.

하나의 monolithic full-body solver에 Look과 Weapon을 동시에 넣어 경쟁시키지 않는다.

## Pitch

pitch는 View와 VisualBody의 yaw 정책과 별도로:

- head / neck 우선
- extreme pitch에서 chest 일부 참여
- up / down 비대칭 limit 허용
- yaw + pitch combined envelope

를 적용한다.

---

# 10. Revolver Weapon Aim

Weapon Aim은 Look Distribution 뒤에 올라가는 별도 presentation pass다.

## Keep current target math

현재 `UpdateRightHandAim()`의 다음 부분은 유지 가치가 있다.

```text
view trace
raw target
minimum convergence distance
maximum parallax clamp
desired muzzle transform
HandToMuzzle inverse
muzzle error debug
```

## Change final solve

현재:

```text
DesiredHandRotation
→ direct hand bone replacement
```

목표:

```text
Desired muzzle / hand effector
→ chest / clavicle / upper arm / lower arm
→ wrist handles final residual
```

## Relaxed

- Look pass active
- WeaponAimAlpha low / zero
- head can look away without gun rigidly chasing it

## ADS

- WeaponAimAlpha high
- chest / shoulder participate early
- muzzle converges to presentation target

## Hip fire

필요하면 shot 순간 짧은 WeaponAimAlpha pulse를 사용한다.

server ballistic trace는 절대 presentation solver에 의존하지 않는다.

---

# 11. Moving locomotion presentation

새 UX에서는 movement와 body orientation이 강하게 연결된다.

따라서 기존 'moving에서도 view를 따라가는 body' 전제를 제거한다.

## Goal

```text
actual travel direction
→ visual body direction
→ locomotion pose supports that body direction
```

## Existing Blend Space

`BS_Lux_Locomotion`은 현재 local binary asset이 수정되어 있다.

bundle만으로 graph 내부를 검증할 수 없으므로:

1. VB-01 body direction이 안정된 뒤 실제 asset graph를 확인한다.
2. 현재 Blend Space를 당장 삭제하지 않는다.
3. body가 movement direction과 정렬된 상태에서 불필요해진 strafe/backward complexity가 있는지 측정한다.
4. foot slide / transition 품질을 기준으로 단순화 여부를 결정한다.

Orientation Warping / Motion Matching은 이번 문제의 기본 해결책이 아니다.

필요성이 실제로 확인될 때만 별도 실험한다.

---

# 12. Network model

## Principle

network는 skeleton 결과가 아니라 최소 canonical orientation만 전달한다.

## Owner

```text
local view input
→ local ViewBody policy
→ immediate presentation
```

owner는 server round-trip을 기다리지 않는다.

## Authority

server는 gameplay-relevant actor/controller state와 movement를 기준으로 canonical presentation body yaw를 계산한다.

## Remote

```text
replicated VisualBodyYaw
+ available view state
→ derive RootYawOffset
→ derive ViewBodyResidual
→ local Look / Weapon solve
```

## Replicate

우선:

```text
VisualBodyYaw
```

만 유지한다.

현재 local diff의 `COND_SkipOwner` 방향을 유지한다.

## Do not replicate

```text
RootYawOffset
ViewBodyResidualYaw
turn intent strength
Head rotation
Neck rotation
Spine rotations
WeaponAimAlpha
Hand effector transform
Control Rig outputs
```

continuous turn intent는 owner / authority가 local sequential view samples로 계산한다.

remote observer는 replicated body orientation 결과를 표현하면 된다.

실제 측정에서 부족한 정보가 확인될 때만 state를 추가한다.

---

# 13. AnimGraph target order

Root axis / coordinate-space contract가 증명된 뒤 목표 순서:

```text
Locomotion / Turn-in-Place base
        ↓
Verified whole-body VisualBody / RootYaw correction
        ↓
Revolver authored upper-body animation layer
        ↓
Inertialization where required
        ↓
Control Rig: Look Distribution
        ↓
Control Rig: Weapon Aim
        ↓
Optional foot stabilization
        ↓
Output Pose
```

중요:

- Look yaw는 actual VisualBody orientation 이후의 residual을 사용한다.
- weapon correction이 pelvis / feet를 움직이지 않는다.
- root correction 실패를 upper-body IK로 숨기지 않는다.
- mesh `-90 degree` correction을 여러 함수에서 중복 적용하지 않는다.

---


# 14. Revised implementation checkpoints

## Scope freeze

현재 local working tree는 이미 기존 VB-01/02 실험 위에 많은 변경이 쌓여 있다.

따라서 현재 단계에서는 checkpoint 순서를 다시 삽입하거나 renumber하지 않는다.

고정 순서:

~~~text
VB-01  View / Body Intent Foundation
VB-02  Turn-in-Place
VB-03  Look Distribution
VB-04  Revolver Weapon Aim
VB-05  Movement Presentation Polish
VB-06  Multiplayer Presentation QA
~~~

현재 집중 범위는 **VB-01과 VB-02뿐**이다.

다음 항목은 필요성이 확정되어 있지만 지금 checkpoint에 끼워 넣지 않는다.

~~~text
Future locomotion state work
- Crouch
- Walk
- Jog
- Sprint
- Jump / Fall / Land

Future combat collision work
- capsule-only weapon hit 제거
- Skeletal Mesh / Physics Asset 기반 weapon trace
- dedicated weapon trace channel
~~~

이 항목들은 VB-01/02를 끝낸 뒤 별도 계획으로 배치한다.

단, VB-01/02가 미래 locomotion state 추가를 막는 hard-coded 구조를 만들면 안 된다.

---

## VB-01 - View / Body Intent Foundation

### Purpose

VB-01은 **Visual Body가 어느 방향을 향해야 하는가**를 완전히 결정한다.

이 단계의 완료 기준은 애니메이션이 예쁜지가 아니다.

~~~text
View
Movement
Turn intent
ADS context
        ↓
DesiredVisualBodyYaw
        ↓
VisualBodyYaw
        ↓
ViewBodyResidual
~~~

이 semantic chain이 owner / authority / remote에서 일관되고, 여러 바퀴 회전과 +-180 wrap에서도 방향 의도가 깨지지 않는 것이 목표다.

발 움직임과 Turn-in-Place의 최종 품질은 VB-02가 소유한다.

### Existing local foundation - Keep

현재 local implementation에서 다음 구조는 유지한다.

- ULuxViewBodyRotationComponent
- component-owned Tick
- canonical VisualBodyYaw
- AnimInstance에서 파생하는 RootYawOffset
- VisualBodyYaw의 최소 replication / COND_SkipOwner
- actual equipped revolver를 확인하는 upper-body condition
- Character가 controller yaw를 유지하는 현재 gameplay control model

현재 dirty local을 초기화하거나 이전 implementation으로 되돌리는 것을 VB-01 작업에 포함하지 않는다.

### Non-goals

VB-01에서는 하지 않는다.

- Crouch / Walk / Sprint / Jump 입력 추가
- locomotion animation asset 교체
- Turn-in-Place animation 완성
- head / neck / spine 최종 분배
- Control Rig Look solve
- weapon arm IK 개선
- Physics Asset weapon hit collision
- ragdoll
- generic movement-state framework

### Canonical semantic contract

#### PresentationViewYaw

다른 플레이어가 보아야 하는 플레이어의 현재 시선 yaw.

owner에서는 local view에 즉시 반응한다.

#### MovementDirectionYaw

유효한 horizontal movement가 있을 때 실제 이동 방향.

기본 source는 실제 horizontal velocity다.

단, movement start 직후 velocity가 아직 거의 0인 구간 때문에 body target이 불안정해지는 것이 확인되면 current movement input / acceleration을 **초기 방향 힌트**로만 사용할 수 있다.

실제 이동이 발생한 뒤에는 velocity가 truth다.

#### DesiredVisualBodyYaw

현재 policy가 몸 전체에 요구하는 목표 방향.

이 값은 다음 이유에 의해서만 바뀐다.

1. Movement
2. Continuous same-direction turn intent
3. Anatomical-limit correction
4. ADS / 이후 명시적으로 허용한 strong-action context

**단순 dwell time은 이유가 아니다.**

#### VisualBodyYaw

실제로 presentation body가 향하고 있다고 간주하는 canonical yaw.

VB-01에서 숫자/방향 계약을 검증한다.

VB-02 이후에는 idle Turn-in-Place execution과 동기화된 실제 presentation 결과를 계속 이 값으로 표현한다.

#### ViewBodyResidualYaw

~~~text
Delta(VisualBodyYaw, PresentationViewYaw)
~~~

Head / Neck / Spine가 이후 표현해야 할 yaw의 canonical source다.

BaseAimRotation - ActorRotation을 최종 Look yaw truth로 사용하지 않는다.

#### RootYawOffset

canonical state가 아니다.

~~~text
RootYawOffset
= Delta(ActorYaw, VisualBodyYaw)
~~~

AnimInstance / graph adapter에서 파생한다.

### Update order

한 frame에서 policy 계산 순서를 고정한다.

~~~text
1. Read presentation view
2. Compute signed wrap-safe frame view delta
3. Update view angular velocity / same-direction turn continuity
4. Resolve horizontal movement state and MovementDirectionYaw
5. Resolve body target cause / priority
6. Produce DesiredVisualBodyYaw
7. Apply body angular response
8. Produce VisualBodyYaw
9. Derive ViewBodyResidualYaw
10. Replicate canonical body yaw only when authority requires it
~~~

derived state가 이 순서를 역으로 drive하지 않는다.

### Body-target priority

#### A. Ground movement

실제 horizontal movement가 충분하면 Movement Direction이 기본 body target이다.

~~~text
moving
→ DesiredVisualBodyYaw ≈ MovementDirectionYaw
~~~

View는 body target을 직접 덮어쓰지 않는다.

이동하면서 옆을 보는 표현은 ViewBodyResidual로 남긴다.

#### B. Idle continuous turn intent

이동하지 않고 player가 같은 방향으로 계속 view를 회전하면 body가 그 방향으로 뒤따를 수 있다.

판단 신호:

- signed frame view delta
- view angular velocity
- same-direction continuity
- current residual magnitude
- anatomical limit proximity

~~~text
quick look and stop
→ no continuous-turn body chase

keep rotating right
→ right-turn intent grows
→ body follows right
~~~

player가 mouse rotation을 멈추면 body가 View center까지 자동 추적하지 않는다.

현재 body orientation에서 안정화하거나 필요한 anatomical correction만 수행한다.

#### C. Anatomical-limit correction

abs(ViewBodyResidualYaw)가 허용 표현 범위를 넘어가려고 하면 body가 최소한으로 따라간다.

초기 tuning range:

~~~text
comfortable expressive region   0 - about 90 deg
soft protection region          about 90 - 100 deg
hard maximum target             about 100 - 110 deg
~~~

정확한 값은 skeleton visual test에서 확정한다.

hard limit에 도달한 프레임에 snap하지 않는다.

soft region에서 body correction이 자연스럽게 증가해야 한다.

#### D. ADS context

ADS는 body ownership을 바꾸지 않는다.

허용되는 변화:

- preferred residual 축소
- continuous-turn follow sensitivity 증가
- anatomical soft zone이 조금 일찍 시작
- body response speed 증가

금지:

- ADS 시작 순간 body snap
- camera 제한
- movement direction 무시
- ADS가 끝났다고 반대 방향 body chase

### Continuous-turn contract

#### Wrap-safe signed delta

normalized yaw를 저장하더라도 frame delta는 항상 wrap-safe shortest signed delta로 계산한 뒤 누적한다.

예:

~~~text
178
179
-179
-178
~~~

가 실제 오른쪽 회전이면:

~~~text
+1
+2
+1
~~~

처럼 같은 signed turn intent로 이어져야 한다.

#### Same-direction continuity

한두 frame의 mouse noise 때문에 turn direction이 즉시 뒤집히지 않는다.

필요하면 매우 작은 angular velocity dead zone을 사용한다.

하지만 긴 dwell timer로 body follow를 시작하지 않는다.

#### Direction reversal

오른쪽으로 회전하다 즉시 왼쪽으로 반전하면:

~~~text
old right intent decays / cancels
→ body angular velocity brakes
→ new left intent takes over
~~~

body가 한 frame에 반대 방향으로 튀면 안 된다.

#### Multi-revolution

2~3회전이 아니라 이론상 임의 횟수 회전에서도 동일해야 한다.

normalized API output과 내부 turn intent를 혼동하지 않는다.

### Movement edge cases

#### Speed threshold

정지/이동 경계에서 body target source가 매 frame idle/movement로 흔들리지 않도록 작은 hysteresis 또는 안정된 movement threshold를 둔다.

#### Sudden movement direction change

앞으로 이동하다 왼쪽으로 즉시 방향을 바꿔도 body target은 새 movement 방향으로 연속적으로 이동한다.

snap 금지.

#### Stop moving while looking elsewhere

이동이 끝나는 순간:

~~~text
VisualBodyYaw = last resolved body orientation
View remains independent
~~~

정지했다고 View 방향으로 자동 center하지 않는다.

#### Future Walk / Sprint / Crouch

VB-01은 speed mode 이름으로 body policy를 hard-code하지 않는다.

향후 Walk / Jog / Sprint가 추가되어도 기본 truth는 actual movement direction이다.

Crouch / Jump는 향후 explicit context를 추가할 수 있지만 현재 구현 범위가 아니다.

### Lifecycle edge cases

#### Spawn / possession / restart

초기 VisualBodyYaw는 현재 Actor orientation과 일치시켜 첫 frame에 큰 residual이 생기지 않게 한다.

#### Death

death 이후 새로운 body-follow policy를 계속 적분하지 않는다.

현재 death presentation의 final orientation을 안정적으로 유지하거나 현재 기존 death rule을 따른다.

#### Teleport / authoritative large correction

큰 teleport / possession correction처럼 정상 turn이 아닌 yaw discontinuity가 감지되면 continuous-turn intent로 해석하지 않는다.

필요하면 explicit reset entrypoint를 둔다.

generic event system은 만들지 않는다.

### Network contract

#### Owner

- local view 즉시 사용
- local body policy 즉시 실행
- server replicated yaw가 owner prediction을 되감지 않음

#### Authority

- 같은 semantic policy로 canonical body orientation 계산
- 필요한 VisualBodyYaw만 remote용으로 replicate

#### Simulated proxy

- body policy를 중복 적분하지 않음
- replicated canonical orientation 사용
- bone solve는 local animation에서 derive

#### Network non-goals

다음은 replicate하지 않는다.

- RootYawOffset
- turn-intent strength
- signed frame view delta
- head / neck / spine rotations
- animation curve progress
- Control Rig outputs

VB-02에서 Turn-in-Place event synchronization이 실제로 필요한 것이 확인될 때만 최소 semantic event를 검토한다.

### Implementation order

현재 dirty local 위에서 순서를 고정한다.

#### VB-01A - Baseline capture

코드를 바꾸기 전에 현재 상태를 캡처한다.

최소:

- idle 0 / +-30 / +-60 / +-90
- right continuous spin
- left continuous spin
- movement forward / left / right while View differs
- ADS on / off

기존 문제를 영상 또는 debug 값으로 남긴다.

#### VB-01B - Semantic input correction

- PresentationViewYaw source 확정
- MovementDirectionYaw 추가
- ViewBodyResidualYaw를 canonical output으로 정리
- AnimInstance Look yaw가 Actor-relative truth에 의존하지 않도록 이후 VB-03용 source 준비

아직 head/spine final solve를 바꾸지 않는다.

#### VB-01C - Body target policy replacement

현재 Actor/View-follow 중심 relaxed / moving policy를 제거 또는 축소한다.

대신:

- movement body target
- idle hold
- anatomical correction
- ADS adjustment

을 구현한다.

#### VB-01D - Continuous turn

- wrap-safe signed view delta
- turn angular velocity
- same-direction continuity
- reversal braking
- multi-revolution body follow

을 구현한다.

#### VB-01E - Root / pelvis truth verification

계산값만 보지 않는다.

다음을 같은 프레임에서 측정한다.

- ActorYaw
- PresentationViewYaw
- DesiredVisualBodyYaw
- VisualBodyYaw
- RootYawOffset
- actual measured pelvis / visual body yaw

현재 diagnostic assets는 이 검증에 사용 가능하다.

#### VB-01F - Owner / authority / proxy sanity

최소 2-player PIE에서:

- owner immediate response
- host-owned character
- client-owned character
- remote VisualBodyYaw
- no owner correction pop

을 확인한다.

이 단계는 최종 network polish가 아니라 VB-01 semantic sanity gate다.

### VB-01 acceptance matrix

| Case | Expected |
|---|---|
| Idle +30 deg glance | body stays |
| Idle +60 deg hold 5s | body stays; no dwell chase |
| Idle +90 deg hold | expressive residual remains if skeleton envelope allows |
| Approach 100-110 deg | body begins minimum anatomical recovery |
| Continuous right spin | body keeps following right; no wrap flip |
| Continuous left spin | same mirrored behavior |
| Fast right -> left reversal | body brakes then reverses without snap |
| Move forward, View right | body = movement, residual = right look |
| Change movement direction | body target follows new travel direction smoothly |
| Stop moving while View offset | body retains last orientation |
| ADS enter / exit | tighter response allowed, no snap |
| +-180 crossing | turn intent sign remains correct |
| Spawn / restart | no first-frame large offset |
| 2-player PIE | owner and observer body intent agree |

### VB-01 completion gate

VB-01은 아래가 모두 만족될 때만 완료다.

1. body orientation source가 View-follow가 아니라 명시된 intent policy다.
2. dwell-only body catch-up이 없다.
3. multi-revolution turn이 양방향 모두 안정적이다.
4. +-180 wrap에서 direction flip이 없다.
5. movement direction과 body direction이 의미상 일치한다.
6. ViewBodyResidual이 이후 Look solve의 올바른 canonical source다.
7. calculated VisualBodyYaw와 실제 visual pelvis/body orientation이 일치한다.
8. owner / authority / simulated proxy에서 큰 모순이 없다.
9. 현재 로컬의 불필요한 diagnostic / editor 파일을 production change와 구분할 수 있다.
10. 사용자 visual gate를 통과한다.

### Known acceptable debt after VB-01

VB-01 완료 시에도 다음은 남아도 된다.

- 발 미끄러짐
- turn step animation 부재
- head / neck / spine가 아직 기존 임시 분배
- weapon hand aim가 기존 방식
- final locomotion polish 미완료

이 debt를 이유로 VB-01 방향 계약을 다시 흔들지 않는다.

### Stop rule

VB-01 검수 전 VB-02로 자동 진행하지 않는다.

---

## VB-02 - Turn-in-Place

### Purpose

VB-02는 **VB-01이 이미 결정한 idle body rotation을 실제 사람의 발과 골반 움직임으로 believable하게 표현한다.**

VB-02는 body가 왜 돌아야 하는지 결정하지 않는다.

~~~text
VB-01
Desired / resolved body turn intent
        ↓
VB-02
How do the feet and pelvis perform that turn?
~~~

VB-02가 View / Movement 정책을 다시 판단하면 안 된다.

### Inputs

VB-02가 사용할 수 있는 semantic input:

- current VisualBodyYaw
- desired body yaw or signed body turn delta
- body angular velocity
- grounded / moving 여부
- current ViewBodyResidual for gaze preservation
- optional ADS context for animation selection only if visually required

정확한 API는 구현 시 최소화한다.

### Outputs

VB-02의 결과:

- selected turn direction
- selected turn size / pose
- turn animation playback
- root / mesh yaw consumption presentation
- optional foot-lock state
- turn completion / interruption state

bone transform 자체를 network truth로 만들지 않는다.

### Non-goals

VB-02에서는 하지 않는다.

- VB-01 body target algorithm 변경
- Crouch / Walk / Sprint / Jump 추가
- final Look Distribution
- weapon IK
- Physics Asset hit collision
- generic foot IK framework
- Motion Matching 전환
- full locomotion rewrite

### Asset inventory first

코드부터 작성하지 않는다.

먼저 현재 사용 가능한 project / purchased asset에서 확인한다.

우선 후보:

~~~text
Turn Left small / 45 if available
Turn Right small / 45 if available
Turn Left 90
Turn Right 90
Turn Left 180
Turn Right 180
~~~

정확한 asset이 없다면 없는 상태를 기록하고 최소 project-owned solution을 만든다.

Marketplace 원본 asset을 직접 destructive edit하지 않는다.

### Turn size policy

VB-01이 body target을 움직였다고 매번 90도 clip을 재생하면 안 된다.

turn presentation은 남은 idle body delta에 맞춰 가장 자연스러운 규모를 선택한다.

초기 의미:

~~~text
very small correction
→ no authored step or subtle procedural/root response

small / medium correction
→ small turn asset if available

large correction
→ 90 turn

very large correction
→ 180 turn only if it improves the result
~~~

정확한 cutoff는 asset 실제 root/pelvis motion을 보고 결정한다.

### Continuous spin behavior

이 checkpoint의 핵심 테스트다.

player가 계속 오른쪽으로 회전하면:

~~~text
head / upper-body lead is preserved
VB-01 keeps producing rightward body intent
VB-02 performs successive believable rightward body turns
feet replant
next turn can begin
~~~

금지:

- 90도 clip 종료 때까지 View 입력 지연
- clip 때문에 body turn intent sign이 뒤집힘
- 매 clip 시작마다 head center reset
- left/right animation ping-pong
- turn completion 후 body가 View center를 과추적

### Gaze preservation

Turn-in-Place 동안 View target은 독립적이다.

~~~text
View fixed at world direction
Body rotates toward its target
→ ViewBodyResidual decreases naturally
→ later Look Distribution unwinds naturally
~~~

VB-02가 head / neck 값을 0으로 reset하지 않는다.

현재 VB-03가 아직 구현 전이라도 debug line / semantic residual 값으로 이 계약을 검증할 수 있어야 한다.

### Yaw-consumption contract

가장 중요한 구현 위험은 **body semantic yaw와 turn animation이 서로 두 번 회전시키는 것**이다.

따라서 구현 전에 둘 중 하나의 계약을 명시적으로 선택하고 테스트한다.

#### Preferred contract

~~~text
VB-01 supplies body turn target / intent.
VB-02 turn execution owns the visible idle yaw progress.
VisualBodyYaw represents the resolved visible body orientation.
~~~

즉 Turn-in-Place가 활성인 동안에는 authored turn progress와 실제 visible yaw가 하나의 progress source를 공유해야 한다.

가능하면 project-owned TurnYaw curve 또는 동등한 deterministic progress를 사용한다.

금지:

~~~text
VB-01 independently rotates VisualBodyYaw
+
turn animation/root correction independently rotates again
= double turn
~~~

#### Fallback contract

사용 가능한 turn asset이 yaw progress curve를 안정적으로 제공하지 못하면:

- VisualBodyYaw의 canonical angular response를 유지
- turn asset은 pose/foot presentation만 보조
- actual measured pelvis yaw가 canonical yaw와 중복되지 않도록 graph order를 맞춘다.

어느 계약이 채택됐는지는 VB-02 Result에 기록한다.

### Turn interruption

#### Player starts moving during idle turn

- locomotion이 우선한다.
- turn pose를 자연스럽게 blend out한다.
- VB-01 movement body target으로 전환한다.
- stale turn completion event가 이후 body를 되돌리지 않는다.

#### Player reverses view direction during turn

- 현재 발이 이미 turn 중이면 즉시 반대 clip으로 snap하지 않는다.
- 현재 turn을 얼마나 중단할지 / 마무리할지는 asset 결과로 결정하되, visual body와 new target 사이의 delta를 다시 계산한다.
- stale signed turn request를 재사용하지 않는다.

#### ADS changes during turn

- View 입력과 body semantic target은 계속 유효하다.
- ADS 전환 때문에 turn animation을 처음부터 restart하지 않는다.

#### Death / possession loss

- turn state 즉시 종료
- stale turn event / timer 정리

### Foot quality

Turn-in-Place의 성공 여부는 pelvis yaw만으로 판단하지 않는다.

측정:

- left foot world displacement
- right foot world displacement
- planted phase에서 slide
- toe / heel pivot plausibility
- pelvis pop
- turn start / stop pose pop

Foot IK / foot lock은 **실제 asset만으로 부족한 것이 확인될 때만** 추가한다.

### Inertialization

turn 진입/종료 pop을 줄이기 위해 우선 검토한다.

단:

- 잘못된 turn target을 숨기는 용도로 사용하지 않는다.
- 너무 긴 inertialization으로 입력 반응을 흐리지 않는다.

### Network behavior

Turn-in-Place의 기본 network truth는 계속 VisualBodyYaw다.

remote에서 동일한 exact animation frame까지 반드시 맞출 필요는 없다.

관찰자에게 필요한 것은:

- turn direction이 맞음
- body yaw가 맞음
- feet가 크게 미끄러지지 않음
- head/body intention이 모순되지 않음

실제 QA에서 remote turn phase mismatch가 눈에 띄는 문제로 확인될 때만 최소 semantic event를 추가한다.

가능한 최소 event 예:

~~~text
TurnSequence
SignedTurnAmount
~~~

검증 전에는 추가하지 않는다.

### Implementation order

#### VB-02A - Asset inventory / preview

- turn asset 후보 목록
- root motion 여부
- in-place 여부
- skeleton compatibility
- left/right symmetry
- actual turn degrees
- curve availability

을 기록한다.

#### VB-02B - Root-only turn integration

head / spine / weapon solve를 끈 상태에서:

~~~text
idle body turn target
→ chosen turn presentation
→ measured pelvis yaw
~~~

만 검증한다.

#### VB-02C - Yaw progress synchronization

- double rotation 없음
- turn curve / canonical yaw source 하나로 통일
- 45 / 90 / 180 또는 실제 사용 asset 규모 검증

#### VB-02D - Continuous turn chaining

- repeated right turns
- repeated left turns
- turn while View continues moving
- no animation ping-pong
- no View block

#### VB-02E - Interruptions

- move during turn
- reverse during turn
- ADS during turn
- death during turn

검증.

#### VB-02F - Foot polish

필요한 만큼만:

- blend
- inertialization
- foot lock
- small-turn fallback

을 적용한다.

#### VB-02G - 2-player sanity

observer가:

- correct turn direction
- correct body yaw
- no severe pop
- no stale/replayed turn

을 보는지 확인한다.

### VB-02 acceptance matrix

| Case | Expected |
|---|---|
| Small body correction | no oversized 90-deg theatrical turn |
| 90-deg idle turn | believable feet/pelvis, correct yaw |
| Large idle turn | appropriate larger turn or chained turn |
| Continuous right spin | repeated right body turns, no ping-pong |
| Continuous left spin | mirrored stable behavior |
| View stops mid-turn | body finishes/settles without View-center chase |
| View reverses | no instant animation snap; new target respected |
| Start moving mid-turn | locomotion takes over cleanly |
| ADS during turn | no restart/pop |
| Death mid-turn | state cleans immediately |
| Remote observer | same body intent, no stale turn replay |

### VB-02 completion gate

1. VB-01 body policy를 수정하지 않고 동작한다.
2. visible body yaw와 turn animation이 double-rotate하지 않는다.
3. continuous spin이 양방향 모두 자연스럽게 이어진다.
4. View는 Turn-in-Place 때문에 차단되거나 기다리지 않는다.
5. gaze residual이 turn 동안 연속적으로 유지된다.
6. movement interruption이 깨끗하다.
7. foot slide가 사용자 visual gate 기준에서 허용 범위다.
8. 2-player observer에서 turn direction/body intent가 일치한다.
9. 불필요한 network animation state를 추가하지 않는다.
10. 사용자 visual gate를 통과한다.

### Known acceptable debt after VB-02

VB-02 완료 시에도 다음은 후속 단계다.

- head / neck / spine 최종 high-quality distribution
- revolver shoulder / arm IK
- crouch / walk / sprint / jump state foundation
- final movement locomotion polish
- Physics Asset weapon hit collision
- full multiplayer latency QA

이 debt를 VB-01/02에 끌어들여 현재 local scope를 다시 넓히지 않는다.

### Stop rule

VB-02 검수 전 VB-03로 자동 진행하지 않는다.

---

## VB-03 - Look Distribution

### Purpose

VB-03은 **VB-01/02 이후 남아 있는 View-Body 차이를 Head / Neck / Upper Spine가 사람답게 표현하는 단계**다.

이 단계는 body orientation을 다시 결정하지 않는다.

~~~text
VB-01
Where should the body face?

VB-02
How should the feet / pelvis perform an idle turn?

VB-03
How should Head / Neck / Upper Spine express the remaining ViewBodyResidual?
~~~

목표는 '많은 bone을 돌리는 것'이 아니라 **관찰자가 플레이어의 관심 방향을 자연스럽게 읽게 하는 것**이다.

---

### Canonical input

Yaw의 canonical source는 다음이다.

~~~text
ViewBodyResidualYaw
= View relative to VisualBody
~~~

기존처럼:

~~~text
BaseAimRotation - ActorRotation
~~~

을 최종 Look yaw truth로 사용하지 않는다.

Pitch는 presentation view pitch를 기준으로 별도 처리하되 yaw와 함께 anatomical envelope를 만족해야 한다.

---

### Core rule - Residual Budget

각 bone에 같은 Look delta를 비율로 복사하지 않는다.

현재 local implementation의:

~~~text
Spine01 = 15%
Spine02 = 20%
Spine03 = 25%
Neck     = 35%
Head     = 65%
~~~

같은 additive chain은 parent-child 누적으로 최종 시선이 원래 residual보다 과도하게 회전할 수 있다.

VB-03에서는 하나의 **Residual Budget**을 사용한다.

예:

~~~text
ViewBodyResidual = 80 deg

Head / Neck / Upper Spine
→ together express that 80 deg
→ total visible gaze should not overshoot the intended residual
~~~

구현 방식은 고정하지 않는다.

Control Rig, constrained chain solve, residual consumption, target-based solve 등 더 나은 방법을 선택할 수 있다.

단 결과 계약은 다음이다.

1. 전체 chain의 최종 gaze는 View target을 향한다.
2. 각 joint contribution이 서로 중복되어 over-rotation하지 않는다.
3. body가 회전해 residual이 줄면 upper-body contribution도 연속적으로 줄어든다.

---

### Participation priority - Not exclusive stages

Head -> Neck -> Spine는 **반응 우선순위**이지 배타적인 단계가 아니다.

잘못된 해석:

~~~text
0-30 deg
→ Head only

30-60 deg
→ Neck only

60+ deg
→ Spine only
~~~

원하는 의미:

~~~text
Small residual
Head       dominant
Neck       naturally participates
UpperSpine minimal

Medium residual
Head       still leads
Neck       participates strongly
UpperSpine starts supporting

Large residual
Head       stays on gaze target
Neck       near strong participation
UpperSpine clearly joins
~~~

즉 한 관절이 끝까지 버틴 뒤 다음 관절이 갑자기 켜지는 방식이 아니다.

---

### Response timing

현재 local implementation처럼 하나의 SmoothedLookRotation을 모든 upper-body segment가 공유하지 않는다.

최종 체감 목표:

~~~text
View moves immediately
      ↓
Head reacts fastest
      ↓
Neck follows shortly after
      ↓
Upper Spine follows more slowly
      ↓
Visual Body changes only through VB-01/02
~~~

정확한 spring / interpolation 방식과 parameter는 구현자가 더 적합한 방법을 선택할 수 있다.

중요한 것은:

- head가 가장 빠르다.
- neck가 약간 늦다.
- upper spine가 더 느리다.
- overshoot / oscillation이 눈에 띄지 않는다.
- 빠른 glance가 짧은 head-led gesture로 읽힌다.

---

### Angle behavior

정확한 수치는 skeleton preview에서 조정하되 초기 UX target은 다음 정도다.

~~~text
Small        about 0 - 30 deg
Medium       about 30 - 60 deg
Large        about 60 - 90 deg
Extreme      about 90 - 110 deg
~~~

#### Small

- pelvis unchanged
- upper spine contribution very small
- head / neck 중심
- quick glance가 명확히 읽힘

#### Medium

- neck contribution 증가
- upper spine가 자연스럽게 참여 시작
- 여전히 head가 관심 방향을 명확히 표현

#### Large

- upper torso twist가 명확히 보임
- head / neck가 gaze를 유지
- observer가 '몸은 이쪽, 시선은 저쪽'을 쉽게 읽을 수 있어야 함

#### Extreme

- over-shoulder look처럼 보여도 기괴하지 않아야 함
- VB-01 anatomical correction과 자연스럽게 연결
- hard clamp 직전 pose가 갑자기 멈추거나 꺾이지 않음

---

### Gaze preservation during body turn

VB-02가 body를 회전시키는 동안 Head / Neck를 center로 reset하지 않는다.

~~~text
View stays at world target

VisualBody
0 -> 30 -> 60

ViewBodyResidual
90 -> 60 -> 30

Head / Neck / UpperSpine
→ naturally unwind while staying on the same gaze target
~~~

즉 body turn이 시작되면 Look solve가 새 residual을 계속 소비한다.

이 과정에서:

- head snap 금지
- residual sign flip 금지
- body turn 직후 one-frame gaze pop 금지

---

### Yaw + Pitch anatomical envelope

Yaw와 Pitch를 각각 독립 max clamp만 하는 것으로 끝내지 않는다.

예:

~~~text
large yaw
+
large up/down pitch
~~~

가 동시에 최대치가 되어 목이 기괴하게 꺾이지 않도록 combined anatomical envelope를 둔다.

개념적으로는 다음과 같은 제한이 가능하다.

~~~text
(yaw / MaxYaw)^2 + (pitch / MaxPitch)^2 <= 1
~~~

정확한 식과 비대칭 up/down limit은 구현자가 skeleton 결과를 보고 조정할 수 있다.

결과 기준:

- neck twist 없음
- roll contamination 없음
- extreme diagonal look가 plausible함

---

### Coordinate-space contract

Mesh의 -90 degree relative yaw 같은 correction을 여러 함수 / graph에서 각각 적용하지 않는다.

Look target은 한 번 명확한 space로 변환한 뒤 일관된 기준으로 solve한다.

허용되는 최종 선택:

- Component Space
- Rig Global
- 다른 명시적 single-space contract

중요한 것은 **중복 보정이 없어야 한다는 것**이다.

---

### Interaction with VB-04 Weapon Aim

VB-03와 VB-04는 서로 다른 목적을 가진다.

~~~text
VB-03
Look / Attention

VB-04
Weapon / Muzzle Alignment
~~~

문제:

VB-04가 Chest / Clavicle / Arm을 움직이면 그 자식인 Neck / Head의 최종 gaze가 다시 틀어질 수 있다.

따라서 최종 presentation graph는 **Weapon solve 이후에도 gaze가 유지되는 구조**여야 한다.

가능한 방향 중 하나:

~~~text
ViewBodyResidual
        ↓
Upper Torso Look contribution
        ↓
Weapon Chest / Shoulder / Arm solve
        ↓
Final Neck / Head gaze correction
        ↓
View target preserved
~~~

정확한 node order는 구현자가 더 좋은 결과를 내는 방향으로 조정할 수 있다.

고정할 결과 계약:

1. Weapon Aim이 chest를 움직여도 최종 head gaze는 View를 유지한다.
2. relaxed 상태에서는 weapon이 head gaze를 끌고 가지 않는다.
3. ADS에서는 torso와 weapon participation이 커져도 head가 sight / View와 모순되지 않는다.
4. Look과 Weapon이 같은 spine 자유도를 무제한으로 두고 경쟁하지 않는다.

---

### Relaxed / ADS behavior

#### Relaxed

- Head / Neck participation high
- Upper Spine progressive
- Weapon Aim influence low / zero
- side glance가 사회적 gesture로 잘 읽혀야 함

#### ADS

- Head가 View / sight line에 가깝게 유지
- torso participation은 더 적극적일 수 있음
- Weapon solve 이후 final gaze correction 유지
- rigid mannequin pose가 되지 않도록 작은 natural residual 허용 가능

정확한 비율은 고정하지 않는다.

---

### Fast reversal

빠르게 오른쪽을 보다가 왼쪽으로 바꾸는 경우:

~~~text
Head reacts first
Neck follows
Spine brakes / follows
~~~

가 되어야 한다.

금지:

- 모든 bone이 같은 frame에 동일 속도로 방향 반전
- spring overshoot로 head가 target을 왕복
- previous-direction residual이 오래 남아 twist 발생

---

### Moving look

이동 중에도 Look solve는 유지된다.

~~~text
Body
→ Movement Direction

View
→ another direction

VB-03
→ express ViewBodyResidual on upper body
~~~

moving이라는 이유만으로 Head lead를 0으로 만들지 않는다.

다만 실제 locomotion pose와 충돌이 심하면 upper-spine 허용량을 줄이는 것은 가능하다.

이 조정은 결과 기반 tuning으로 둔다.

---

### Implementation freedom

다음은 요구가 아니라 후보다.

- Control Rig
- constrained chain distribution
- per-layer critically damped spring
- residual-consuming sequential solve
- target-based head/neck aim
- small spine support pass

더 단순한 방법으로 동일 결과를 낼 수 있으면 허용한다.

반대로 solver를 복잡하게 만드는 것 자체는 성공 조건이 아니다.

---

### Debug measurements

최소 다음을 볼 수 있어야 한다.

~~~text
ViewBodyResidualYaw
PresentationViewPitch
Head contribution
Neck contribution
UpperSpine contribution
Final Head Forward vs View Forward error
Final Chest Forward
Current VisualBodyYaw
~~~

가능하면 실제 world-space bone forward와 ViewForward를 함께 비교한다.

---

### Implementation order

#### VB-03A - Baseline / current additive measurement

현재 local fixed additive chain에서:

- 30
- 60
- 90

degree residual을 주고 실제 Head world forward가 View target을 얼마나 overshoot하는지 측정한다.

#### VB-03B - Canonical residual input

- Look yaw source를 ViewBodyResidual 기반으로 정리
- Actor-relative look dependency 제거
- Pitch source와 coordinate-space contract 확정

#### VB-03C - Residual distribution

- fixed additive percentage chain 제거
- residual budget 기반 distribution 적용
- small / medium / large participation 검증

#### VB-03D - Response timing

- Head fastest
- Neck next
- Upper Spine slower

의 시간차를 적용한다.

#### VB-03E - Anatomical envelope

- extreme yaw
- extreme pitch
- diagonal yaw + pitch

를 검증하고 combined limit 적용.

#### VB-03F - Body-turn gaze preservation

VB-02 turn 중 residual이 줄어드는 동안 gaze가 유지되는지 검증한다.

#### VB-03G - Weapon-order compatibility spike

실제 VB-04를 구현하지는 않는다.

다만 chest / clavicle에 임시 correction을 넣었을 때 final head gaze를 유지할 수 있는 graph order인지 확인한다.

필요하면 최종 Neck / Head correction 위치를 확정한다.

---

### Acceptance matrix

| Case | Expected |
|---|---|
| Idle 20-30 deg glance | head/neck dominant, spine minimal |
| Idle 45-60 deg look | neck strong, upper spine joins smoothly |
| Idle 80-90 deg look | clear upper torso participation, gaze stable |
| 100-110 deg extreme | plausible over-shoulder pose, no grotesque twist |
| Hold look | no jitter / no progressive additive drift |
| Body turns underneath | gaze preserved, residual unwinds naturally |
| Fast right -> left | head leads reversal, chain does not snap together |
| Look up/down | plausible asymmetric pitch if needed |
| Large yaw + pitch | anatomical envelope prevents impossible pose |
| Move while looking elsewhere | body movement intent + gaze intent both readable |
| Temporary chest correction | final head gaze still reaches View target |

---

### Completion gate

VB-03은 아래가 모두 만족될 때 완료다.

1. Look yaw가 canonical ViewBodyResidual을 사용한다.
2. fixed additive chain의 over-rotation 문제가 제거된다.
3. Head / Neck / Upper Spine가 residual 하나를 협력해서 표현한다.
4. Head -> Neck -> Spine response timing 차이가 시각적으로 읽힌다.
5. small glance가 head-led gesture로 보인다.
6. large / extreme look가 해부학적으로 납득 가능하다.
7. body turn 중 gaze가 끊기지 않는다.
8. yaw + pitch에서 neck twist / roll contamination이 없다.
9. temporary weapon/chest correction 이후에도 final gaze를 보존할 구조가 증명된다.
10. 사용자 visual gate를 통과한다.

### Known acceptable debt after VB-03

- final revolver shoulder / arm solve
- muzzle convergence tuning
- locomotion-state expansion
- final multiplayer latency QA

### Stop rule

VB-03 검수 전 VB-04로 자동 진행하지 않는다.

---

## VB-04 - Revolver Weapon Aim

### Work

- keep current convergence / parallax target math
- convert target into typed muzzle / hand effector input
- remove direct hand-only final correction
- distribute across chest / clavicle / arm
- wrist handles final residual
- separate WeaponAimAlpha from Look
- ADS sustained alpha
- optional hip-fire pulse

### Gate

- relaxed side look does not drag gun rigidly with head
- ADS muzzle presentation aligns
- close target clamp remains stable
- shoulder / elbow / wrist share correction
- pelvis / feet never snap because of weapon aim
- server ballistic result unchanged

### Stop rule

VB-04 검수 전 VB-05로 자동 진행하지 않는다.

---

## VB-05 - Movement Presentation Polish

### Work

- inspect current `BS_Lux_Locomotion` local changes
- verify body-facing-movement UX across forward / side / backward inputs
- simplify directional blend requirements if body orientation makes them redundant
- polish turn-to-move / move-to-idle transitions
- foot stabilization only if measured necessary
- isolated Orientation Warping experiment only if a concrete artifact remains

### Gate

- travel direction and body presentation do not contradict each other
- View can remain off-body while moving
- no body snap on movement start
- no obvious foot skate
- no double spine twist

### Stop rule

VB-05 검수 전 VB-06으로 자동 진행하지 않는다.

---

## VB-06 - Multiplayer Presentation QA

### Work

- confirm minimal network state
- 2 / 3 player PIE
- host + client ownership cases
- 50ms / 100ms latency
- JIP
- repeated continuous spin
- movement while looking away
- ADS while moving / turning

### Gate

- owner has immediate response
- observer reads the same movement / attention intent
- remote body direction stable
- remote head look stable
- no packet-step neck twitch
- no stale turn event replay
- no wrap-direction reversal

---

# 15. Debug measurements

각 checkpoint에서 화면 느낌과 함께 최소 다음 값을 본다.

```text
PresentationViewYaw
PreviousViewYaw
SignedViewDelta
ViewAngularVelocity
ActorYaw
MovementDirectionYaw
VisualBodyYaw
DesiredVisualBodyYaw
ViewBodyResidualYaw
MeasuredPelvisWorldYaw
RootYawOffset
ContinuousTurnActive
HeadWorldForward vs ViewForward error
MuzzleForward vs PresentationTarget error
Foot world displacement during Turn-in-Place
```

계산값이 맞다는 이유만으로 성공 처리하지 않는다.

실제 pelvis / head / muzzle bone transform과 관찰자 화면을 함께 본다.

---

# 16. Acceptance scenarios

## Scenario A - Quick glance

플레이어가 정지한 채 오른쪽을 잠깐 본다.

Expected:

- camera immediate
- head first
- body stays
- return view → head returns first

## Scenario B - Long stare

플레이어가 정지한 채 오른쪽 사람을 5초 이상 본다.

Expected:

- body does not automatically center toward View
- upper body keeps the social signal
- observer clearly sees the stare

## Scenario C - Walk while looking

플레이어가 한 방향으로 이동하면서 다른 방향을 본다.

Expected:

- legs / pelvis communicate travel direction
- head / torso communicate attention direction
- both intentions are readable simultaneously

## Scenario D - Spin

플레이어가 마우스를 오른쪽으로 계속 돌려 2~3바퀴 돈다.

Expected:

- head leads
- body joins
- residual continuously recovers
- no +-180 flip
- no left/right head oscillation

## Scenario E - Extreme over-shoulder look

플레이어가 body와 약 100~110 deg 이상 벌어지려 한다.

Expected:

- upper body reaches expressive extreme
- body moves only enough to stay plausible
- gaze remains continuous

## Scenario F - ADS

Expected:

- torso / shoulder commitment increases
- preferred residual becomes tighter
- weapon aligns without wrist-only distortion
- movement/body meaning remains coherent

## Scenario G - Two-player social read

한 플레이어가 다른 플레이어와 마주보고 있다가 제3의 방향을 힐끔 본다.

Expected:

- body relationship can remain
- head / upper body visibly redirect attention
- observer can read the glance without emote or HUD

---

# 17. Do not do

- '하이엔드 IK'라는 이름 때문에 solver 복잡도를 목표로 삼지 않는다.
- idle sustained-look dwell만으로 body를 자동 center하지 않는다.
- ActorYaw를 곧 VisualBodyYaw의 최종 목적지로 두지 않는다.
- moving state를 단순 View-follow gain 증가로 구현하지 않는다.
- +-180 wrap에서 shortest-path 결과만 보고 turn intent를 결정하지 않는다.
- 160% additive 문제를 숫자만 줄여 숨기지 않는다.
- Head / Neck / Spine가 하나의 SmoothedLookRotation으로 동시에 반응하게 두지 않는다.
- Look과 Weapon Aim을 하나의 monolithic solver에서 경쟁시키지 않는다.
- bone transform을 replicate하지 않는다.
- RootYawOffset을 canonical gameplay/presentation truth로 승격하지 않는다.
- root/body graph 오류를 upper-body IK로 숨기지 않는다.
- generic CharacterIKFramework를 만들지 않는다.
- Motion Matching 전체 전환을 이번 문제의 해결책으로 사용하지 않는다.
- `Config/DefaultEditor.ini`를 stage하지 않는다.
- user visual gate 없이 자동 측정만으로 완료 처리하지 않는다.

---

# 18. Unreal implementation references

필요 시 구현 단계에서 공식 문서를 다시 확인한다.

- Control Rig Full Body IK
  - https://dev.epicgames.com/documentation/en-us/unreal-engine/control-rig-full-body-ik-in-unreal-engine
- IK Rig solvers
  - https://dev.epicgames.com/documentation/unreal-engine/ik-rig-solvers-in-unreal-engine?lang=en-US
- Pose Warping
  - https://dev.epicgames.com/documentation/en-us/unreal-engine/pose-warping-in-unreal-engine
- Inertialization
  - https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FAnimNode_Inertialization/RequestInertialization

---

# 19. Codex completion rule

각 checkpoint는 다음 순서를 따른다.

```text
Implement current checkpoint only
→ Build
→ automated measurements
→ visual capture / user gate
→ report
→ stop
```

사용자 검수 전에 다음 checkpoint로 자동 진행하지 않는다.

보고 형식:

1. Changed Files
2. Responsibility Changes
3. View / Movement / Body Contract
4. Measured Results
5. Visual Result
6. Network Result if applicable
7. Known Issues
8. Ready for Review
