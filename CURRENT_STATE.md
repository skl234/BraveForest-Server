# 현재 구현 기준 — 2026-09-07

과거 검증 문서나 백업 이미지 대신 아래 실제 소스·설정·씬을 기준으로 설명합니다. `Build/LocalArchive`는 변경 전 복구 자료이며 현재 기능 설명에 사용하지 않습니다.

## 맵과 접속

| 구분 | 마을 | 비기너존 |
| --- | --- | --- |
| ZoneId | 1 | 2 |
| 씬 | TownScene | BeginnerZoneScene |
| 이름 | 평온한 마을 | 비기너존 |
| 논리 필드 크기 | 100 × 100 | 220 × 220 |
| 섹터 폭 | 80 | 80 |
| 채널 수 / 채널 정원 | 10 / 256 | 10 / 256 |
| 파일 스폰 수 | 0 | 150 |
| 새 캐릭터 시작 존 | 예 | 아니오 |

위 필드 크기는 실제 파일 및 씬의 좌표 범위입니다. 화면 밖을 가리기 위한 장식 지형의 전체 크기와는 구분합니다. 베테랑존은 미구현이며 실제 입장 가능한 존은 위 두 개입니다.

Unity 빌드 씬 순서: `LoginScene → BeginnerZoneScene → TownScene`. 이것은 빌드 등록 순서입니다. 실제 신규 캐릭터 입장 순서는 `로그인 → 캐릭터 선택 → 마을`이며 서버의 ZoneId에 따라 씬을 선택합니다.

## 코드 기준

- 현재 더미 접속은 `Dummy0001~Dummy0500` 500명입니다. 안전 초기 배치는 마을 250명 / 비기너존 250명이며, `t + Enter` 채팅은 온라인 더미 중 무작위 최대 100명만 전송합니다. 남은 계정은 삭제하지 않고 접속 대상에서 제외했습니다.

- 서버: `ZoneServer/Channel`, `Field`, `SectorGrid`, `MonsterScheduler`, `Navigator`, `PacketTask`.
- 네트워크 송수신: `Network_Core/TCPConnection`, 프록시 라우팅: `ProxyServer`.
- 설정: `Utility_Core/CINIFile`과 각 서버의 `CProxyServerConfig`, `CLoginServerConfig`, `CZoneServerConfig`. main()은 설정 로드와 서비스 실행만 담당합니다.
- 공격 대상: Player와 Monster 모두 `m_attackTargetObjectNum`. 대상 없음은 `UINT64_MAX`, 0번은 정상 객체입니다. 몬스터는 같은 채널 PlayerManager에서 번호로 조회하며, 풀 반환 전에 대상 번호를 해제합니다. 예전 포트폴리오의 `CPlayer* m_attackTarget` 설명은 현재 코드와 다릅니다.
- 파일 원본: `Data/ZoneInfo.txt`, `TownField.txt`, `BeginnerZoneField.txt`, `TownSpawn.txt`, `BeginnerZoneSpawn.txt`.
- Unity 씬: `Assets/MYFolder/Scenes`의 LoginScene, TownScene, BeginnerZoneScene만 실제 빌드 씬입니다.
- Unity 지도: `Assets/MYFolder/Image/Map/TownMap.png`, `BeginnerZoneMap.png`.
- UI와 지도 연결: `ZoneSceneInfo`, `ZoneMapUI`, `ZoneInterface.prefab`.
- 지도 갱신: `Assets/MYFolder/Editor/ZoneMapBuilder.cs`. 저장된 현재 씬을 촬영하고 Block 경계를 부드럽게 표시합니다. 옛 마을을 재생성하지 않습니다.
- NavMesh: 비기너존의 기존 두 데이터는 `Scenes/BeginnerZoneScene`으로 이동했습니다. GUID와 내용은 유지합니다. 지형별 데이터는 공유하지 않고, 빌드 도구/구성만 재사용합니다.

## 실제 최신 이미지 찾기

Unity 프로젝트의 `Build/Validation/Current`에 현재 저장된 씬을 직접 읽어 생성한 지도·45도 전경·참조 목록이 있습니다. 이 폴더도 자동 업로드 대상은 아닙니다. 포트폴리오에 쓸 이미지만 검토해서 별도로 복사합니다.

`Build/LocalArchive/PreviousValidation`의 화면은 과거 검증 중 촬영된 자료입니다. `OfflineCombat`, 옛 Town 생성 코드, 예전 문서를 현재 온라인 기능의 근거로 사용하지 않습니다.

## 공개 전 유의사항

- 로컬 DB 계정/캐릭터 덤프, 비밀번호, `Database.local.ini`는 업로드하지 않습니다.
- 원본 에셋의 제작자·라이선스는 별개이며 외부 에셋을 직접 제작했다고 기술하지 않습니다.
- UI, 네트워크, 동기화는 실제 소스와 이번 검증 결과를 구분해서 설명합니다. 단위/시뮬레이션 테스트를 실제 1,000개 Unity 화면 동시 실행으로 표현하지 않습니다.
