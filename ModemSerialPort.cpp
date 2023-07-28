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

#include "ModemSerialPort.h"
#include "MQTTConnection.h"

#include <cstdio>
#include <cassert>

// In Log.cpp
extern CMQTTConnection* m_mqtt;

CModemSerialPort::CModemSerialPort(const std::string& mmdvmName) :
m_serialName(),
m_buffer(500U, "Modem serial")
{
	m_serialName = mmdvmName + "/display-in";
}

CModemSerialPort::~CModemSerialPort()
{
}

bool CModemSerialPort::open()
{
	return true;
}

int CModemSerialPort::write(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);
	assert(m_mqtt != NULL);

	m_mqtt->publish(m_serialName.c_str(), data, length);

	return length;
}

int CModemSerialPort::read(unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	unsigned int len = m_buffer.dataSize();

	if (len == 0U)
		return 0;

	if (length > len)
		length = len;

	m_buffer.getData(data, length);

	return length;
}

void CModemSerialPort::close()
{
}

void CModemSerialPort::readData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	m_buffer.addData(data, length);
}

