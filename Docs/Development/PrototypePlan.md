# PROJECT LUX - Prototype Development Roadmap

> 문서 상태: Pre-Production / Implementation Roadmap  
> 기준 엔진: Unreal Engine 5.8  
> 목적: PROJECT LUX의 전체 개발 순서를 요약하고, 현재 확정된 초기 구현 범위를 Codex가 별도 설계 논의 없이 실행할 수 있도록 Phase 문서로 연결한다.

---

# 0. 로드맵 원칙

## 0.1 Phase 크기

하나의 Phase는 다음 조건을 만족해야 한다.

- 하나의 Codex 작업 세션에서 집중할 수 있는 크기
- 완료 여부를 독립적으로 테스트 가능
- 다음 Phase의 기반이 됨
- 미정 기획을 임의로 확정하지 않음
- 구현 결과를 별도 플레이테스트로 검증 가능

## 0.2 에셋 원칙

PROJECT LUX는 임시 에셋을 사용한 뒤 전부 교체하는 개발 방식을 기본으로 하지 않는다.

- 각 Phase에서 실제 게임에 계속 사용할 가능성이 높은 에셋을 먼저 검증한다.
- 에셋이 기술적/시각적으로 부적합하면 해당 Phase 안에서 대체 후보를 검토한다.
- 외부 에셋의 Blueprint 여부만으로 배제하지 않는다.
- 개발시간 절약과 품질 가치가 충분하면 Blueprint 에셋도 사용 가능하다.
- 핵심 서버 권한, 게임 규칙, 승패, 기생물 로직은 외부 Blueprint Framework에 종속시키지 않는다.
- 라이선스가 공개 페이지에서 확정되지 않거나 과거 Marketplace 취득 라이선스가 중요한 경우 사용자에게 확인한다.

상세 후보와 검증 시점은 [AssetPlan.md](AssetPlan.md)를 따른다.

## 0.3 플레이어 UI 원칙

PROJECT LUX는 전통적인 게임 HUD를 사용하지 않는다.

- 크로스헤어 없음
- 체력 HUD 없음
- 탄약 HUD 없음
- 탄종 HUD 없음
- 정전 카운트다운 HUD 없음
- 탈출 준비도 및 과제 상태도 최종적으로 시설 화면, 장치, 사운드 등 다이에제틱 수단으로 전달

개발용 로그, 콘솔 출력, 비Shipping 디버그 시각화는 허용한다.


## 0.4 네트워크 구조 원칙

- PROJECT LUX는 PROJECT-MA와 동일하게 **Listen Server 구조**를 사용한다.
- Online Session, Listen Travel, Client Travel, 서버 권한 Gameplay라는 기술적 방향을 공유한다.
- PROJECT-MA의 현재 코드와 UI를 그대로 복사하는 것은 아니다.
- 특히 Main Menu, Lobby UI, Session 참여 UX, Invite UX, 내부 Session 이름 정책은 PROJECT LUX에서 별도로 결정한다.
- Phase 00에서는 최종 UI와 독립적인 Session Backend를 먼저 만든다.
- 로컬 6인 검증과 별도로 Steam을 통한 실제 원격 2인 이상 검증을 첫 FPS Milestone의 완료 조건에 포함한다.

### 아직 미확정인 Session UX

- Main Menu 구조
- Host / Join 화면
- 방 코드 또는 Session Key 사용 여부
- Steam Friend Invite 중심 여부
- 공개 Session Browser 여부
- Quick Join 여부
- Lobby / Ready / Room Settings 화면 구조

Codex는 위 항목을 임의로 결정하지 않는다.

---

# 1. 첫 번째 개발 Milestone

## 목표

**5~6명의 플레이어가 같은 Listen Server에 접속해 1인칭으로 이동하고, 리볼버를 사용해 서로 사격 및 사망 판정을 검증할 수 있는 상태. 로컬 6인 검증과 Steam 원격 2인 이상 검증을 모두 포함한다.**

이 Milestone에서 PROJECT LUX의 기생물 시스템은 아직 구현하지 않는다.

검증할 기반:

1. 멀티플레이 접속
2. PlayerController / PlayerState / Character 생명주기
3. 1인칭 카메라와 이동
4. 타 플레이어의 3인칭 캐릭터 표시
5. 서버 권한 리볼버 발사
6. 실린더 기반 6 Chamber 상태
7. 실탄 / 공탄 / 고무탄 타입 구조
8. 실탄 피격 시 부위와 관계없이 즉사
9. 사망 복제
10. HUD 없이 조준/사격
11. 1인칭/3인칭 리볼버 애니메이션 및 사운드 연동

상세 구현은 다음 문서를 따른다.

- [Phase 00 - Multiplayer Foundation](Phases/Phase00_MultiplayerFoundation.md)
- [Phase 01 - Revolver FPS Foundation](Phases/Phase01_RevolverFPSFoundation.md)

---

# 2. 전체 Phase 로드맵

아래 순서는 현재 개발 방향이며, **Phase 02 이후는 잠정 순서**다. 후반 Phase의 세부 구현은 해당 단계에 도달했을 때 다시 설계한다.

| Phase | 이름 | 목표 | 상태 |
|---:|---|---|---|
| 00 | Multiplayer Foundation | 접속, Spawn, Possess, 이동, PlayerState 기반 | 상세 계획 완료 |
| 01 | Revolver FPS Foundation | 리볼버, Chamber, 탄종, 사격, 즉사, 애니메이션 | 상세 계획 완료 |
| 02 | Round Foundation | 매치 상태, 시작/종료, 플레이어 생존 상태 | 잠정 |
| 03 | Parasite / Host Foundation | Citizen / Host / Parasite 상태 분리 | 잠정 |
| 04 | Host Control Technical Spike | 같은 몸을 두 플레이어가 제어하는 기술 검증 | 잠정 |
| 05 | Activity & Resistance | 기생물 활성도와 숙주 저항 | 잠정 |
| 06 | Detach / External / Reinfect | 이탈, 외부 이동, 재기생 | 잠정 |
| 07 | Light Detection | 실제 3D 밝기와 활성도 연결 | 잠정 |
| 08 | Blackout | 전력 ON / OFF 주기와 조명 제어 | 잠정 |
| 09 | Parasite Control Expansion | 부분 제어 및 완전 장악 확장 | 잠정 |
| 10 | Parasite Perception | 기생물 전용 청각/감각 정보 | 잠정 |
| 11 | Host-Parasite Communication | 숙주-기생물 비공개 통신 | 잠정 |
| 12 | Interaction Framework | 공통 상호작용 구조 | 잠정 |
| 13 | Task Framework | 실험 과제 공통 구조 | 잠정 |
| 14 | Core Tasks | 핵심 실험 과제 구현 | 잠정 |
| 15 | Task Randomization | 과제 조합/위치 랜덤화 | 잠정 |
| 16 | Escape System | 100% 준비도, 수동 개방, 300초 탈출 | 잠정 |
| 17 | Death / Spectator / Result | 관전, 정보 차단, 승패 | 잠정 |
| 18 | Room Settings | 전력 주기 등 방 설정 | 잠정 |
| 19 | Multi-Parasite | 2기생물 게임 규칙 | 잠정 |
| 20 | Map / UX / Polish | 시설 완성, 다이에제틱 정보, 최적화 | 잠정 |

---

# 3. Phase 00~01 이후의 재검토 지점

FPS Milestone이 끝난 시점에 다음을 확인한 뒤 후속 Phase 순서를 다시 확정한다.

- 5~6인 Listen Server가 안정적인가
- Character와 Revolver 에셋 조합이 최종 게임에 사용할 품질인가
- 1인칭/3인칭 애니메이션 Retarget 및 Layering이 안정적인가
- 실탄 1발 즉사가 사회적 플레이의 기반으로 적절한가
- Chamber 상태를 서버 전용으로 보관하면서 필요한 시각 정보만 복제할 수 있는가
- HUD 없는 조작이 불편하지 않은가
- 리볼버 한 종류만으로 충분한가
- 공탄/고무탄이 실제 심리전 도구가 될 가능성이 있는가

이 검증 이후 Phase 02부터 다시 세부 계획을 작성한다.

---

# 4. 현재 확정된 FPS 게임 규칙

## 4.1 무기

- 플레이어용 총기는 리볼버 1종을 기본으로 한다.
- 다른 총기 종류를 전제로 범용 무기 Framework를 만들지 않는다.
- 리볼버는 6 Chamber 구조를 사용한다.
- 총기 확장은 플레이테스트에서 필요성이 확인되기 전까지 고려하지 않는다.

## 4.2 실탄

- 플레이어의 어느 부위에 맞더라도 즉사한다.
- 헤드샷 배율, 팔다리 배율, 일반적인 HP 기반 TTK 설계를 사용하지 않는다.
- 서버가 적중을 확정한 뒤 사망 처리를 실행한다.

## 4.3 탄약 희소성

- 탄약은 매우 제한적인 전략 자원이다.
- 정확한 게임 시작 탄 수와 시설 전체 Spawn 수는 아직 확정하지 않는다.
- 희소성은 이후 실제 멀티플레이 QA로 조정한다.

## 4.4 탄종

기본 탄종 구조:

- Live: 실탄. 플레이어 적중 시 즉사.
- Blank: 공탄. 발사 연출은 발생하지만 살상 탄도 판정 없음.
- Rubber: 고무탄. 비살상 탄도 판정을 수행하되 구체적인 피격 효과는 TBD.
- Empty: 빈 Chamber.

정확한 고무탄 효과는 Phase 01에서 임의로 설계하지 않는다.

## 4.5 숨겨진 탄종 정보

- 리볼버 내부의 정확한 탄종 배열은 서버 권한 정보다.
- 모든 Client에게 탄종 배열을 복제하지 않는다.
- 다른 플레이어는 총을 넘겨받아도 탄종을 UI로 확인할 수 없다.
- 장전한 플레이어가 알고 있는 정보는 플레이어의 기억과 관찰에 의존한다.
- 실린더를 열었을 때 Loaded / Empty 여부를 물리적으로 보여줄 수 있으나, 장전된 탄의 타입은 외형으로 확정할 수 없게 한다.
- 탄의 입수/장전 장면을 직접 목격한 플레이어가 추론하는 것은 허용한다.

---

# 5. 문서 갱신 규칙

각 Phase 완료 후 해당 Phase 문서 하단에 다음을 기록한다.

- Result
- Status
- Commit
- Implemented
- Changed from Plan
- Remaining

후속 기획이 변경되면 먼저 GameDesign 또는 Development 문서를 갱신한 뒤 코드를 수정한다.


---

# 6. Checkpoint 실행 원칙

Phase 00~01은 Checkpoint 단위로 구현, 검수, 커밋한다.

Phase 00:
- 00-A Project & Asset Preflight
- 00-B C++ Framework Skeleton
- 00-C Local First-Person Character
- 00-D Replicated Character & Third-Person Body
- 00-E Listen Server Session Flow
- 00-F Six-Player Integration QA

Phase 01:
- 01-A R21 Asset Integration Spike
- 01-B Revolver Actor & Chamber State
- 01-C Server Fire, Round Resolution & Death
- 01-D Cylinder Reload & Hidden Ammo Flow
- 01-E First-Person Presentation
- 01-F Third-Person Body, Upper-Body Blend & Head Look
- 01-G Six-Player Revolver QA & Phase Close

각 Checkpoint는 구현 후 테스트 결과를 보고하고 검수 전에는 다음 Checkpoint로 넘어가지 않는다. 검수 통과 후에만 커밋한다.
