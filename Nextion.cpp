/*
 *   Copyright (C) 2016,2017,2018,2020,2023 by Jonathan Naylor G4KLX
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

#include "NetworkInfo.h"
#include "Nextion.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>
#include <clocale>

#define LAYOUT_COMPAT_MASK	(7 << 0) // compatibility for old setting
#define LAYOUT_TA_ENABLE	(1 << 4) // enable Talker Alias (TA) display
#define LAYOUT_TA_COLOUR	(1 << 5) // TA display with font colour change
#define LAYOUT_TA_FONTSIZE	(1 << 6) // TA display with font size change
#define LAYOUT_DIY		(1 << 7) // use ON7LDS-DIY layout

// bit[3:2] is used in Display.cpp to set connection speed for LCD panel.
// 00:low, others:high-speed. bit[2] is overlapped with LAYOUT_COMPAT_MASK.
#define LAYOUT_HIGHSPEED	(3 << 2)

const unsigned int MAX_REPLY_LENGTH = 9U;

CNextion::CNextion(const std::string& callsign, unsigned int id, bool duplex, ISerialPort* serial, unsigned int brightness, bool displayClock, bool utc, unsigned int idleBrightness, unsigned int screenLayout, bool displayTempInF) :
CDisplay(),
m_callsign(callsign),
m_id(id),
m_duplex(duplex),
m_ipAddress("(ip unknown)"),
m_serial(serial),
m_brightness(brightness),
m_mode(MODE_IDLE),
m_displayClock(displayClock),
m_utc(utc),
m_idleBrightness(idleBrightness),
m_screenLayout(0),
m_clockDisplayTimer(1000U, 0U, 400U),
m_displayTempInF(displayTempInF),
m_output(200U, "Nextion buffer"),
m_mutex(),
m_reply(NULL),
m_waiting(false)
{
	assert(serial != NULL);
	assert(brightness >= 0U && brightness <= 100U);

	static const unsigned int feature_set[] = {
		0,				// 0: G4KLX
		0,				// 1: (reserved, low speed)
						// 2: ON7LDS
		LAYOUT_TA_ENABLE | LAYOUT_TA_COLOUR | LAYOUT_TA_FONTSIZE,
		LAYOUT_TA_ENABLE | LAYOUT_DIY,	// 3: ON7LDS-DIY
		LAYOUT_TA_ENABLE | LAYOUT_DIY,	// 4: ON7LDS-DIY (high speed)
		0,				// 5: (reserved, high speed)
		0,				// 6: (reserved, high speed)
		0,				// 7: (reserved, high speed)
	};

	if (screenLayout & ~LAYOUT_COMPAT_MASK)
		m_screenLayout = screenLayout & ~LAYOUT_COMPAT_MASK;
	else
		m_screenLayout = feature_set[screenLayout];

	m_reply = new unsigned char[MAX_REPLY_LENGTH];
}

CNextion::~CNextion()
{
	delete[] m_reply;
}

bool CNextion::open()
{
	unsigned char info[100U];
	CNetworkInfo* m_network;

	bool ret = m_serial->open();
	if (!ret) {
		LogError("Cannot open the port for the Nextion display");
		delete m_serial;
		return false;
	}

	info[0] = 0;
	m_network = new CNetworkInfo;
	m_network->getNetworkInterface(info);
	m_ipAddress = (char*)info;

	sendCommand("bkcmd=3");
	sendCommandAction(0U);

	setIdle();

	return true;
}

void CNextion::setIdleInt()
{
	// a few bits borrowed from Lieven De Samblanx ON7LDS, NextionDriver
	char command[100U];

	sendCommand("page MMDVM");
	sendCommandAction(1U);

	if (m_brightness > 0U) {
		::sprintf(command, "dim=%u", m_idleBrightness);
		sendCommand(command);
	}

	::sprintf(command, "t0.txt=\"%s/%u\"", m_callsign.c_str(), m_id);
	sendCommand(command);

	if (m_screenLayout & LAYOUT_DIY) {
		::sprintf(command, "t4.txt=\"%s\"", m_callsign.c_str());
		sendCommand(command);
		::sprintf(command, "t5.txt=\"%u\"", m_id);
		sendCommand(command);
		sendCommandAction(17U);

		// CPU temperature
		FILE* fp = ::fopen("/sys/class/thermal/thermal_zone0/temp", "rt");
		if (fp != NULL) {
			double val = 0.0;
			int n = ::fscanf(fp, "%lf", &val);
			::fclose(fp);

			if (n == 1) {
				val /= 1000.0;
				if (m_displayTempInF) {
					val = (1.8 * val) + 32.0;
					::sprintf(command, "t20.txt=\"%2.1f %cF\"", val, 176);
				} else {	
					::sprintf(command, "t20.txt=\"%2.1f %cC\"", val, 176);
				}
				sendCommand(command);
				sendCommandAction(22U);
			}
		}
	} else {
		sendCommandAction(17U);
	}
	
	sendCommand("t1.txt=\"MMDVM IDLE\"");
	sendCommandAction(11U);

	::sprintf(command, "t3.txt=\"%s\"", m_ipAddress.c_str());
	sendCommand(command);
	sendCommandAction(16U);

	m_clockDisplayTimer.start();

	m_mode = MODE_IDLE;
}

void CNextion::setErrorInt()
{
	sendCommand("page MMDVM");
	sendCommandAction(1U);

	char command[20];
	if (m_brightness>0) {
		::sprintf(command, "dim=%u", m_brightness);
		sendCommand(command);
	}

	::sprintf(command, "t0.txt=\"ERROR\"");
	sendCommandAction(13U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_ERROR;
}

void CNextion::setLockoutInt()
{
	sendCommand("page MMDVM");
	sendCommandAction(1U);

	char command[20];
	if (m_brightness>0) {
		::sprintf(command, "dim=%u", m_brightness);
		sendCommand(command);
	}

	sendCommand("t0.txt=\"LOCKOUT\"");
	sendCommandAction(15U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_LOCKOUT;
}

void CNextion::setQuitInt()
{
	sendCommand("page MMDVM");
	sendCommandAction(1U);

	char command[100];
	if (m_brightness>0) {
		::sprintf(command, "dim=%u", m_idleBrightness);
		sendCommand(command);
	}

	::sprintf(command, "t3.txt=\"%s\"", m_ipAddress.c_str());
	sendCommand(command);
	sendCommandAction(16U);

	sendCommand("t0.txt=\"MMDVM STOPPED\"");
	sendCommandAction(19U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_QUIT;
}

void CNextion::writeDStarInt(const std::string& my1, const std::string& my2, const std::string& your, const std::string& type, const std::string& reflector)
{
	if (m_mode != MODE_DSTAR) {
		sendCommand("page DStar");
		sendCommandAction(2U);
	}

	char text[50U];
	if (m_brightness>0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	::sprintf(text, "t0.txt=\"%s %.8s/%4.4s\"", type.c_str(), my1.c_str(), my2.c_str());
	sendCommand(text);
	sendCommandAction(42U);

	::sprintf(text, "t1.txt=\"%.8s\"", your.c_str());
	sendCommand(text);
	sendCommandAction(45U);

	if (reflector != "        ") {
		::sprintf(text, "t2.txt=\"via %.8s\"", reflector.c_str());
		sendCommand(text);
		sendCommandAction(46U);
	}

	m_clockDisplayTimer.stop();

	m_mode = MODE_DSTAR;
}

void CNextion::writeDStarRSSIInt(int rssi)
{
	char text[25U];
	::sprintf(text, "t3.txt=\"%ddBm\"", rssi);
	sendCommand(text);
	sendCommandAction(47U);
}

void CNextion::writeDStarBERInt(float ber)
{
	char text[25U];
	::sprintf(text, "t4.txt=\"%.1f%%\"", ber);
	sendCommand(text);
	sendCommandAction(48U);
}

void CNextion::writeDStarTextInt(const std::string& text)
{
	char buffer[25U];
	::sprintf(buffer, "t5.txt=\"%s\"", text.c_str());

	sendCommand(buffer);
	sendCommandAction(49U);
}

void CNextion::clearDStarInt()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommandAction(41U);
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
	sendCommand("t4.txt=\"\"");
	sendCommand("t5.txt=\"\"");
}

void CNextion::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, unsigned int dst, const std::string& type)
{
	if (m_mode != MODE_DMR) {
		sendCommand("page DMR");
		sendCommandAction(3U);


		if (slotNo == 1U) {
			if (m_screenLayout & LAYOUT_TA_ENABLE) {
				if (m_screenLayout & LAYOUT_TA_COLOUR)
					sendCommand("t2.pco=0");
				if (m_screenLayout & LAYOUT_TA_FONTSIZE)
					sendCommand("t2.font=4");
			}

			sendCommand("t2.txt=\"2 Listening\"");
			sendCommandAction(69U);
		} else {
			if (m_screenLayout & LAYOUT_TA_ENABLE) {
				if (m_screenLayout & LAYOUT_TA_COLOUR)
					sendCommand("t0.pco=0");
				if (m_screenLayout & LAYOUT_TA_FONTSIZE)
					sendCommand("t0.font=4");
			}

			sendCommand("t0.txt=\"1 Listening\"");
			sendCommandAction(61U);
		}
	}

	char text[50U];
	if (m_brightness>0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	if (slotNo == 1U) {
		::sprintf(text, "t0.txt=\"1 %s %s\"", type.c_str(), src.c_str());

		if (m_screenLayout & LAYOUT_TA_ENABLE) {
			if (m_screenLayout & LAYOUT_TA_COLOUR)
				sendCommand("t0.pco=0");
			if (m_screenLayout & LAYOUT_TA_FONTSIZE)
				sendCommand("t0.font=4");
		}

		sendCommand(text);
		sendCommandAction(62U);

		::sprintf(text, "t1.txt=\"%s%u\"", group ? "TG" : "", dst);
		sendCommand(text);
		sendCommandAction(65U);
	} else {
		::sprintf(text, "t2.txt=\"2 %s %s\"", type.c_str(), src.c_str());

		if (m_screenLayout & LAYOUT_TA_ENABLE) {
			if (m_screenLayout & LAYOUT_TA_COLOUR)
				sendCommand("t2.pco=0");
			if (m_screenLayout & LAYOUT_TA_FONTSIZE)
				sendCommand("t2.font=4");
		}

		sendCommand(text);
		sendCommandAction(70U);

		::sprintf(text, "t3.txt=\"%s%u\"", group ? "TG" : "", dst);
		sendCommand(text);
		sendCommandAction(73U);
	}

	m_clockDisplayTimer.stop();

	m_mode = MODE_DMR;
}

void CNextion::writeDMRRSSIInt(unsigned int slotNo, int rssi)
{
	if (slotNo == 1U) {
		char text[25U];
		::sprintf(text, "t4.txt=\"%ddBm\"", rssi);
		sendCommand(text);
		sendCommandAction(66U);
	} else {
		char text[25U];
		::sprintf(text, "t5.txt=\"%ddBm\"", rssi);
		sendCommand(text);
		sendCommandAction(74U);
	}
}

void CNextion::writeDMRTAInt(unsigned int slotNo, const std::string& talkerAlias)
{
	if (!(m_screenLayout & LAYOUT_TA_ENABLE))
		return;

	if (slotNo == 1U) {
		char text[50U];
		::sprintf(text, "t0.txt=\"1 %s\"", talkerAlias.c_str());

		if (m_screenLayout & LAYOUT_TA_FONTSIZE) {
			if (talkerAlias.size() > (16U-4U))
				sendCommand("t0.font=3");
			if (talkerAlias.size() > (20U-4U))
				sendCommand("t0.font=2");
			if (talkerAlias.size() > (24U-4U))
				sendCommand("t0.font=1");
		}

		if (m_screenLayout & LAYOUT_TA_COLOUR)
			sendCommand("t0.pco=1024");

		sendCommand(text);
		sendCommandAction(63U);
	} else {
		char text[50U];
		::sprintf(text, "t2.txt=\"2 %s\"", talkerAlias.c_str());

		if (m_screenLayout & LAYOUT_TA_FONTSIZE) {
			if (talkerAlias.size() > (16U-4U))
				sendCommand("t2.font=3");
			if (talkerAlias.size() > (20U-4U))
				sendCommand("t2.font=2");
			if (talkerAlias.size() > (24U-4U))
				sendCommand("t2.font=1");
		}

		if (m_screenLayout & LAYOUT_TA_COLOUR)
			sendCommand("t2.pco=1024");

		sendCommand(text);
		sendCommandAction(71U);
	}
}

void CNextion::writeDMRBERInt(unsigned int slotNo, float ber)
{
	if (slotNo == 1U) {
		char text[25U];
		::sprintf(text, "t6.txt=\"%.1f%%\"", ber);
		sendCommand(text);
		sendCommandAction(67U);
	} else {
		char text[25U];
		::sprintf(text, "t7.txt=\"%.1f%%\"", ber);
		sendCommand(text);
		sendCommandAction(75U);
	}
}

void CNextion::clearDMRInt(unsigned int slotNo)
{
	if (slotNo == 1U) {
		sendCommand("t0.txt=\"1 Listening\"");
		sendCommandAction(61U);

		if (m_screenLayout & LAYOUT_TA_ENABLE) {
			if (m_screenLayout & LAYOUT_TA_COLOUR)
				sendCommand("t0.pco=0");
			if (m_screenLayout & LAYOUT_TA_FONTSIZE)
				sendCommand("t0.font=4");
		}

		sendCommand("t1.txt=\"\"");
		sendCommand("t4.txt=\"\"");
		sendCommand("t6.txt=\"\"");
	} else {
		sendCommand("t2.txt=\"2 Listening\"");
		sendCommandAction(69U);

		if (m_screenLayout & LAYOUT_TA_ENABLE) {
			if (m_screenLayout & LAYOUT_TA_COLOUR)
				sendCommand("t2.pco=0");
			if (m_screenLayout & LAYOUT_TA_FONTSIZE)
				sendCommand("t2.font=4");
		}

		sendCommand("t3.txt=\"\"");
		sendCommand("t5.txt=\"\"");
		sendCommand("t7.txt=\"\"");
	}
}

void CNextion::writeFusionInt(const std::string& source, const std::string& dest, unsigned char dgid, const std::string& type, const std::string& origin)
{
	if (m_mode != MODE_YSF) {
		sendCommand("page YSF");
		sendCommandAction(4U);
	}

	char text[30U];
	if (m_brightness>0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	::sprintf(text, "t0.txt=\"%s %.10s\"", type.c_str(), source.c_str());
	sendCommand(text);
	sendCommandAction(82U);

	::sprintf(text, "t1.txt=\"DG-ID %u\"", dgid);
	sendCommand(text);
	sendCommandAction(83U);

	if (origin != "          ") {
		::sprintf(text, "t2.txt=\"at %.10s\"", origin.c_str());
		sendCommand(text);
		sendCommandAction(84U);
	}

	m_clockDisplayTimer.stop();

	m_mode = MODE_YSF;
}

void CNextion::writeFusionRSSIInt(int rssi)
{
	char text[25U];
	::sprintf(text, "t3.txt=\"%ddBm\"", rssi);
	sendCommand(text);
}

void CNextion::writeFusionBERInt(float ber)
{
	char text[25U];
	::sprintf(text, "t4.txt=\"%.1f%%\"", ber);
	sendCommand(text);
	sendCommandAction(86U);
}

void CNextion::clearFusionInt()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommandAction(81U);
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
	sendCommand("t4.txt=\"\"");
}

void CNextion::writeP25Int(const std::string& source, bool group, unsigned int dest, const std::string& type)
{
	if (m_mode != MODE_P25) {
		sendCommand("page P25");
		sendCommandAction(5U);
	}

	char text[30U];
	if (m_brightness>0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	::sprintf(text, "t0.txt=\"%s %.10s\"", type.c_str(), source.c_str());
	sendCommand(text);
	sendCommandAction(102U);

	::sprintf(text, "t1.txt=\"%s%u\"", group ? "TG" : "", dest);
	sendCommand(text);
	sendCommandAction(103U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_P25;
}

void CNextion::writeP25RSSIInt(int rssi)
{
	char text[25U];
	::sprintf(text, "t2.txt=\"%ddBm\"", rssi);
	sendCommand(text);
	sendCommandAction(104U);
}

void CNextion::writeP25BERInt(float ber)
{
	char text[25U];
	::sprintf(text, "t3.txt=\"%.1f%%\"", ber);
	sendCommand(text);
	sendCommandAction(105U);
}

void CNextion::clearP25Int()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommandAction(101U);
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
}

void CNextion::writeNXDNInt(const std::string& source, bool group, unsigned int dest, const std::string& type)
{
	if (m_mode != MODE_NXDN) {
		sendCommand("page NXDN");
		sendCommandAction(6U);
	}

	char text[30U];
	if (m_brightness>0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	::sprintf(text, "t0.txt=\"%s %.10s\"", type.c_str(), source.c_str());
	sendCommand(text);
	sendCommandAction(122U);

	::sprintf(text, "t1.txt=\"%s%u\"", group ? "TG" : "", dest);
	sendCommand(text);
	sendCommandAction(123U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_NXDN;
}

void CNextion::writeNXDNRSSIInt(int rssi)
{
	char text[25U];
	::sprintf(text, "t2.txt=\"%ddBm\"", rssi);
	sendCommand(text);
	sendCommandAction(124U);
}

void CNextion::writeNXDNBERInt(float ber)
{
	char text[25U];
	::sprintf(text, "t3.txt=\"%.1f%%\"", ber);
	sendCommand(text);
	sendCommandAction(125U);
}

void CNextion::clearNXDNInt()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommandAction(121U);
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
}

void CNextion::writeM17Int(const std::string& source, const std::string& dest, const std::string& type)
{
	if (m_mode != MODE_M17) {
		sendCommand("page M17");
		sendCommandAction(8U);
	}

	char text[30U];
	if (m_brightness > 0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	::sprintf(text, "t0.txt=\"%s %.10s\"", type.c_str(), source.c_str());
	sendCommand(text);
	sendCommandAction(142U);

	::sprintf(text, "t1.txt=\"%s\"", dest.c_str());
	sendCommand(text);
	sendCommandAction(143U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_M17;
}

void CNextion::writeM17RSSIInt(int rssi)
{
	char text[25U];
	::sprintf(text, "t2.txt=\"%ddBm\"", rssi);
	sendCommand(text);
	sendCommandAction(144U);
}

void CNextion::writeM17BERInt(float ber)
{
	char text[25U];
	::sprintf(text, "t3.txt=\"%.1f%%\"", ber);
	sendCommand(text);
	sendCommandAction(145U);
}

void CNextion::writeM17TextInt(const std::string& text)
{
	char buffer[105U];
	::sprintf(buffer, "t4.txt=\"%s\"", text.c_str());

	sendCommand(buffer);
	sendCommandAction(146U);
}

void CNextion::clearM17Int()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommandAction(141U);
	sendCommand("t1.txt=\"\"");
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
	sendCommand("t4.txt=\"\"");
}

void CNextion::writeFMInt(const std::string& status)
{
	if (m_mode != MODE_FM) {
		sendCommand("page FM");
		sendCommandAction(9U);
	}

	char text[100U];
	if (m_brightness > 0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	::sprintf(text, "t0.txt=\"%s\"", status.c_str());
	sendCommand(text);
	sendCommandAction(147U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_FM;
}

void CNextion::writeFMRSSIInt(int rssi)
{
	char text[25U];
	::sprintf(text, "t2.txt=\"%ddBm\"", rssi);
	sendCommand(text);
	sendCommandAction(148U);
}

void CNextion::clearFMInt()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommandAction(149U);
	sendCommand("t2.txt=\"\"");
}

void CNextion::writeAX25Int(const std::string& source, const std::string& source_cs, const std::string& destination_cs, const std::string& type, const std::string& pid, const std::string& data, int rssi)
{
	if (m_mode != MODE_AX25) {
		sendCommand("page AX25");
		sendCommandAction(10U);
	}

	char text[300U];
	if (m_brightness > 0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	std::string status = source + ": " + source_cs + ">" + destination_cs + " <" + type + "> 0x" + pid + " " + data;

	::sprintf(text, "t0.txt=\"%s\"", status.c_str());
	sendCommand(text);
	sendCommandAction(150U);

	::sprintf(text, "t2.txt=\"%ddBm\"", rssi);
	sendCommand(text);
	sendCommandAction(151U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_AX25;
}

void CNextion::writeAX25Int(const std::string& source, const std::string& source_cs, const std::string& destination_cs, const std::string& type, const std::string& pid, const std::string& data)
{
	if (m_mode != MODE_AX25) {
		sendCommand("page AX25");
		sendCommandAction(10U);
	}

	char text[300U];
	if (m_brightness > 0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	std::string status = source + ": " + source_cs + ">" + destination_cs + " <" + type + "> 0x" + pid + " " + data;

	::sprintf(text, "t0.txt=\"%s\"", status.c_str());
	sendCommand(text);
	sendCommandAction(150U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_AX25;
}

void CNextion::writeAX25Int(const std::string& source, const std::string& source_cs, const std::string& destination_cs, const std::string& type, int rssi)
{
	if (m_mode != MODE_AX25) {
		sendCommand("page AX25");
		sendCommandAction(10U);
	}

	char text[300U];
	if (m_brightness > 0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	std::string status = source + ": " + source_cs + ">" + destination_cs + " <" + type + ">";

	::sprintf(text, "t0.txt=\"%s\"", status.c_str());
	sendCommand(text);
	sendCommandAction(150U);

	::sprintf(text, "t2.txt=\"%ddBm\"", rssi);
	sendCommand(text);
	sendCommandAction(151U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_AX25;
}

void CNextion::writeAX25Int(const std::string& source, const std::string& source_cs, const std::string& destination_cs, const std::string& type)
{
	if (m_mode != MODE_AX25) {
		sendCommand("page AX25");
		sendCommandAction(10U);
	}

	char text[300U];
	if (m_brightness > 0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	std::string status = source + ": " + source_cs + ">" + destination_cs + " <" + type + ">";

	::sprintf(text, "t0.txt=\"%s\"", status.c_str());
	sendCommand(text);
	sendCommandAction(150U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_AX25;
}

void CNextion::clearAX25Int()
{
	sendCommand("t0.txt=\"Listening\"");
	sendCommandAction(153U);
	sendCommand("t2.txt=\"\"");
	sendCommand("t3.txt=\"\"");
}

void CNextion::writePOCSAGInt(uint32_t ric, const std::string& message)
{
	if (m_mode != MODE_POCSAG) {
		sendCommand("page POCSAG");
		sendCommandAction(7U);
	}

	char text[200U];
	if (m_brightness>0) {
		::sprintf(text, "dim=%u", m_brightness);
		sendCommand(text);
	}

	::sprintf(text, "t0.txt=\"RIC: %u\"", ric);
	sendCommand(text);
	sendCommandAction(132U);

	::sprintf(text, "t1.txt=\"%s\"", message.c_str());
	sendCommand(text);
	sendCommandAction(133U);

	m_clockDisplayTimer.stop();

	m_mode = MODE_POCSAG;
}

void CNextion::clearPOCSAGInt()
{
	sendCommand("t0.txt=\"Waiting\"");
	sendCommandAction(134U);
	sendCommand("t1.txt=\"\"");
}

void CNextion::writeCWInt()
{
	sendCommand("t1.txt=\"Sending CW Ident\"");
	sendCommandAction(12U);
	m_clockDisplayTimer.start();

	m_mode = MODE_CW;
}

void CNextion::clearCWInt()
{
	sendCommand("t1.txt=\"MMDVM IDLE\"");
	sendCommandAction(11U);
}

void CNextion::clockInt(unsigned int ms)
{
	assert(m_serial != NULL);

	// Update the clock display in IDLE mode every 400ms
	m_clockDisplayTimer.clock(ms);
	if (m_displayClock && (m_mode == MODE_IDLE || m_mode == MODE_CW) && m_clockDisplayTimer.isRunning() && m_clockDisplayTimer.hasExpired()) {
		time_t currentTime;
		struct tm *Time;
		::time(&currentTime);                   // Get the current time

		if (m_utc)
			Time = ::gmtime(&currentTime);
		else
			Time = ::localtime(&currentTime);

		::setlocale(LC_TIME,"");
		char text[50U];
		::strftime(text, 50, "t2.txt=\"%x %X\"", Time);
		sendCommand(text);

		m_clockDisplayTimer.start(); // restart the clock display timer
	}

	unsigned char c;
	unsigned int len;
	while ((len = m_serial->read(&c, 1U)) == 1U) {
		::memmove(m_reply + 1U, m_reply + 0U, MAX_REPLY_LENGTH - 1U);
		m_reply[0U] = c;

		// Do we have a valid reply?
		if (m_reply[0U] == 0xFFU && m_reply[1U] == 0xFFU && m_reply[2U] == 0xFFU) {
			switch (m_reply[3U]) {
				case 0x00U:	// Invalid instruction
				case 0x02U:	// Invalid component ID
				case 0x03U:	// Invalid page ID
				case 0x04U:	// Invalid picture ID
				case 0x05U:	// Invalid font ID
				case 0x06U:	// Invalid file operation
				case 0x09U:	// Invalid CRC
				case 0x11U:	// Invalid baud rate setting
				case 0x12U:	// Invalid waveform ID or channel number
				case 0x1AU:	// Invalid variable name or operation
				case 0x1BU:	// Invalid variable operation
				case 0x1CU:	// Assignment failed to assign
				case 0x1DU:	// EEPROM operation failed
				case 0x1EU:	// Invalid quantity of parameters
				case 0x1FU:	// IO operation failed
				case 0x20U:	// Escape character invalid
				case 0x23U:	// Variable name too long
				case 0x24U:	// Serial buffer overflow
					LogWarning("Nextion error response - 0x%02X", m_reply[3U]);
					break;
				case 0x01U:	// Instruction successful
					break;
				default:	// Unhandled response
					LogWarning("Unknown Nextion response - 0x%02X", m_reply[3U]);
					break;
			}

			m_waiting = false;
			break;
		}
	}

	if (!m_waiting) {
		if (!m_output.isEmpty()) {
			m_mutex.lock();

			unsigned char len = 0U;
			m_output.getData(&len, 1U);

			unsigned char buffer[200U];
			m_output.getData(buffer, len);

			m_mutex.unlock();

			CUtils::dump(1U, "Nextion output", buffer, len);

			m_serial->write(buffer, len);

			m_waiting = true;
		}
	}
}

void CNextion::close()
{
	m_serial->close();
	delete m_serial;
}

void CNextion::sendCommandAction(unsigned int status)
{
	if (!(m_screenLayout & LAYOUT_DIY))
		return;

	char text[30U];
	::sprintf(text, "MMDVM.status.val=%u", status);

	sendCommand(text);
	sendCommand("click S0,1");
}

void CNextion::sendCommand(const std::string& command)
{
	unsigned char len = (unsigned char)command.size();
	unsigned char c = len + 3U;

	m_mutex.lock();

	m_output.addData(&c, 1U);
	m_output.addData((unsigned char*)command.c_str(), len);
	m_output.addData((unsigned char*)"\xFF\xFF\xFF", 3U);

	m_mutex.unlock();
}

