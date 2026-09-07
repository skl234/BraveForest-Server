# 서버 설정 구조

`INI_CONFIG`는 서비스에 넘길 설정값을 담는 구조체입니다. `main()`은 설정 클래스의 `Load()`를 호출한 뒤 서비스의 `Initialize → Run → Stop`만 실행합니다.

| 위치 | 클래스 | 역할 |
| --- | --- | --- |
| Utility_Core | CINIFile | INI 열기, 문자열/숫자 읽기, 상대경로 해석, EXE 위치 확인 |
| ProxyServer | CProxyServerConfig | 기존 프록시 기본 설정과 백엔드 목록 구성 |
| LoginServer | CLoginServerConfig | 로그인 기본 설정, DB INI, ZoneInfo 경로 구성 |
| ZoneServer | CZoneServerConfig | 존 INI, DB INI, 맵/스폰 파일 경로 구성 |

공용 파일 읽기는 유틸리티에 두고, 서버마다 다른 정책과 기본값은 각 서버에 둡니다. 유틸리티는 서버나 DB 클래스를 참조하지 않습니다. 기존 구조체와 서비스는 그대로 사용합니다.

## ProxyServer

`CProxyServerConfig::Load()`는 INI를 읽지 않고 아래 값을 설정합니다.

| 필드 | 설정 클래스의 값 |
| --- | --- |
| ip / port | 192.168.219.100 / 30002 |
| iocpThreadSize / logicThreadSize | 12 / 24 |
| postAcceptSize | 500 |
| maxClientConnection | 100000 |
| logPath | Log.log |
| backServerInfoList | 로그인 30003, ZoneId 1 마을 30004, ZoneId 2 비기너존 30005 |

연결 풀 상한은 메모리에 확보할 연결 객체 수입니다. 실제 동시 접속 처리 성능을 측정한 값은 아닙니다.

## LoginServer

`LOGINSERVER_CONFIG : INI_CONFIG`에 값을 채웁니다.

| 출처 | 필드 / 값 |
| --- | --- |
| Config/Database.local.ini | dbUser ← User, dbPassword ← Password, accountDsn ← AccountDsn, gameDsn ← GameDsn |
| CLoginServerConfig | ip=192.168.219.100, port=30003, iocpThreadSize=6, logicThreadSize=1 |
| CLoginServerConfig | postAcceptSize=1, maxClientConnection=1, logPath=Log.log |
| CLoginServerConfig | accountConnectionCount=1, gameConnectionCount=1, packetTaskPoolSize=100000 |
| 외부 txt | Data/ZoneInfo.txt를 CZoneData로 읽음. INI가 아님 |

로그인의 maxClientConnection=1은 **프록시와 연결하는 소켓 수**입니다. 게임 유저를 1명만 받는다는 뜻이 아닙니다. 첫 실행 인자를 주면 ZoneInfo 파일 경로를 대체합니다.

## ZoneServer

`ZONESERVER_CONFIG : INI_CONFIG`에 값을 채웁니다. 같은 exe가 폴더별 설정으로 마을/비기너존을 구분합니다.

| 출처 | 키 → 필드 |
| --- | --- |
| Config/ZoneServer.ini [Zone] | ZoneId → zoneId, IP → ip, Port → port, IOCPThreadSize → iocpThreadSize |
| Config/ZoneServer.ini [Zone] | ZoneInfoPath / FieldInfoPath / MonsterSpawnPath → 각 파일 경로 |
| Config/ZoneServer.ini [Channel] | ChannelCount / MaxPlayerPerChannel / MaxMonsterPerChannel |
| Config/ZoneServer.ini [Database] | ConnectionCount → dbConnectionCount. 생략하면 채널 수 |
| Config/Database.local.ini [Database] | User → dbUser, Password → dbPassword, GameDsn → gameDsn |
| CZoneServerConfig | logPath=ZoneServer.log, postAcceptSize=1, maxClientConnection=1, packetTaskPoolSize=65536 |

현재 마을은 ZoneId 1 / 30004, 비기너존은 ZoneId 2 / 30005입니다. 둘 다 IOCP 6개, 채널 10개, 채널별 플레이어 최대 256명입니다. 채널별 논리 실행기는 채널이 소유하므로 로그인/프록시처럼 logicThreadSize에 별도 개수를 넣지 않습니다.

현재 파일에는 MaxMonsterPerChannel=150이 들어 있지만, 코드에서 이 키를 읽을 때의 기본값은 512입니다. **파일에 키가 있으면 파일 값이 우선**합니다. 키 누락 시 기본값과 운영 설정은 구분해야 합니다.

ZoneInfo/Field/Spawn 파일은 INI와 같은 디렉터리를 기준으로 상대 경로를 해석합니다. DB 로컬 설정은 exe 디렉터리의 Config/Database.local.ini입니다. ZoneServer의 첫 실행 인자는 ZoneServer.ini 경로를 대체할 수 있지만 DB 로컬 설정의 위치를 바꾸지는 않습니다.

## DB 설정과 라이브러리 연결

DB User / Password / DSN은 로컬 INI에서 읽습니다.

`Config/Database.example.ini`는 공개할 수 있는 예제입니다. `Database.local.ini`는 실제 값이므로 Git에서 제외합니다. VS 빌드가 로컬 파일을 개발 출력 및 Release 배포 폴더의 Config로 복사합니다. exe 옆 파일이 없으면 오류를 출력하고 종료하며 하드코딩된 비밀번호로 대체하지 않습니다.

Debug와 Release 모두 `.vcxproj`의 프로젝트 참조로 라이브러리를 연결합니다.
