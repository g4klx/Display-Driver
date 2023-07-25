/*
 *   Copyright (C) 2016,2017 by Tony Corbett G0WFV
 *   Copyright (C) 2018,2020,2023 by Jonathan Naylor G4KLX
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

#if !defined(LCDproc_H)
#define	LCDproc_H

#include "Display.h"
#include "Timer.h"

#include <string>

class CLCDproc : public CDisplay
{
public:
	CLCDproc(const std::string& callsign,unsigned int id, bool duplex, const std::string& address, unsigned int port, unsigned short localPort, bool displayClock, bool utc, bool dimOnIdle);
	virtual ~CLCDproc();

	virtual bool open();

	virtual void close();

protected:
	virtual void setIdleInt();
	virtual void setErrorInt(const std::string& text);
	virtual void setLockoutInt();
	virtual void setQuitInt();
	virtual void setFMInt();
  
	virtual void writeDStarInt(const std::string& my1, const std::string& my2, const std::string& your, const std::string& type, const std::string& reflector);
	virtual void writeDStarRSSIInt(unsigned char rssi);
	virtual void clearDStarInt();

	virtual void writeDMRInt(unsigned int slotNo, const std::string& src, bool group, const std::string& dst, const std::string& type);
	virtual void writeDMRRSSIInt(unsigned int slotNo, unsigned char rssi); 
	virtual void clearDMRInt(unsigned int slotNo);

	virtual void writeFusionInt(const std::string& source, const std::string& dest, unsigned char dgid, const std::string& type, const std::string& origin);
	virtual void writeFusionRSSIInt(unsigned char rssi); 
	virtual void clearFusionInt();

	virtual void writeP25Int(const std::string& source, bool group, unsigned int dest, const std::string& type);
	virtual void writeP25RSSIInt(unsigned char rssi); 
	virtual void clearP25Int();

	virtual void writeNXDNInt(const std::string& source, bool group, unsigned int dest, const std::string& type);
	virtual void writeNXDNRSSIInt(unsigned char rssi);
	virtual void clearNXDNInt();

	virtual void writeM17Int(const std::string& source, const std::string& dest, const std::string& type);
	virtual void writeM17RSSIInt(unsigned char rssi);
	virtual void clearM17Int();

	virtual void writePOCSAGInt(uint32_t ric, const std::string& message);
	virtual void clearPOCSAGInt();

	virtual void writeCWInt();
	virtual void clearCWInt();

	virtual void clockInt(unsigned int ms);

private:
	std::string  m_callsign;
	unsigned int m_id;
	bool         m_duplex;
	std::string  m_address;
	unsigned int m_port;
	unsigned short m_localPort;
	bool         m_displayClock;
	bool         m_utc;
	bool         m_dimOnIdle;
	bool         m_dmr;
	CTimer       m_clockDisplayTimer;
	unsigned int m_rssiCount1; 
	unsigned int m_rssiCount2; 

	int  socketPrintf(int fd, const char *format, ...);
	void defineScreens();
};

#endif
