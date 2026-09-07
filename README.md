# Brave Forest — MMO Server

Windows IOCP 기반 C++ MMORPG 서버 프로젝트입니다. Unity 클라이언트와 연결하여 로그인, 캐릭터 입장, 존/채널 이동, 섹터 가시성 관리, 몬스터 AI 및 전투를 처리합니다.

## 시연 영상

[BraveForest 플레이 영상 보기](https://youtu.be/IJQr8NuZfT0)

[Unity 클라이언트 소스](https://github.com/skl234/BraveForest-Client)

## 구성

- `ProxyServer`: 클라이언트 연결과 로그인/존 서버 라우팅
- `LoginServer`: 계정 인증, 캐릭터 목록·생성, 입장 정보
- `ZoneServer`: 채널별 Field, Player/Monster, SectorGrid, Navigator, 전투 처리
- `Network_Core`: IOCP, TCPConnection, 비동기 송수신
- `Utility_Core`: 실행기, 작업 큐/풀, 버퍼 등 공용 기능
- `ODBC_Core`: DB 연결, 연결 풀, 쿼리
- `Common`: 공용 패킷과 존 설정 로더
- `Tools/DummyClient`: 한 프로세스에서 여러 테스트 접속을 구동하는 더미 클라이언트

채널마다 MPSC 작업 실행기와 필드를 소유합니다. 섹터 진입/이탈 시 가시 객체의 차이를 전송하며, 몬스터 스케줄러는 채널의 몬스터 목록을 순회합니다. 몬스터는 Navigator의 경로와 경과 시간으로 이동합니다.

## 빌드

1. Windows, Visual Studio 2019 C++ 데스크톱 개발 도구(v142), Windows SDK를 준비합니다.
2. `Config/Database.example.ini`를 `Config/Database.local.ini`로 복사하고 자신의 DB 계정 정보를 입력합니다. 로컬 파일은 Git에서 제외됩니다.
3. 64비트 MySQL ODBC 드라이버와 `account_db`, `game_db` DSN을 준비합니다. **새 빈 DB에만** `Database/Schema.sql` 다음 `Database/GameBalance.sql`을 적용합니다. 이미 데이터가 있는 DB에는 초기화 SQL을 무작정 다시 실행하지 않습니다. DB 사용자 생성/권한 부여는 자신의 환경에서 별도로 설정합니다. 로컬 DB의 계정/캐릭터 데이터는 저장소에 포함하지 않습니다.
4. `MMONetwork.sln`을 열고 `Release / x64`를 선택합니다.
5. `LoginServer`, `ProxyServer`, `ZoneServer`를 빌드합니다. 필요한 공용 라이브러리는 프로젝트 의존성으로 빌드됩니다.
6. `MMOServerExes`에 로그인·프록시·마을 존·비기너 존의 실행 폴더가 생성됩니다. 각 폴더의 EXE를 실행합니다. 로그인/존을 먼저 켜고 프록시를 마지막에 켭니다.

빌드 후 DB 로컬 설정도 각 실행 폴더의 `Config`로 복사됩니다. 실행 폴더 전체를 공개 배포하면 안 됩니다. `Database.local.ini`를 반드시 제외해야 합니다.

현재 개발 네트워크 주소는 `192.168.219.100`입니다. 다른 PC에서는 바인딩/접속 주소, 방화벽, ODBC DSN을 해당 환경에 맞춰 설정해야 합니다.

## 문서

- [존/필드 파일과 DB 역할](Data/README.md)
- [서버 설정 클래스와 INI 구분](Docs/ServerConfiguration.md)
- [실행 폴더 구성](Build/ServerRuntime/README.md)

## 더미 클라이언트

`Tools/DummyClient/Build.cmd`로 빌드합니다. 한 프로세스에서 500개 계정에 접속하고 이동 패킷을 전송합니다. `t` 입력 후 Enter를 누르면 접속 중인 더미 중 무작위로 최대 100명이 채팅하고, `q` 입력 후 Enter를 누르면 종료합니다. 테스트용 계정과 캐릭터는 별도로 준비해야 합니다.

채널 수와 연결 풀 크기는 설정값이며, 해당 인원의 동시 처리 성능을 측정한 결과를 뜻하지 않습니다.
