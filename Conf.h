/*
 *   Copyright (C) 2015-2023 by Jonathan Naylor G4KLX
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

#if !defined(CONF_H)
#define	CONF_H

#include <string>
#include <vector>

class CConf
{
public:
	CConf(const std::string& file);
	~CConf();

	bool read();

	// The General section
	std::string  getCallsign() const;
	unsigned int getId() const;
	bool         getDuplex() const;
  	std::string  getMMDVMName() const;
	std::string  getDisplay() const;
	bool         getDaemon() const;

	// The Log section
	unsigned int getLogMQTTLevel() const;
	unsigned int getLogDisplayLevel() const;

	// The MQTT section
	std::string    getMQTTHost() const;
	unsigned short getMQTTPort() const;
	unsigned int   getMQTTKeepalive() const;
	std::string    getMQTTName() const;

	// The TFTSERIAL section
	std::string  getTFTSerialPort() const;
	unsigned int getTFTSerialBrightness() const;

	// The HD44780 section
	unsigned int getHD44780Rows() const;
	unsigned int getHD44780Columns() const;
	std::vector<unsigned int> getHD44780Pins() const;
	unsigned int getHD44780i2cAddress() const;
	bool         getHD44780PWM() const;
	unsigned int getHD44780PWMPin() const;
	unsigned int getHD44780PWMBright() const;
	unsigned int getHD44780PWMDim() const;
	bool         getHD44780DisplayClock() const;
	bool         getHD44780UTC() const;

	// The Nextion section
	std::string  getNextionPort() const;
	unsigned int getNextionBrightness() const;
	bool         getNextionDisplayClock() const;
	bool         getNextionUTC() const;
	unsigned int getNextionIdleBrightness() const;
	unsigned int getNextionScreenLayout() const;
	bool         getNextionTempInFahrenheit() const;

	// The OLED section
	unsigned char  getOLEDType() const;
	unsigned char  getOLEDBrightness() const;
	bool           getOLEDInvert() const;
	bool           getOLEDScroll() const;
	bool           getOLEDRotate() const;
	bool           getOLEDLogoScreensaver() const;

	// The LCDproc section
	std::string  getLCDprocAddress() const;
	unsigned short getLCDprocPort() const;
	unsigned short getLCDprocLocalPort() const;
	bool         getLCDprocDisplayClock() const;
	bool         getLCDprocUTC() const;
	bool         getLCDprocDimOnIdle() const;

private:
	std::string  m_file;
	std::string  m_callsign;
	unsigned int m_id;
	bool         m_duplex;
	std::string  m_mmdvmName;
	std::string  m_display;
	bool         m_daemon;

	unsigned int m_logMQTTLevel;
	unsigned int m_logDisplayLevel;

	std::string  m_mqttHost;
	unsigned short m_mqttPort;
	unsigned int m_mqttKeepalive;
	std::string  m_mqttName;

	std::string  m_tftSerialPort;
	unsigned int m_tftSerialBrightness;

	unsigned int m_hd44780Rows;
	unsigned int m_hd44780Columns;
	std::vector<unsigned int> m_hd44780Pins;
	unsigned int m_hd44780i2cAddress;
	bool         m_hd44780PWM;
	unsigned int m_hd44780PWMPin;
	unsigned int m_hd44780PWMBright;
	unsigned int m_hd44780PWMDim;
	bool         m_hd44780DisplayClock;
	bool         m_hd44780UTC;

	std::string  m_nextionPort;
	unsigned int m_nextionBrightness;
	bool         m_nextionDisplayClock;
	bool         m_nextionUTC;
	unsigned int m_nextionIdleBrightness;
	unsigned int m_nextionScreenLayout;
	bool         m_nextionTempInFahrenheit;
  
	unsigned char m_oledType;
	unsigned char m_oledBrightness;
	bool          m_oledInvert;
	bool          m_oledScroll;
	bool          m_oledRotate;
	bool          m_oledLogoScreensaver;

	std::string  m_lcdprocAddress;
	unsigned short m_lcdprocPort;
	unsigned short m_lcdprocLocalPort;
	bool         m_lcdprocDisplayClock;
	bool         m_lcdprocUTC;
	bool         m_lcdprocDimOnIdle;
};

#endif
