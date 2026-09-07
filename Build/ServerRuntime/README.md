# MMOServerExes — EXE 직접 실행

Visual Studio에서 MMONetwork.sln을 **Release / x64**로 빌드하면 실행 파일과 필요한 설정·데이터가 서버별 폴더에 자동 복사된다. 원래 Release 출력 폴더도 유지한다. CMD나 실행 인자를 사용할 필요가 없고, EXE 이름도 바꾸지 않는다.

## 폴더 구성

    MMOServerExes/
      README.md
      ProxyServer/
        ProxyServer.exe
      LoginServer/
        LoginServer.exe
        Config/Database.local.ini
        Data/ZoneInfo.txt
      ZoneServer_Town/
        ZoneServer.exe
        Config/ZoneServer.ini
        Config/Database.local.ini
        Data/ZoneInfo.txt
        Data/TownField.txt
        Data/TownSpawn.txt
      ZoneServer_Beginner/
        ZoneServer.exe
        Config/ZoneServer.ini
        Config/Database.local.ini
        Data/ZoneInfo.txt
        Data/BeginnerZoneField.txt
        Data/BeginnerZoneSpawn.txt

두 ZoneServer.exe는 같은 빌드 결과물이다. 하지만 각자 자기 EXE가 들어 있는 폴더의 Config/ZoneServer.ini를 읽는다. 마을 설정은 ZoneId 1·포트 30004, 비기너 설정은 ZoneId 2·포트 30005이므로 별도 프로세스로 각각 다른 지역을 담당한다.

## 실행 순서

아래 EXE를 직접 더블클릭한다. 앞선 서버의 기동을 확인하고 다음 서버를 실행한다.

1. LoginServer/LoginServer.exe
2. ZoneServer_Town/ZoneServer.exe
3. ZoneServer_Beginner/ZoneServer.exe
4. ProxyServer/ProxyServer.exe

Proxy는 Login과 두 Zone에 연결하므로 마지막에 실행한다. Zone 콘솔에 ZoneServer RunStart가 나오면 기동된 것이다. 종료할 때는 각 서버 콘솔에서 Q를 누른다. 빌드 전에 서버를 종료해야 EXE 잠금으로 인한 복사 오류를 피할 수 있다.

| 서버 | 주소 | 포트 | IOCP | 로직 / 채널 |
| --- | --- | --- | --- | --- |
| Proxy | 192.168.219.100 | 30002 | 12 | 로직 24개 |
| Login | 192.168.219.100 | 30003 | 6 | 로직 1개 |
| 마을 / ZoneId 1 | 192.168.219.100 | 30004 | 6 | 10채널, 채널당 로직 1개·256명 |
| 비기너 / ZoneId 2 | 192.168.219.100 | 30005 | 6 | 10채널, 채널당 로직 1개·256명 |

## 상대경로

ZoneServer는 GetModuleFileNameW로 실행 파일의 위치를 구해 Config/ZoneServer.ini를 찾는다. 로그인도 같은 방식으로 자기 폴더의 Data/ZoneInfo.txt를 찾는다. 현재 작업 디렉터리가 다른 곳이어도 실행 파일 기준으로 찾는다.

INI 안의 ../Data/TownField.txt 같은 경로는 INI가 있는 Config 폴더 기준이다. 따라서 같은 서버 폴더 안의 Data를 가리킨다. EXE 하나만 옮기지 말고 해당 서버 폴더를 통째로 옮기면 된다. 각 존은 다른 존 폴더의 파일을 참조하지 않는다.

개발용 Debug/Release 출력 폴더에서 실행할 때는 기존 개발 경로를 찾는 처리를 유지했다. 명시적으로 INI 인자를 주는 기존 방법도 사용할 수 있지만 위 배포 폴더에서는 필요 없다. 배포 폴더에 INI가 없으면 다른 존의 설정으로 대체하지 않고 실패한다.

## 원본과 자동 복사

- 설정 원본: 프로젝트의 ZoneServer/ZoneServer_Town.ini, ZoneServer/ZoneServer.ini.
- 마을 설정은 복사할 때만 ZoneServer_Town/Config/ZoneServer.ini라는 이름을 사용한다.
- 데이터 원본: 프로젝트의 Data 폴더. 영구 수정은 원본에서 하고 다시 빌드한다.
- 복사 설정: Build/MMOServerExes.targets. 입력·출력을 VS의 파일 복사 항목으로 추적한다.
- 이 안내의 원본: Build/ServerRuntime/README.md.

처음에는 솔루션을 빌드하거나 LoginServer·ZoneServer·ProxyServer 세 프로젝트를 모두 빌드한다. 이후에는 해당 프로젝트를 빌드할 때 그 서버의 파일이 갱신된다. Debug/Win32는 이 배포 폴더로 복사하지 않는다.

Release 라이브러리는 프로젝트 참조로 연결한다. 예전의 고정 .lib 경로를 따라 남아 있던 라이브러리를 연결하는 방식이 아니다.

## DB와 실행 환경

존·포탈·지형·스폰은 파일에서 읽고 몬스터 능력치·플레이어 성장값·캐릭터 정보는 DB에서 읽는다.

현재 PC의 64비트 ODBC 드라이버와 account_db, game_db DSN이 필요하다. 다른 PC에서는 DB 접속 환경, Visual C++ x64 런타임, 서버 IP와 방화벽 환경을 별도로 준비해야 한다. DB 비밀번호는 EXE에 내장하지 않고 로컬 INI에서 읽는다. 실행 폴더의 실제 INI가 외부에 공개되지 않게 주의한다.

마을 몬스터는 0마리, 비기너는 채널당 150마리다. 두 존 모두 채널별 MonsterScheduler Job을 약 500ms 간격으로 실행한다.

## 로컬 DB 설정

로그인·존의 DB 접속 정보는 프로젝트 Config/Database.local.ini에서 관리하며, 빌드 후 각 EXE 옆 Config 폴더로 복사된다. 공개용 예제는 Config/Database.example.ini이고 실제 비밀번호 파일은 Git에 올리지 않는다.
