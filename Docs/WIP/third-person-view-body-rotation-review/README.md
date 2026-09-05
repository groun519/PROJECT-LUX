# Third-Person View / Body Rotation Failure Review

> Status: **Diagnosis complete / recovery design open**
>
> Date: 2026-09-05 KST
>
> Stable implementation reference: `main@0275e299b9400488e7770f322f285f3b1e82792c`
>
> Experimental implementation: local `main` worktree only, **uncommitted and currently visually broken**
>
> Scope: third-person idle view lead, body orientation, Root Yaw Offset, head/neck/spine aim, directional locomotion
>
> Purpose: 현재 실패 상태와 측정 근거를 CHAT에 전달해 다음 구현 방향을 논의한다.

---

# 0. Reading rule

이 문서는 코드 스냅샷이 아니다.

- **Fact**는 코드, 에디터 그래프 또는 자동 측정으로 확인한 내용이다.
- **Requirement**는 플레이 경험과 프로젝트 경계에서 반드시 지켜야 하는 조건이다.
- **Candidate**는 가능한 원인이지만 아직 격리 실험으로 증명하지 않은 내용이다.
- **Experiment**는 로컬 작업 트리에서 시도했으나 아직 채택하지 않은 구현이다.
- `main@0275e299` 이후의 view/body rotation 변경은 커밋되지 않았다.
- 따라서 이 문서를 보고 현재 실험 코드를 안정 구현으로 간주하거나 그대로 병합하면 안 된다.
- 이 문서의 목표는 패치를 제안하는 것이 아니라 **문제 경계, 확인된 증거, 다음 검증 순서**를 고정하는 것이다.

---

# 1. Intended player-facing behavior

## Requirement - View leads, body follows context

원하는 최종 인상은 다음과 같다.

1. 정지 상태에서 카메라를 조금 돌리면 시선, 머리, 상체가 먼저 반응한다.
2. 몸 전체가 카메라와 동시에 같은 각도로 기계적으로 회전하지 않는다.
3. 일정 각도를 넘었다는 이유만으로 몸이 즉시 목표 각도까지 스냅하지 않는다.
4. 이동을 시작하면 이동 의도와 방향에 맞춰 몸이 자연스럽게 정렬된다.
5. 기존 WASD 방향별 Blend Space가 계속 의미 있게 작동해야 한다.
6. 큰 정지 회전은 나중에 실제 Turn-in-Place 애니메이션과 발 고정으로 고도화할 수 있다.

이 기능은 게임의 핵심 규칙 자체는 아니지만, 최종적으로는 하이엔드한 FPS 리볼버 표현의 일부가 된다. 다만 지금 단계에서는 완성형 Turn-in-Place 프레임워크를 한 번에 만들지 않고, 조정 가능한 최소 구조와 올바른 좌표계 계약부터 확립한다.

## Requirement - Existing presentation must survive

다음 이미 작동하는 기능을 깨면 안 된다.

- `BS_Lux_Locomotion`의 Idle / Forward / Backward / Left / Right 이동 표현
- R21 기반 First-Person 리볼버 표현
- Third-Person 리볼버 상체 Aim과 오른손 IK
- 총구 화염과 muzzle socket 정렬
- Manual Chamber Reload와 실린더/탄환 표시
- 사용자가 직접 정리한 `ABP_LuxCharacter` 그래프 배치

## Requirement - Project boundaries

- `PROJECT-MA`는 참고만 하며 절대 수정하지 않는다.
- R21 및 Marketplace 원본 에셋은 수정하지 않는다.
- 프로젝트 소유 변경은 `Content/LUX`와 `Source/PROJECT_LUX` 안에서만 수행한다.
- smoothing, dead zone, 추가 interpolation으로 현재 오류를 숨기지 않는다.
- Rifle/Shotgun까지 예상한 generic locomotion 또는 weapon framework를 만들지 않는다.
- 현재 결과를 완성으로 선언하지 않는다. 이후 값 조정과 세부 polish가 예정되어 있다.

---

# 2. Stable baseline

## Fact

안정 기준은 `main@0275e299`다.

이 기준에는 다음이 있다.

- 방향성 locomotion Blend Space
- Third-Person revolver upper-body pose
- spine / neck / head procedural aim
- right-hand aim correction
- Manual Chamber Reload

이 기준에는 이번 Root Yaw Offset 실험이 없다.

현재 로컬 `main` 작업 트리에는 아래 파일이 변경된 상태다.

```text
Modified
  Content/LUX/Animation/Locomotion/ABP_LuxCharacter.uasset
  Content/LUX/Animation/Locomotion/BS_Lux_Locomotion.uasset
  Source/PROJECT_LUX/Private/Player/LuxCharacter.cpp
  Source/PROJECT_LUX/Private/Player/LuxCharacterAnimInstance.cpp
  Source/PROJECT_LUX/Public/Player/LuxCharacter.h
  Source/PROJECT_LUX/Public/Player/LuxCharacterAnimInstance.h

Untracked experiment
  Source/PROJECT_LUX/Private/Player/LuxViewBodyRotationComponent.cpp
  Source/PROJECT_LUX/Public/Player/LuxViewBodyRotationComponent.h

Local-only, never stage
  Config/DefaultEditor.ini
```

`BS_Lux_Locomotion.uasset`에는 사용자가 수행한 유효한 조정이 함께 있을 수 있으므로, 실험 제거 과정에서 파일 단위로 무작정 되돌리면 안 된다.

---

# 3. Experiment chronology

## Experiment 1 - Controller-facing Actor plus Root Yaw Offset

검토한 방향은 Lyra 계열의 기본 아이디어였다.

```text
Controller / View rotates
        |
        v
Actor follows controller yaw immediately
        |
        v
AnimGraph counter-rotates visual root while idle
        |
        v
Head / torso aim relative to visual body
```

캐릭터는 controller yaw를 따르고, 정지 중 visual body는 `RootYawOffset`으로 반대 회전시켜 화면 회전과 몸 회전을 분리하려 했다.

## Experiment 2 - `ULuxViewBodyRotationComponent`

로컬 실험 컴포넌트가 다음 상태를 계산한다.

```text
ELuxViewBodyTurnState
  Aligned
  Offset
  TurnRequested
  Turning

ELuxRootYawOffsetMode
  Accumulate
  Hold
  BlendOut
```

현재 실험값은 다음과 같다.

- idle actor yaw delta를 반대로 누적
- maximum root yaw offset: `90 degrees`
- idle turn request angle: `60 degrees`
- 이동 시작 후 짧게 hold
- 이후 responsiveness `8`로 offset blend-out
- Actor는 controller/view yaw를 즉시 따름

네트워크 쪽은 중간에 여러 presentation smoothing을 시도했으나, 사용자의 요구에 따라 불필요한 계층을 제거했다. 남긴 최소 개념은 absolute `ReplicatedVisualBodyYaw`를 보내고 simulated proxy가 다음처럼 상대 offset을 구하는 방식이다.

```text
RemoteRootYawOffset = ReplicatedVisualBodyYaw - ActorYaw
```

이 네트워크 설계도 아직 채택된 것이 아니다. 로컬 pose 계약이 증명되기 전에는 평가 대상이 아니다.

## Experiment 3 - AnimInstance integration

`ULuxCharacterAnimInstance`에 다음 값이 노출되었다.

```text
RootYawOffset
RootYawOffsetMode
BodyTurnDirection
BodyTurnState
```

기존 aim delta에는 다음 보정이 추가되었다.

```cpp
AimDelta.Yaw = NormalizeAxis(AimDelta.Yaw - RootYawOffset);
```

의도는 visual root가 Actor를 반대 회전한 만큼, 목/상체가 visual body를 기준으로 카메라를 향하도록 하는 것이었다.

## Experiment 4 - AnimGraph `Rotate Root Bone`

처음 사용자 캡처를 확인했을 때 `Rotate Root Bone` 노드는 실제 그래프에 없었다. 이후 자동화로 노드를 추가하고 `RootYawOffset`을 Yaw에 연결했다.

현재 실험 그래프의 큰 순서는 다음과 같다.

```text
BS_Lux_Locomotion
  -> Layered Blend per Bone (Revolver upper body)
  -> Rotate Root Bone (Yaw = RootYawOffset)
  -> Local To Component
  -> spine / right-hand / neck / head Transform Bone chain
  -> Component To Local
  -> Output Pose
```

노드 연결과 변수 binding은 에셋 재로드 후에도 존재하는 것으로 확인했다.

실험 전 `ABP_LuxCharacter` 백업은 로컬 ignored 경로에 있다.

```text
Saved/Codex/ABP_LuxCharacter.before_root_yaw.uasset
```

이 백업과 자동 진단 결과는 mockup 브랜치에는 포함하지 않는다.

---

# 4. Observed failures

## Failure A - Initial double rotation

`Rotate Root Bone`이 실제 그래프에 없던 시점의 증상이다.

- 화면을 약 `20 degrees` 돌림
- Actor/몸도 약 `20 degrees` 회전
- neck/head procedural aim이 다시 같은 방향으로 반응
- 관찰상 머리가 총 `40 degrees` 가까이 돌아간 것처럼 보임

이 증상은 Actor 회전과 procedural look이 중복 적용된 결과로 설명할 수 있었다.

## Failure B - Shaking after adding Rotate Root Bone

노드를 넣은 뒤, 정지 상태에서 고개만 돌릴 때 미세하게 떨리는 현상이 관찰되었다.

그러나 자동 측정상 controller yaw, actor yaw, `RootYawOffset` 값은 안정적이었다. 따라서 이 증상을 즉시 replication smoothing 또는 입력 노이즈 문제로 분류하면 안 된다.

## Failure C - Neck twists abnormally

현재 가장 중요한 실패다.

- 몸과 머리의 기준이 일치하지 않는다.
- 목이 자연스러운 yaw lead가 아니라 비정상적으로 꺾이거나 비튼다.
- 어느 계층부터 잘못되었는지 복합 그래프만 보고 판단하기 어렵다.

이 때문에 spine, hand IK, neck, head가 모두 켜진 상태에서 계속 수치를 조정하는 접근은 중단했다.

---

# 5. Automated evidence

## Fact - Build and basic policy test

- Unreal Engine 5.8 Development Editor C++ build: pass
- local numeric policy test: pass on confirmed rerun
- example results:

| Input | Expected root offset | Measured result |
|---|---:|---:|
| Actor yaw `+40` | `-40` | `-40` |
| Actor yaw `+100` | clamped | `-90` |
| movement begins | approaches `0` | approximately `-1.29` at sampled point |

첫 자동 실행에서는 초기 `+40` 구간을 PIE 준비 전에 샘플링해 `0`을 읽었으나, 동일 테스트 재실행은 통과했다. 이것은 현재 visual pose 실패의 증거로 사용하지 않는다.

## Fact - Smooth-turn stability probe

로컬 controller를 `0 -> 30 degrees`까지 2초 동안 부드럽게 돌리고 마지막 구간을 hold했다.

진단 파일은 로컬 ignored 경로에 있다.

```text
Saved/Codex/phase01_root_yaw_stability.py
Saved/Codex/phase01_root_yaw_stability.json
```

핵심 결과:

| Metric | Result |
|---|---:|
| maximum computed body-anchor error | approximately `2.84e-14` |
| settled `RootYawOffset` span | `0` |
| settled pelvis yaw span | `0` |
| settled head yaw span | `0` |
| maximum reverse head step during turn | `0` |

최종 샘플 근처 값:

| Value | Start / target relationship |
|---|---:|
| target view yaw | `30` |
| measured view yaw | approximately `29.98 - 30` |
| measured actor yaw | approximately `29.99 - 30` |
| component root offset | approximately `-29.99 - -30` |
| AnimInstance root offset | approximately `-29.98 - -30` |
| computed visual body yaw | `0` |

정책 수식만 보면 다음이 정확히 성립한다.

```text
ActorYaw + RootYawOffset = VisualBodyYaw = 0
```

## Fact - Actual pose contradicts computed visual body

같은 테스트에서 실제 bone world yaw는 다음처럼 변했다.

| Bone | Start | End | Delta |
|---|---:|---:|---:|
| pelvis | approximately `101.034` | approximately `131.034` | `+30` |
| head | approximately `43.547` | approximately `73.547` | `+30` |

또한 샘플한 `root` component-space rotation은 테스트 내내 `0`이었다.

따라서 현재 확인된 사실은 다음과 같다.

1. `RootYawOffset` 계산은 안정적이다.
2. 값은 AnimInstance까지 정확히 도착한다.
3. 수식상 visual body yaw는 고정되어야 한다.
4. 실제 pelvis는 Actor와 함께 `+30 degrees` 회전했다.
5. 즉, 현재 AnimGraph에서 기대한 root counter-rotation이 실제 visual body에 적용되지 않았다.
6. 그런데 neck/head 계산은 counter-rotation이 성공했다는 전제로 `AimDelta - RootYawOffset`을 사용한다.
7. 이 잘못된 전제가 neck/head over-correction 또는 twist를 만들 수 있다.

## Diagnosis boundary

현재 증거로는 문제의 1차 경계가 다음 쪽이다.

```text
Not primarily:
  input smoothing
  controller yaw noise
  RootYawOffset numeric accumulation
  replicated proxy interpolation

Investigate first:
  AnimGraph node execution
  root/pelvis application
  mesh-to-component axis contract
  coordinate-space and node ordering
```

---

# 6. What is not yet proven

아래는 모두 **Candidate**이며 사실로 단정하면 안 된다.

## Candidate A - `MeshToComponent` axis mismatch

`Rotate Root Bone`은 delta rotation을 `MeshToComponent`로 변환해 root bone rotation에 적용한다. 현재 Character mesh는 Actor에 대해 yaw `-90 degrees`의 relative rotation을 사용한다.

이 skeleton/mesh에서 `MeshToComponent` 기본값 또는 설정값이 기대 축과 맞지 않을 가능성이 있다. 하지만 고정 `+30/-30` 격리 실험을 아직 하지 않았으므로 원인으로 확정하지 않는다.

## Candidate B - Runtime graph path or binding issue

노드와 `RootYawOffset` 연결은 에셋 구조상 확인되었다. 그래도 실제 runtime path, exposed pin evaluation, compile 결과가 기대와 다를 가능성은 남아 있다.

## Candidate C - Wrong bone/space measurement

테스트에서 샘플한 `root` bone 이름 또는 component-space 해석이 이 skeleton에 적절하지 않을 가능성이 있다. 다만 pelvis world yaw가 Actor와 정확히 함께 변한 결과는 visual body counter-rotation이 실패했다는 판단을 강하게 지지한다.

## Candidate D - Node ordering conflict

`Layered Blend per Bone`, `Rotate Root Bone`, `Local To Component`, 연속 `Transform (Modify) Bone` 사이의 실행 순서 또는 space 변환이 기대와 다를 수 있다.

## Candidate E - Current procedural neck/head method is unsuitable

여러 `Transform Bone`으로 spine, neck, head 회전을 순차 가산하는 현재 방식 자체가 Root Yaw Offset과 결합할 때 불안정할 수 있다. Aim Offset, additive pose, Control Rig 또는 다른 분배 방식이 더 적절할 가능성이 있다. 이 또한 root-only 경로가 통과한 뒤 비교해야 한다.

---

# 7. Recommended recovery sequence

한 번에 완성하려 하지 않고 아래 gate를 순서대로 통과한다. 앞 단계가 실패하면 뒤 계층을 추가하지 않는다.

## Stage 0 - Preserve and restore stable baseline

### Codex work

- 현재 broken experiment의 diff와 진단 결과를 보존한다.
- `Saved/Codex/ABP_LuxCharacter.before_root_yaw.uasset`을 기준으로 실험 전 AnimBP를 복구한다.
- view/body experimental source와 `AimDelta -= RootYawOffset`만 제거한다.
- 사용자의 `BS_Lux_Locomotion` 변경과 기존 revolver IK/reload는 보존한다.

### User visual gate

- 기존 이동 Blend Space가 정상인지 확인
- 기존 Third-Person aim이 기준 상태로 돌아왔는지 확인
- 비정상 neck twist가 사라졌는지 확인

### Pass condition

`main@0275e299`의 기존 기능을 잃지 않은 명확한 baseline.

## Stage 1 - Root axis diagnostic asset

### Codex work

- production `ABP_LuxCharacter`를 바로 수정하지 않는다.
- diagnostic duplicate AnimBP 또는 최소 test graph를 만든다.
- 그래프를 다음 하나로 제한한다.

```text
Base Pose -> Rotate Root Bone -> Output Pose
```

- spine / neck / head / hand Modify Bone을 모두 끈다.
- runtime variable 대신 고정 yaw `+30`과 `-30`을 각각 넣는다.
- `MeshToComponent`, 축, 부호를 한 변수씩 바꿔 실제 pelvis world orientation을 측정한다.

### User visual gate

- pelvis/body가 정확히 의도한 yaw 축으로만 도는지 확인
- pitch/roll 오염, 비틀림, 떨림이 없는지 확인

### Pass condition

- 고정 입력의 크기와 실제 body yaw delta가 일치
- 부호가 양방향에서 대칭
- hold 중 bone transform이 완전히 안정

이 gate 전에는 production graph에 root separation을 다시 넣지 않는다.

## Stage 2 - Root-only idle separation

### Codex work

- 검증한 축/공간 계약만 production AnimBP에 적용한다.
- 정지 상태에서 Actor yaw와 visual body yaw 분리만 구현한다.
- neck, head, spine aim은 baseline 값 또는 zero로 둔다.
- 우선 작은 범위 `+-45 degrees`만 검증한다.

### User visual gate

- 카메라를 돌려도 골반과 발 방향이 고정되는지 확인
- body가 반대로 회전하거나 떠는 현상이 없는지 확인

### Pass condition

실제 measured pelvis/body orientation이 계산된 visual body orientation과 일치.

## Stage 3 - Head and neck only

### Codex work

- look yaw를 Actor가 아니라 **검증된 actual visual body direction** 기준으로 계산한다.
- 먼저 neck 하나만 추가하고 확인한다.
- 이후 head를 추가한다.
- yaw/pitch 분배와 clamp는 데이터로 노출하되 과도한 tuning은 하지 않는다.

### User visual gate

- 작은 회전에서 고개가 먼저 자연스럽게 반응하는지 확인
- roll 오염, 목 twist, 이중 회전이 없는지 확인

### Pass condition

view yaw 변화와 실제 head direction이 단조롭게 일치하고, hold 중 떨림이 없다.

## Stage 4 - Spine and right-hand IK

### Codex work

- spine을 한 bone씩 다시 추가한다.
- 각 단계에서 누적 회전량을 측정한다.
- 마지막에 right-hand aim IK를 복구한다.

### User visual gate

- 총구가 target을 유지하는지 확인
- 상체가 과회전하지 않는지 확인
- 손목/팔이 튀지 않는지 확인

### Pass condition

각 계층을 켰을 때 이전 계층의 결과가 변하지 않고, target alignment가 유지된다.

## Stage 5 - Directional locomotion integration

### Codex work

- 기존 `BS_Lux_Locomotion`을 그대로 사용한다.
- 이동 시작 시 body alignment 규칙을 root-only 검증 경로 위에 얹는다.
- WASD 입력 방향을 body yaw로 강제 치환하지 않는다.
- 이동 방향 Blend Space와 visual body alignment의 책임을 분리한다.

### User visual gate

- strafe / backward animation이 사라지지 않는지 확인
- 이동 시작 시 몸이 목표 yaw로 기계적으로 스냅하지 않는지 확인
- 정지 전환 시 발과 골반이 튀지 않는지 확인

### Pass condition

방향성 이동 pose를 보존하면서 body가 이동 문맥에 맞춰 자연스럽게 정렬된다.

## Stage 6 - Network only after local proof

### Codex work

- 2-player PIE에서 owner와 simulated proxy를 비교한다.
- 필요 최소 state만 복제한다.
- smoothing은 패킷 샘플링 문제를 실제로 측정한 뒤에만 추가한다.

### User visual gate

- 다른 클라이언트에서 body/head/weapon 방향이 동일한 의도로 보이는지 확인
- late join 또는 잠깐의 packet delay 후 pose가 회복되는지 확인

### Pass condition

로컬에서 증명한 pose 계약이 remote proxy에서도 유지된다.

## Commit policy

각 stage는 자동 측정과 사용자 visual gate를 모두 통과한 뒤 별도 checkpoint commit으로 남긴다. 실패한 실험을 다음 stage와 한 commit에 섞지 않는다.

---

# 8. Questions for CHAT

다음 질문에 대해 구체적인 AnimGraph 순서, 좌표계, 검증법 중심의 답을 원한다.

1. Controller-facing Actor와 cardinal strafe Blend Space를 유지하면서 정지 view/body separation을 구현하는 최소 구조는 무엇인가?
2. 이 skeleton/mesh에서 `Rotate Root Bone`의 실제 축과 `MeshToComponent` 계약을 가장 작은 AnimBP로 어떻게 증명해야 하는가?
3. `Rotate Root Bone`이 적합하지 않다면 root/pelvis `Transform Bone`, additive animation, Aim Offset, Control Rig 중 어떤 대안이 현재 범위에 가장 작고 안전한가?
4. 아래 순서가 올바른가? 아니라면 어떤 순서와 space conversion이어야 하는가?

```text
Locomotion
-> Layered upper body
-> Root yaw correction
-> Local To Component
-> spine / hand / neck / head correction
-> Component To Local
```

5. sequential `Transform Bone` 대신 Aim Offset 또는 additive pose를 사용해야 neck/head 누적 회전과 roll 오염을 피할 수 있는가?
6. actual Turn-in-Place animation과 foot locking은 어느 acceptance gate 뒤에 도입해야 하는가?
7. 로컬 pose가 증명된 후 network에서는 absolute visual body yaw, relative root yaw offset, 또는 별도 facing state 중 무엇을 authority/state truth로 두는 것이 적절한가?
8. pelvis world yaw는 Actor와 함께 `+30` 변했는데 `root` component-space sample은 `0`이었다. 이 측정 조합에서 먼저 의심해야 할 skeleton hierarchy 또는 Unreal bone-space 해석 오류는 무엇인가?

답변에는 가능하면 다음을 포함해 달라.

- 최소 재현 graph
- 각 노드의 coordinate space
- 이 mesh의 actor-relative yaw `-90 degrees`를 처리하는 방식
- 고정 `+-30 degrees` 입력에서 기대할 bone transform
- 어느 bone을 world/component/local space에서 측정해야 하는지
- production graph로 되돌리는 순서

---

# 9. Do not recommend yet

현재 단계에서 다음은 해결책으로 제안하지 않는다.

- jitter를 숨기기 위한 smoothing 계층 추가
- 더 큰 dead zone 또는 clamp만 추가
- root 적용이 실패한 상태에서 neck/head 수치 tuning
- 모든 문제를 한 Control Rig 또는 generic animation framework로 즉시 교체
- Turn-in-Place animation set이 없는 상태에서 완성형 foot-lock 시스템 구축
- R21 원본 에셋 수정
- `PROJECT-MA` 수정
- 기존 Blend Space 삭제 또는 WASD 방향 애니메이션 무력화
- 현재 broken experiment의 커밋/병합

---

# 10. Existing references

공식 Unreal 문서에서 확인한 관련 개념:

- [FAnimNode_RotateRootBone API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AnimGraphRuntime/FAnimNode_RotateRootBone)
- [Animation in Lyra Sample Game](https://dev.epicgames.com/documentation/unreal-engine/animation-in-lyra-sample-game-in-unreal-engine)

Lyra의 Root Yaw Offset 개념은 방향 참고일 뿐, PROJECT-LUX의 skeleton/graph에서 노드 계약이 검증되었다는 증거가 아니다.

---

# 11. Desired decision output

CHAT 논의 결과는 거대한 최종 설계가 아니라 다음 네 가지로 좁히는 것이 좋다.

1. Stage 1 diagnostic graph의 정확한 노드/space/값
2. root 또는 pelvis를 측정할 정확한 방법
3. Stage 2 production root-only graph의 최소 형태
4. Stage 3에서 neck/head aim을 visual body 기준으로 계산하는 식

이 네 항목이 합의되면 Codex가 단계별로 구현하고, 사용자가 각 단계의 시각 결과를 짧게 판정한다.
