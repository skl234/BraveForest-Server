# 소스 정리 및 검증 결과 — 2026-09-07

설정 클래스 분리와 타겟 번호 통일 이후의 최신 결과는 [최종 정리 기록](FinalRefactor.md)에 있습니다. 아래 내용은 그 이전 정리 단계의 기록입니다.

## 변경 범위

- ZoneServer의 59개 C++ 소스/헤더를 기존 Proxy 스타일에 맞춰 정리했습니다. 탭 들여쓰기, 멤버 정렬, 한 줄 함수 매개변수, 생성자 초기화 목록 및 중괄호/문장 줄바꿈을 정돈했습니다.
- 형식 변환 단계마다 공백·주석을 제외한 코드 토큰과 문자열 리터럴의 동일성을 비교했습니다. 전투·이동·섹터·스케줄러의 알고리즘은 바꾸지 않았습니다.
- 자체 소스/프로젝트 파일 315개의 줄바꿈을 CRLF로 확인했습니다. 최종 검사에서 LF 단독 또는 CR 단독 줄바꿈은 0개입니다. 자체 C++/C# 소스 인코딩도 UTF-8 BOM으로 통일했습니다. 외부 에셋/패키지 소스와 바이너리 파일은 일괄 변환하지 않았습니다.
- `.editorconfig`, `.gitattributes`로 이후 편집 및 Git 체크아웃 기준을 추가했습니다. Visual Studio에서 파일 변경을 다시 읽으라는 안내는 외부 편집 알림으로, 혼합 줄바꿈 경고와 다릅니다.
- 자체 실행 코드/Editor 도구의 작업 도구명을 제거하고 검증 출력 경로를 `Build/Validation`으로 바꿨습니다. 저작권·외부 에셋 라이선스는 삭제하지 않았습니다.
- 로그인·존의 DB 접속 정보만 로컬 INI로 옮겼습니다. 원래 값 및 배포/개발 출력의 6개 설정 사본이 동일한 것을 확인했습니다.
- Git 공개 제외 목록에서 캐시, 실행 파일, DB 계정 목록/백업, 로컬 비밀번호, 외부 에셋 원본을 제외했습니다. 공개 후보 파일에서 현재 DB 비밀번호가 검색되지 않습니다.
- 현재 DB의 테이블 구조 4개와 몬스터 타입 4행/성장표 40행만 공개용 SQL로 내보냈습니다. 계정/캐릭터 행과 권한·접속 비밀번호는 포함하지 않았습니다. 이 SQL을 DB에 실행한 것은 아닙니다.

## 오래된 자료 정리

- `Scenes/SampleScene`에서 현재 비기너존이 쓰는 NavMesh 2개를 `Scenes/BeginnerZoneScene`으로 옮겼습니다. 원본 데이터/메타데이터의 바이트가 동일하며 GUID도 유지합니다.
- 참조하지 않는 NavMesh 1개, 옛 3슬롯 배경 백업, 오래된 설명 문서 3개를 활성 Assets에서 제거했습니다.
- 옛 `TownAndWorldUISetup`의 마을 재생성 코드를 제거하고 현재 지도 렌더링 함수만 `ZoneMapBuilder`로 분리했습니다. 이를 호출하는 현재 편집기 도구도 연결을 변경했습니다.
- 잘못 남아 있던 시작 존/채널 수/포트 설명을 현재 파일로 갱신했습니다. 예전 문서와 2025년 클라이언트 빌드/ZIP은 로컬 보관함으로 옮겼습니다.
- 현재 씬 전경과 실제 연결된 지도 이미지는 Unity `Build/Validation/Current`에 다시 기록했습니다.

복구 위치: 서버 `Build/LocalArchive/SourceCleanup_20260907`, 양쪽 프로젝트의 `Build/LocalArchive/PreviousValidation`, Unity `Build/LocalArchive/LegacyOfflineBuild`. 공개하지 않는 로컬 백업이며 과거 구현 설명으로 사용하지 않습니다. 실제 DB의 테이블/계정/캐릭터는 삭제하지 않았습니다.

## 검증

| 검사 | 결과 |
| --- | --- |
| Login / Proxy / Zone, Debug x64 | 성공 |
| Login / Proxy / Zone, Release x64 | 성공 |
| 공개 후보 소스만 별도 빈 폴더에 복사한 Release 새 빌드 | 3개 서버 모두 성공. 기존 lib/obj/exe 없이 빌드 |
| 실제 Release 로그인·마을·비기너존 기동 | INI/DB 초기화 및 30003/30004/30005 Listen 확인 |
| 기동한 3개 서버 정상 종료 | q 입력, 모두 종료 코드 0 |
| 서버 전투 회귀 | 29개 PASS. 100 세션, 채널 분리, 공격/피격/취소/속사/광역/해제 검사 |
| 실제 비기너 필드 경로 검사 | 1,504개 샘플, 실패 0 |
| 더미 채팅 | 38개 PASS. 최대 200명, 중복 선택 방지, 오프라인 제외 등 |
| Unity 전투/동기화 | 70개 검사 및 원격 공격 39개 PASS. 실제 프리팹/패킷/루프백 사용 |
| Unity 현재 씬 참조 | 3개 빌드 씬, Missing Script 0, 현재 지도 참조 확인 |
| Unity 온라인 빌드 | Build/Online/BraveForest.exe 빌드 성공 |

수동 마우스 조작으로 전체 게임을 새로 실플레이한 검증은 아닙니다. 계정/캐릭터 DB에 대한 쓰기는 하지 않았습니다. 테스트 수치를 Unity 실행 창 100개 또는 최대 동접 성능 측정으로 해석하지 않습니다.

기존 소스의 C4244/C4267 형 변환 경고는 남아 있습니다. 기존 출력 폴더의 Release 재링크에서는 오래된 PDB에 대한 LNK4020도 나왔지만, 공개 소스만 복사한 새 빌드에서는 나오지 않았습니다. 줄바꿈이나 이번 기능 변경 오류와 구분합니다.

세부 로그는 공개하지 않는 `Build/Validation`에 있습니다: `SourceCleanupDebugFinal.log`, `SourceCleanupReleaseFinal.log`, `PublicServerBuild.log`, `SourceCleanupRuntime.log`, `SourceCleanupCombatServer.log`, `SourceCleanupDummyChat.log`. Unity에는 `SourceCleanupUnity.log`, `SourceCleanupCombat.log`, `SourceCleanupOnlineBuild.log`가 있습니다.
