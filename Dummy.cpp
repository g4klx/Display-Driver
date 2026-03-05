/*
 *   Copyright (C) 2026 by Jonathan Naylor G4KLX
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
#include "Dummy.h"
#include "Log.h"


CDummy::CDummy() :
CDisplay()
{
}

CDummy::~CDummy()
{
}

bool CDummy::open()
{
	LogDebug("Dummy: open called");

	return true;
}

void CDummy::setIdleInt()
{
	LogDebug("Dummy: setIdleInt called");
}

void CDummy::setErrorInt()
{
	LogDebug("Dummy: setErrorInt called");
}

void CDummy::setLockoutInt()
{
	LogDebug("Dummy: setLockoutInt called");
}

void CDummy::setQuitInt()
{
	LogDebug("Dummy: setQuitInt called");
}

void CDummy::writeDStarInt(const std::string& my1, const std::string& my2, const std::string& your, const std::string& type, const std::string& reflector)
{
	LogDebug("Dummy: writeDStarInt called, my=%s/%s, your=%s, type=%s, reflector=%s", my1.c_str(), my2.c_str(), your.c_str(), type.c_str(), reflector.c_str());
}

void CDummy::clearDStarInt()
{
	LogDebug("Dummy: clearDStarInt called");
}

void CDummy::writeDMRInt(unsigned int slotNo, const std::string& src, bool group, unsigned int dst, const std::string& type)
{
	LogDebug("Dummy: writeDMRInt called, slotNo=%u, src=%s, group=%s, dst=%u, type=%s", slotNo, src.c_str(), group ? "yes" : "no", dst, type.c_str());
}

void CDummy::clearDMRInt(unsigned int slotNo)
{
	LogDebug("Dummy: clearDMRInt called, slotNo=%u", slotNo);
}

void CDummy::writeFusionInt(const std::string& source, const std::string& dest, unsigned char dgid, const std::string& type, const std::string& origin)
{
	LogDebug("Dummy: writeFusionInt called, source=%s, dest=%s, dgid=%u, type=%s, origin=%s", source.c_str(), dest.c_str(), dgid, type.c_str(), origin.c_str());
}

void CDummy::clearFusionInt()
{
	LogDebug("Dummy: clearFusionInt called");
}

void CDummy::writeP25Int(const std::string& source, bool group, unsigned int dest, const std::string& type)
{
	LogDebug("Dummy: writeP25Int called, source=%s, group=%s, dest=%u, type=%s", source.c_str(), group ? "yes" : "no", dest, type.c_str());
}

void CDummy::clearP25Int()
{
	LogDebug("Dummy: clearP25Int called");
}

void CDummy::writeNXDNInt(const std::string& source, bool group, unsigned int dest, const std::string& type)
{
	LogDebug("Dummy: writeNXDNInt called, source=%s, group=%s, dest=%u, type=%s", source.c_str(), group ? "yes" : "no", dest, type.c_str());
}

void CDummy::clearNXDNInt()
{
	LogDebug("Dummy: clearNXDNInt called");
}

void CDummy::writePOCSAGInt(uint32_t ric, const std::string& message)
{
	LogDebug("Dummy: writePOCSAGInt called, ric=%u, message=\"%s\"", ric, message.c_str());
}

void CDummy::clearPOCSAGInt()
{
	LogDebug("Dummy: clearPOCSAGInt called");
}

void CDummy::writeCWInt()
{
	LogDebug("Dummy: writeCWInt called");
}

void CDummy::clearCWInt()
{
	LogDebug("Dummy: clearCWInt called");
}

void CDummy::writeFMInt(const std::string& state)
{
	LogDebug("Dummy: writeFMInt called, state=\"%s\"", state.c_str());
}

void CDummy::clearFMInt()
{
	LogDebug("Dummy: clearFMInt called");
}

void CDummy::close()
{
	LogDebug("Dummy: close called");
}

void CDummy::clockInt(unsigned int ms)
{
}
