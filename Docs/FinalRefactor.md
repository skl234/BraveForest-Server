# 설정 클래스와 공격 대상 정리

## 서버 시작

```text
main()
  ├─ 서버별 Config::Load()
  │    ├─ 고정 설정값
  │    └─ CINIFile → 로컬 INI / 상대경로
  └─ Service.Initialize() → Run() → Stop()
```

`Network_Core/INI_CONFIG.h`는 설정 구조체로 유지했습니다. INI를 읽는 기존 공용 클래스는 없어서 `Utility_Core/INIFile.h/.cpp`에 `CINIFile`을 추가했습니다. 특정 서버에 종속되지 않아 다른 곳에서도 사용할 수 있습니다.

`CProxyServerConfig`, `CLoginServerConfig`, `CZoneServerConfig`는 각 서버의 `ServerConfig.h/.cpp`에 있습니다. 설정을 구조체에 채우는 정적 `Load()`만 제공합니다. 서버별 기본값과 정책을 유틸리티로 옮기지는 않았습니다.

포트, 스레드 수, 풀 크기, 경로 기준은 기존 값을 유지합니다. [전체 설정표](ServerConfiguration.md)를 참고합니다. 로그인·존의 DB 비밀번호는 계속 `Config/Database.local.ini`에서만 읽습니다.

프로젝트 참조는 Debug/Release 동일하게 사용합니다. 설정/초기화/실행 실패 시 main()은 1을 반환하고 정상 종료는 0을 반환합니다.

## 공격 대상

| 공격 주체 | 보관 값 | 대상을 찾는 곳 |
| --- | --- | --- |
| CPlayer | uint64_t m_attackTargetObjectNum | 같은 채널의 MonsterManager::Find(number) |
| CMonster | uint64_t m_attackTargetObjectNum | 같은 채널의 PlayerManager::Find(number) |

두 클래스 모두 `GetAttackTargetObjectNum()`으로 번호를 확인합니다. `UINT64_MAX`는 대상 없음이고, `0`은 정상 객체 번호입니다. Player와 Monster의 번호 공간은 별개이며 채널마다 풀/목록도 따로 존재합니다.

몬스터 스케줄러가 `Update(now, deltaTime, playerManager)`로 같은 채널의 플레이어 매니저를 전달합니다. 몬스터는 그때 번호로 조회하고 상태 함수에는 그 호출 중에만 사용하는 포인터를 전달합니다. `CPlayer*`를 타겟 멤버로 보관하지 않습니다. 패킷을 구성할 때도 같은 번호를 조회합니다.

```text
Channel.RemovePlayer()
  → Field.Leave()
  → 각 Monster.ReleaseTarget(playerNumber)
  → PlayerManager.Release()
  → 같은 번호 재사용 가능
```

채널 이동·존 이동·접속 종료·마을 부활에서도 이 해제 경로를 사용합니다. 번호가 재사용되기 전에 몬스터의 추적을 끊습니다. 조회한 플레이어가 비활성/사망/다른 필드에 있다면 타겟을 해제하고 복귀합니다. 다른 채널의 플레이어가 같은 번호를 갖더라도 공격 대상이 섞이지 않습니다.

패킷 번호, 구조체 크기, 멤버 순서는 변경하지 않았습니다. 몬스터 AI 간격과 이동/피격 연출 정책도 유지합니다. 스크린샷의 옛 `CPlayer* m_attackTarget` 설명은 위 표로 수정해야 합니다. 포트폴리오 원본 파일 자체는 이번 작업에서 수정하지 않았습니다.

## 코드 정리

- 불필요한 main() 안내/중복 주석, 주석 처리된 옛 코드를 정리했습니다.
- 존의 긴 설명 주석은 필요한 이유만 짧게 남겼습니다.
- 탭 들여쓰기, 멤버 정렬, 한 줄 함수 매개변수, 중괄호 줄바꿈을 기존 방식에 맞췄습니다.
- 직접 관리하는 C++/C# 소스와 VS 소스·프로젝트 파일은 UTF-8 BOM/CRLF입니다. 외부 에셋과 바이너리는 변환하지 않습니다.
- `.editorconfig`와 `.gitattributes`도 유지했습니다.

순수 서식 변경은 코드 토큰/문자열 또는 공백을 제외한 내용이 동일한지 비교했습니다. 클래스 분리와 대상 번호 변경은 별도 동작 테스트로 검사합니다.

계정·캐릭터 데이터 삭제나 수정, DB 스키마 변경, GitHub 업로드는 하지 않았습니다. 변경 전 소스와 임시 검증 결과는 공개 제외된 Build 폴더에 보관합니다.

## 최종 검증 결과

| 검사 | 결과 |
| --- | --- |
| Login / Proxy / Zone Debug x64 | 빌드 성공 |
| Login / Proxy / Zone Release x64 | 빌드 성공 |
| 공개 후보 소스만 새 폴더에서 Release 빌드 | 세 서버 성공. 기존 obj/lib/exe 없이 빌드 |
| 배포 EXE 기동 | 로그인 30003, 마을 30004, 비기너 30005, 프록시 30002 Listen 확인 |
| 배포 EXE 종료 | 네 프로세스 모두 q 정상 종료, 종료 코드 0 |
| 배포 EXE 비교 | x64/Release 원본과 네 배포 파일의 SHA-256 일치 |
| INI / 설정 | 23개 통과: 기본값, 한글, 긴 값, 상대경로, 누락 파일, 재로드 |
| 타겟 번호 | 18개 통과: 0번, 타겟 변경, 다른 채널, 풀 재사용, 사망/비활성 해제 |
| 전투 | 29개 통과: 속사, 범위기, 피격/취소, 관찰자 전송, 채널 분리 |
| 채널 이동 | 17개 통과: 목적지 초기화, 이동 스냅샷, 타겟 해제 |
| 사망/부활 | 21개 통과: 사망 제한, 저장 실패/재시도, 중복 요청, 마을 복귀 |
| 실제 비기너 필드 경로 | 1,504개 경로 샘플, Block 진입 실패 0 |
| 더미 채팅 | 이후 500명 접속 / 무작위 최대 100명 채팅으로 조정. 38개 재검증 통과 |
| Unity 전투 / 원격 표시 | 70개 + 39개 통과, 런타임 오류 없음 |
| 소스·프로젝트 파일 | 직접 관리하는 321개 UTF-8 BOM / CRLF. 혼합 줄바꿈 0 |
| 공개 후보 검사 | 로컬 DB 설정, 빌드 결과, 계정 덤프, 실제 비밀번호 노출 없음 |

100개 세션, 채널 두 개, 채널당 몬스터 512개를 사용하는 서버 코드 테스트도 포함합니다. 속사 결과 100,000개의 대상/수신 채널을 검사했습니다. 실제 Unity 창 100개나 100개의 TCP 접속으로 최대 성능을 측정한 결과는 아닙니다. Unity는 실제 프리팹과 서버 패킷을 사용하는 자동 회귀 검사이며, 이번 작업에서 수동 실플레이나 영상 촬영은 하지 않았습니다.

최종 빌드에는 기존 `Network_Core/ServerService.cpp`의 C4244, `ProxyServer/ClientConnection.cpp`의 C4267 경고가 남아 있습니다. 새 소스의 인코딩 경고는 없습니다. 중간 재링크에서 나타났던 오래된 PDB 경고는 관련 프로젝트를 새로 빌드한 최종 Release에서는 나오지 않았습니다.

세부 로그는 서버 `Build/Validation/ConfigTarget_*.log`, `ConfigVerification.log`, `TargetNumberVerification.log`와 Unity `Build/Validation/ConfigTarget_UnityCombat.log`에 있습니다.

## 실행 파일

`MMOServerExes` 아래 기존 폴더를 그대로 사용합니다.

1. `LoginServer/LoginServer.exe`
2. `ZoneServer_Town/ZoneServer.exe`
3. `ZoneServer_Beginner/ZoneServer.exe`
4. `ProxyServer/ProxyServer.exe`

백엔드 세 개를 먼저 켜고 프록시를 마지막에 켭니다. 존 EXE는 동일한 파일이며 각 폴더의 `Config/ZoneServer.ini`로 구분합니다. EXE만 다른 곳에 옮기지 말고 Config/Data를 포함한 폴더 단위로 사용합니다. 비밀번호가 있는 배포 폴더는 공개하지 않습니다.

이후 VS에서 Release x64로 빌드하면 기존 배포 대상 설정이 EXE와 필요한 설정/데이터를 이 폴더로 복사합니다. [GitHub 업로드 안내](../GITHUB_GUIDE.md)는 별도 문서를 참고합니다.
