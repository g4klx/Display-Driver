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

#include "NextionUpdater.h"
#include "ModemSerialPort.h"
#include "MQTTConnection.h"
#include "UARTController.h"
#include "StopWatch.h"
#include "Version.h"
#include "Defines.h"
#include "Thread.h"
#include "Utils.h"
#include "Conf.h"
#include "Log.h"
#include "GitVersion.h"

#include <cstdio>
#include <vector>

#include <cstdlib>

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "NextionUpdater.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/NextionUpdater.ini";
#endif

// In Log.cpp
extern CMQTTConnection* m_mqtt;

CNextionUpdater* driver = NULL;

#if defined(_WIN32) || defined(_WIN64)
char* optarg = NULL;
int optind = 1;

int getopt(int argc, char* const argv[], const char* optstring)
{
	if ((optind >= argc) || (argv[optind][0] != '-') || (argv[optind][0] == 0))
		return -1;

	int opt = argv[optind][1];
	const char *p = strchr(optstring, opt);

	if (p == NULL) {
		return '?';
	}

	if (p[1] == ':') {
		optind++;
		if (optind >= argc)
			return '?';

		optarg = argv[optind];
	}

	optind++;

	return opt;
}
#else
#include <unistd.h>
#endif

int main(int argc, char** argv)
{
	std::string iniFile = DEFAULT_INI_FILE;

	int c;
	while ((c = ::getopt(argc, argv, "c:v")) != -1) {
		switch (c) {
		case 'c':
			iniFile = std::string(optarg);
			break;
		case 'v':
			::fprintf(stdout, "NextionUpdater version %s git #%.7s\n", VERSION, gitversion);
			return 0;
		case '?':
			break;
		default:
			::fprintf(stderr, "Usage: NextionUpdater [-v|--version] [-c <config filename>] <filename>\n");
			break;
		}
	}

	if (optind > (argc - 1)) {
		::fprintf(stderr, "Usage: NextionUpdater [-v|--version] [-c <config filename>] <filename>\n");
		return 1;
	}

	driver = new CNextionUpdater(std::string(iniFile), std::string(argv[argc - 1]));
	int ret = driver->run();
	delete driver;

	::LogFinalise();

	return ret;
}

CNextionUpdater::CNextionUpdater(const std::string& confFile, const std::string& filename) :
m_filename(filename),
m_conf(confFile),
m_serial(NULL),
m_msp(NULL)
{
}

CNextionUpdater::~CNextionUpdater()
{
}

int CNextionUpdater::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "NextionUpdater: cannot read the .ini file\n");
		return 1;
	}

	FILE* file = ::fopen(m_filename.c_str(), "rb");
	if (file == NULL) {
		::fprintf(stderr, "NextionUpdater: cannot read the firmware file\n");
		return 1;
	}

	struct stat statbuf;
	::stat(m_filename.c_str(), &statbuf);

	::LogInitialise(m_conf.getLogDisplayLevel(), m_conf.getLogMQTTLevel());

	const std::string displayName = m_conf.getMMDVMName() + "/display-out";

	std::vector<std::pair<std::string, void (*)(const unsigned char*, unsigned int)>> subscriptions;
	subscriptions.push_back(std::make_pair(displayName, CNextionUpdater::onDisplay));

	m_mqtt = new CMQTTConnection(m_conf.getMQTTHost(), m_conf.getMQTTPort(), m_conf.getMQTTName(), subscriptions, m_conf.getMQTTKeepalive());
	ret = m_mqtt->open();
	if (!ret) {
		::fprintf(stderr, "NextionUpdater: unable to start the MQTT Publisher\n");
		delete m_mqtt;
		return 1;
	}

	LogInfo("NextionUpdater-%s is starting", VERSION);
	LogInfo("Built %s %s (GitID #%.7s)", __TIME__, __DATE__, gitversion);

	writeJSONMessage("NextionUpdater is starting");

	ret = createPort();
	if (!ret) {
		LogInfo("Cannot open the serial port");
		writeJSONMessage("Cannot open the serial port");

		m_serial->close();
		delete m_serial;

		::fclose(file);

		return 1;
	}

	unsigned int screenLayout = m_conf.getNextionScreenLayout();
	unsigned int baudrate = 9600U;
	if (screenLayout == 4U)
		baudrate = 115200U;

	char command[100U];
	::sprintf(command, "whmi-wri %ld,%u,0\xFF\xFF\xFF", statbuf.st_size, baudrate);
	m_serial->write((unsigned char*)command, ::strlen(command));

	CUtils::dump(1U, "Nextion command", (unsigned char*)command, ::strlen(command));

	ret = waitForResponse(false);
	if (!ret) {
		LogInfo("No response to the upload command");
		writeJSONMessage("No response to the upload command");

		m_serial->close();
		delete m_serial;

		::fclose(file);

		return 1;
	}

	ret = uploadFile(file, statbuf.st_size);
	if (!ret) {
		LogInfo("File upload failure");
		writeJSONMessage("File upload failure");

		m_serial->close();
		delete m_serial;

		::fclose(file);

		return 1;
	}

	LogInfo("File uploaded");
	writeJSONMessage("File uploaded");

	m_serial->close();
	delete m_serial;

	::fclose(file);

	return 0;
}

bool CNextionUpdater::createPort()
{
	std::string type = m_conf.getDisplay();

	if (type == "Nextion") {
		std::string port          = m_conf.getNextionPort();
		unsigned int screenLayout = m_conf.getNextionScreenLayout();

		LogInfo("Nextion port: %s", port.c_str());

		if (port == "modem") {
			m_serial = m_msp = new CModemSerialPort(m_conf.getMMDVMName());
		} else {
			unsigned int baudrate = 9600U;
			if (screenLayout == 4U)
				baudrate = 115200U;
			
			m_serial = new CUARTController(port, baudrate);
		}
	} else {
		LogError("No Nextion display found");
		return false;
	}

	return m_serial->open();
}

bool CNextionUpdater::waitForResponse(bool wait)
{
	assert(m_serial != NULL);

	CStopWatch stopWatch;
	stopWatch.start();

	unsigned int ms = 0U;
	bool found = false;

	while (ms < 500U) {
		unsigned char c;
		unsigned int n = m_serial->read(&c, 1U);

		if (n == 1U) {
			if (c == 0x05U) {
				found = true;
				if (!wait)
					break;
			}
		}

		CThread::sleep(10U);

		ms = stopWatch.elapsed();
	}

	return found;
}

bool CNextionUpdater::uploadFile(FILE* file, long fileSize)
{
	assert(file != NULL);

	unsigned int sendCount = fileSize / 4096U + 1U;
	unsigned int lastSize  = fileSize % 4096U;

	do {
		if (sendCount == 1U) {
			for (unsigned int i = 0U; i < lastSize; i++) {
				unsigned char c = ::fgetc(file);
				m_serial->write(&c, 1U);
			}
		} else {
			for (unsigned int i = 0U; i < 4096U; i++) {
				unsigned char c = ::fgetc(file);
				m_serial->write(&c, 1U);
			}
		}

		bool ret = waitForResponse(true);
		if (!ret)
			return false;

		sendCount--;

	} while (sendCount > 0U);

	return true;
}

void CNextionUpdater::writeJSONMessage(const std::string& message)
{
	nlohmann::json json;

	json["timestamp"] = CUtils::createTimestamp();
	json["message"]   = message;

	WriteJSON("status", json);
}

void CNextionUpdater::readDisplay(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (m_msp != NULL)
		m_msp->readData(data, length);
}

void CNextionUpdater::onDisplay(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);
	assert(driver != NULL);

	driver->readDisplay(data, length);
}

