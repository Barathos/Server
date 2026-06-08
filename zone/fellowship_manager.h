/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#pragma once

class Client;
class EQApplicationPacket;
class Seperator;

class FellowshipManager {
public:
	void HandleCommand(Client *client, const Seperator *sep);
	void HandleClientPacket(Client *client, const EQApplicationPacket *app) const;
	void LogDiscoveryPacket(Client *client, const EQApplicationPacket *app, const char *context) const;
	void SendClientState(Client *client) const;
};

extern FellowshipManager fellowship_manager;
