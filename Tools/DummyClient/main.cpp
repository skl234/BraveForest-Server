#include "DummyClient.h"
#include <filesystem>
#include <iostream>

int main(int _argc, char* _argv[])
{
	static_assert(sizeof(wchar_t) == 2 && sizeof(CHARACTER) == 90 && sizeof(FIELD_OBJECT_INFO) == 97, "Packet layout changed");
	static_assert(sizeof(PACKET_C2S_MOVE) == 33, "Movement layout changed");
	wchar_t path[32768]{};
	GetModuleFileNameW(nullptr, path, 32768);
	std::filesystem::current_path(std::filesystem::path(path).parent_path());
	CDummyClient client;
	if (!client.Initialize()) { std::cerr << "Cannot load DummyClient.ini / Data files. Keep the entire folder together.\n"; return 1; }
	uint64_t seconds = 0;
	bool autoChat = false;
	for (int i = 1; i < _argc; ++i)
	{
		std::string argument = _argv[i];
		if (argument == "--generate")
		{
			if (client.GenerateAccounts()) return 0;
			return 1;
		}
		if (argument == "--self-test")
		{
			if (client.VerifyPaths()) return 0;
			return 1;
		}
		if (argument == "--duration" && i + 1 < _argc) seconds = std::stoull(_argv[++i]);
		else if (argument == "--auto-chat") autoChat = true;
		else { std::cerr << "Options: --generate | --self-test | --duration SECONDS --auto-chat\n"; return 1; }
	}
	return client.Run(seconds, autoChat);
}
