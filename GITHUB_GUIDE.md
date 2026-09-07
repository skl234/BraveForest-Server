# GitHub 첫 업로드 안내

## 먼저 읽기

이번 정리에서는 GitHub에 업로드하거나 원격 저장소를 만들지 않았습니다. `.gitignore`는 업로드 제외 목록이고, `.gitattributes`와 `.editorconfig`는 코드 줄바꿈/편집 기준입니다. 제외된 파일도 PC에는 남아 있습니다.

마감 전에는 **서버 저장소 1개 + Unity 소스 검토용 저장소 1개**가 가장 단순합니다. 전체 Unity 에셋의 공개 가능 여부를 급하게 판단하지 않도록 클라이언트는 소스 중심으로 준비했습니다.

## 올리는 것 / 올리지 않는 것

| 구분 | 올릴 것 | 제외할 것 |
| --- | --- | --- |
| 서버 | `.sln`, 각 프로젝트의 `.vcxproj`/`.filters`, `.h`/`.cpp`, 공용 라이브러리 **소스**, `Data`, 예제 DB 설정, 문서 | `.vs`, x64/Debug/Release, obj/lib/pdb/exe/dll, 로그, 계정 목록/덤프, 실제 비밀번호 |
| 빌드 연결 | `Directory.Build.targets`, `Build/MMOServerExes.targets`, `Build/ServerRuntime/README.md` | Build의 나머지 임시 결과/테스트 백업, MMOServerExes |
| Unity | 프로젝트의 Script/Editor, 현재 씬 연결 구조, ExportedFieldData, Packages, ProjectSettings, 해당 `.meta` | Library, Logs, obj, UserSettings, Build, 자동 생성 csproj/sln, 외부 에셋 원본 |
| 포트폴리오 | 검토한 PDF, 최신 캡처, 영상 링크 | 옛 지도 캡처, 작업 중 문서, DB 정보가 보이는 화면 |

**임시 검증 결과는 올리지 않습니다.** 현재 결과는 `Build/Validation`, 과거 결과와 복구 자료는 `Build/LocalArchive`로 분리했습니다. 핵심 테스트 코드 자체는 원한다면 나중에 정리해서 `Tests`에 올릴 수 있지만, 지금 임시 폴더를 통째로 공개하지 마세요.

서버의 `.lib`는 포함하지 않아도 라이브러리 프로젝트 소스로 다시 빌드합니다. 단, Release 복사를 담당하는 위 두 `.targets` 파일은 반드시 포함해야 합니다.

Unity는 에셋을 이동할 때 `.meta`도 같이 관리해야 합니다. 현재 공개 설정에서 프리팹/모델/텍스처/폰트 등을 제외했으므로 **다운로드 즉시 실행되는 전체 프로젝트가 아닙니다.** README에도 이를 적었습니다. 이후 원본 에셋을 올리고 싶다면 각 라이선스를 먼저 확인하고 공개 범위를 조정해야 합니다. 무료 에셋도 원본 재배포가 자동으로 허용되는 것은 아닙니다. [Unity Asset Store EULA](https://unity.com/legal/as-terms)

## GitHub Desktop으로 올리기

명령어보다 GitHub Desktop을 이용하는 방법을 권합니다.

1. GitHub 계정을 만들고 GitHub Desktop을 설치한 뒤 로그인합니다. [공식 첫 저장소 안내](https://docs.github.com/en/desktop/overview/creating-your-first-repository-using-github-desktop)
2. `File → Add local repository → Choose`에서 **MMONetwork2 폴더 자체**를 고릅니다.
3. Git 저장소가 아니라는 안내가 나오면 `create a repository here`를 선택합니다. 생성 위치가 기존 `MMONetwork2` 폴더인지 확인합니다. `MMONetwork2/MMONetwork2`처럼 새 하위 폴더를 만들지 않습니다. 준비된 README와 ignore 파일은 덮어쓰지 않습니다.
4. 왼쪽 `Changes` 목록을 확인합니다. `Database.local.ini`, 실제 비밀번호, 계정 목록, Build 로그, `.vs`, `.exe`가 보이면 **중단**하고 제외부터 확인합니다. `.gitignore` 파일은 포함되는 것이 정상입니다. 첫 Commit이 자동 생성된 경우에는 `History`에서 그 Commit에 들어간 파일 목록을 확인합니다.
5. 아직 Commit하지 않은 변경이 있으면 `Summary`에 `Initial source release`를 적고 `Commit to main`을 누릅니다. **Commit은 내 PC에 기록하는 것**입니다.
6. 상단 `Publish repository`를 누릅니다. 저장소 이름은 예를 들어 `BraveForest-Server`로 정할 수 있습니다. 처음에는 `Keep this code private`를 켜고 올립니다. **Publish/Push가 GitHub로 전송하는 것**입니다. [기존 프로젝트 공개 안내](https://docs.github.com/en/desktop/adding-and-cloning-repositories/adding-an-existing-project-to-github-using-github-desktop)
7. GitHub 웹에서 파일 목록을 다시 확인합니다. 비밀번호·개인정보·외부 에셋 원본이 없고 공개 범위를 확인한 후 `Settings → General → Danger Zone → Change repository visibility`에서 Public으로 바꿀 수 있습니다. 비공개로 두면 면접관에게 링크만 보내서는 열리지 않으므로 공개하거나 별도 접근 권한을 줘야 합니다.
8. 같은 순서로 **MMORPG 폴더**를 추가하고 `BraveForest-Client` 등의 이름으로 공개용 소스 저장소를 만듭니다. 에셋 미포함 소스 검토용이라는 README 설명은 남깁니다.
9. 이후 파일을 수정하면 `Changes 확인 → Summary → Commit → Push origin` 순서입니다. 삭제도 변경사항이므로 확인하고 Commit합니다.

처음부터 공개 저장소로 올려도 되지만, 시간에 쫓겨 비밀번호를 한 번 올린 뒤 지우는 상황은 피해야 합니다. 이미 Commit된 비밀은 파일만 삭제해도 과거 기록에 남습니다. 유출됐다면 비밀번호를 바꾸고 기록 정리까지 해야 합니다.

원본 루트의 `Portfolio` 폴더는 과거 자료가 자동 업로드되지 않도록 제외해 두었습니다. 완성한 PDF만 서버 저장소의 `Docs/Portfolio.pdf`처럼 별도 경로로 복사해서 검토 후 추가하세요.

## 영상·큰 파일

3분 정도의 플레이 영상이면 핵심 기능을 보여주기 충분하다고 봅니다. 긴 접속 대기나 걷기 장면을 줄이고 **동작이 구분되는 장면**을 보여주는 쪽을 권합니다.

| 시간 | 보여줄 내용 |
| --- | --- |
| 0:00–0:20 | 게임명, 로그인, 캐릭터 입장 |
| 0:20–0:45 | 마을, 지도, 포탈을 통한 비기너존 이동 |
| 0:45–1:30 | 전사/궁수 공격·스킬, HP/데미지와 몬스터 반응 |
| 1:30–2:10 | 두 클라이언트에서 상대 이동·전투·채팅 확인 |
| 2:10–2:40 | 채널 이동, 채널별 분리, 더미/모니터링 화면 |
| 2:40–3:00 | 서버 구성 요약과 소스/포트폴리오 안내 |

모든 대사에 자막을 달 필요는 없습니다. 다만 **“다른 클라이언트 시점”, “같은 채널 채팅”, “채널 이동” 정도의 짧은 설명은 추천**합니다. 포트폴리오를 읽지 않고 영상만 먼저 보는 사람도 기능을 알아볼 수 있습니다. 영상에 DB 비밀번호·관리 콘솔의 개인정보가 나오지 않는지 확인하세요.

영상은 YouTube 일부 공개 등으로 올리고 README/포트폴리오에 링크를 넣는 방식이 편합니다. 소스 저장소에 MP4를 통째로 넣을 필요는 없습니다. GitHub는 일반 Git에 100 MiB 초과 파일을 막으며, 큰 파일에는 Git LFS 같은 별도 관리가 필요합니다. 지금은 영상 링크를 쓰면 이를 피할 수 있습니다. [GitHub 대용량 파일 안내](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github)

## 최종 5개 확인

1. README에 실제 실행/환경 설명이 있는가?
2. 소스와 문서의 시작 존·채널 수·최신 지도 설명이 일치하는가?
3. 비밀번호·계정 덤프·원본 외부 에셋이 제외됐는가?
4. 공개 저장소 및 영상 링크를 로그아웃 상태에서도 열 수 있는가?
5. 테스트 수치와 설명이 실제 확인한 범위를 넘어가지 않는가?
