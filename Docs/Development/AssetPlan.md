# PROJECT LUX - Asset Plan

> 문서 상태: Pre-Production / Candidate Registry  
> 목적: 보유 Fab 라이브러리와 신규 후보를 PROJECT LUX의 개발 Phase에 연결하고, 실제 채택 전 검증 항목을 고정한다.  
> 원칙: 에셋은 이름만 보고 채택하지 않는다. 필요한 Phase에서 직접 프로젝트에 넣어 기술/시각/라이선스/성능을 검증한 뒤 채택한다.

---

# 0. 사용 원칙

## 0.1 임시 에셋 교체 방식 지양

PROJECT LUX는 가능한 한 처음부터 실제 출시 빌드에 계속 사용할 수 있는 에셋을 선택한다.

단, 다음은 허용한다.

- 같은 최종 에셋 팩의 일부만 먼저 사용
- 실제 기능 검증용 Test Map
- 개발 전용 Debug Actor / Console Command
- Phase에서 후보를 검증한 뒤 부적합하면 해당 Phase 안에서 교체

## 0.2 Blueprint 정책

Blueprint 사용 여부만으로 에셋을 탈락시키지 않는다.

채택 판단 순서:

1. PROJECT LUX와의 적합성
2. 직접 구현 대비 절약되는 시간
3. 멀티플레이/서버 권한 구조와의 충돌 여부
4. C++ 프로젝트에 격리 또는 확장 가능한지
5. Blueprint 의존도
6. UE 5.8 호환성
7. 라이선스
8. 유지보수 비용

게임의 핵심 권한 구조는 외부 Blueprint Framework에 맡기지 않는다.

## 0.3 라이선스

- 공개 Fab 페이지에서 현재 라이선스 유형을 확인한다.
- 과거 UE Marketplace 취득 에셋의 실제 보유 라이선스가 중요한 경우 사용자에게 확인한다.
- 상용 출시 가능한 권리가 확인되지 않은 에셋은 채택 완료로 표시하지 않는다.
- CC 계열 에셋은 표시 의무를 별도 기록한다.

---

# 1. Phase 00~01 우선 Asset Set

현재 첫 번째 목표는 **멀티플레이 리볼버 FPS 기반**이다.

## 1.1 환경 - Big Star Station

보유 에셋.

Fab:
https://www.fab.com/listings/9cff72cf-bd72-4f4b-bab0-ec556a25e37d

현재 조사:

- 142 Unique Meshes
- Modular walls / floors / ceilings
- Corridors
- Large rooms
- Terminal panels
- Light fixtures
- Cables
- Caution decals
- Lumen setup
- 폐쇄 SF 시설 컨셉과 현재 보유 환경 중 가장 높은 적합성

Phase 00 검증:

- UE 5.8에서 정상 로드
- Character scale과 문/복도 폭 확인
- Collision 품질
- 5~6인 플레이 시 시야/동선 문제
- Lumen 사용 시 기본 성능
- 가까운 1인칭 시점에서 텍스처/메시 품질
- 조명 Mesh를 향후 파괴 가능한 Actor로 분리하기 쉬운지

채택 기준:

- 위 조건에 치명적 문제가 없으면 시설 기본 Kit 1순위로 유지
- 전체 Demo Map을 그대로 쓰지 않고, 실제 게임에 사용할 모듈로 Test Facility를 구성

현재 상태: **Primary Candidate / Phase 00 Validation**

---

## 1.2 시민 캐릭터 - Stylized Character Kit: Casual 01

보유 에셋.

Legacy Marketplace:
https://www.unrealengine.com/marketplace/en-US/product/stylized-male-character-kit-casual

현재 조사:

- RocketArts 제작
- Modular stylized male character
- Head / Head Accessory / Torso / Arms / Legs
- 3 pre-constructed characters
- Epic Skeleton Rig
- 23k~31k vertex 수준
- 원래 지원 표기는 UE 5.4까지
- 일반 실험 참가자 설정에는 군인 캐릭터보다 적합
- Epic 공식 Modular Character 문서에서도 예시용 자산으로 사용된 이력이 있음

주의:

- Fab로 정상 마이그레이션되지 않은 Legacy 자산
- UE 5.8 공식 지원 표기가 없음
- IK bones 없음
- 남성 중심 구성
- 최종 게임에 필요한 성별/외형 다양성은 부족할 수 있음

Phase 00 검증:

- UE 5.8 Migration
- Skeleton / IK Retarget
- Animation Starter Pack과의 호환
- 5~6개 외형 Variation 확보 가능 여부
- 1인칭 카메라에서 자기 몸 일부를 사용할지 여부와 호환
- 타 플레이어 3인칭 표시 품질

채택 기준:

- 기술적으로 안정적이고 아트 방향에 맞으면 시민 기본 Character Kit로 사용
- 시각 방향 또는 다양성 문제가 크면 Phase 00 안에서 대체 후보를 확정한 뒤 완료

현재 상태: **Primary Owned Character Candidate / Phase 00 Validation**

---

## 1.3 기본 로코모션 - Animation Starter Pack

보유 에셋 / Epic 무료.

Fab:
https://www.fab.com/listings/98ff449d-79db-4f54-9303-75486c4fb9d9

현재 조사:

- Epic Games
- 62 animations
- Classic Mannequin 기준
- Legacy 태그가 있으나 UE 5.8 공식 문서에서도 Layered Animation / Retarget 예제로 계속 사용
- PROJECT LUX에서는 외부 Framework가 아니라 Animation source로만 사용

사용 목적:

- 기본 Idle / Walk / Run / Jump
- Phase 01 Third-Person Revolver upper-body animation과 lower-body locomotion blend의 기반
- 필요 애니메이션은 UE5 IK Retargeter로 프로젝트 캐릭터에 맞춤

현재 상태: **Use / Phase 00~01**

---

## 1.4 1인칭 리볼버 애니메이션 - Revolver one hand animations | First Person Gameplay

보유 에셋.

Fab:
https://www.fab.com/listings/b638db29-63a3-4eca-80eb-1fc8d932f0b9

현재 사용 방향:

- Phase 01의 FP 리볼버 팔 애니메이션 1순위
- Aim / Fire / Single Bullet Reload 사용
- Character / Revolver Control Rig 검증
- 포함 Revolver Mesh 2개를 최종 총기 후보로 먼저 확인
- R21 상체 애니메이션을 TP에도 재사용 가능한지 검증
- TP 하체는 별도 Locomotion과 Layered Blend Per Bone으로 결합
- Head / Neck은 Camera 방향 기반 Look / IK로 별도 처리

Phase 01 검증:

- UE 5.8 Import
- FP skeleton
- Single Bullet Reload
- Aim / Fire
- Control Rig
- 포함 Revolver Mesh 품질
- TP upper-body retarget 품질
- 포함 총기 SFX 실제 존재 여부
- Fire / Dry Fire / Hammer·Trigger / Cylinder Open·Close / Round Insert / Handling 커버 여부
- Local owner FP visibility
- Remote player FP arms 비노출

채택 기준:

- UE 5.8에서 문제가 없으면 기본 FP 리볼버 애니메이션 세트로 채택
- TP 상체 재사용이 자연스러우면 별도 TP 리볼버 애니메이션은 구매하지 않음
- 포함 Revolver Mesh가 부적합하면 애니메이션은 유지하고 Mesh만 교체
- 트레일러에서 확인된 총기 사운드가 실제 배포 Content에 포함되어 있고 품질이 충분하면 Phase 01 기본 Revolver SFX로 채택
- 사운드가 일부 부족한 경우 별도 SFX 팩 전체를 사기보다 부족한 항목만 보강

현재 상태: **Owned / Phase 01 Validation**

오디오 방향:

- 트레일러에서 리볼버 사운드가 확인되었으므로 우선 R21 내부 SFX 사용을 전제로 한다.
- 실제 SoundWave / SoundCue / MetaSound 포함 여부는 01-A에서 확인한다.
- 확인 전에는 별도 리볼버 SFX 에셋을 구매하지 않는다.
---

## 1.5 총격 VFX - Muzzle Flash

VLCG Revolver Animations는 Fire particle을 포함하지 않는다.

1차 후보:
https://www.fab.com/listings/435b3bcb-d7f5-467d-99aa-2edc97a6c5fd

Muzzle Flash (Niagara System)

현재 조사:

- Unreal Engine용
- Niagara
- 무료
- 학습용 무료 자산으로 배포
- 총구 화염 단일 목적이라 의존성이 작음

Phase 01 검증:

- 리볼버 크기와 비율
- 어두운 시설에서 순간 조명 느낌
- Multiplayer multicast 시 성능
- Live / Blank / Rubber에 동일한 발사 위협 연출로 사용할 수 있는지

채택 기준:

- 비주얼 품질이 충분하면 최종 사용
- 부족하면 Phase 01 안에서 Niagara 기반 유료 후보로 교체

현재 상태: **Primary Free Candidate / Phase 01 Validation**

---

## 1.6 피격 / 혈흔 / 환경 VFX - Realistic Starter VFX Pack Vol 2

보유 에셋.

Fab:
https://www.fab.com/listings/ac2818b3-7d35-4cf5-a1af-cbf8ff5c61c1

현재 조사:

- 56 unique effects
- Blood
- Hit
- Sparks
- Smoke
- Steam
- Fire
- Destruction
- Explosion 등
- MuzzleFlash는 이 Vol 2 설명에는 명시되지 않음
- 제작사의 별도 Niagara Pack이 존재하므로 이 팩의 내부 파티클 방식은 실제 프로젝트에서 확인 필요

Phase 01 사용 목적:

- 실탄 피격 Blood 후보
- 벽/금속 Impact 후보
- 향후 환경 Steam / Spark 재활용 가능성 확인

채택 기준:

- UE 5.8에서 문제 없고 기술 방식이 유지 가능한 경우 필요한 Effect만 사용
- Muzzle Flash 자산으로는 기대하지 않음

현재 상태: **Owned Candidate / Phase 01 Validation**

---

# 2. FPS 시스템에 사용하지 않는 기존 후보

## FPS Automatic Rifle 01 Animations

보유 에셋이지만 PROJECT LUX는 리볼버 단일 총기 방향으로 변경되었으므로 Phase 01에서 사용하지 않는다.

상태: **Not Used for PROJECT LUX FPS**

## Low Poly Shooter Pack - Free Sample

구형 UE 4.26~4.27 기반 Shooter Framework이므로 PROJECT LUX 코드 기반으로 사용하지 않는다.

필요한 독립 Mesh/Content가 특별히 발견되는 경우만 별도 검토.

상태: **Rejected as Framework**

## Ultimate Character

보유 에셋이지만 전체 Blueprint Character Framework이므로 현재 Player 기반으로 사용하지 않는다.

상태: **Not Used as Core Character Framework**

---

# 3. 후속 Phase Asset Candidate Registry

아래 후보는 지금 프로젝트에 전부 추가하지 않는다. 해당 Phase에 도달했을 때 검증한다.

| 에셋 | 예상 Phase | 목적 | 현재 판단 |
|---|---:|---|---|
| LXR - Light Detection | 07 | 실제 3D 광량 측정 | 핵심 후보 |
| Customizable Interaction Plugin | 12 | 공통 Interaction | 핵심 후보 |
| Replicated Grab System | 13~14 | 샘플/전력셀 물리 운반 | 핵심 후보 |
| Realistic Lab. Laboratory Equipment | Map/Task | 실험실 Props | 최종 사용 유력 |
| Modular Sci-Fi Indoor/Outdoor | Map | 시설 보조 Kit | 보조 후보 |
| Mission to Minerva | Map | 발전/산업 설비 Props | 보조 후보 |
| OLD OFFICE Interior | Map | 관리/기록/폐쇄 구역 | 선택 후보 |
| Surface Forge | 20 | 환경 Material Polish | 후반 후보 |
| Military Training Facility | 필요 시 | 산업/훈련 구역 Props | 보조 후보 |
| Mechanic Character Sarah | Character 재검토 | 인간 캐릭터 대체/추가 | 후보 |
| Sci-Fi Troopers Collection | Character fallback | 네트워크 Character 대역 | 낮은 우선도 |
| Elite Soldiers | Character fallback | 네트워크 Character 대역 | 낮은 우선도 |
| Primitive Characters | - | 컨셉 불일치 | 제외 |
| Hands for VR SciFi | - | Revolver kit와 중복 가능 | 보류 |
| Interaction System | - | Customizable Interaction Plugin보다 우선도 낮음 | 보류 |
| Inventory System | - | 현재 필요 범위보다 큼 | 제외 |
| Ninja Input | - | Native Enhanced Input으로 충분 | 제외 |
| Dunpix Weapon Customization | - | 단일 리볼버 게임에 과도함 | 제외 |

---

# 4. Phase별 Asset 검증 기록 방식

각 Phase 문서의 Asset Validation 결과에 다음을 기록한다.

- Asset name
- Source
- Owned / Free / Purchase
- License
- UE Version
- Import result
- Dependencies
- Blueprint/C++ dependency
- Network dependency
- Visual fit
- Performance issue
- Adopt / Partial Adopt / Reject
- Reason

에셋이 Reject되면 그 이유와 대체 후보를 남겨 같은 조사를 반복하지 않는다.

---

# 5. 현재 결론

Phase 00~01에서 우선 검증할 조합:

1. Big Star Station - 시설 Kit
2. Stylized Character Kit: Casual 01 - 시민 Character 후보
3. Animation Starter Pack - lower-body locomotion
4. Revolver one hand animations | First Person Gameplay - 보유 / FP 리볼버 애니메이션
5. Muzzle Flash (Niagara System) - 총구 VFX
6. Realistic Starter VFX Pack Vol 2 - 피격/혈흔 후보

이 조합이 통과하면 첫 번째 FPS Milestone에서 별도의 임시 총/캐릭터/맵으로 교체하는 과정 없이 후속 Phase로 이어간다.
