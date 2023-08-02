/*
 *   Copyright (C) 2016,2017,2018,2020,2021,2023 by Jonathan Naylor G4KLX
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

#include "Display.h"
#include "Defines.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CDisplay::CDisplay() :
m_timer1(3000U, 3U),
m_timer2(3000U, 3U),
m_mode1(MODE_IDLE),
m_mode2(MODE_IDLE)
{
}

CDisplay::~CDisplay()
{
}

void CDisplay::setIdle()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setIdleInt();
}

void CDisplay::setLockout()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setLockoutInt();
}

void CDisplay::setError()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_IDLE;
	m_mode2 = MODE_IDLE;

	setErrorInt();
}

void CDisplay::setQuit()
{
	m_timer1.stop();
	m_timer2.stop();

	m_mode1 = MODE_QUIT;
	m_mode2 = MODE_QUIT;

	setQuitInt();
}

void CDisplay::writeDStar(const std::string& my1, const std::string& my2, const std::string& your, const std::string& type, const std::string& reflector)
{
	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeDStarInt(my1, my2, your, type, reflector);
}

void CDisplay::writeDStarRSSI(float rssi)
{
	if (rssi != 0U)
		writeDStarRSSIInt(rssi);
}

void CDisplay::writeDStarBER(float ber)
{
	writeDStarBERInt(ber);
}

void CDisplay::writeDStarText(const std::string& text)
{
	writeDStarTextInt(text);
}

void CDisplay::clearDStar()
{
	if (m_timer1.hasExpired()) {
		clearDStarInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_DSTAR;
	}
}

void CDisplay::writeDMR(unsigned int slotNo, const std::string& src, bool group, unsigned int dst, const std::string& type)
{
	if (slotNo == 1U) {
		m_timer1.start();
		m_mode1 = MODE_IDLE;
	} else {
		m_timer2.start();
		m_mode2 = MODE_IDLE;
	}

	writeDMRInt(slotNo, src, group, dst, type);
}

void CDisplay::writeDMRRSSI(unsigned int slotNo, float rssi)
{
	if (rssi != 0U)
		writeDMRRSSIInt(slotNo, rssi);
}

void CDisplay::writeDMRTA(unsigned int slotNo, const std::string& talkerAlias)
{
	writeDMRTAInt(slotNo, talkerAlias);
}

void CDisplay::writeDMRBER(unsigned int slotNo, float ber)
{
	writeDMRBERInt(slotNo, ber);
}

void CDisplay::clearDMR(unsigned int slotNo)
{
	if (slotNo == 1U) {
		if (m_timer1.hasExpired()) {
			clearDMRInt(slotNo);
			m_timer1.stop();
			m_mode1 = MODE_IDLE;
		} else {
			m_mode1 = MODE_DMR;
		}
	} else {
		if (m_timer2.hasExpired()) {
			clearDMRInt(slotNo);
			m_timer2.stop();
			m_mode2 = MODE_IDLE;
		} else {
			m_mode2 = MODE_DMR;
		}
	}
}

void CDisplay::writeFusion(const std::string& source, const std::string& dest, unsigned char dgid, const std::string& type, const std::string& origin)
{
	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeFusionInt(source, dest, dgid, type, origin);
}

void CDisplay::writeFusionRSSI(float rssi)
{
	if (rssi != 0U)
		writeFusionRSSIInt(rssi);
}

void CDisplay::writeFusionBER(float ber)
{
	writeFusionBERInt(ber);
}

void CDisplay::clearFusion()
{
	if (m_timer1.hasExpired()) {
		clearFusionInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_YSF;
	}
}

void CDisplay::writeP25(const std::string& source, bool group, unsigned int dest, const std::string& type)
{
	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeP25Int(source, group, dest, type);
}

void CDisplay::writeP25RSSI(float rssi)
{
	if (rssi != 0U)
		writeP25RSSIInt(rssi);
}

void CDisplay::writeP25BER(float ber)
{
	writeP25BERInt(ber);
}

void CDisplay::clearP25()
{
	if (m_timer1.hasExpired()) {
		clearP25Int();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_P25;
	}
}

void CDisplay::writeNXDN(const std::string& source, bool group, unsigned int dest, const std::string& type)
{
	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeNXDNInt(source, group, dest, type);
}

void CDisplay::writeNXDNRSSI(float rssi)
{
	if (rssi != 0U)
		writeNXDNRSSIInt(rssi);
}

void CDisplay::writeNXDNBER(float ber)
{
	writeNXDNBERInt(ber);
}

void CDisplay::clearNXDN()
{
	if (m_timer1.hasExpired()) {
		clearNXDNInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_NXDN;
	}
}

void CDisplay::writeM17(const std::string& source, const std::string& dest, const std::string& type)
{
	m_timer1.start();
	m_mode1 = MODE_IDLE;

	writeM17Int(source, dest, type);
}

void CDisplay::writeM17RSSI(float rssi)
{
	if (rssi != 0U)
		writeM17RSSIInt(rssi);
}

void CDisplay::writeM17BER(float ber)
{
	writeM17BERInt(ber);
}

void CDisplay::writeM17Text(const std::string& text)
{
	writeM17TextInt(text);
}

void CDisplay::clearM17()
{
	if (m_timer1.hasExpired()) {
		clearM17Int();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_M17;
	}
}

void CDisplay::writePOCSAG(uint32_t ric, const std::string& message)
{
	m_timer1.start();
	m_mode1 = MODE_POCSAG;

	writePOCSAGInt(ric, message);
}

void CDisplay::clearPOCSAG()
{
	if (m_timer1.hasExpired()) {
		clearPOCSAGInt();
		m_timer1.stop();
		m_mode1 = MODE_IDLE;
	} else {
		m_mode1 = MODE_POCSAG;
	}
}

void CDisplay::writeFM(const std::string& status)
{
	m_timer1.start();
	m_mode1 = MODE_FM;

	writeFMInt(status);
}

void CDisplay::writeFMRSSI(float rssi)
{
	if (rssi != 0U)
		writeFMRSSIInt(rssi);
}

void CDisplay::writeAX25(const std::string& source, const std::string& source_cs, const std::string& destination_cs, const std::string& type)
{
	m_timer1.start();
	m_mode1 = MODE_AX25;

	writeAX25Int(source, source_cs, destination_cs, type);
}

void CDisplay::writeAX25(const std::string& source, const std::string& source_cs, const std::string& destination_cs, const std::string& type, float rssi)
{
	m_timer1.start();
	m_mode1 = MODE_AX25;

	writeAX25Int(source, source_cs, destination_cs, type, rssi);
}

void CDisplay::writeAX25(const std::string& source, const std::string& source_cs, const std::string& destination_cs, const std::string& type, const std::string& pid, const std::string& data)
{
	m_timer1.start();
	m_mode1 = MODE_AX25;

	writeAX25Int(source, source_cs, destination_cs, type, pid, data);
}

void CDisplay::writeAX25(const std::string& source, const std::string& source_cs, const std::string& destination_cs, const std::string& type, const std::string& pid, const std::string& data, float rssi)
{
	m_timer1.start();
	m_mode1 = MODE_AX25;

	writeAX25Int(source, source_cs, destination_cs, type, pid, data, rssi);
}

void CDisplay::writeCW()
{
	m_timer1.start();
	m_mode1 = MODE_CW;

	writeCWInt();
}

void CDisplay::clock(unsigned int ms)
{
	m_timer1.clock(ms);
	if (m_timer1.isRunning() && m_timer1.hasExpired()) {
		switch (m_mode1) {
		case MODE_DSTAR:
			clearDStarInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_DMR:
			clearDMRInt(1U);
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_YSF:
			clearFusionInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_P25:
			clearP25Int();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_NXDN:
			clearNXDNInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_M17:
			clearM17Int();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_POCSAG:
			clearPOCSAGInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_AX25:
			clearAX25Int();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_FM:
			clearFMInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		case MODE_CW:
			clearCWInt();
			m_mode1 = MODE_IDLE;
			m_timer1.stop();
			break;
		default:
			break;
		}
	}

	// Timer/mode 2 are only used for DMR
	m_timer2.clock(ms);
	if (m_timer2.isRunning() && m_timer2.hasExpired()) {
		if (m_mode2 == MODE_DMR) {
			clearDMRInt(2U);
			m_mode2 = MODE_IDLE;
			m_timer2.stop();
		}
	}

	clockInt(ms);
}

void CDisplay::clockInt(unsigned int ms)
{
}

void CDisplay::writeDStarRSSIInt(float rssi)
{
}

void CDisplay::writeDStarBERInt(float ber)
{
}

void CDisplay::writeDStarTextInt(const std::string& text)
{
}

void CDisplay::writeDMRRSSIInt(unsigned int slotNo, float rssi)
{
}

void CDisplay::writeDMRTAInt(unsigned int slotNo, const std::string& talkerAlias)
{
}

void CDisplay::writeDMRBERInt(unsigned int slotNo, float ber)
{
}

void CDisplay::writeFusionRSSIInt(float rssi)
{
}

void CDisplay::writeFusionBERInt(float ber)
{
}

void CDisplay::writeP25RSSIInt(float rssi)
{
}

void CDisplay::writeP25BERInt(float ber)
{
}

void CDisplay::writeNXDNRSSIInt(float rssi)
{
}

void CDisplay::writeNXDNBERInt(float ber)
{
}

void CDisplay::writeM17RSSIInt(float rssi)
{
}

void CDisplay::writeM17BERInt(float ber)
{
}

void CDisplay::writeM17TextInt(const std::string& text)
{
}

void CDisplay::writeFMRSSIInt(float rssi)
{
}

