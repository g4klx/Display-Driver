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

#include "DisplayDriver.h"
#include "UARTController.h"
#include "MQTTConnection.h"
#include "Version.h"
#include "StopWatch.h"
#include "Defines.h"
#include "Thread.h"
#include "Utils.h"
#include "Log.h"
#include "GitVersion.h"

#include <cstdio>
#include <vector>

#include <cstdlib>

#include <nlohmann/json.hpp>

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "DisplayDriver.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/DisplayDriver.ini";
#endif

static bool m_killed = false;
static int  m_signal = 0;
static bool m_reload = false;

// In Log.cpp
extern CMQTTConnection* m_mqtt;

CDisplayDriver* driver = NULL;

#if !defined(_WIN32) && !defined(_WIN64)
static void sigHandler1(int signum)
{
	m_killed = true;
	m_signal = signum;
}

static void sigHandler2(int signum)
{
	m_reload = true;
}
#endif

int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
 		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "DisplayDriver version %s git #%.7s\n", VERSION, gitversion);
				return 0;
			} else if (arg.substr(0,1) == "-") {
				::fprintf(stderr, "Usage: DisplayDriver [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

#if !defined(_WIN32) && !defined(_WIN64)
	::signal(SIGINT,  sigHandler1);
	::signal(SIGTERM, sigHandler1);
	::signal(SIGHUP,  sigHandler1);
	::signal(SIGUSR1, sigHandler2);
#endif

	int ret = 0;

	do {
		m_signal = 0;

		driver = new CDisplayDriver(std::string(iniFile));
		ret = driver->run();
		delete driver;

		switch (m_signal) {
			case 2:
				::LogInfo("DisplayDriver-%s exited on receipt of SIGINT", VERSION);
				break;
			case 15:
				::LogInfo("DisplayDriver-%s exited on receipt of SIGTERM", VERSION);
				break;
			case 1:
				::LogInfo("DisplayDriver-%s exited on receipt of SIGHUP", VERSION);
				break;
			case 10:
				::LogInfo("DisplayDriver-%s is restarting on receipt of SIGUSR1", VERSION);
				break;
			default:
				::LogInfo("DisplayDriver-%s exited on receipt of an unknown signal", VERSION);
				break;
		}
	} while (m_signal == 10);

	::LogFinalise();

	return ret;
}

CDisplayDriver::CDisplayDriver(const std::string& confFile) :
m_conf(confFile),
m_display(NULL)
{
}

CDisplayDriver::~CDisplayDriver()
{
}

int CDisplayDriver::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "DisplayDriver: cannot read the .ini file\n");
		return 1;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	bool m_daemon = m_conf.getDaemon();
	if (m_daemon) {
		// Create new process
		pid_t pid = ::fork();
		if (pid == -1) {
			::fprintf(stderr, "Couldn't fork() , exiting\n");
			return -1;
		} else if (pid != 0) {
			exit(EXIT_SUCCESS);
		}

		// Create new session and process group
		if (::setsid() == -1){
			::fprintf(stderr, "Couldn't setsid(), exiting\n");
			return -1;
		}

		// Set the working directory to the root directory
		if (::chdir("/") == -1){
			::fprintf(stderr, "Couldn't cd /, exiting\n");
			return -1;
		}

#if !defined(HD44780) && !defined(OLED) && !defined(_OPENWRT)
		// If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == NULL) {
				::fprintf(stderr, "Could not get the mmdvm user, exiting\n");
				return -1;
			}

			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			// Set user and group ID's to mmdvm:mmdvm
			if (::setgid(mmdvm_gid) != 0) {
				::fprintf(stderr, "Could not set mmdvm GID, exiting\n");
				return -1;
			}

			if (::setuid(mmdvm_uid) != 0) {
				::fprintf(stderr, "Could not set mmdvm UID, exiting\n");
				return -1;
			}

			// Double check it worked (AKA Paranoia)
			if (::setuid(0) != -1){
				::fprintf(stderr, "It's possible to regain root - something is wrong!, exiting\n");
				return -1;
			}
		}
	}
#else
	::fprintf(stderr, "Dropping root permissions in daemon mode is disabled.\n");
	}
#endif
#endif
	::LogInitialise(m_conf.getLogDisplayLevel(), m_conf.getLogMQTTLevel());

	std::vector<std::pair<std::string, void (*)(const unsigned char*, unsigned int)>> subscriptions;
	// subscriptions.push_back(std::make_pair("display", CMMDVMHost::onDisplay));
	// subscriptions.push_back(std::make_pair("command", CMMDVMHost::onCommand));

	m_mqtt = new CMQTTConnection(m_conf.getMQTTHost(), m_conf.getMQTTPort(), m_conf.getMQTTName(), subscriptions, m_conf.getMQTTKeepalive());
	ret = m_mqtt->open();
	if (!ret) {
		::fprintf(stderr, "MMDVMHost: unable to start the MQTT Publisher\n");
		delete m_mqtt;
		return 1;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	if (m_daemon) {
		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
	}
#endif
	LogInfo("DisplayDriver-%s is starting", VERSION);
	LogInfo("Built %s %s (GitID #%.7s)", __TIME__, __DATE__, gitversion);

	writeJSONMessage("DisplayDriver is starting");

	m_display = CDisplay::createDisplay(m_conf);
	if (m_display == NULL)
		return 1;

	CStopWatch stopWatch;
	stopWatch.start();

	while (!m_killed) {
		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		m_display->clock(ms);

		if (ms < 5U)
			CThread::sleep(5U);
	}

	LogInfo("DisplayDriver is stopping");
	writeJSONMessage("DisplayDriver is stopping");

	m_display->close();
	delete m_display;

	return 0;
}

void CDisplayDriver::writeJSONMessage(const std::string& message)
{
	nlohmann::json json;

	json["timestamp"] = CUtils::createTimestamp();
	json["message"]   = message;

	WriteJSON("MMDVM", json);
}

