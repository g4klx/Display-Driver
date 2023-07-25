/*
*   Copyright (C) 2016,2020,2021,2023 by Jonathan Naylor G4KLX
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

#ifndef ModemSerialPort_H
#define ModemSerialPort_H

#include "SerialPort.h"

#include <string>

class CModemSerialPort : public ISerialPort {
public:
	CModemSerialPort(const std::string& mmdvmName);
	virtual ~CModemSerialPort();

	virtual bool open();

	virtual int read(unsigned char* buffer, unsigned int length);

	virtual int write(const unsigned char* buffer, unsigned int length);

	virtual void close();

	void readData(const unsigned char* data, unsigned int length);

private:
	std::string    m_serialName;
	unsigned char* m_data;
	unsigned int   m_length;
};

#endif
