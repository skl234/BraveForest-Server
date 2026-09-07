# 존/필드 파일 구조

현재 기준은 루트의 [CURRENT_STATE.md](../CURRENT_STATE.md)입니다. 과거 테스트용 포트·채널 수·시작 존을 현재 설정과 혼동하지 않습니다.

## 원본과 사용처

| 원본 | 로더 / 사용처 | 읽는 시점 |
| --- | --- | --- |
| ZoneInfo.txt | CZoneData → LoginServer / ZoneServer | 서버 초기화 |
| TownField.txt / BeginnerZoneField.txt | CField → CellGrid / SectorGrid | 채널 초기화 |
| TownSpawn.txt / BeginnerZoneSpawn.txt | CMonsterSpawnData → MonsterManager | 서버 시작 시 읽고 채널별 생성 |
| DB MonsterTypes | CMonsterDBLoader | 서버 시작 시 공통 능력치 로드 |
| DB PlayerLevelStats | CPlayerStatDB | 서버 시작 시 성장표 로드 |
| DB Characters | CCharacterDB | 캐릭터 입장 시 로드, 퇴장/존 이동/정상 종료 시 저장 |

파일 변경은 실행 중인 서버에 자동 반영되지 않습니다. 내보낸 뒤 서버를 정상 종료하고 재시작합니다. 로그인과 모든 존은 같은 ZoneInfo 파일을 사용합니다.

## 현재 설정

- 새 캐릭터 시작 존: 마을(ZoneId 1), 위치 (50, 0, 20).
- 비기너존: ZoneId 2, 기본 입장 위치 (22, 0, 23).
- 포탈: 마을 → 비기너존, 비기너존 → 마을. 베테랑존은 미구현.
- 마을 논리 필드 100 × 100, 비기너존 220 × 220. Cell 1단위, Sector 폭 80.
- 마을 스폰 0, 비기너존 스폰 150. 스폰 파일은 타입과 위치/Y회전만 가짐.
- 각 존 고정 채널 10개, 채널별 최대 플레이어 256명.
- IP 192.168.219.100, 프록시 30002 / 로그인 30003 / 마을 30004 / 비기너존 30005.

## 파일 형식

ZoneInfo는 UTF-8 텍스트이며 이름/씬이름은 따옴표로 감쌉니다.

```text
ZoneCount 2
Zone 1 "평온한 마을" "TownScene" 50.000 0 20.000 1 1
Zone 2 "비기너존" "BeginnerZoneScene" 22.000 0 23.000 0 1
PortalCount 2
Portal 1 1 2 22.000 0 23.000 1
Portal 2 1 1 27.000 0 80.000 1
```

- Zone: ZoneId, 이름, 씬이름, 기본입장 X/Y/Z, 시작존 여부, 사용 여부.
- Portal: 출발ZoneId, PortalId, 목적ZoneId, 목적입장 X/Y/Z, 사용 여부.
- 목적 입장 위치는 포탈 문 transform과 다릅니다. 이동 직후 다시 포탈을 타지 않도록 분리합니다.

Field 파일은 WidthX/Z, CellRows/Cols, SectorWidth/Rows/Cols, BlockCount 및 Block index 목록을 가집니다. 서버 Y 좌표는 0입니다. 스폰은 별도 파일로 읽습니다.

```text
ZoneId 2
SpawnCount 1
Spawn 0 3 45.500 39.500 0.000
```

Spawn 항목은 index, monsterType, X, Z, rotationY 순서입니다. HP, 경험치, 속도, AI 설정은 MonsterTypes에 있습니다. 비정상 좌표나 index, 잘못된 파일 형식은 로더가 실패로 처리합니다.

## Unity 내보내기

1. 실제 존씬의 ZoneSceneInfo와 ZonePortal 설정을 수정합니다.
2. MonsterSpawnPoint와 공통 몬스터 프리팹 설정을 관리합니다.
3. 지형/장애물이 바뀌었다면 FieldDataExporter로 Field 파일을 갱신합니다.
4. MYFolder → FieldData → 서버용 존 및 스폰 파일 내보내기를 실행합니다.
5. Assets/MYFolder/ExportedFieldData와 형제 MMONetwork2/Data에 반영된 파일을 확인합니다.
6. 타입 능력치를 바꿨다면 생성된 MonsterTypes SQL을 검토하여 별도로 적용합니다. 에디터 내보내기 자체는 DB에 쓰지 않습니다.
7. 맵 이미지 변경은 ZoneMapBuilder의 RefreshTownMap / RefreshBeginnerMap을 사용합니다. 현재 저장된 씬을 기준으로 생성하며 지형을 새로 만들지 않습니다.

같은 타입의 몬스터 설정은 공통 프리팹에서 통일합니다. 서버는 몬스터 타입 데이터와 파일 스폰을 조합해 채널별 객체를 만듭니다.

## 실행 폴더와 DB

Visual Studio Release/x64 빌드 후 MMOServerExes의 LoginServer, ProxyServer, ZoneServer_Town, ZoneServer_Beginner 폴더를 사용합니다. ZoneServer.exe 이름은 같지만 각 폴더의 Config/ZoneServer.ini로 구분됩니다. Field/Spawn/ZoneInfo 경로는 INI 디렉터리 기준입니다.

DB 접속 정보는 Config/Database.local.ini에서 읽습니다. 예제 파일은 Config/Database.example.ini이며 로컬 파일은 Git에 올리지 않습니다. 빌드 시 각 실행 폴더의 Config로 복사됩니다.

LoginServer는 Account/Game 풀을 각각 사용합니다. ZoneServer는 Game 풀 하나를 CharacterDB/PlayerStatDB/MonsterDBLoader가 공유합니다. ConnectionCount를 생략하면 채널 수를 사용하며 채널 수 미만은 거부합니다. SQL 호출은 동기 방식입니다.

레벨·경험치·위치는 이동/타격마다 DB에 기록하지 않고 퇴장·존 이동·정상 종료 시 저장합니다. 프로세스 강제 종료나 전원 차단에서 마지막 저장 이후 변경까지 보장하지는 않습니다.
