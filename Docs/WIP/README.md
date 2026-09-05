# PROJECT-LUX WIP Design Mockups

> Branch: `codex/mockups`  
> Storage model: orphan branch / design-only workspace  
> Implementation reference: current `main`  
> Purpose: 구현 전 설계와 상세 계획을 기능별로 분리해 관리한다.

## 1. Branch policy

1. 새로운 목업 주제마다 별도 Git branch를 만들지 않는다.
2. 주제별로 `Docs/WIP/<topic>/` 폴더를 추가한다.
3. 이 브랜치는 orphan 기반이며 실제 프로젝트 코드의 스냅샷을 보관하지 않는다.
4. Source/Content/Config/Plugins 및 실제 구현 파일을 이 브랜치에 추가하지 않는다.
5. 현재 코드 확인이 필요하면 반드시 `main`의 최신 코드를 직접 읽는다.
6. 확정 전 설계와 실험안은 이 브랜치에서 자유롭게 수정한다.
7. 구현 가능한 수준으로 확정된 내용만 `main`의 정식 GameDesign / Development / Phase 문서로 이관한다.
8. 목업 브랜치 전체를 `main`에 merge하지 않는다.

## 2. Status labels

- **Fact**: 현재 코드/엔진/프로젝트에서 확인된 사실
- **Requirement**: 반드시 만족해야 하는 요구
- **Decision**: 현재 채택된 설계
- **Candidate**: 유력하지만 아직 검증 중인 안
- **Experiment**: 실제 구현/플레이 테스트로 판단할 내용
- **Deferred**: 현재 범위에서 구현하지 않는 내용
- **Open Question**: 의도적으로 미결정 상태인 내용

## 3. Current workspaces

### `third-person-view-body-rotation-review/`

`main@0275e299` 이후 로컬에서 진행한 uncommitted Root Yaw Offset 실험의 실패 분석. 입력/정책 값은 안정적이지만 실제 pelvis가 counter-rotate하지 않은 채 neck/head 계산만 visual-root 보정을 전제로 진행하는 현재 불일치를 측정값으로 기록한다. CHAT 논의를 위한 최소 재현 질문과 baseline -> root axis -> root-only -> head/neck -> spine/hand -> locomotion -> network 순서의 recovery gate를 포함한다.

### `revolver-presentation-architecture-rework/`

`main@0275e299` 기준 Phase 01-E/F, Third-Person Aim IK, Manual Chamber Reload까지 다시 검수한 현재 구현 기준 문서. Gameplay Revolver, shared mechanical visual, FP/TP Presentation 책임을 분리하고 R21 animation timing이 production reload rule에 침투한 결합을 제거하는 implementation-ready architecture correction.

### `round-game-rule/`

Death Event, GameRuleManager, EndCondition, GameMode commit, GameState replication을 포함한 Round/Game Rule Foundation 상세 초안.

## 4. Superseded workspaces

### `revolver-fp-presentation-review/`

`main@49e3f7ee`만을 대상으로 했던 과거 01-E review. 이후 01-E correction과 01-F가 구현되어 현재 작업 기준으로 사용하지 않는다. 최신 판단은 `revolver-presentation-architecture-rework/`를 따른다.

Room Frame / Map 구조는 현재 별도 논의 대상이며 아직 이 workspace에서 확정하지 않는다.
