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

기존 VB-01 / VB-02는 통합한다.

새 순서:

```text
VB-01  View / Body Intent Foundation
VB-02  Turn-in-Place
VB-03  Look Distribution
VB-04  Revolver Weapon Aim
VB-05  Movement Presentation Polish
VB-06  Multiplayer Presentation QA
```

---

## VB-01 - View / Body Intent Foundation

### Existing local foundation to keep

- `ULuxViewBodyRotationComponent`
- canonical `VisualBodyYaw`
- component-owned Tick
- AnimInstance-derived `RootYawOffset`
- minimal `VisualBodyYaw` replication
- equipped-revolver condition correction

### Rework

- replace View-follow body policy with movement / turn-intent body policy
- derive valid MovementDirectionYaw
- idle body holds orientation by default
- add wrap-safe signed View delta
- preserve continuous same-direction turn intent
- add anatomical soft / hard envelope around approximately 100-110 deg
- allow continuous turn to move body before hard limit
- remove dwell-only body catch-up
- ADS may tighten residual / response without owning the whole body
- expose ViewBodyResidual based on VisualBodyYaw
- stop using Actor-relative yaw as the final Look residual

### Gate

**Idle glance**

- stand still
- turn view approximately 30 deg
- pelvis / feet remain nearly fixed
- no body auto-follow after waiting

**Held side look**

- hold approximately 60-90 deg for several seconds
- body does not center just because time passed
- residual remains stable

**Movement**

- begin moving in a direction different from View
- visual body turns toward actual movement direction
- head / upper body can continue looking elsewhere

**Continuous spin**

- rotate view continuously right for multiple full revolutions
- body follows right
- no head left/right flip at +-180
- repeat left

**Anatomical limit**

- push View-Body residual toward approximately 100-110 deg
- body recovers enough range before impossible pose
- gaze target remains continuous

**ADS**

- ADS can reduce preferred residual
- no camera/input blocking

**Measured truth**

- calculated VisualBodyYaw matches measured pelvis/body visual orientation within expected graph offset

### Stop rule

VB-01 검수 전 VB-02로 자동 진행하지 않는다.

---

## VB-02 - Turn-in-Place

### Work

- inventory existing left/right 90/180 turn assets
- use project-owned assets / curves
- trigger only when VB-01 actually moves idle body target
- curve-driven yaw consumption if required
- gaze preservation during turn
- Inertialization
- foot lock only if measured necessary

### Gate

- continuous idle turn produces believable feet
- anatomical correction turn produces believable feet
- simply holding a side look does not trigger a turn
- head does not snap to center
- no visible foot skating beyond accepted threshold

### Stop rule

VB-02 검수 전 VB-03로 자동 진행하지 않는다.

---

## VB-03 - Look Distribution

### Work

- create / update `CR_LuxUpperBodyPresentation` Look pass
- source yaw from ViewBodyResidual
- head-first response
- neck second
- progressive upper-spine participation
- different response timing
- combined yaw / pitch anatomical envelope
- remove fixed additive 15/20/25 + 35/65 final chain
- preserve gaze while body turns underneath

### Gate

- small glance reads clearly as head-led
- medium side look adds neck / upper torso naturally
- 90+ deg look remains readable without grotesque twist
- no 160% over-rotation
- no roll contamination
- body turn naturally releases residual
- no common shared-response robotic motion

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
