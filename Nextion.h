/*
 *   Copyright (C) 2016,2017,2018,2020,2023,2026 by Jonathan Naylor G4KLX
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

#if !defined(NEXTION_H)
#define	NEXTION_H

#include "Display.h"
#include "Defines.h"
#include "RingBuffer.h"
#include "SerialPort.h"
#include "Mutex.h"
#include "Timer.h"
#include "Thread.h"
#include <string>

class CNextion : public CDisplay
{
public:
	CNextion(const std::string& callsign, unsigned int id, bool duplex, ISerialPort* serial, unsigned int brightness, bool displayClock, bool utc, unsigned int idleBrightness, unsigned int screenLayout, bool displayTempInF);
	virtual ~CNextion();

	virtual bool open();

	virtual void close();

protected:
	virtual void setIdleInt();
	virtual void setErrorInt();
	virtual void setLockoutInt();
	virtual void setQuitInt();

	virtual void writeDStarInt(const std::string& my1, const std::string& my2, const std::string& your, const std::string& type, const std::string& reflector);
	virtual void writeDStarRSSIInt(int rssi);
	virtual void writeDStarBERInt(float ber);
	virtual void writeDStarTextInt(const std::string& text);
	virtual void clearDStarInt();

	virtual void writeDMRInt(unsigned int slotNo, const std::string& src, bool group, unsigned int dst, const std::string& type);
	virtual void writeDMRRSSIInt(unsigned int slotNo, int rssi);
	virtual void writeDMRTAInt(unsigned int slotNo, const std::string& talkerAlias);
	virtual void writeDMRBERInt(unsigned int slotNo, float ber);
	virtual void clearDMRInt(unsigned int slotNo);

	virtual void writeFusionInt(const std::string& source, const std::string& dest, unsigned char dgid, const std::string& type, const std::string& origin);
	virtual void writeFusionRSSIInt(int rssi);
	virtual void writeFusionBERInt(float ber);
	virtual void clearFusionInt();

	virtual void writeP25Int(const std::string& source, bool group, unsigned int dest, const std::string& type);
	virtual void writeP25RSSIInt(int rssi);
	virtual void writeP25BERInt(float ber);
	virtual void clearP25Int();

	virtual void writeNXDNInt(const std::string& source, bool group, unsigned int dest, const std::string& type);
	virtual void writeNXDNRSSIInt(int rssi);
	virtual void writeNXDNBERInt(float ber);
	virtual void clearNXDNInt();

	virtual void writePOCSAGInt(uint32_t ric, const std::string& message);
	virtual void clearPOCSAGInt();

	virtual void writeFMInt(const std::string& state);
	virtual void writeFMRSSIInt(int rssi);
	virtual void clearFMInt();

	virtual void writeCWInt();
	virtual void clearCWInt();

	virtual void clockInt(unsigned int ms);

private:
	std::string    m_callsign;
	unsigned int   m_id;
	bool           m_duplex;
	std::string    m_ipAddress;
	ISerialPort*   m_serial;
	unsigned int   m_brightness;
	unsigned char  m_mode;
	bool           m_displayClock;
	bool           m_utc;
	unsigned int   m_idleBrightness;
	unsigned int   m_screenLayout;
	CTimer         m_clockDisplayTimer;
	bool           m_displayTempInF;
	CRingBuffer<unsigned char> m_output;
	CMutex         m_mutex;
	unsigned char* m_reply;
	bool           m_waiting;

	void sendCommand(const std::string& command);
	void sendCommandAction(unsigned int status);
};

#endif
