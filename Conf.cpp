/*
 *   Copyright (C) 2015-2023,2025 by Jonathan Naylor G4KLX
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

#include "Conf.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

const int BUFFER_SIZE = 500;

enum class SECTION {
	NONE,
	GENERAL,
	LOG,
	MQTT,
	TFTSERIAL,
	HD44780,
	NEXTION,
	OLED,
	LCDPROC
};

CConf::CConf(const std::string& file) :
m_file(file),
m_callsign(),
m_id(0U),
m_duplex(true),
m_mmdvmName("mmdvm"),
m_display("Nextion"),
m_daemon(false),
m_logMQTTLevel(0U),
m_logDisplayLevel(0U),
m_mqttAddress("127.0.0.1"),
m_mqttPort(1883),
m_mqttKeepalive(60U),
m_mqttName("display-driver"),
m_mqttAuthEnabled(false),
m_mqttUsername(),
m_mqttPassword(),
m_tftSerialPort("/dev/ttyAMA0"),
m_tftSerialBrightness(50U),
m_tftSerialScreenLayout(0U),
m_hd44780Rows(2U),
m_hd44780Columns(16U),
m_hd44780Pins(),
m_hd44780i2cAddress(),
m_hd44780PWM(false),
m_hd44780PWMPin(),
m_hd44780PWMBright(),
m_hd44780PWMDim(),
m_hd44780DisplayClock(false),
m_hd44780UTC(false),
m_nextionPort("/dev/ttyAMA0"),
m_nextionBrightness(50U),
m_nextionDisplayClock(false),
m_nextionUTC(false),
m_nextionIdleBrightness(20U),
m_nextionScreenLayout(0U),
m_nextionTempInFahrenheit(false),
m_oledType(3U),
m_oledBrightness(0U),
m_oledInvert(false),
m_oledScroll(false),
m_oledRotate(false),
m_oledLogoScreensaver(true),
m_lcdprocAddress(),
m_lcdprocPort(0U),
m_lcdprocLocalPort(0U),
m_lcdprocDisplayClock(false),
m_lcdprocUTC(false),
m_lcdprocDimOnIdle(false)
{
}

CConf::~CConf()
{
}

bool CConf::read()
{
	FILE* fp = ::fopen(m_file.c_str(), "rt");
	if (fp == nullptr) {
		::fprintf(stderr, "Couldn't open the .ini file - %s\n", m_file.c_str());
		return false;
	}

	SECTION section = SECTION::NONE;

	char buffer[BUFFER_SIZE];
	while (::fgets(buffer, BUFFER_SIZE, fp) != nullptr) {
		if (buffer[0U] == '#')
			continue;

		if (buffer[0U] == '[') {
			if (::strncmp(buffer, "[General]", 9U) == 0)
				section = SECTION::GENERAL;
			else if (::strncmp(buffer, "[Log]", 5U) == 0)
				section = SECTION::LOG;
			else if (::strncmp(buffer, "[MQTT]", 6U) == 0)
				section = SECTION::MQTT;
			else if (::strncmp(buffer, "[TFT Serial]", 12U) == 0)
				section = SECTION::TFTSERIAL;
			else if (::strncmp(buffer, "[HD44780]", 9U) == 0)
				section = SECTION::HD44780;
			else if (::strncmp(buffer, "[Nextion]", 9U) == 0)
				section = SECTION::NEXTION;
			else if (::strncmp(buffer, "[OLED]", 6U) == 0)
				section = SECTION::OLED;
			else if (::strncmp(buffer, "[LCDproc]", 9U) == 0)
				section = SECTION::LCDPROC;
			else
				section = SECTION::NONE;

			continue;
		}

		char* key = ::strtok(buffer, " \t=\r\n");
		if (key == nullptr)
			continue;

		char* value = ::strtok(nullptr, "\r\n");
		if (value == nullptr)
			continue;

		// Remove quotes from the value
		size_t len = ::strlen(value);
		if (len > 1U && *value == '"' && value[len - 1U] == '"') {
			value[len - 1U] = '\0';
			value++;
		} else {
			char *p;

			// if value is not quoted, remove after # (to make comment)
			if ((p = strchr(value, '#')) != nullptr)
				*p = '\0';

			// remove trailing tab/space
			for (p = value + strlen(value) - 1U; p >= value && (*p == '\t' || *p == ' '); p--)
				*p = '\0';
		}

		if (section == SECTION::GENERAL) {
			if (::strcmp(key, "Callsign") == 0) {
				// Convert the callsign to upper case
				for (unsigned int i = 0U; value[i] != 0; i++)
					value[i] = ::toupper(value[i]);
				m_callsign = value;
			} else if (::strcmp(key, "Id") == 0)
				m_id = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Duplex") == 0)
				m_duplex = ::atoi(value) == 1;
			else if (::strcmp(key, "MMDVMName") == 0)
				m_mmdvmName = value;
			else if (::strcmp(key, "Display") == 0)
				m_display = value;
			else if (::strcmp(key, "Daemon") == 0)
				m_daemon = ::atoi(value) == 1;
		} else if (section == SECTION::LOG) {
			if (::strcmp(key, "MQTTLevel") == 0)
				m_logMQTTLevel = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DisplayLevel") == 0)
				m_logDisplayLevel = (unsigned int)::atoi(value);
		} else if (section == SECTION::MQTT) {
			if (::strcmp(key, "Host") == 0)
				m_mqttAddress = value;
			else if (::strcmp(key, "Port") == 0)
				m_mqttPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "Keepalive") == 0)
				m_mqttKeepalive = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Name") == 0)
				m_mqttName = value;
			else if (::strcmp(key, "Auth") == 0)
				m_mqttAuthEnabled = ::atoi(value) == 1;
			else if (::strcmp(key, "Username") == 0)
				m_mqttUsername = value;
			else if (::strcmp(key, "Password") == 0)
				m_mqttPassword = value;
		} else if (section == SECTION::TFTSERIAL) {
			if (::strcmp(key, "Port") == 0)
				m_tftSerialPort = value;
			else if (::strcmp(key, "Brightness") == 0)
				m_tftSerialBrightness = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ScreenLayout") == 0)
				m_tftSerialScreenLayout = (unsigned int)::atoi(value);
		} else if (section == SECTION::HD44780) {
			if (::strcmp(key, "Rows") == 0)
				m_hd44780Rows = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Columns") == 0)
				m_hd44780Columns = (unsigned int)::atoi(value);
			else if (::strcmp(key, "I2CAddress") == 0)
				m_hd44780i2cAddress = (unsigned int)::strtoul(value, nullptr, 16);
			else if (::strcmp(key, "PWM") == 0)
				m_hd44780PWM = ::atoi(value) == 1;
			else if (::strcmp(key, "PWMPin") == 0)
				m_hd44780PWMPin = (unsigned int)::atoi(value);
			else if (::strcmp(key, "PWMBright") == 0)
				m_hd44780PWMBright = (unsigned int)::atoi(value);
			else if (::strcmp(key, "PWMDim") == 0)
				m_hd44780PWMDim = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DisplayClock") == 0)
				m_hd44780DisplayClock = ::atoi(value) == 1;
			else if (::strcmp(key, "UTC") == 0)
				m_hd44780UTC = ::atoi(value) == 1;
			else if (::strcmp(key, "Pins") == 0) {
				char* p = ::strtok(value, ",\r\n");
				while (p != nullptr) {
					unsigned int pin = (unsigned int)::atoi(p);
					m_hd44780Pins.push_back(pin);
					p = ::strtok(nullptr, ",\r\n");
				}
			}
		} else if (section == SECTION::NEXTION) {
			if (::strcmp(key, "Port") == 0)
				m_nextionPort = value;
			else if (::strcmp(key, "Brightness") == 0)
				m_nextionIdleBrightness = m_nextionBrightness = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DisplayClock") == 0)
				m_nextionDisplayClock = ::atoi(value) == 1;
			else if (::strcmp(key, "UTC") == 0)
				m_nextionUTC = ::atoi(value) == 1;
			else if (::strcmp(key, "IdleBrightness") == 0)
				m_nextionIdleBrightness = (unsigned int)::atoi(value);
			else if (::strcmp(key, "ScreenLayout") == 0)
				m_nextionScreenLayout = (unsigned int)::strtoul(value, nullptr, 0);
			else if (::strcmp(key, "DisplayTempInFahrenheit") == 0)
				m_nextionTempInFahrenheit = ::atoi(value) == 1;
		} else if (section == SECTION::OLED) {
			if (::strcmp(key, "Type") == 0)
				m_oledType = (unsigned char)::atoi(value);
			else if (::strcmp(key, "Brightness") == 0)
				m_oledBrightness = (unsigned char)::atoi(value);
			else if (::strcmp(key, "Invert") == 0)
				m_oledInvert = ::atoi(value) == 1;
			else if (::strcmp(key, "Scroll") == 0)
				m_oledScroll = ::atoi(value) == 1;
			else if (::strcmp(key, "Rotate") == 0)
				m_oledRotate = ::atoi(value) == 1;
			else if (::strcmp(key, "LogoScreensaver") == 0)
				m_oledLogoScreensaver = ::atoi(value) == 1;
		} else if (section == SECTION::LCDPROC) {
			if (::strcmp(key, "Address") == 0)
				m_lcdprocAddress = value;
			else if (::strcmp(key, "Port") == 0)
				m_lcdprocPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "LocalPort") == 0)
				m_lcdprocLocalPort = (unsigned short)::atoi(value);
			else if (::strcmp(key, "DisplayClock") == 0)
				m_lcdprocDisplayClock = ::atoi(value) == 1;
			else if (::strcmp(key, "UTC") == 0)
				m_lcdprocUTC = ::atoi(value) == 1;
			else if (::strcmp(key, "DimOnIdle") == 0)
				m_lcdprocDimOnIdle = ::atoi(value) == 1;
		}
	}

	::fclose(fp);

	return true;
}

std::string CConf::getCallsign() const
{
	return m_callsign;
}

unsigned int CConf::getId() const
{
	return m_id;
}

bool CConf::getDuplex() const
{
	return m_duplex;
}

std::string CConf::getMMDVMName() const
{
	return m_mmdvmName;
}

std::string CConf::getDisplay() const
{
	return m_display;
}

bool CConf::getDaemon() const
{
	return m_daemon;
}

unsigned int CConf::getLogMQTTLevel() const
{
	return m_logMQTTLevel;
}

unsigned int CConf::getLogDisplayLevel() const
{
	return m_logDisplayLevel;
}

std::string CConf::getMQTTAddress() const
{
	return m_mqttAddress;
}

unsigned short CConf::getMQTTPort() const
{
	return m_mqttPort;
}

unsigned int CConf::getMQTTKeepalive() const
{
	return m_mqttKeepalive;
}

std::string CConf::getMQTTName() const
{
	return m_mqttName;
}

bool CConf::getMQTTAuthEnabled() const
{
	return m_mqttAuthEnabled;
}

std::string CConf::getMQTTUsername() const
{
	return m_mqttUsername;
}

std::string CConf::getMQTTPassword() const
{
	return m_mqttPassword;
}

std::string CConf::getTFTSerialPort() const
{
	return m_tftSerialPort;
}

unsigned int CConf::getTFTSerialBrightness() const
{
	return m_tftSerialBrightness;
}

unsigned int CConf::getTFTSerialScreenLayout() const
{
	return m_tftSerialScreenLayout;
}

unsigned int CConf::getHD44780Rows() const
{
	return m_hd44780Rows;
}

unsigned int CConf::getHD44780Columns() const
{
	return m_hd44780Columns;
}

std::vector<unsigned int> CConf::getHD44780Pins() const
{
	return m_hd44780Pins;
}

unsigned int CConf::getHD44780i2cAddress() const
{
  return m_hd44780i2cAddress;
}

bool CConf::getHD44780PWM() const
{
	return m_hd44780PWM;
}

unsigned int CConf::getHD44780PWMPin() const
{
	return m_hd44780PWMPin;
}

unsigned int CConf::getHD44780PWMBright() const
{
	return m_hd44780PWMBright;
}

unsigned int CConf::getHD44780PWMDim() const
{
	return m_hd44780PWMDim;
}

bool CConf::getHD44780DisplayClock() const
{
	return m_hd44780DisplayClock;
}

bool CConf::getHD44780UTC() const
{
	return m_hd44780UTC;
}

std::string CConf::getNextionPort() const
{
	return m_nextionPort;
}

unsigned int CConf::getNextionBrightness() const
{
	return m_nextionBrightness;
}

bool CConf::getNextionDisplayClock() const
{
	return m_nextionDisplayClock;
}

bool CConf::getNextionUTC() const
{
	return m_nextionUTC;
}

unsigned int CConf::getNextionIdleBrightness() const
{
	return m_nextionIdleBrightness;
}

unsigned int CConf::getNextionScreenLayout() const
{
	return m_nextionScreenLayout;
}

unsigned char CConf::getOLEDType() const
{
	return m_oledType;
}

unsigned char CConf::getOLEDBrightness() const
{
	return m_oledBrightness;
}

bool CConf::getOLEDInvert() const
{
	return m_oledInvert;
}

bool CConf::getOLEDScroll() const
{
	return m_oledScroll;
}

bool CConf::getOLEDRotate() const
{
	return m_oledRotate;
}

bool CConf::getOLEDLogoScreensaver() const
{
	return m_oledLogoScreensaver;
}

std::string CConf::getLCDprocAddress() const
{
	return m_lcdprocAddress;
}

unsigned short CConf::getLCDprocPort() const
{
	return m_lcdprocPort;
}

unsigned short CConf::getLCDprocLocalPort() const
{
	return m_lcdprocLocalPort;
}

bool CConf::getLCDprocDisplayClock() const
{
	return m_lcdprocDisplayClock;
}

bool CConf::getLCDprocUTC() const
{
	return m_lcdprocUTC;
}

bool CConf::getLCDprocDimOnIdle() const
{
	return m_lcdprocDimOnIdle;
}

bool CConf::getNextionTempInFahrenheit() const
{
	return m_nextionTempInFahrenheit;
}

