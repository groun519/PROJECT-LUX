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

### `round-game-rule/`

Death Event, GameRuleManager, EndCondition, GameMode commit, GameState replication을 포함한 Round/Game Rule Foundation 상세 초안.

### `revolver-fp-presentation-review/`

`main@49e3f7ee` Phase 01-E First-Person Presentation의 코드/네트워크/에셋 경계 검수와 01-F 진행 전 correction 요구사항.

Room Frame / Map 구조는 현재 별도 논의 대상이며 아직 이 workspace에서 확정하지 않는다.
