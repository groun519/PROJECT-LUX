# PROJECT-LUX 작업 지침

## 적용 범위와 우선순위

- 이 지침은 `PROJECT-LUX`에만 적용한다.
- 작업 전에 요청과 관련된 설계 문서 및 현재 Checkpoint를 읽고, 실제 흐름을 이해한 뒤 변경한다.
- 구현이 기획 문서와 충돌하면 임의로 한쪽을 선택하지 말고 충돌 내용을 먼저 알린다.
- 현재 Checkpoint 범위만 완성한다. 후속 Phase를 위한 추측성 코드, 확장 지점, 빈 추상화는 만들지 않는다.

모든 작업의 기본 문서:

- `README.md`
- `Docs/GameDesign.md`
- `Docs/Development/PrototypePlan.md`
- `Docs/Development/AssetPlan.md`

Phase 작업은 해당 문서도 함께 읽는다.

- Phase 00: `Docs/Development/Phases/Phase00_MultiplayerFoundation.md`
- Phase 01: `Docs/Development/Phases/Phase01_RevolverFPSFoundation.md`

## PROJECT-MA 경계

`S:\gitrepos\PROJECT-MA`는 참고 전용이다.

- 파일 및 에셋 생성, 수정, 삭제, 이동, 저장을 금지한다.
- 브랜치, 인덱스, 커밋, 태그와 원격 상태를 변경하지 않는다.
- Unreal Editor, 빌드, 리세이브 또는 생성 도구를 MA를 대상으로 실행하지 않는다.
- 구조 확인을 위한 읽기 전용 검색과 열람만 허용한다.
- MA의 패턴을 가져올 때도 LUX 요구사항에 맞는 최소 부분만 새로 구현한다.

## Ponytail-lite 구현 원칙

최소 구현은 이해를 생략하는 수단이 아니다. 먼저 변경이 닿는 코드와 호출 흐름을 끝까지 확인한 다음 아래 순서로 판단한다.

1. 현재 Checkpoint에 실제로 필요한가?
2. LUX에 이미 같은 역할의 코드나 패턴이 있는가?
3. C++ 표준 라이브러리 또는 Unreal Engine API로 해결되는가?
4. 이미 활성화된 엔진 기능이나 의존성으로 해결되는가?
5. 그 이후에만 동작하는 최소 코드를 추가한다.

추가 규칙:

- 요청하지 않은 인터페이스, 팩토리, 범용 프레임워크와 설정 계층을 만들지 않는다.
- 두 번째 실제 사용처가 생기기 전에는 확장성을 위한 추상화를 추가하지 않는다.
- 새 의존성보다 Unreal 기본 기능과 기존 의존성을 우선한다.
- 영리한 압축보다 읽기 쉬운 평범한 구현을 택한다.
- 버그는 증상이 아니라 공통 원인을 고치며, 수정 대상의 호출자와 관련 경로를 함께 검색한다.
- 의도적으로 한계를 둔 단순화는 한계와 확장 조건을 짧은 주석으로 남긴다.
- 명시적으로 요구된 기능은 단순화를 이유로 생략하지 않는다.

## Unreal 및 네트워크 안전장치

다음은 제거 대상 보일러플레이트로 취급하지 않는다.

- Unreal Header Tool에 필요한 `UCLASS`, `USTRUCT`, `UENUM`, `UPROPERTY`, `UFUNCTION` 구성
- Module, Target 및 Build 설정
- UObject 수명, Garbage Collection과 Asset Reference 안전성
- 서버 권한, Replication, RPC, Join In Progress와 Host Exit 처리
- Blueprint 노출 및 Editor 직렬화에 필요한 명시적 구조
- 입력 검증, 데이터 손실 방지, 오류 처리와 보안 경계

네트워크 게임 상태는 서버 권한을 기본으로 한다. 클라이언트 표시 편의를 위해 권한 규칙을 우회하지 않는다. 짧은 코드보다 복제 조건과 수명 주기가 분명한 코드를 우선한다.

## 에셋과 저장소 정책

- Marketplace 및 Fab 원본 에셋은 로컬에만 두고 Git에 포함하거나 재배포하지 않는다.
- 외부 에셋의 폴더 구조를 바꿔야 한다면 파일 시스템이 아니라 Unreal Editor에서 이동하고 Redirector를 정리한다.
- 프로젝트 소유 에셋은 `Content/LUX/` 아래에 둔다.
- 프로젝트 소유 `.uasset`, `.umap`을 처음 추가하기 전에 Git LFS 추적 규칙을 확인한다.
- 새 외부 에셋을 프로젝트에 추가하면 실제 Content 경로, 버전, Skeleton 및 용도를 `Docs/Development/AssetPlan.md`에 기록하고 정확한 경로를 `.gitignore`에 추가한다.

## Checkpoint와 검증

- 각 작업은 현재 Phase 문서의 작업 순서, 검수 기준과 커밋 게이트를 따른다.
- 비자명한 로직에는 가장 작은 실행 가능한 검증을 남긴다. 새 테스트 프레임워크는 실제 필요가 없으면 추가하지 않는다.
- C++ 변경은 Unreal Engine 5.8 Development Editor 빌드를 확인한다.
- 에셋 및 Blueprint 변경은 Editor 로드와 관련 에셋 오류 여부를 확인한다.
- 멀티플레이 변경은 Phase 문서가 요구하는 PIE 플레이어 수와 시나리오로 검증한다.
- 실행하지 않은 빌드, PIE, 패키지 로드 또는 네트워크 검증을 완료했다고 기록하지 않는다.
- 구현 후 일반 정확성 검토를 먼저 하고, 별도로 과잉 구현과 삭제 가능한 구조를 검토한다.

## Git 기록

- `main`에는 Checkpoint 검수 기준을 통과한 완결 커밋만 남긴다.
- 커밋 제목은 `YY/MM/DD <Workstream> NN - <Title>` 형식을 사용한다.
- 커밋 본문은 `DONE` 다음에 실제 구현 결과와 실행한 검증을 한국어 항목으로 기록한다.
- 수정 과정의 작은 커밋은 원격 공유 전에 의미 있는 Checkpoint 단위로 정리한다.
- 외부 에셋, 생성물, 캐시와 로컬 사용자 파일이 스테이징되지 않았는지 커밋 전에 확인한다.

## 출처

최소 구현 원칙은 Dietrich Gebert의 [Ponytail](https://github.com/DietrichGebert/ponytail) 프로젝트에서 영감을 받아 PROJECT-LUX와 Unreal 네트워크 게임 개발에 맞게 조정했다. Ponytail은 MIT License로 공개되어 있다.
