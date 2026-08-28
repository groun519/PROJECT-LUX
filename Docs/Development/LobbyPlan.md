# PROJECT LUX - Lobby / Main Menu Direction

> 상태: Design Direction / Pre-Implementation  
> 목적: PROJECT LUX의 메인메뉴 이후 멀티플레이 진입 흐름과 3D 실험 대기실 Lobby의 역할을 정리한다.

---

# 1. 기본 방향

PROJECT LUX는 단순한 2D Lobby 목록 화면보다 **실험 참가자들이 실제로 모여 있는 3D 실험 대기실**을 기본 Lobby로 사용한다.

Lobby는 게임 세계관의 일부다.

플레이어는 실험 시작 전에 대기실에 입장하고, 실제 캐릭터로 이동하면서 다른 참가자와 상호작용한다.

네트워크 구조는 PROJECT-MA와 동일한 Listen Server 방향을 사용하지만, Lobby UX와 Session 참여 방식은 PROJECT LUX에 맞게 별도로 구성한다.

---

# 2. 메인메뉴

현재 확정 방향:

- Main Menu에서 Multiplayer 진입
- Host가 Listen Server Session을 생성
- Client가 해당 Session에 참가
- Session Backend는 UI와 독립적으로 유지

정확한 Main Menu 버튼 구성과 최종 시각 디자인은 아직 TBD다.

현재 구현 단계에서는 Main Menu UI를 Session Backend보다 먼저 만들지 않는다.

---

# 3. 3D 실험 대기실 Lobby

Lobby의 컨셉은 **실험 참가 접수 및 대기 공간**이다.

기본 공간 구성 방향:

- 참가자들이 실제 Character로 Spawn
- 여러 플레이어가 같은 공간에서 이동 가능
- 실험 시설 특유의 접수/행정 분위기
- NPC 및 시설 오브젝트를 통해 Lobby 기능에 접근
- 실제 실험 공간으로 이동하기 전의 별도 공간

Lobby에서는 Citizen / Host / Parasite 역할을 공개하지 않는다.

Role 배정 또는 감염 관련 처리는 실제 실험 시작 이후에 수행한다.

---

# 4. 접수원 NPC

**[확정 방향]**

접수원은 Session / Invite 관련 Lobby 기능의 다이에제틱 진입점이다.

주요 역할:

- Steam 친구 초대 기능 열기
- 현재 Session 참가 관련 기능 접근

구조 원칙:

```text
Player
  -> Receptionist Interaction
  -> Lobby Session UI
  -> ULuxSessionSubsystem
  -> Steam Invite / Session Backend
```

접수원 NPC 자체가 Online Session 로직을 소유하지 않는다.

NPC는 기능 접근점이며 실제 Session 상태와 Invite 처리는 Session Subsystem이 담당한다.

---

# 5. 문서 관리원 NPC

**[확정 방향]**

문서 관리원은 Room Settings 기능의 다이에제틱 진입점이다.

주요 역할:

- 방 설정 UI 열기
- Host가 실험 조건을 조정

초기 Room Settings 후보:

- Parasite 수
- Power ON 시간
- Blackout 시간
- 모든 Parasite 사망 시 즉시 종료 여부

추가 설정은 GameDesign 및 Room Settings Phase에서 확정한다.

구조 원칙:

```text
Player
  -> Document Administrator Interaction
  -> Room Settings UI
  -> Server-authoritative Room Settings State
```

문서 관리원 NPC 자체가 게임 규칙 상태를 보관하지 않는다.

---

# 6. NPC 책임 분리

접수원:

- 사람을 모으는 기능
- Steam Invite
- Session 참가 관련 접근

문서 관리원:

- 실험 조건 관리
- Room Settings 접근

이 둘의 기능을 한 NPC에 합치지 않는다.

NPC는 UI를 여는 표현 계층이며 핵심 네트워크/게임 규칙은 C++ 시스템에 남긴다.

---

# 7. Lobby에서 하지 않는 것

현재 Lobby에서 다음은 하지 않는다.

- Citizen / Host / Parasite 역할 공개
- 감염 여부 공개
- 기생물 정보 공개
- 실제 Round 진행
- 실제 과제 진행
- 전력/정전 Gameplay
- Revolver PvP를 기본 Lobby 기능으로 사용
- Lobby NPC에 Session/GameRule truth 저장

---

# 8. 아직 미확정

다음은 아직 결정하지 않는다.

- Main Menu의 정확한 버튼 구성
- Host / Join 화면의 최종 구조
- 방 코드 / Session Key 사용 여부
- 공개 Session Browser 여부
- Quick Join 여부
- Steam Invite 외 추가 참가 방식
- Ready 시스템 사용 여부
- 실험 시작을 어떤 NPC/단말기/출입문에서 수행할지
- Lobby에서 Character customization을 제공할지
- Lobby NPC의 최종 외형 및 대사 시스템
- Lobby Map의 정확한 레이아웃

Codex는 위 항목을 임의로 확정하지 않는다.

---

# 9. 구현 시점

Phase 00의 Session Backend는 Lobby UI와 독립적으로 먼저 구현한다.

초기 FPS Milestone에서는 개발용 Host / Join 경로만으로 원격 테스트가 가능해야 한다.

3D Lobby와 NPC 상호작용은 공통 Interaction Framework 또는 Lobby 전용 구현 시점에 별도 Checkpoint로 상세화한다.

Session Backend API는 이후 접수원 NPC가 그대로 호출할 수 있어야 한다.

Room Settings 구조는 이후 Room Settings Phase에서 문서 관리원 NPC UI와 연결한다.
