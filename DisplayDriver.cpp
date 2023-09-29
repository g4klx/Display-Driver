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
#include "MQTTConnection.h"
#include "UARTController.h"
#include "TFTSurenoo.h"
#include "StopWatch.h"
#include "LCDproc.h"
#include "Nextion.h"
#include "Version.h"
#include "Defines.h"
#include "Thread.h"
#include "Utils.h"
#include "Conf.h"
#include "Log.h"
#include "GitVersion.h"

#if defined(HD44780)
#include "HD44780.h"
#endif

#if defined(OLED)
#include "OLED.h"
#endif

#include <cstdio>
#include <vector>

#include <cstdlib>

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
m_display(NULL),
m_msp(NULL)
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

	const std::string displayName = m_conf.getMMDVMName() + "/display-out";
	const std::string jsonName    = m_conf.getMMDVMName() + "/json";

	std::vector<std::pair<std::string, void (*)(const unsigned char*, unsigned int)>> subscriptions;
	subscriptions.push_back(std::make_pair(displayName, CDisplayDriver::onDisplay));
	subscriptions.push_back(std::make_pair(jsonName,    CDisplayDriver::onJSON));

	m_mqtt = new CMQTTConnection(m_conf.getMQTTHost(), m_conf.getMQTTPort(), m_conf.getMQTTName(), subscriptions, m_conf.getMQTTKeepalive());
	ret = m_mqtt->open();
	if (!ret) {
		::fprintf(stderr, "DisplayDriver: unable to start the MQTT Publisher\n");
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

	ret = createDisplay();
	if (!ret)
		return 1;

	CStopWatch stopWatch;
	stopWatch.start();

	while (!m_killed) {
		unsigned int ms = stopWatch.elapsed();
		stopWatch.start();

		m_display->clock(ms);

		if (ms < 10U)
			CThread::sleep(10U);
	}

	LogInfo("DisplayDriver is stopping");
	writeJSONMessage("DisplayDriver is stopping");

	m_display->close();
	delete m_display;

	return 0;
}

bool CDisplayDriver::createDisplay()
{
	std::string type = m_conf.getDisplay();

	LogInfo("Display Parameters");
	LogInfo("    Type: %s", type.c_str());

	if (type == "TFT Surenoo") {
		std::string port        = m_conf.getTFTSerialPort();
		unsigned int brightness = m_conf.getTFTSerialBrightness();

		LogInfo("    Port: %s", port.c_str());
		LogInfo("    Brightness: %u", brightness);

		ISerialPort* serial = NULL;
		if (port == "modem")
			serial = m_msp = new CModemSerialPort(m_conf.getMMDVMName());
		else
			serial = new CUARTController(port, 115200U);

		m_display = new CTFTSurenoo(m_conf.getCallsign(), m_conf.getId(), m_conf.getDuplex(), serial, brightness);
	} else if (type == "Nextion") {
		std::string port            = m_conf.getNextionPort();
		unsigned int brightness     = m_conf.getNextionBrightness();
		bool displayClock           = m_conf.getNextionDisplayClock();
		bool utc                    = m_conf.getNextionUTC();
		unsigned int idleBrightness = m_conf.getNextionIdleBrightness();
		unsigned int screenLayout   = m_conf.getNextionScreenLayout();
		bool displayTempInF         = m_conf.getNextionTempInFahrenheit();

		LogInfo("    Port: %s", port.c_str());
		LogInfo("    Brightness: %u", brightness);
		LogInfo("    Clock Display: %s", displayClock ? "yes" : "no");
		if (displayClock)
			LogInfo("    Display UTC: %s", utc ? "yes" : "no");
		LogInfo("    Idle Brightness: %u", idleBrightness);
		LogInfo("    Temperature in Fahrenheit: %s ", displayTempInF ? "yes" : "no");
 
		switch (screenLayout) {
		case 0U:
			LogInfo("    Screen Layout: G4KLX (Default)");
			break;
		case 2U:
			LogInfo("    Screen Layout: ON7LDS");
			break;
		case 3U:
			LogInfo("    Screen Layout: DIY by ON7LDS");
			break;
		case 4U:
			LogInfo("    Screen Layout: DIY by ON7LDS (High speed)");
			break;
		default:
			LogInfo("    Screen Layout: %u (Unknown)", screenLayout);
			break;
		}

		if (port == "modem") {
			ISerialPort* serial = m_msp = new CModemSerialPort(m_conf.getMMDVMName());
			m_display = new CNextion(m_conf.getCallsign(), m_conf.getId(), m_conf.getDuplex(), serial, brightness, displayClock, utc, idleBrightness, screenLayout, displayTempInF);
		} else {
			unsigned int baudrate = 9600U;
			if (screenLayout == 4U)
				baudrate = 115200U;
			
			LogInfo("    Display baudrate: %u ", baudrate);
			ISerialPort* serial = new CUARTController(port, baudrate);
			m_display = new CNextion(m_conf.getCallsign(), m_conf.getId(), m_conf.getDuplex(), serial, brightness, displayClock, utc, idleBrightness, screenLayout, displayTempInF);
		}
	} else if (type == "LCDproc") {
		std::string address       = m_conf.getLCDprocAddress();
		unsigned int port         = m_conf.getLCDprocPort();
		unsigned int localPort    = m_conf.getLCDprocLocalPort();
		bool displayClock         = m_conf.getLCDprocDisplayClock();
		bool utc                  = m_conf.getLCDprocUTC();
		bool dimOnIdle            = m_conf.getLCDprocDimOnIdle();

		LogInfo("    Address: %s", address.c_str());
		LogInfo("    Port: %u", port);

		if (localPort == 0U)
			LogInfo("    Local Port: random");
		else
			LogInfo("    Local Port: %u", localPort);

		LogInfo("    Dim Display on Idle: %s", dimOnIdle ? "yes" : "no");
		LogInfo("    Clock Display: %s", displayClock ? "yes" : "no");

		if (displayClock)
			LogInfo("    Display UTC: %s", utc ? "yes" : "no");

		m_display = new CLCDproc(m_conf.getCallsign(), m_conf.getId(), m_conf.getDuplex(), address, port, localPort, displayClock, utc, dimOnIdle);
#if defined(HD44780)
	} else if (type == "HD44780") {
		unsigned int rows              = m_conf.getHD44780Rows();
		unsigned int columns           = m_conf.getHD44780Columns();
		std::vector<unsigned int> pins = m_conf.getHD44780Pins();
		unsigned int i2cAddress        = m_conf.getHD44780i2cAddress();
		bool pwm                       = m_conf.getHD44780PWM();
		unsigned int pwmPin            = m_conf.getHD44780PWMPin();
		unsigned int pwmBright         = m_conf.getHD44780PWMBright();
		unsigned int pwmDim            = m_conf.getHD44780PWMDim();
		bool displayClock              = m_conf.getHD44780DisplayClock();
		bool utc                       = m_conf.getHD44780UTC();

		if (pins.size() == 6U) {
			LogInfo("    Rows: %u", rows);
			LogInfo("    Columns: %u", columns);

#if defined(ADAFRUIT_DISPLAY) || defined(PCF8574_DISPLAY)
			LogInfo("    Device Address: %#x", i2cAddress);
#else
			LogInfo("    Pins: %u,%u,%u,%u,%u,%u", pins.at(0U), pins.at(1U), pins.at(2U), pins.at(3U), pins.at(4U), pins.at(5U));
#endif

			LogInfo("    PWM Backlight: %s", pwm ? "yes" : "no");
			if (pwm) {
				LogInfo("    PWM Pin: %u", pwmPin);
				LogInfo("    PWM Bright: %u", pwmBright);
				LogInfo("    PWM Dim: %u", pwmDim);
			}

			LogInfo("    Clock Display: %s", displayClock ? "yes" : "no");
			if (displayClock)
				LogInfo("    Display UTC: %s", utc ? "yes" : "no");

			m_display = new CHD44780(m_conf.getCallsign(), m_conf.getId(), m_conf.getDuplex(), rows, columns, pins, i2cAddress, pwm, pwmPin, pwmBright, pwmDim, displayClock, utc);
		}
#endif
#if defined(OLED)
	} else if (type == "OLED") {
	        unsigned char type       = m_conf.getOLEDType();
	        unsigned char brightness = m_conf.getOLEDBrightness();
	        bool          invert     = m_conf.getOLEDInvert();
	        bool          scroll     = m_conf.getOLEDScroll();
		bool          rotate     = m_conf.getOLEDRotate();
		bool          logosaver  = m_conf.getOLEDLogoScreensaver();

		m_display = new COLED(m_conf.getCallsign(), m_conf.getId(), m_conf.getDuplex(), type, brightness, invert, scroll, rotate, logosaver);
#endif
	} else {
		LogError("No valid display found");
		return false;
	}

	bool ret = m_display->open();
	if (!ret) {
		delete m_display;
		return false;
	}

	return true;
}

void CDisplayDriver::writeJSONMessage(const std::string& message)
{
	nlohmann::json json;

	json["timestamp"] = CUtils::createTimestamp();
	json["message"]   = message;

	WriteJSON("status", json);
}

void CDisplayDriver::readDisplay(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (m_msp != NULL)
		m_msp->readData(data, length);
}

void CDisplayDriver::readJSON(const std::string& text)
{
	try {
		nlohmann::json j = nlohmann::json::parse(text);

		bool exists = j["MMDVM"].is_object();
		if (exists) {
			parseMMDVM(j["MMDVM"]);
			return;
		}

		exists = j["RSSI"].is_object();
		if (exists) {
			parseRSSI(j["RSSI"]);
			return;
		}

		exists = j["BER"].is_object();
		if (exists) {
			parseBER(j["BER"]);
			return;
		}

		exists = j["Text"].is_object();
		if (exists) {
			parseText(j["Text"]);
			return;
		}

		exists = j["D-Star"].is_object();
		if (exists) {
			parseDStar(j["D-Star"]);
			return;
		}

		exists = j["DMR"].is_object();
		if (exists) {
			parseDMR(j["DMR"]);
			return;
		}

		exists = j["YSF"].is_object();
		if (exists) {
			parseYSF(j["YSF"]);
			return;
		}

		exists = j["P25"].is_object();
		if (exists) {
			parseP25(j["P25"]);
			return;
		}

		exists = j["NXDN"].is_object();
		if (exists) {
			parseNXDN(j["NXDN"]);
			return;
		}

		exists = j["POCSAG"].is_object();
		if (exists) {
			parsePOCSAG(j["POCSAG"]);
			return;
		}

		exists = j["M17"].is_object();
		if (exists) {
			parseM17(j["M17"]);
			return;
		}

		exists = j["FM"].is_object();
		if (exists) {
			parseFM(j["FM"]);
			return;
		}

		exists = j["AX.25"].is_object();
		if (exists) {
			parseAX25(j["AX.25"]);
			return;
		}
	}
	catch (nlohmann::json::parse_error& ex) {
		LogError("Error parsing: \"%s\" at byte %d", text.c_str(), ex.byte);
	}
}

void CDisplayDriver::parseMMDVM(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string mode = json["mode"];

	if (mode == "idle")
		m_display->setIdle();
	else if (mode == "CW")
		m_display->writeCW();
	else if (mode == "lockout") 
		m_display->setLockout();
	else if (mode == "error")
		m_display->setError();
}

void CDisplayDriver::parseRSSI(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string mode = json["mode"];
	int value        = json["value"];

	if (mode == "D-Star") {
		m_display->writeDStarRSSI(value);
	} else if (mode == "DMR") {
		int slot = json["slot"];
		m_display->writeDMRRSSI(slot, value);
	} else if (mode == "YSF") {
		m_display->writeFusionRSSI(value);
	} else if (mode == "P25") {
		m_display->writeP25RSSI(value);
	} else if (mode == "NXDN") {
		m_display->writeNXDNRSSI(value);
	} else if (mode == "M17") {
		m_display->writeM17RSSI(value);
	} else if (mode == "FM") {
		m_display->writeFMRSSI(value);
	}
}

void CDisplayDriver::parseBER(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string mode = json["mode"];
	float value      = json["value"];

	if (mode == "D-Star") {
		m_display->writeDStarBER(value);
	} else if (mode == "DMR") {
		int slot = json["slot"];
		m_display->writeDMRBER(slot, value);
	} else if (mode == "YSF") {
		m_display->writeFusionBER(value);
	} else if (mode == "P25") {
		m_display->writeP25BER(value);
	} else if (mode == "NXDN") {
		m_display->writeNXDNBER(value);
	} else if (mode == "M17") {
		m_display->writeM17BER(value);
	}
}

void CDisplayDriver::parseText(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string mode = json["mode"];
	if (mode == "D-Star") {
		std::string value = json["value"];
		m_display->writeDStarText(value);
	} else if (mode == "DMR") {
		int slot          = json["slot"];
		std::string value = json["value"];
		m_display->writeDMRTA(slot, value);
	} else if (mode == "M17") {
		std::string value = json["value"];
		m_display->writeM17Text(value);
	}
}

void CDisplayDriver::parseDStar(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string action = json["action"];
	if (action == "start" || action == "late_entry") {
		std::string source_cs      = json["source_cs"];
		std::string source_ext     = json["source_ext"];
		std::string destination_cs = json["destination_cs"];
		std::string reflector      = json["reflector"];
		std::string source         = json["source"] == "rf" ? "R" : "N";

		m_display->writeDStar(source_cs, source_ext, destination_cs, source, reflector);
	} else if (action == "end" || action == "lost") {
		m_display->clearDStar();
	}
}

void CDisplayDriver::parseDMR(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string action = json["action"];
	if (action == "start" || action == "late_entry") {
		int slot                     = json["slot"];
		std::string source_info      = json["source_info"];
		int destination_id           = json["destination_id"];
		std::string destination_type = json["destination_type"];
		std::string source           = json["source"] == "rf" ? "R" : "N";

		bool group = destination_type == "group";
		m_display->writeDMR(slot, source_info, group, destination_id, source);
	} else if (action == "end" || action == "lost") {
		int slot = json["slot"];
		m_display->clearDMR(slot);
	}
}

void CDisplayDriver::parseYSF(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string action = json["action"];
	if (action == "start" || action == "late_entry") {
		std::string source_cs = json["source_cs"];
		int dgId              = json["dg-id"];
		std::string reflector = json["reflector"];
		std::string source    = json["source"] == "rf" ? "R" : "N";

		m_display->writeFusion(source_cs, "ALL", dgId, source, reflector);
	} else if (action == "end" || action == "lost") {
		m_display->clearFusion();
	}
}

void CDisplayDriver::parseP25(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string action = json["action"];
	if (action == "start" || action == "late_entry") {
		std::string source_info      = json["source_info"];
		int destination_id           = json["destination_id"];
		std::string destination_type = json["destination_type"];
		std::string source           = json["source"] == "rf" ? "R" : "N";

		bool group = destination_type == "group";
		m_display->writeP25(source_info, group, destination_id, source);
	} else if (action == "end" || action == "lost") {
		m_display->clearP25();
	}
}

void CDisplayDriver::parseNXDN(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string action = json["action"];
	if (action == "start" || action == "late_entry") {
		std::string source_info      = json["source_info"];
		int destination_id           = json["destination_id"];
		std::string destination_type = json["destination_type"];
		std::string source           = json["source"] == "rf" ? "R" : "N";

		bool group = destination_type == "group";
		m_display->writeNXDN(source_info, group, destination_id, source);
	} else if (action == "end" || action == "lost") {
		m_display->clearNXDN();
	}
}

void CDisplayDriver::parsePOCSAG(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string functional = json["functional"];
	if (functional == "end") {
		m_display->clearPOCSAG();
	} else {
		int ric             = json["ric"];
		std::string message = json["message"];
		m_display->writePOCSAG(ric, message);
	}
}

void CDisplayDriver::parseM17(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string action = json["action"];
	if (action == "start" || action == "late_entry") {
		std::string source_cs      = json["source_cs"];
		std::string destination_cs = json["destination_cs"];
		std::string source         = json["source"] == "rf" ? "R" : "N";

		m_display->writeM17(source_cs, destination_cs, source);
	} else if (action == "end" || action == "lost") {
		m_display->clearM17();
	}
}

void CDisplayDriver::parseFM(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string state = json["state"];

	if (state == "listening")
		m_display->clearFM();
	else if (state == "kerchunk_rf")
		m_display->writeFM("R: Kerchunk");
	else if (state == "relaying_rf")
		m_display->writeFM("R: Relaying");
	else if (state == "relaying_wait_rf" || state == "timeout_wait_rf" || state == "hang")
		m_display->writeFM("R: Wait");
	else if (state == "timeout_rf")
		m_display->writeFM("R: Timeout");
	else if (state == "kerchunk_ext")
		m_display->writeFM("N: Kerchunk");
	else if (state == "relaying_ext")
		m_display->writeFM("N: Relaying");
	else if (state == "relaying_wait_ext" || state == "timeout_wait_ext")
		m_display->writeFM("N: Wait");
	else if (state == "timeout_ext")
		m_display->writeFM("N: Timeout");
	else
		m_display->writeFM(state);
}

void CDisplayDriver::parseAX25(const nlohmann::json& json)
{
	assert(m_display != NULL);

	std::string source_cs      = json["source_cs"];
	std::string destination_cs = json["destination_cs"];
	std::string type           = json["type"];
	std::string source         = json["source"] == "rf" ? "R" : "N";

	int rssi = 0;
	if (json["rssi"].is_number_integer())
		rssi = json["rssi"];

	if (json["pid"].is_string()) {
		std::string pid  = json["pid"];
		std::string data = json["data"];

		if (rssi == 0.0F)
			m_display->writeAX25(source, source_cs, destination_cs, type, pid, data);
		else
			m_display->writeAX25(source, source_cs, destination_cs, type, pid, data, rssi);
	} else {
		if (rssi == 0.0F)
			m_display->writeAX25(source, source_cs, destination_cs, type);
		else
			m_display->writeAX25(source, source_cs, destination_cs, type, rssi);
	}
}

void CDisplayDriver::onDisplay(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);
	assert(driver != NULL);

	driver->readDisplay(data, length);
}

void CDisplayDriver::onJSON(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);
	assert(driver != NULL);

	driver->readJSON(std::string((char*)data, length));
}

