/*
 *   Copyright (C) 2015,2016,2017,2018,2020,2021,2023,2025 by Jonathan Naylor G4KLX
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

#if !defined(Defines_H)
#define	Defines_H

const unsigned char MODE_IDLE    = 0U;
const unsigned char MODE_DSTAR   = 1U;
const unsigned char MODE_DMR     = 2U;
const unsigned char MODE_YSF     = 3U;
const unsigned char MODE_P25     = 4U;
const unsigned char MODE_NXDN    = 5U;
const unsigned char MODE_POCSAG  = 6U;
const unsigned char MODE_M17     = 7U;

const unsigned char MODE_FM      = 10U;
const unsigned char MODE_AX25    = 11U;

const unsigned char MODE_CW      = 98U;
const unsigned char MODE_LOCKOUT = 99U;
const unsigned char MODE_ERROR   = 100U;
const unsigned char MODE_QUIT    = 110U;

const unsigned char TAG_HEADER = 0x00U;
const unsigned char TAG_DATA   = 0x01U;
const unsigned char TAG_LOST   = 0x02U;
const unsigned char TAG_EOT    = 0x03U;

const unsigned int  DSTAR_MODEM_DATA_LEN = 220U;

enum class HW_TYPE {
	MMDVM,
	DVMEGA,
	MMDVM_ZUMSPOT,
	MMDVM_HS_HAT,
	MMDVM_HS_DUAL_HAT,
	NANO_HOTSPOT,
	NANO_DV,
	D2RG_MMDVM_HS,
	MMDVM_HS,
	OPENGD77_HS,
	SKYBRIDGE,
	UNKNOWN
};

enum class RPT_RF_STATE {
	LISTENING,
	LATE_ENTRY,
	AUDIO,
	DATA_AUDIO,
	DATA,
	REJECTED,
	INVALID
};

enum class RPT_NET_STATE {
	IDLE,
	AUDIO,
	DATA_AUDIO,
	DATA
};

enum class DMR_BEACONS {
	OFF,
	NETWORK,
	TIMED
};

enum class DMR_OVCM_TYPES {
	OFF,
	RX_ON,
	TX_ON,
	ON,
	FORCE_OFF
};

enum class DSTAR_ACK_MESSAGE {
	BER,
	RSSI,
	SMETER
};

#endif
