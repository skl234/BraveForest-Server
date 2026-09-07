// 검증에서 직접 실행한 서버에만 q 입력을 전달하는 종료 보조 도구
#include <Windows.h>
#include <cstdlib>

int main(int _argc, char* _argv[])
{
	if (_argc != 2) return 1;
	DWORD processId = static_cast<DWORD>(std::strtoul(_argv[1], nullptr, 10));
	if (processId == 0) return 1;
	FreeConsole();
	if (!AttachConsole(processId)) return 2;
	HANDLE input = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (input == INVALID_HANDLE_VALUE) { FreeConsole(); return 3; }
	INPUT_RECORD records[2]{};
	for (int i = 0; i < 2; ++i)
	{
		records[i].EventType = KEY_EVENT;
		records[i].Event.KeyEvent.bKeyDown = TRUE;
		if (i == 1) records[i].Event.KeyEvent.bKeyDown = FALSE;
		records[i].Event.KeyEvent.wRepeatCount = 1;
		records[i].Event.KeyEvent.wVirtualKeyCode = 'Q';
		records[i].Event.KeyEvent.wVirtualScanCode = 0x10;
		records[i].Event.KeyEvent.uChar.UnicodeChar = L'q';
	}
	DWORD written = 0;
	BOOL result = WriteConsoleInputW(input, records, 2, &written);
	CloseHandle(input);
	FreeConsole();
	if (!result || written != 2) return 4;
	return 0;
}
