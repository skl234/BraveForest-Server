# Brave Forest — Server

C++로 만든 Windows IOCP 기반 MMORPG 서버입니다. 계정 인증, 캐릭터 관리, 존·채널 이동, 전투, 채팅과 파티 기능을 구현했습니다.

## 구성

| 프로젝트 | 역할 |
| --- | --- |
| ProxyServer | 클라이언트 연결 관리, 요청 라우팅과 응답 중계 |
| LoginServer | 회원가입, 계정 인증과 접속 기록 |
| WorldServer | 캐릭터 생성·조회·선택, 세션과 존 이동, 전체채팅·귓속말·파티 |
| ZoneServer | 채널별 이동·전투·몬스터 처리, 섹터 기반 가시영역 관리 |
| Network_Core | IOCP와 비동기 TCP 송수신 |
| Utility_Core | Job 실행기, 큐와 객체 풀, 공통 자료구조 |
| ODBC_Core | DB 연결 풀과 쿼리·결과 처리 |
| Common | 서버와 클라이언트가 사용하는 패킷·데이터 형식 |

IOCP 스레드에서 받은 패킷을 Job으로 넘겨 처리합니다. Proxy는 세션 기준으로 실행기를 선택하고, Zone은 채널별 실행기에서 게임 로직을 처리합니다.

[시연 영상](https://youtu.be/tBjsei8geok)
