/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#pragma once

#include <string>

class Client;
class Seperator;

class FactionReputationManager {
public:
	void HandleCommand(Client *client, const Seperator *sep);
	void SendNativeSnapshot(Client *client);
	void SendNativeSnapshot(Client *client, const std::string &mode, const std::string &search);

private:
	void SendChatList(Client *client);
	void SendChatList(Client *client, const std::string &mode, const std::string &search);
};

extern FactionReputationManager faction_reputation_manager;
