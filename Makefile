# This makefile is for all platforms, but doesn't include support for the HD44780, OLED, or PCF8574 displays on the
# Raspberry Pi by default.

CC      = cc
CXX     = c++
CFLAGS  = -g -O3 -Wall -std=c++0x -pthread -I/usr/local/include
LIBS    = -lpthread -lutil -lmosquitto
LDFLAGS = -g -L/usr/local/lib

# This makefile is for use with the Raspberry Pi when using an HD44780 compatible display. The wiringpi library is needed.
# Support for the Adafruit i2c 16 x 2 RGB LCD Pi Plate
#CFLAGS  = -g -O3 -Wall -std=c++0x -pthread -DHD44780 -DADAFRUIT_DISPLAY -I/usr/local/include
#LIBS    = -lwiringPi -lwiringPiDev -lpthread -lutil -lmosquitto

# This makefile is for use with the Raspberry Pi when using an HD44780 compatible display. The wiringpi library is needed.
#CFLAGS  = -g -O3 -Wall -std=c++0x -pthread -DHD44780 -I/usr/local/include
#LIBS    = -lwiringPi -lwiringPiDev -lpthread -lutil -lmosquitto

# This makefile is for use with the Raspberry Pi when using an OLED display. The wiringpi library is not needed.
#CFLAGS  = -g -O3 -Wall -std=c++0x -pthread -DOLED -I/usr/local/include
#LIBS    = -lArduiPi_OLED -lpthread -lutil -lmosquitto

# This makefile is for use with the Raspberry Pi when using an HD44780 compatible display. The wiringpi library is needed.
# Support for the HD44780 connected via a PCF8574 8-bit GPIO expander IC
#CFLAGS  = -g -O3 -Wall -std=c++0x -pthread -DHD44780 -DPCF8574_DISPLAY -I/usr/local/include
#LIBS    = -lwiringPi -lwiringPiDev -lpthread -lutil -lmosquitto

OBJECTS =	Conf.o Display.o DisplayDriver.o HD44780.o LCDproc.o Log.o MQTTConnection.o ModemSerialPort.o Mutex.o NetworkInfo.o \
		Nextion.o OLED.o SerialPort.o StopWatch.o TFTSurenoo.o Thread.o Timer.o UARTController.o Utils.o

all:		DisplayDriver

DisplayDriver:	$(OBJECTS) 
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o DisplayDriver

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

DisplayDriver.o: GitVersion.h FORCE

.PHONY: GitVersion.h

FORCE:

install:
		install -m 755 DisplayDriver /usr/local/bin/

clean:
		$(RM) DisplayDriver *.o *.d *.bak *~ GitVersion.h

# Export the current git version if the index file exists, else 000...
GitVersion.h:
ifneq ("$(wildcard .git/index)","")
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@
else
	echo "const char *gitversion = \"0000000000000000000000000000000000000000\";" > $@
endif
