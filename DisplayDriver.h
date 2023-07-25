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

#include "Display.h"
#include "Conf.h"

#include <string>

class CDisplayDriver
{
public:
	CDisplayDriver(const std::string& confFile);
	~CDisplayDriver();

	int run();

private:
	CConf     m_conf;
	CDisplay* m_display;

	void writeJSONMessage(const std::string& message);
};

#endif
