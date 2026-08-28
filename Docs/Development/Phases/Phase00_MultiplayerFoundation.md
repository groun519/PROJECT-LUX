# Phase 00 - Multiplayer Foundation

> 상태: Planned  
> 엔진: Unreal Engine 5.8  
> 목표: PROJECT LUX의 모든 후속 시스템이 올라갈 최소 멀티플레이 Character / Session 기반을 만든다.

---

# 1. 완료 목표

Phase 00 완료 시 다음이 가능해야 한다.

1. 한 플레이어가 Listen Server 세션을 생성할 수 있다.
2. 다른 플레이어가 세션을 검색하고 참가할 수 있다.
3. 최소 6개의 Player Spawn을 지원한다.
4. 각 플레이어가 자신의 Character를 Possess한다.
5. 모든 플레이어의 위치와 회전이 정상 복제된다.
6. 1인칭 카메라로 이동하고 바라볼 수 있다.
7. 다른 Client에서는 해당 플레이어의 3인칭 Character가 보인다.
8. PlayerState가 접속/퇴장 과정에서 정상 유지/정리된다.
9. Host 종료 시 세션이 정상 종료된다.
10. Phase 01에서 Revolver Actor를 붙일 위치와 애니메이션 기반이 준비된다.

Phase 완료 Milestone:

**6인 Listen Server에서 각 플레이어가 같은 시설 공간을 돌아다니며 서로의 캐릭터를 정상적으로 볼 수 있다.**


---

# 1A. Checkpoint 실행 계획

Phase 00은 한 번에 구현하지 않는다. 아래 Checkpoint를 순서대로 하나씩 완료하고, 매 단계에서 검수 후 커밋한다.

## 공통 실행 규칙

Codex는 현재 Checkpoint만 구현한다.

- 다음 Checkpoint의 코드를 미리 작성하지 않는다.
- 현재 Checkpoint 범위를 벗어난 리팩토링을 하지 않는다.
- 구현 후 빌드 또는 지정 테스트를 수행한다.
- 변경 파일 목록, 구현 요약, 테스트 결과, 남은 문제를 보고한다.
- 보고 후 작업을 멈춘다.
- 검수 승인 전에는 다음 Checkpoint로 넘어가지 않는다.
- 커밋은 검수 통과 후 수행한다.

Checkpoint 완료 보고 형식:

1. Changed Files
2. Implemented
3. Tests Run
4. Test Result
5. Manual Editor Steps
6. Known Issues
7. Ready for Review

---

## 00-A - Project & Asset Preflight

### 목적

코드를 쌓기 전에 UE 5.8 프로젝트와 Phase 00에서 사용할 실제 에셋이 정상적으로 열리는지 확인한다.

### 범위

- 프로젝트 UE 5.8 Open / Build 확인
- Big Star Station 확인
- Stylized Character Kit: Casual 01 확인
- Animation Starter Pack 확인
- 프로젝트 Content 구조 확인
- 작은 FPS Test Facility를 만들 수 있는 환경인지 확인

### 하지 않는 것

- Session 구현
- Character gameplay 코드
- Revolver 코드
- R21 리볼버 애니메이션 통합
- 최종 맵 제작

### 작업 순서

1. 프로젝트를 UE 5.8에서 연다.
2. C++ Target이 존재하면 Development Editor 빌드를 확인한다.
3. 다운로드/추가된 세 에셋이 Content Browser에서 정상 로드되는지 확인한다.
4. 각 에셋의 주요 Skeleton / Mesh / Animation 위치를 기록한다.
5. Big Star Station의 벽, 바닥, 천장, 복도, 조명 중 최소 구성으로 작은 Test Facility를 만든다.
6. PlayerStart 6개를 배치할 수 있는 공간을 확보한다.
7. Casual 01의 Skeleton과 Animation Starter Pack의 Skeleton 차이를 확인하고 Retarget 필요 여부를 기록한다.

### 검수 기준

- UE 5.8에서 프로젝트 Open 성공
- Editor 치명 오류 없음
- Big Star Station 주요 Mesh 로드 가능
- Casual 01 Mesh/Skeleton 로드 가능
- Animation Starter Pack 로드 가능
- Test Facility 저장 가능
- Asset 경로와 Skeleton 정보가 보고됨

### 실행 결과 - 2026-08-28 (Ready for Review)

환경:

- Unreal Engine 5.8.1에서 프로젝트 Open 및 D3D12 오프스크린 맵 로드 성공
- C++ Target이 아직 없으므로 Development Editor 빌드는 해당 없음
- Marketplace/Fab 원본은 로컬 전용 Content 경로를 유지하고 Git에서 제외
- 프로젝트 소유 `.uasset` / `.umap`은 Git LFS 추적 대상으로 설정

에셋 로드 결과:

- Big Star Station: 400 Assets 확인, 142 Static Mesh 전부 로드 성공
- Stylized Character Kit: Casual 01: 208 Assets 확인, Skeleton 1 / Skeletal Mesh 16 / Animation 8 로드 성공
- Animation Starter Pack: 85 Assets 확인, Skeleton 1 / Skeletal Mesh 1 / Animation 62 로드 성공
- 세 팩의 대상 에셋 로드 실패 0건, Editor 치명 오류 0건

주요 경로:

- Big Star Floor: `/Game/BigStarStation/StaticMesh/Building/SM_RoomStyle_Floor01`
- Big Star Wall: `/Game/BigStarStation/StaticMesh/Building/SM_RoomStyle01_WallBig`
- Big Star Ceiling: `/Game/BigStarStation/StaticMesh/Building/SM_CorridorRoof01`
- Big Star Corridor Wall: `/Game/BigStarStation/StaticMesh/Building/SM_CorridorWall01`
- Big Star Corridor Ceiling: `/Game/BigStarStation/StaticMesh/Building/SM_CorridorRoofSet01`
- Big Star Corridor Light: `/Game/BigStarStation/StaticMesh/Building/SM_CorridorLight01`
- Casual 01 Skeleton: `/Game/SCK_Casual01/Mannequin/Character/Mesh/UE4_Mannequin_Skeleton`
- Casual 01 Character Mesh: `/Game/SCK_Casual01/Models/Premade_Characters/MESH_PC_00`
- Casual 01 Animation Sample: `/Game/SCK_Casual01/Mannequin/Animations/ThirdPersonIdle`
- Animation Starter Pack Skeleton: `/Game/AnimStarterPack/UE4_Mannequin/Mesh/UE4_Mannequin_Skeleton`
- Animation Starter Pack Mesh: `/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin`
- Animation Starter Pack Animation Sample: `/Game/AnimStarterPack/Idle_Rifle_Hip`

Skeleton / Retarget 결론:

- 두 팩 모두 UE4 Mannequin 계열 Bone 이름과 표준 IK Chain을 포함한다.
- 그러나 서로 다른 Skeleton Package에 바인딩되어 있으므로 동일 Skeleton으로 직접 공유할 수 없다.
- Casual 01 Animation에는 IK Track이 포함되지만 Animation Starter Pack Animation Sample에는 IK Track이 없다.
- Phase 00 Character에 적용할 때 UE5 IK Retargeter 또는 명시적 Skeleton Retarget 작업이 필요하다.

Test Facility:

- 저장 경로: `/Game/LUX/Maps/L_FPS_TestFacility`
- 7.8 m 정사각 Spawn Room과 7.8 m Corridor를 Big Star 모듈로 구성
- Static Mesh Actor 43개, Character Scale Reference 2개, Point Light 5개 배치
- PlayerStart 6개를 두 줄로 배치하고 최소 중심 간격 260 cm 확인
- 핵심 Floor / Wall / Corridor Wall의 Convex Collision Hull 존재 확인
- 맵 저장 후 UE 5.8.1 D3D12 재로드, 에셋 참조, 액터 구성 검증 통과

남은 수동 검증:

- Character가 추가되는 00-C에서 PIE 보행 충돌과 카메라 기준 근거리 Scale 확인
- 00-F에서 6 Player 실제 Spawn Overlap, Lumen 노출, GPU / CPU 부담 최종 확인

### 커밋 게이트

검수 통과 후에만 커밋한다.

권장 커밋 메시지:

```text
chore: validate phase 00 assets and test facility
```

---

## 00-B - C++ Framework Skeleton

### 목적

후속 시스템이 의존할 최소 Framework 클래스만 만든다.

### 범위

- ULuxGameInstance
- ULuxSessionSubsystem
- ALuxGameMode
- ALuxGameState
- ALuxPlayerController
- ALuxPlayerState
- ALuxCharacter
- 필요한 Build.cs 모듈 의존성

### 하지 않는 것

- Session Create / Find / Join 구현
- Move / Look 구현
- Weapon
- Damage / Death
- Round / Role

### 작업 순서

1. 클래스 파일을 역할별 폴더에 생성한다.
2. GameMode 기본 Class 연결을 준비한다.
3. SessionSubsystem은 생성만 하고 API 구현은 비워둔다.
4. Character는 ACharacter 기반으로 생성하고 CameraComponent를 둘 자리만 준비한다.
5. 프로젝트 전체를 컴파일한다.
6. Editor 재실행 후 기본 GameMode 설정이 깨지지 않는지 확인한다.

### 검수 기준

- 전체 C++ 빌드 성공
- Editor 실행 성공
- 새 클래스가 올바른 모듈에 존재
- 서로 불필요하게 참조하지 않음
- Phase 00 범위를 벗어난 게임 규칙 없음

### 실행 결과 - 2026-08-28 (Ready for Review)

C++ Module / Target:

- `PROJECT_LUX` Runtime Module 생성
- `PROJECT_LUX` Game Target과 `PROJECT_LUXEditor` Editor Target 생성
- UE 5.8 기준 `BuildSettingsVersion.V7`, `EngineIncludeOrderVersion.Unreal5_8` 적용
- 현재 필요한 `Core`, `CoreUObject`, `Engine`만 Public Dependency로 구성

Framework:

- `ULuxGameInstance`: 기본 GameInstance 뼈대
- `ULuxSessionSubsystem`: API 없는 GameInstanceSubsystem 뼈대
- `ALuxGameMode`: 기본 Pawn / Controller / PlayerState / GameState Class 연결
- `ALuxGameState`: 상태 필드 없는 GameStateBase 뼈대
- `ALuxPlayerController`: 입력 코드 없는 PlayerController 뼈대
- `ALuxPlayerState`: 게임 상태 필드 없는 PlayerState 뼈대
- `ALuxCharacter`: Camera / Input 없이 ACharacter 기반만 준비

기본 설정:

- `GameInstanceClass=/Script/PROJECT_LUX.LuxGameInstance`
- `GlobalDefaultGameMode=/Script/PROJECT_LUX.LuxGameMode`
- `ALuxGameMode`의 기본 Class를 `ALuxCharacter`, `ALuxPlayerController`, `ALuxPlayerState`, `ALuxGameState`로 연결

검증:

- `PROJECT_LUXEditor Win64 Development` UHT / Compile / Link 성공
- Unreal Engine 5.8.1 D3D12 Editor에서 `UnrealEditor-PROJECT_LUX.dll` 로드 성공
- 네이티브 Class 7개 로드 및 GameMode 기본 Class 연결 자동 검증 통과
- `/Game/LUX/Maps/L_FPS_TestFacility` 재로드 및 Map Check 0 Error / 0 Warning
- Session / Move / Look / Camera / Weapon / Damage / Round / Role 구현 없음 확인

### 커밋 게이트

권장 커밋 메시지:

```text
feat: add multiplayer framework skeleton
```

---

## 00-C - Local First-Person Character

### 목적

네트워크와 세션을 붙이기 전에 로컬 1인칭 이동 기반을 확정한다.

### 범위

- CameraComponent
- Enhanced Input
- IA_Move
- IA_Look
- IMC_Player
- Move
- Look
- Local Possess

### 하지 않는 것

- Jump
- Crouch
- Sprint
- Revolver
- Session
- 별도 FPS Arms

### 작업 순서

1. ALuxCharacter에 CameraComponent를 구성한다.
2. IA_Move / IA_Look / IMC_Player를 만든다.
3. C++에서 Input Mapping Context를 등록한다.
4. Move를 CharacterMovement에 연결한다.
5. Look을 Controller Rotation에 연결한다.
6. Test Facility에서 단일 플레이어 Possess를 검증한다.
7. 카메라 높이와 캐릭터 스케일을 Big Star Station 환경에 맞춘다.

### 검수 기준

- 로컬 Move 정상
- Look 정상
- 입력 중복 없음
- HUD / Crosshair 없음
- Jump/Crouch/Sprint가 임의 추가되지 않음
- 시설 스케일이 1인칭에서 자연스러움

### 실행 결과 - 2026-08-28 (Ready for Review)

Character / Camera:

- `ALuxCharacter`에 `UCameraComponent` 기반 `FirstPersonCamera` 구성
- Capsule 반경 34 cm / 반높이 96 cm, Camera 상대 Z 64 cm로 바닥 기준 눈높이 160 cm 적용
- Camera는 Pawn Control Rotation을 사용하고 Character는 Controller Yaw를 따르도록 구성
- 이동 방향은 Controller Yaw 기준 Forward / Right로 계산하며 별도 Tick은 사용하지 않음

Enhanced Input:

- `EnhancedInput`을 Runtime Module의 Private Dependency로 추가
- 프로젝트 소유 Input Asset을 `/Game/LUX/Input`에 생성
  - `IA_Move`: Axis2D
  - `IA_Look`: Axis2D
  - `IMC_Player`: W / A / S / D / Mouse2D
- W / S는 Y축 Forward, A / D는 X축 Right로 구성
- Mouse2D는 Y축만 반전해 일반적인 마우스 상하 시점 방향으로 구성
- `PawnClientRestart`에서 기존 Context를 제거한 뒤 Priority 0으로 다시 등록해 재시작 경로의 중복 등록 방지
- `SetupPlayerInputComponent`에서 Move / Look의 `Triggered` 이벤트만 연결

Test Facility:

- `/Game/LUX/Maps/L_FPS_TestFacility` 단일 플레이어 PIE 검증
- 00-A의 스케일 참고용 Casual Mesh는 외부 원본을 변경하지 않고 참고 Actor의 불필요한 Shadow만 비활성화해 Map Check 경고 제거
- 160 cm 눈높이의 1인칭 캡처에서 복도 폭, 벽체, 천장 및 성인형 Scale Reference가 자연스러운 비율임을 확인
- Gameplay HUD와 Crosshair가 표시되지 않음을 확인

검증:

- `PROJECT_LUXEditor Win64 Development` UHT / Compile / Link 성공
- `ALuxCharacter`, Camera, Capsule, Input Asset 참조와 Modifier 구성을 자동 검증
- `L_FPS_TestFacility` 로드 성공, PlayerStart 6개 확인, Map Check 0 Error / 0 Warning
- 단일 플레이어 PIE에서 `ALuxPlayerController`가 `ALuxCharacter`를 Possess함을 확인
- Runtime 활성 Key가 Move W/A/S/D, Look Mouse2D 각각 1세트만 존재함을 확인
- 입력 주입 Smoke Test에서 수평 이동 63.07 cm, Yaw 35.625도, Pitch 11.875도 변화 확인
- Jump / Crouch / Sprint / HUD / Crosshair / Weapon / Session 구현 없음 확인

남은 사항:

- 시설 조명과 노출의 최종 품질 및 부하는 00-F 통합 QA에서 확인

### 커밋 게이트

권장 커밋 메시지:

```text
feat: add first person movement foundation
```

---

## 00-D - Replicated Character & Third-Person Body

### 목적

2인 PIE에서 서로의 캐릭터와 이동을 정상적으로 볼 수 있게 한다.

### 범위

- CharacterMovement 기본 복제
- Remote Third-Person Mesh
- Casual 01 적용
- Animation Starter Pack Retarget
- 최소 Locomotion Animation Blueprint
- 2 Player PIE

### 하지 않는 것

- 상체 리볼버 레이어
- Head IK
- Revolver
- Session 검색 UI

### 작업 순서

1. ALuxCharacter의 Replication 기본 설정을 확인한다.
2. 불필요한 자체 Transform RPC를 만들지 않는다.
3. Casual 01 캐릭터를 Remote Mesh에 연결한다.
4. Animation Starter Pack의 필요한 Idle/Walk 계열을 Retarget한다.
5. 프로젝트용 최소 Third-Person AnimBP를 만든다.
6. Speed / Direction 기반 Locomotion을 연결한다.
7. 2 Player PIE에서 양쪽 이동/회전/메시/애니메이션을 확인한다.

### 검수 기준

- Host에서 Client 이동 정상
- Client에서 Host 이동 정상
- 심각한 위치 jitter 없음
- Remote Mesh 정상 표시
- Locomotion 정상
- Local owner 표시 정책이 의도대로 동작
- Animation BP가 Gameplay truth를 소유하지 않음

### 실행 결과 - 2026-08-28 (Ready for Review)

Character / Visibility:

- `ACharacter`가 기본 제공하는 `CharacterMovement` 복제를 그대로 사용하고 별도 Transform RPC나 복제 필드를 추가하지 않음
- 상속된 Character Mesh에 Casual 01 `MESH_PC_00`을 연결하고 Capsule 기준 위치/회전을 보정
- Third-Person Mesh의 Collision/Overlap을 비활성화해 Capsule 이동과 Gameplay Collision에 관여하지 않도록 구성
- Local Owner에서는 `OwnerNoSee`, 다른 플레이어에게는 전신 Mesh가 보이는 표시 정책 적용

Retarget / Locomotion:

- Animation Starter Pack과 Casual 01용 IK Rig 및 Retargeter를 `/Game/LUX/Animation/Retarget`에 생성
- Idle, Forward/Backward/Left/Right Jog 5종을 Casual 01 Skeleton으로 Retarget
- Direction -180~180 / Speed 0~600 기반 2D BlendSpace와 최소 `ABP_LuxCharacter`를 `/Game/LUX/Animation/Locomotion`에 생성
- `ULuxCharacterAnimInstance`는 Pawn의 복제된 Velocity에서 표현용 Speed/Direction만 계산하며 Gameplay truth를 소유하지 않음

2 Player PIE:

- Listen Server 1 + Client 1, 동일 프로세스의 PIE World 2개 생성 확인
- 각 World에서 `ALuxCharacter` 2개, Casual 01 Mesh, 프로젝트 AnimBP, `OwnerNoSee` 정책 확인
- Host/Client 양쪽 입력 주입 후 Local Character와 반대 World의 Remote Proxy 이동을 동시 추적
- Host 604.09 cm / Client 74.13 cm 이동 및 양쪽 Remote Proxy 50 cm 이상 복제 확인
- 이동 중 Remote Proxy 최대 추적 오차 11.87 cm, 최대 단일 이동 Step 16.91 cm
- Remote 경로/직선 이동 비율 Host 1.005 / Client 1.002로 심각한 backtracking 또는 jitter 없음
- 입력 정지 후 Remote Proxy 최종 오차 Host 0.005 cm / Client 0.001 cm 미만으로 수렴
- Remote AnimInstance에서 Host Forward 600 cm/s, 0도 / Client Lateral 502.14 cm/s, 90도 확인
- Big Star Station Test Facility에서 Remote Casual 01 전신과 Retarget Pose가 정상 표시되는 캡처 확인

검증:

- `PROJECT_LUXEditor Win64 Development` UHT / Compile / Link 성공
- 프로젝트 소유 Animation Asset 10개 로드 및 Skeleton/Graph/Pin/Sample 자동 검증 통과
- `/Game/LUX/Maps/L_FPS_TestFacility` 로드 성공, PlayerStart 6개 확인, Map Check 0 Error / 0 Warning
- Revolver, 상체 Layer, Head IK, Session UI 및 자체 Transform RPC가 추가되지 않음

남은 사항:

- 실제 Steam 원격 환경의 Character 복제는 00-E Session Flow 이후 검증
- 6인 Spawn/부하/복제 검증은 00-F에서 수행

### 커밋 게이트

권장 커밋 메시지:

```text
feat: replicate player character and locomotion
```

---

## 00-E - Listen Server Session Flow

### 목적

동일 Character 기반을 실제 Session 흐름으로 연결한다.

PROJECT LUX의 네트워크 토폴로지는 PROJECT-MA와 동일한 **Listen Server 기반**을 사용한다. 다만 PROJECT-MA의 현재 메뉴, Lobby UI, Session 이름 사용 방식, Invite UX를 그대로 복사하지 않는다. 재사용하는 것은 Listen Server / Online Session의 기술적 패턴과 서버 권한 원칙이다.

### 범위

- Create Session
- Find Sessions
- Join Session
- Destroy Session
- Listen Travel
- Client Travel
- ActiveSessionName 관리
- 개발용 Console/Exec 진입점
- Steam Online Subsystem을 통한 실제 원격 접속 검증 경로
- 최종 UI와 독립적인 Session Backend API

### Lobby 방향은 정해졌지만 이 Phase에서 구현하지 않는다

현재 확정된 방향:

- 게임 시작 전 3D 실험 대기실 Lobby를 사용
- 접수원 NPC에서 Steam Invite / Session 관련 기능에 접근
- 문서 관리원 NPC에서 Room Settings 기능에 접근
- NPC는 표현 계층이며 실제 상태는 Session / Room Settings 시스템이 소유
- 상세 방향은 `Docs/Development/LobbyPlan.md` 참고

아직 TBD:

- Main Menu의 정확한 화면 구조
- Host / Join 메뉴의 최종 구성
- 방 코드 또는 Session Key 사용 여부
- 공개 Session Browser 사용 여부
- Quick Join 사용 여부
- Ready 시스템 사용 여부
- 실제 실험 시작 장치/상호작용 방식

Phase 00-E는 위 Lobby를 나중에 연결할 수 있는 UI 독립 Session Backend까지만 만든다.

### 하지 않는 것

- Lobby UI
- Matchmaking UI
- Room Settings
- Ready System
- Role assignment

### 작업 순서

1. ULuxSessionSubsystem에 Online Session Interface를 연결한다.
2. Create API를 구현한다.
3. Find API를 구현한다.
4. 특정 Search Result로 Join하는 API를 구현한다.
5. ActiveSessionName을 Subsystem이 관리한다.
6. Destroy API를 구현한다.
7. Host Listen Travel과 Client Travel을 연결한다.
8. 개발용 호출 경로로 UI 없이 테스트 가능하게 한다.
9. NULL/LAN 또는 현재 사용 가능한 Online Subsystem에서 기본 흐름을 검증한다.
10. Steam Online Subsystem이 설정되어 있으면 동일 API로 Steam Session Create / Find / Join을 검증한다.
11. 최소 2대의 실제 PC 또는 동등한 원격 환경에서 Host와 Client의 Steam 원격 접속을 확인한다.
12. 최종 메뉴가 없어도 개발용 호출 경로로 원격 테스트가 가능해야 한다.

### 검수 기준

- Host session 생성 성공
- Client 검색 성공
- Client join 성공
- 두 플레이어 Spawn/Possess 성공
- Destroy 성공
- NAME_GameSession 하드코딩 의존 없음
- Character/GameMode에 Session 로직이 섞이지 않음
- Session Backend가 특정 Main Menu/Lobby UI에 종속되지 않음
- Steam 원격 Host/Join이 개발용 경로에서 동작
- 최종 참여 UX를 임의로 확정하지 않음

### 커밋 게이트

권장 커밋 메시지:

```text
feat: add listen server session flow
```

---

## 00-F - Six-Player Integration QA

### 목적

Phase 00 전체를 5~6인 실제 목표 규모에서 검증하고 닫는다.

### 범위

- 6 Player Spawn
- Join In Progress
- Disconnect
- Host Exit
- PlayerState
- Standalone / Multi-process
- 로컬 6인 부하/복제 검증
- Steam 원격 2인 이상 접속 검증
- 최종 Phase 00 문서 결과 기록

### 작업 순서

1. PlayerStart 6개를 충돌 없이 배치한다.
2. 2 Player 기본 회귀 테스트를 먼저 수행한다.
3. 6 Player PIE 또는 Multi-process 테스트를 수행한다.
4. 모든 PlayerState 존재 여부를 확인한다.
5. 진행 중 Session에 Client를 추가한다.
6. Client Disconnect를 확인한다.
7. Host 종료 시 Session/Client 정리를 확인한다.
8. Standalone 환경에서 최소 Host/Join 흐름을 재검증한다.
9. Steam을 통해 최소 2대의 실제 PC 또는 동등한 원격 환경에서 Host/Join을 검증한다.
10. 원격 Client에서 이동, Possess, PlayerState, Disconnect를 확인한다.
11. Phase 00 Completion Checklist를 갱신한다.
12. Result / Changed from Plan / Remaining을 작성한다.

### 검수 기준

- 6 Player Spawn overlap 없음
- 6명 모두 이동 가능
- 서로의 Remote Character 확인 가능
- PlayerState 정상
- JIP 정상
- Client 이탈 후 치명 오류 없음
- Host 종료 후 hanging 없음
- Standalone에서 핵심 흐름 작동
- Steam 원격 2인 이상 접속 성공
- 최종 Main Menu/Lobby UX가 없어도 실제 인터넷 멀티 테스트 가능

### Phase 완료 커밋

권장 커밋 메시지:

```text
feat: complete multiplayer foundation
```

---

# 2. 구현 범위

## 2.1 C++ Framework

생성할 기본 클래스:

- ULuxGameInstance
- ULuxSessionSubsystem
- ALuxGameMode
- ALuxGameState
- ALuxPlayerController
- ALuxPlayerState
- ALuxCharacter

필요한 경우 이름은 프로젝트 기존 Naming Convention에 맞춰 조정할 수 있으나 역할은 유지한다.

## 2.2 세션

Online Subsystem 인터페이스를 사용한다.

기본 기능:

- Create Session
- Find Sessions
- Join Session
- Destroy Session
- Host Listen Travel
- Client Travel
- Join-in-progress 허용
- Host 종료 시 Destroy

구현 원칙:

- 세션 로직을 Character나 GameMode에 넣지 않는다.
- ULuxSessionSubsystem이 Session Interface를 소유한다.
- UI와 독립적인 API로 작성한다.
- Phase 00에서는 개발용 Console/Exec 진입점을 허용한다.
- 후속 Lobby UI는 같은 API를 호출한다.
- 하드코딩된 NAME_GameSession에 전체 로직을 의존하지 않는다.
- Runtime에서 생성한 ActiveSessionName을 Subsystem이 보관한다.

테스트 우선순위:

1. OnlineSubsystem NULL / LAN 또는 PIE 환경
2. Standalone Listen Server
3. Steam 설정이 준비된 경우 동일 API로 Steam 세션 추가 검증

NULL 테스트는 임시 네트워크 구조가 아니라 동일 IOnlineSessionInterface 구현의 개발용 Backend로 취급한다.

---

# 3. Character 구조

## 3.1 ALuxCharacter

기반:

- ACharacter
- CharacterMovementComponent 기본 네트워크 복제 사용
- CameraComponent
- Third-Person Skeletal Mesh
- 향후 Equipped Revolver 참조 슬롯

Phase 00에서 구현할 입력:

- Move
- Look

Jump / Crouch / Sprint는 현재 게임 기획에서 확정되지 않았으므로 임의로 추가하지 않는다.

## 3.2 1인칭 / 3인칭 표시

Local owner:

- 1인칭 Camera 사용
- Phase 00에서는 자기 3인칭 Mesh를 필요에 따라 OwnerNoSee 처리
- 별도 FPS Arms는 Phase 01 Revolver Asset 검증 시 연결

Remote player:

- 전체 3인칭 Character Mesh 표시
- 기본 locomotion 재생

목표는 최종적으로 다음 구조로 확장 가능한 상태다.

```text
ALuxCharacter
├─ Camera
├─ ThirdPersonCharacterMesh
└─ EquippedRevolver
   ├─ FirstPersonWeaponVisual
   └─ ThirdPersonWeaponVisual
```

---

# 4. PlayerState / GameMode 책임

## ALuxPlayerState

Phase 00에서 최소 보관:

- Player identity 관련 기본 정보
- Ready/Role 등 후속 필드를 추가할 수 있는 확장 지점

현재 Citizen / Host / Parasite Role은 구현하지 않는다.

## ALuxGameMode

Server only 책임:

- Player Spawn
- 기본 PlayerController / PlayerState / Character class 지정
- 접속/퇴장 이벤트
- 향후 Round System 연결 지점

## ALuxGameState

현재 최소 상태만 둔다.

Phase 00에서 Round timer, Blackout, Escape 상태를 넣지 않는다.

---

# 5. Enhanced Input

Native Enhanced Input을 사용한다.

필요 Asset:

- IA_Move
- IA_Look
- IMC_Player

입력 로직은 C++ Character / Controller에서 처리한다.

Ninja Input 등 외부 Input Framework는 사용하지 않는다.

---

# 6. Asset Setup

Phase 00에서는 실제 게임에 계속 사용할 가능성이 높은 자산을 검증한다.

상세 후보는 ../AssetPlan.md 참고.

## 6.1 Big Star Station

목적:

- 시설 기본 Environment Kit 검증
- FPS scale 검증
- 실제 네트워크 테스트 공간 제작

작업:

1. 에셋을 UE 5.8 프로젝트에 추가
2. Demo Map 전체를 Game Map으로 채택하지 않음
3. 필요한 modular wall/floor/ceiling/door/light/terminal asset만 사용
4. 작은 L_FPS_TestFacility 레벨 제작
5. 최소 6개의 PlayerStart 배치
6. 긴 복도, 작은 방, 교차 공간을 포함해 네트워크 이동 테스트

검증:

- Collision
- Scale
- Lumen
- 1인칭 근거리 품질
- 기본 GPU/CPU 부담
- 조명 Mesh 분리 가능성

Phase 00 완료 전에 Adopt / Reject 기록.

## 6.2 Stylized Character Kit: Casual 01

목적:

- 일반 실험 참가자 3인칭 Character 후보

작업:

1. UE 5.8 Migration
2. Epic Skeleton 상태 확인
3. UE5 IK Retargeter 설정
4. 최소 1개 완성 Character 구성
5. 가능하면 3개 이상의 외형 Variation 확인
6. Network Character Mesh로 적용

검증:

- UE 5.8 안정성
- Skeleton
- Material
- Modular Mesh 조합
- Animation retarget
- 5~6인 화면에서 구분 가능성

기술 또는 아트 문제가 크면 Phase 00을 끝내기 전에 대체 Character 후보를 정한다.

## 6.3 Animation Starter Pack

목적:

- Remote Character lower-body locomotion 기반
- Phase 01의 Revolver upper-body layer와 결합

작업:

- 필요한 Idle / Walk 계열만 Retarget
- 불필요한 애니메이션을 프로젝트 로직에 연결하지 않음
- Animation Blueprint는 프로젝트용으로 별도 제작

외부 Demo Blueprint를 게임 로직 기반으로 사용하지 않는다.

---

# 7. Animation 구조

Third-Person Character Animation Blueprint는 최소 구조로 만든다.

Phase 00:

```text
Speed
Direction
IsInAir (future use 가능)
    ↓
Locomotion State
    ↓
Output Pose
```

Phase 01에서 UpperBody Slot / Layered Blend를 추가할 수 있도록 구조를 열어둔다.

Animation State의 Gameplay 판단을 Blueprint 안에 과도하게 넣지 않는다.

Character의 실제 상태는 C++에서 제공한다.

---

# 8. 폴더 기준

권장:

```text
Source/PROJECT_LUX/
├─ Framework/
│  ├─ LuxGameInstance
│  ├─ LuxSessionSubsystem
│  ├─ LuxGameMode
│  └─ LuxGameState
├─ Player/
│  ├─ LuxPlayerController
│  ├─ LuxPlayerState
│  └─ LuxCharacter
└─ ...

Content/
├─ LUX/
│  ├─ Characters/
│  ├─ Input/
│  ├─ Maps/
│  ├─ Weapons/
│  ├─ Animation/
│  ├─ VFX/
│  └─ Audio/
└─ ThirdParty/
   └─ <AssetName>/
```

Marketplace asset 원본 폴더 구조가 Migration 때문에 유지되어야 하는 경우 억지로 이동하지 않는다.

프로젝트에서 직접 만든 연결 Asset은 Content/LUX 아래에 둔다.

---

# 9. 네트워크 권한 원칙

- GameMode: Server only
- Session creation/join: Local GameInstance Subsystem
- Character transform: CharacterMovement replication
- Gameplay 상태 변경: Server authority
- Client는 자신이 소유한 Pawn의 입력만 전송
- 이후 Weapon / Parasite 시스템도 같은 원칙을 따른다

Phase 00에서 불필요한 자체 Transform RPC를 만들지 않는다.

---

# 10. 구현하지 않는 것

Phase 00에서 다음은 하지 않는다.

- Revolver
- Damage
- Death
- Health
- Ammo
- Inventory
- Citizen / Host / Parasite
- Round State
- Blackout
- Light Detection
- Flashlight
- Tasks
- Interaction Framework
- Escape
- Spectator
- Voice Chat
- Gameplay HUD
- Crosshair
- Character customization UI
- Steam Lobby UI
- Matchmaking UI

---

# 11. 테스트

## 11.1 2 Player PIE

- Host 생성
- Client 참가
- 양쪽 Spawn
- 이동 복제
- 회전 복제
- 서로의 Character 표시
- Disconnect cleanup

## 11.2 6 Player PIE / Multi-process

- 6 PlayerStart 사용
- Spawn overlap 없음
- 모든 PlayerState 존재
- 이동 중 심각한 jitter 없음
- 다른 플레이어 mesh/animation 정상 표시

## 11.3 Join In Progress

- 이미 진행 중인 Listen Server에 추가 Client 접속
- 정상 Spawn / Possess
- 기존 플레이어를 정상적으로 볼 수 있음

## 11.4 Host Exit

- Host 종료
- 세션 cleanup
- Client가 비정상 hanging 상태로 남지 않음

## 11.5 Standalone

PIE에서만 동작하고 Standalone에서 실패하는 코드를 허용하지 않는다.

---

# 12. Completion Checklist

- [ ] C++ Framework classes 생성
- [ ] Enhanced Input Move/Look
- [ ] Listen Server Session Create
- [ ] Find / Join
- [ ] Destroy
- [ ] 6 Player Spawn
- [ ] PlayerState 확인
- [ ] CharacterMovement replication
- [ ] First Person Camera
- [ ] Remote Third Person Character
- [ ] Big Star Station 검증
- [ ] Character Asset 검증
- [ ] Animation Starter Pack retarget
- [ ] 2 Player 테스트 통과
- [ ] 6 Player 테스트 통과
- [ ] Join In Progress 테스트 통과
- [ ] Host Exit 테스트 통과
- [ ] Gameplay HUD 없음
- [ ] Phase 01 Revolver 부착 지점 준비

---

# 13. 완료 후 기록

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
