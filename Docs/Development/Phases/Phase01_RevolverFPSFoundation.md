# Phase 01 - Revolver FPS Foundation

> 상태: Planned  
> 선행 Phase: Phase 00 - Multiplayer Foundation  
> 목표: PROJECT LUX의 유일한 기본 총기인 리볼버를 서버 권한 멀티플레이 구조로 구현하고, 실탄/공탄/고무탄의 숨겨진 탄종 구조와 즉사 규칙까지 검증한다.

---

# 1. 완료 목표

Phase 01 완료 시 다음이 가능해야 한다.

1. 모든 플레이어가 리볼버를 사용할 수 있다.
2. 리볼버는 6개의 Chamber를 가진다.
3. Chamber마다 Empty / Live / Blank / Rubber 상태를 서버가 보관한다.
4. 정확한 탄종 배열은 모든 Client에 복제하지 않는다.
5. Loaded / Empty 시각 정보는 필요한 범위에서만 동기화한다.
6. 1인칭 실제 가늠쇠로 조준할 수 있다.
7. 크로스헤어는 없다.
8. 실탄이 플레이어 어느 부위에 적중해도 즉사한다.
9. 공탄은 사격 연출은 발생하지만 살상 판정을 하지 않는다.
10. 고무탄은 탄도 적중을 판정하되 사망시키지 않는다.
11. 고무탄의 구체적인 제압/경직 효과는 아직 구현하지 않는다.
12. 빈 Chamber에서는 Dry Fire가 발생한다.
13. Fire / Aim / Reload / Cylinder 관련 1인칭 및 3인칭 애니메이션이 동기화된다.
14. Fire / Reload / Mechanical SFX가 동기화된다.
15. 2~6인 Listen Server에서 사격/사망이 일관되게 보인다.

Phase 완료 Milestone:

**6인 멀티플레이 환경에서 HUD 없이 리볼버로 조준/사격할 수 있고, 실탄/공탄/고무탄이 서버 권한으로 올바르게 처리된다.**


---

# 1A. Checkpoint 실행 계획

Phase 01도 한 번에 구현하지 않는다. 각 Checkpoint는 독립적으로 빌드/테스트하고 검수 통과 후 커밋한다.

## 공통 실행 규칙

Codex는 현재 Checkpoint만 구현한다.

- 다음 Checkpoint의 기능을 미리 작성하지 않는다.
- 현재 Checkpoint와 관계없는 리팩토링을 하지 않는다.
- 외부 에셋의 Demo Blueprint를 핵심 Gameplay truth로 사용하지 않는다.
- Server Authority와 Chamber secrecy 규칙을 훼손하지 않는다.
- 구현 후 변경 파일, 테스트 결과, 수동 Editor 작업, 알려진 문제를 보고하고 멈춘다.
- 검수 승인 후에만 커밋하고 다음 Checkpoint로 이동한다.

Checkpoint 완료 보고 형식:

1. Changed Files
2. Implemented
3. Tests Run
4. Test Result
5. Manual Editor Steps
6. Network Notes
7. Known Issues
8. Ready for Review

---

## 01-A - R21 Asset Integration Spike

### 목적

총기 코드를 작성하기 전에 구매한 R21 애니메이션을 실제 UE 5.8 프로젝트에서 검증한다.

### 대상

**Revolver one hand animations | First Person Gameplay**

### 범위

- UE 5.8 로드
- FP Skeleton 확인
- 포함 Revolver Mesh 확인
- Aim / Fire 확인
- Single Bullet Reload 확인
- Character / Revolver Control Rig 확인
- 포함 총기 SFX 확인
- FP 카메라 배치 시험
- TP 상체 재사용 가능성 빠른 확인

### 하지 않는 것

- ALuxRevolver 구현
- Chamber 상태
- Hitscan
- Death
- 네트워크 Fire

### 작업 순서

1. R21의 실제 Content 구조를 확인한다.
2. Skeleton / Animation / Control Rig / Revolver Mesh 경로를 기록한다.
3. 테스트 캐릭터에 FP arms를 붙인다.
4. Aim / Fire / Single Bullet Reload를 순서대로 재생한다.
5. 실제 Sight alignment를 확인한다.
6. 포함 Revolver Mesh 2개의 최종 사용 가능성을 평가한다.
7. 동일 상체 애니메이션을 Third-Person Character에 Retarget 또는 Preview하여 어깨/팔꿈치/손목 변형을 확인한다.
8. TP가 충분하면 별도 TP 리볼버 애니메이션 구매 없이 진행한다고 기록한다.
9. R21 Content에서 SoundWave / SoundCue / MetaSound 등 총기 관련 오디오 자산을 확인한다.
10. 최소한 Fire / Dry Fire / Hammer 또는 Trigger / Cylinder Open / Cylinder Close / Round Insert / Handling 계열이 있는지 기록한다.
11. 트레일러에서 들린 총기 사운드가 실제 배포 자산에 포함되어 있는지 확인한다.
12. 문제가 있으면 Control Rig 또는 별도 보강이 필요한 수준인지 기록한다.

### 검수 기준

- UE 5.8에서 에셋 로드 성공
- Aim / Fire 정상
- Single Bullet Reload를 프로젝트 구조에 사용할 수 있음
- FP 카메라에서 손/총 위치가 수정 가능한 범위
- Control Rig 접근 가능
- 포함 Revolver Mesh의 채택/교체 판단 가능
- TP 상체 재사용 가능성 판단 완료
- 총기 관련 SFX의 실제 포함 여부 확인 완료
- 포함 SFX로 Phase 01을 커버 가능한지 판단 완료

### 실행 결과 - 2026-08-31

- UE 5.8 자동 로드 통과: `/Game/RevolverFPGM` 아래 183개 Asset 확인
- FP Arms와 Aim / Fire / Single Bullet Reload가 동일 `SK_Mannequin_Arms_Skeleton` 사용
- `MCR_Base`, `MCR_Revolver` Control Rig 로드 확인
- `SKM_Revolver_base`, `SKM_Revolver_NoClip` 모두 로드되며 Single Bullet 방식에는 `NoClip`을 우선 후보로 채택
- SoundCue 20개, SoundWave 21개 확인: Fire / Trigger(Dry Fire 후보) / Cylinder / Round Insert / Equip·Inspect 범주 커버
- TP는 Casual01에 직접 연결하지 않고 IK Retarget + Upper Body Layer가 필요하며, 별도 TP 애니메이션은 우선 구매하지 않음
- 판정: **R21 채택 / 01-B 진행 가능**

### 커밋 게이트

연결용 프로젝트 Asset을 생성/수정한 경우 검수 후 커밋한다.

권장 커밋 메시지:

```text
chore: validate R21 revolver animation integration
```

---

## 01-B - Revolver Actor & Chamber State

### 목적

시각 연출과 분리된 서버 권한 리볼버 상태를 먼저 만든다.

### 범위

- ELuxRevolverRoundType
- ALuxRevolver
- 6 Chamber 고정 구조
- CurrentChamberIndex
- LoadedMask
- Cylinder open/closed 상태
- Equipped ownership
- Server authoritative round state
- 정확한 RoundType 비복제

### 하지 않는 것

- Hitscan
- Kill
- Reload animation
- Muzzle VFX
- 최종 SFX

### 작업 순서

1. ELuxRevolverRoundType을 정의한다.
2. ALuxRevolver를 Replicated Actor로 생성한다.
3. Server-only 6 Chamber 배열을 구현한다.
4. LoadedMask를 별도 계산한다.
5. CurrentChamberIndex를 구현한다.
6. bCylinderOpen 등 최소 공개 상태를 둔다.
7. Character에 EquippedRevolver 참조와 Attach 지점을 연결한다.
8. 개발용 검증 함수로 Server chamber를 설정하고 LoadedMask가 타입을 유출하지 않는지 확인한다.
9. 네트워크에서 RoundType 배열 자체가 복제되지 않는지 검증한다.

### 검수 기준

- 6 Chamber 고정
- Server와 Client의 공개 상태 일치
- Live / Blank / Rubber 정보가 Remote Client에 노출되지 않음
- Loaded / Empty만 필요한 범위에서 확인 가능
- 총 상태가 Character가 아니라 Revolver Actor에 남음

### 커밋 게이트

권장 커밋 메시지:

```text
feat: add replicated revolver chamber state
```

---

## 01-C - Server Fire, Round Resolution & Death

### 목적

탄종별 발사 결과와 실탄 즉사를 서버 권한으로 완성한다.

### 범위

- Fire Input
- ServerFire
- Fire cadence validation
- Server-side aim trace
- Live
- Blank
- Rubber
- Empty
- bIsDead
- Die()
- Death replication

### 하지 않는 것

- 완성형 Reload
- 최종 VFX/SFX
- Ragdoll 고도화
- Spectator

### 작업 순서

1. Fire 입력을 연결한다.
2. Client는 발사 요청만 보내고 HitResult를 권위 정보로 보내지 않게 한다.
3. Server가 장착/생존/Cylinder/Chamber/cadence를 검증한다.
4. Server view 기준 Hitscan을 수행한다.
5. Live가 Character에 적중하면 부위와 관계없이 Die()를 호출한다.
6. Blank는 살상 Trace 없이 Chamber만 소비한다.
7. Rubber는 Hitscan과 NonLethalHit event만 발생시킨다.
8. Empty는 Dry Fire event만 발생시킨다.
9. Dead Character의 후속 Fire를 거부한다.
10. Host→Client / Client→Host / Client→Client를 2~3인 테스트한다.

### 검수 기준

- Live: 어느 부위든 즉사
- Blank: 사망 없음
- Rubber: 적중 감지되나 사망 없음
- Empty: Dry Fire
- 모든 Chamber 소비/진행이 Server authoritative
- Dead player fire 차단
- 다른 Client에서도 사망 상태 동일

### 커밋 게이트

권장 커밋 메시지:

```text
feat: add authoritative revolver firing and lethal hits
```

---

## 01-D - Cylinder Reload & Hidden Ammo Flow

### 목적

한 발씩 장전하는 실제 리볼버 조작과 숨겨진 탄종 규칙을 연결한다.

### 범위

- Open Cylinder
- Insert one round
- Close Cylinder
- Reload cancel
- Single Bullet Reload animation 연결
- 개발용 Live / Blank / Rubber Load Driver
- LoadedMask 업데이트

### 하지 않는 것

- 맵 탄약 Pickup
- Inventory UI
- 정확한 시작 탄 수
- Speedloader

### 작업 순서

1. Cylinder Open 요청을 Server에서 검증한다.
2. 빈 Chamber에 한 발을 넣는 공통 Insert API를 만든다.
3. Live / Blank / Rubber 모두 동일 Insert API를 사용한다.
4. 개발용 Load Driver는 같은 API만 호출한다.
5. R21 Single Bullet Reload를 삽입 타이밍에 맞춰 연결한다.
6. 한 발 삽입 후 계속 장전 또는 취소 가능하게 한다.
7. Close Cylinder 후 Fire가 가능해지는지 확인한다.
8. Remote Client에는 탄종이 아니라 Loaded 상태만 전달되는지 재검증한다.

### 검수 기준

- 한 발씩 장전 가능
- 중간 취소 가능
- Chamber occupancy 정확
- 탄종 비밀 유지
- R21 Reload와 실제 Server insertion timing이 어긋나지 않음
- Debug driver가 Production Insert API를 우회하지 않음

### 커밋 게이트

권장 커밋 메시지:

```text
feat: add single round revolver reload flow
```

---

## 01-E - First-Person Presentation

### 목적

HUD 없이 실제 게임에서 사용할 1인칭 리볼버 조작 품질을 완성한다.

### 범위

- R21 FP arms
- Aim / ADS
- Fire animation
- Reload animation
- Dry Fire presentation
- Muzzle Flash
- Fire / mechanical SFX
- Camera/Sight alignment

### 하지 않는 것

- Third-Person 최종 조합
- Head IK
- Gameplay HUD
- Crosshair

### 작업 순서

1. FP Arms를 Local owner에게만 표시한다.
2. Revolver FP visual을 정렬한다.
3. Aim 입력과 Sight alignment를 연결한다.
4. Fire animation을 Local prediction cosmetic으로 연결한다.
5. Server result와 Remote cosmetic event가 중복 재생되지 않게 한다.
6. Reload / Cylinder 연출을 연결한다.
7. Muzzle Flash 후보를 검증한다.
8. 01-A에서 확인한 R21 포함 총기 SFX를 우선 사용해 Fire / Dry Fire / Cylinder / Load Round / Handling 사운드를 연결한다.
9. R21 포함 SFX가 실제로 부족한 항목에 한해서만 별도 사운드 자산을 보강한다.
10. Live / Blank / Rubber의 외부 발사 연출이 불필요하게 구분되지 않게 한다.

### 검수 기준

- Crosshair 없이 실제 Sight 조준 가능
- FP hands가 Remote Client에 보이지 않음
- Fire / Reload 시 손과 총 정렬 자연스러움
- Blank가 애니메이션만으로 쉽게 판별되지 않음
- HUD 없음
- Local Fire 반응 지연이 과도하지 않음

### 커밋 게이트

권장 커밋 메시지:

```text
feat: add first person revolver presentation
```

---

## 01-F - Third-Person Body, Upper-Body Blend & Head Look

### 목적

다른 플레이어가 보는 리볼버 행동을 별도 고가 에셋 없이 완성 가능한지 검증한다.

### 구조

```text
Lower-body Locomotion
        +
R21 Revolver Upper Body
        ↓
Layered Blend Per Bone
        +
Camera-driven Head / Neck Look
        ↓
Third-Person Final Pose
```

### 범위

- 기존 하체 Locomotion
- R21 Aim / Fire / Reload 상체 재사용
- Layered Blend Per Bone
- Head / Neck Look 또는 IK
- Remote Revolver Mesh
- Remote Fire / Reload presentation

### 하지 않는 것

- Foot IK 고도화
- Full-body procedural aiming
- Motion Matching
- 별도 TP 애니메이션 구매

### 작업 순서

1. Phase 00 Locomotion을 Base Pose로 유지한다.
2. R21 상체 애니메이션을 Retarget한다.
3. 적절한 Spine bone부터 Layered Blend한다.
4. Aim / Fire / Reload별 Blend 범위를 조정한다.
5. Camera pitch/yaw를 제한된 Head/Neck Look으로 변환한다.
6. 머리 회전이 팔/총 조준을 깨지 않는지 확인한다.
7. Remote Revolver Mesh를 손 Socket에 맞춘다.
8. 2 Player에서 서로 조준/사격/재장전하는 모습을 확인한다.

### 검수 기준

- 이동 중 상체 Aim 자연스러움
- Fire 시 팔 변형 허용 범위
- Reload 시 몸통/팔 동작 자연스러움
- 머리가 카메라 방향을 제한적으로 추적
- 하체 이동이 상체 애니메이션에 의해 깨지지 않음
- 별도 TP 팩 구매 필요 여부 결론 도출

### 커밋 게이트

권장 커밋 메시지:

```text
feat: add third person revolver animation layering
```

---

## 01-G - Six-Player Revolver QA & Phase Close

### 목적

Phase 01 전체를 6인 멀티플레이 기준으로 검증한다.

### 범위

- Live / Blank / Rubber / Empty
- Host / Client shooter combinations
- Reload
- Chamber secrecy
- FP/TP presentation
- 기본 latency simulation
- 로컬 6인 검증
- Steam 원격 2인 이상 실제 PC 검증
- Phase 문서 결과 기록

### 작업 순서

1. 2 Player 회귀 테스트를 수행한다.
2. 6 Player 환경을 실행한다.
3. Host→Client Live kill을 확인한다.
4. Client→Host Live kill을 확인한다.
5. Client A→Client B Live kill을 확인한다.
6. Blank를 동일 조합으로 확인한다.
7. Rubber non-lethal hit을 확인한다.
8. Empty Dry Fire를 확인한다.
9. 서로 다른 탄종 배열을 Server에 설정하고 Remote Client로 유출되지 않는지 확인한다.
10. 동시에 사격했을 때 Server 결과를 확인한다.
11. 50ms / 100ms 지연에서 기본 Fire flow를 확인한다.
12. FP/TP animation과 sound가 모든 관점에서 정상인지 확인한다.
13. Steam을 통해 최소 2대의 실제 PC 또는 동등한 원격 환경에서 동일 Fire / Reload / Death 흐름을 확인한다.
14. 원격 환경에서 Host→Client, Client→Host 사격과 사망 복제를 확인한다.
15. Completion Checklist를 갱신한다.
16. Result / Changed from Plan / Remaining을 작성한다.

### 검수 기준

- 6인에서 치명적인 네트워크 오류 없음
- 모든 실탄 적중 규칙 일관
- Blank / Rubber / Empty 규칙 일관
- Chamber secrecy 유지
- FP arms owner-only
- TP body 정상
- HUD / Crosshair 없음
- Steam 원격 2인 이상에서 리볼버 FPS 핵심 흐름 정상
- 다음 Phase로 넘어갈 수 있는 상태

### Phase 완료 커밋

권장 커밋 메시지:

```text
feat: complete revolver FPS foundation
```


---

# 2. 게임 규칙

## 2.1 총기 종류

PROJECT LUX의 기본 플레이어 총기는 리볼버 1종이다.

현재 Phase에서는 다른 무기 Type 확장을 전제로 범용 Weapon Framework를 만들지 않는다.

구현은 향후 확장이 불가능할 정도로 하드코딩하지 않되, 다음을 만들지 않는다.

- Weapon Type hierarchy
- Assault Rifle
- Shotgun
- Automatic Fire
- Magazine-fed gun abstraction
- Weapon attachment system
- Weapon rarity
- Weapon stat table

## 2.2 Chamber

고정 Chamber 수:

```text
6
```

논리 상태:

```text
Empty
Live
Blank
Rubber
```

권장 enum:

```text
ELuxRevolverRoundType
```

## 2.3 Live

- 플레이어 적중 시 즉사
- Hit bone / body part와 무관
- Headshot bonus 없음
- Armor 없음
- Damage number 없음
- Hit marker 없음
- Health bar 없음

총격 사망은 숫자 Damage를 크게 넣는 방식보다 명시적인 Lethal Hit 처리로 구현한다.

## 2.4 Blank

- 실제 발사와 가능한 한 동일한 Fire animation
- 가능한 한 동일한 Muzzle Flash
- 가능한 한 동일한 Gunshot sound
- Chamber 소비
- 플레이어 살상 Trace 없음
- Hit marker 없음

공탄이 시각적으로 쉽게 구분되면 블러핑 가치가 사라지므로 **Blank 전용 애니메이션이 명백히 티 나는 경우 사용하지 않는다.**

## 2.5 Rubber

- Fire presentation은 Live와 가능한 한 동일
- Server hitscan 수행
- 적중 대상 식별
- 플레이어를 죽이지 않음
- 구체적인 Stun / Knockdown / Drop / Damage 효과는 TBD
- Phase 01에서는 비살상 Hit Event를 발생시키고 개발 로그로 검증 가능
- Shipping Gameplay HUD에는 아무 정보도 표시하지 않음

## 2.6 Empty

- Trigger input 시 Dry Fire
- Dry Fire sound
- 사격 Trace 없음
- Muzzle Flash 없음
- 다음 Chamber로 진행 여부는 실제 선택한 리볼버 동작 방식에 맞춰 Asset 검증 후 확정하되, Phase 문서 변경 없이 임의 결정하지 않는다.

---

# 3. 숨겨진 탄종 정보

이 Phase의 중요한 네트워크 원칙이다.

## 3.1 Server Authority

정확한 Chamber 배열:

```text
[Live, Blank, Empty, Rubber, Live, Empty]
```

은 Server에서만 권위 있게 보관한다.

이를 일반 Replicated UPROPERTY 배열로 모든 Client에 보내지 않는다.

## 3.2 Client에게 공개할 수 있는 정보

다른 플레이어가 물리적으로 관찰 가능한 정보만 복제한다.

예:

- Cylinder open / closed
- Chamber loaded / empty occupancy
- 현재 무기가 발사되었는지
- Dry Fire 여부
- Cylinder visual rotation

정확한 Live / Blank / Rubber 타입은 복제하지 않는다.

## 3.3 장전한 플레이어의 지식

게임 시스템이 HUD로 기억을 보존해주지 않는다.

- 장전 당시 플레이어는 자신이 어떤 탄을 사용했는지 알 수 있다.
- 이후 탄종 배열을 HUD, Ammo Counter, Inspect UI로 다시 보여주지 않는다.
- 총을 넘겨받은 플레이어에게 탄종 정보를 자동 제공하지 않는다.
- 탄을 어디에서 얻었는지, 장전 장면을 누가 봤는지에 따라 다른 플레이어가 추론하는 것은 허용한다.

Phase 01에서는 실제 탄약 획득 시스템이 아직 없으므로 개발용 Load Command가 이 규칙을 테스트한다.

---

# 4. 클래스 구조

권장 최소 구조:

```text
ALuxCharacter
└─ EquippedRevolver : ALuxRevolver*

ALuxRevolver
├─ FirstPersonMesh
├─ ThirdPersonMesh
├─ Server chamber state
├─ Public visual state
└─ Fire / Aim / Reload state

ELuxRevolverRoundType
- Empty
- Live
- Blank
- Rubber
```

## 4.1 ALuxRevolver를 Actor로 두는 이유

리볼버는 Character Component에 완전히 묶지 않고 Replicated Actor로 둔다.

이유:

- 향후 Drop 가능
- 향후 다른 플레이어에게 전달 가능
- 숙주/기생물 제어 시 동일 Weapon Entity를 공유 가능
- Chamber 상태가 총 자체에 남음
- 사망 시 총 처리 확장 가능

Phase 01에서 Drop / Pickup은 구현하지 않는다.

---

# 5. ALuxRevolver 상태

## 5.1 Server-only

- 6 Chamber의 정확한 ELuxRevolverRoundType
- Fire validation
- Current chamber authoritative index
- Reload insertion validation
- Lethal / Non-lethal result

정확한 RoundType array에 Replicated 지정 금지.

## 5.2 Replicated visual state

필요 최소값만 복제한다.

후보:

- uint8 LoadedMask
- uint8 CurrentChamberIndex
- bool bCylinderOpen
- bool bIsAiming
- Fire event sequence / replicated event state

LoadedMask:

- 6bit 사용
- 각 Chamber가 Empty인지 Loaded인지 표현
- Round 종류는 표현하지 않음

## 5.3 Ownership

- 장착 Character가 Weapon owner
- Server가 attachment 상태 권한 보유
- Local owner용 FP Mesh는 OnlyOwnerSee
- TP Mesh는 OwnerNoSee 또는 Remote 표시용 설정

---

# 6. Fire Flow

## 6.1 Local Input

```text
Fire Input
→ local cosmetic response
→ ServerFire request
```

Local cosmetic은 체감 지연을 줄이기 위한 시각/애니메이션 반응만 담당한다.

사망과 Chamber 소비는 Server가 결정한다.

## 6.2 Server validation

Server는 최소 확인:

- Character alive
- Revolver equipped
- Fire 가능 상태
- Reload 중 충돌 여부
- Cylinder closed
- Chamber state
- Fire cadence
- Aim origin / direction validity

Trace origin:

- Server-side Pawn view location
- Server-side base aim rotation

Client가 임의의 World Hit Result를 보내고 Server가 그대로 신뢰하는 방식은 금지한다.

## 6.3 Round result

### Live

```text
ServerFire
→ Chamber Live
→ Hitscan
→ Character hit?
   → Yes: Kill
   → No: Environment impact
→ Chamber becomes Empty
→ Advance
```

### Blank

```text
ServerFire
→ Chamber Blank
→ Generic fire presentation
→ No lethal ballistic trace
→ Chamber becomes Empty
→ Advance
```

### Rubber

```text
ServerFire
→ Chamber Rubber
→ Hitscan
→ Character hit?
   → NonLethalHit event
→ Chamber becomes Empty
→ Advance
```

### Empty

```text
ServerFire
→ Chamber Empty
→ Dry Fire
```

---

# 7. Death Foundation

Phase 01에서는 총격 테스트에 필요한 최소 사망만 구현한다.

## ALuxCharacter

필요 상태:

- bIsDead replicated
- Server-only Die function
- Death cause 확장 지점

실탄 적중:

```text
Server Revolver Trace
→ Target ALuxCharacter
→ Target.Die()
→ bIsDead = true
```

사망 시:

- Character movement 중지
- Fire input 차단
- Collision 정리
- 모든 Client에 사망 상태 표시

Phase 01에서 관전자 전환은 하지 않는다.

Ragdoll을 넣는 경우:

- bIsDead replication을 기준으로 각 Client가 동일하게 시작
- Ragdoll Physics 자체를 복잡하게 Server replicate하는 구조는 피한다
- 안정성이 낮으면 Phase 01 완료 조건에서 제외 가능

---

# 8. Aim

HUD Crosshair는 사용하지 않는다.

Right Mouse 또는 프로젝트 Input Mapping의 Aim:

- Iron Sight / ADS
- FOV transition 가능
- Asset animation의 Aim-In / Aim-Out / Aim-Idle 사용
- Camera를 실제 Sight alignment에 맞춤

Aim 중에도 Server Trace는 Pawn view direction 기준.

No HUD:

- Crosshair 없음
- Spread indicator 없음
- Hit marker 없음
- Ammo count 없음

---

# 9. Reload Foundation

최종 게임은 한 발씩 장전하는 방향을 우선한다.

Phase 01에서는 Reload 시스템의 Production API를 구현하되 실제 Ammo Loot loop는 만들지 않는다.

필요 동작:

- Open Cylinder
- Insert one round
- Close Cylinder
- Reload cancel 가능 구조
- 6 Chamber occupancy 반영

## 9.1 개발용 Load Driver

Interaction / Inventory Phase 전까지 기능 검증용으로 비Shipping Exec Command 또는 Test Function을 허용한다.

예:

```text
LuxLoadRound Live
LuxLoadRound Blank
LuxLoadRound Rubber
```

규칙:

- Server가 실행/검증
- 실제 Chamber insertion 함수를 호출
- 별도 테스트 전용 Chamber 조작 코드를 만들지 않음
- 향후 Ammo Pickup 시스템은 동일 insertion API를 사용

Debug Command는 Shipping gameplay 기능이 아니다.

## 9.2 탄약 수량

정확한:

- 시작 탄 수
- Reserve 수
- 맵 Spawn 수
- 탄종 비율

은 Phase 01에서 확정하지 않는다.

이 수치는 사회적 QA가 가능한 시점에 결정한다.

---

# 10. Asset Validation

상세 Registry는 ../AssetPlan.md 참고.

## 10.1 Revolver one hand animations | First Person Gameplay

구매 완료한 R21 1인칭 리볼버 애니메이션 에셋을 사용한다.

Fab:
https://www.fab.com/listings/b638db29-63a3-4eca-80eb-1fc8d932f0b9

Phase 01의 우선 검증 대상:

1. UE 5.8 Import
2. FP animation skeleton
3. Aim / Fire
4. Single Bullet Reload
5. Character / Revolver Control Rig
6. 포함 Revolver Mesh
7. Third-Person upper-body 재사용
8. 다른 Revolver Mesh로 교체 가능한지
9. Local owner FP visibility
10. Remote player FP arms 비노출

사용 원칙:

- 이 에셋의 핵심 가치는 FP 팔 애니메이션이다.
- 포함 리볼버 Mesh는 별도 시각 검증 후 채택한다.
- R21 상체가 TP에서도 자연스러우면 별도의 3인칭 리볼버 애니메이션을 구매하지 않는다.
- TP 하체는 별도 Locomotion과 Layered Blend Per Bone으로 결합한다.
- Head / Neck 시선은 Camera direction 기반 Look / IK로 처리한다.


## 10.2 Animation Starter Pack

- lower-body locomotion
- R21 upper-body와 Layered Blend
- 필요한 애니메이션만 Retarget

## 10.3 Muzzle Flash Niagara

1차 무료 후보:
https://www.fab.com/listings/435b3bcb-d7f5-467d-99aa-2edc97a6c5fd

검증:

- 어두운 실내에서 품질
- Revolver muzzle size
- Point-light 느낌 필요 여부
- Multiplayer cosmetic spawn

Live / Blank / Rubber의 외부 Fire presentation은 최대한 동일하게 유지한다.

## 10.4 Realistic Starter VFX Pack Vol 2

보유 에셋.

사용 후보:

- Blood
- Hit
- Metal sparks

검증:

- Particle technology
- UE 5.8
- 성능
- 실제 게임 미술 방향

Muzzle Flash source로 가정하지 않는다.

---

# 11. Animation Layer

Remote Character:

```text
Lower Body Locomotion
        +
Revolver Upper Body Pose
        ↓
Layered Blend Per Bone
        ↓
Third Person Output
```

필요 상태:

- Idle with Revolver
- Aim
- Fire
- Reload
- Cylinder manipulation

Character locomotion 자체는 Phase 00 상태를 유지한다.

Animation Blueprint가 Weapon chamber truth를 보관하지 않는다.

C++ state를 읽어 표현만 한다.

---

# 12. Audio

R21 트레일러에는 총기 사운드가 포함되어 있었으므로 **별도 리볼버 SFX 팩을 선구매하지 않는다.**

다만 트레일러용 별도 후처리/오디오일 가능성은 배제할 수 없으므로, 01-A에서 실제 배포 Content 안의 오디오 자산을 확인한 뒤 확정한다.

Phase 01에서 필요한 최소 범주:

- Revolver Fire
- Dry Fire
- Hammer / Trigger 또는 이에 준하는 mechanical click
- Cylinder Open
- Cylinder Close
- Load Round
- Basic handling

우선순위:

1. R21 포함 SFX 사용
2. 부족한 항목만 개별 보강
3. R21 SFX가 전반적으로 부적합한 경우에만 별도 총기 SFX 팩 검토

PROJECT LUX에서 Gunshot은 향후 Parasite perception 정보원이 되므로 다음을 고려한다.

- 3D spatial sound
- Attenuation Asset 분리
- Sound category 명확화
- 다른 Gameplay 코드가 Gunshot event를 구독할 수 있는 확장 지점

Phase 01에서 Parasite hearing 자체는 구현하지 않는다.

---

# 13. 구현하지 않는 것

- 추가 총기
- Speedloader
- 완성형 Ammo Loot
- Ammo inventory UI
- Weapon inventory
- Weapon swapping
- Drop / Pickup
- Gun ownership transfer
- Rubber stun 세부
- Armor
- Health HUD
- Headshot
- Damage falloff
- Penetration
- Ricochet
- Lag compensation
- Rewind hit validation
- Destructible lights
- Parasite control
- Host control
- Spectator
- Gameplay HUD
- Crosshair
- Hit marker

---

# 14. 네트워크 테스트 Matrix

각 테스트는 Server / Client 양쪽 조합으로 반복한다.

## 14.1 Live

- Host shoots Client
- Client shoots Host
- Client A shoots Client B
- Body hit
- Arm/leg hit
- 모든 경우 즉사
- Shooter 외 다른 Client에서도 동일 사망 확인

## 14.2 Blank

- Host fire
- Client fire
- Muzzle / sound 정상
- 대상 사망 없음
- 다른 Client가 Fire를 동일하게 인지
- 탄종을 UI로 알 수 없음

## 14.3 Rubber

- Host hits Client
- Client hits Host
- Hit registration 확인
- 사망 없음
- 개발 로그에서 NonLethalHit 확인
- 다른 Client에게 탄종 UI 없음

## 14.4 Empty

- Dry Fire
- Muzzle 없음
- Kill 없음
- chamber truth 변화 없음

## 14.5 Chamber secrecy

Server chamber:

```text
Live / Blank / Rubber / Empty / Live / Blank
```

검증:

- Remote Client에 RoundType 배열이 replicate되지 않음
- LoadedMask만 필요한 경우 표시
- Developer network inspection에서 불필요한 RoundType payload 없음

## 14.6 Simultaneous action

- 두 Client가 서로 동시에 사격
- Server authoritative 결과 확인
- Dead player의 후속 Fire request 거부

## 14.7 Latency basic

가능하면 Packet Simulation으로:

- 50ms
- 100ms
- packet loss 소량

에서 기본 Fire flow가 깨지지 않는지 확인.

정교한 lag compensation은 후순위.

---

# 15. Completion Checklist

## Code

- [ ] ELuxRevolverRoundType
- [ ] ALuxRevolver replicated actor
- [ ] 6 Chamber server state
- [ ] RoundType non-replication
- [ ] LoadedMask replication
- [ ] Current chamber state
- [ ] ServerFire
- [ ] Hitscan
- [ ] Live lethal
- [ ] Blank non-lethal/no trace
- [ ] Rubber non-lethal hit event
- [ ] Dry Fire
- [ ] bIsDead
- [ ] Death replication
- [ ] Dead player fire blocked
- [ ] Aim input
- [ ] No crosshair
- [ ] Debug round insertion driver
- [ ] Reload production API

## Asset

- [x] R21 Personal 라이선스 구매 완료
- [x] R21 UE 5.8 import
- [x] Revolver mesh 검증
- [ ] FP animations 연결
- [ ] TP animations 연결
- [ ] lower-body layer 연결
- [ ] Fire SFX
- [ ] Reload/mechanical SFX
- [ ] Muzzle Flash
- [ ] Impact/Blood candidate 검증

## Multiplayer

- [ ] Host → Client Live kill
- [ ] Client → Host Live kill
- [ ] Client → Client Live kill
- [ ] Blank 검증
- [ ] Rubber 검증
- [ ] Dry Fire 검증
- [ ] Chamber secrecy 검증
- [ ] 6 Player 테스트
- [ ] HUD 없음

---

# 16. 완료 후 기록

## Result

미작성.

## Status

Planned.

## Commit

미작성.

## Implemented

미작성.

## Changed from Plan

미작성.

## Remaining

미작성.
