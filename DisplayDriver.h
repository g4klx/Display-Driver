/*
 *   Copyright (C) 2015-2021,2023 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if !defined(DisplayDriver_H)
#define	DisplayDriver_H

#include "ModemSerialPort.h"
#include "Display.h"
#include "Conf.h"

#include <nlohmann/json.hpp>

#include <string>

class CDisplayDriver
{
public:
	CDisplayDriver(const std::string& confFile);
	~CDisplayDriver();

	int run();

private:
	CConf             m_conf;
	CDisplay*         m_display;
	CModemSerialPort* m_msp;

	bool createDisplay();

	void writeJSONMessage(const std::string& message);

	void readJSON(const std::string& text);
	void readDisplay(const unsigned char* data, unsigned int length);

	void parseMMDVM(const nlohmann::json& json);
	void parseRSSI(const nlohmann::json& json);
	void parseBER(const nlohmann::json& json);
	void parseText(const nlohmann::json& json);
	void parseDStar(const nlohmann::json& json);
	void parseDMR(const nlohmann::json& json);
	void parseYSF(const nlohmann::json& json);
	void parseP25(const nlohmann::json& json);
	void parseNXDN(const nlohmann::json& json);
	void parsePOCSAG(const nlohmann::json& json);
	void parseFM(const nlohmann::json& json);
	void parseAX25(const nlohmann::json& json);
	void parseM17(const nlohmann::json& json);

	static void onDisplay(const unsigned char* data, unsigned int length);
	static void onJSON(const unsigned char* data, unsigned int length);
};

#endif
