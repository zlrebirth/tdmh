//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#pragma once

#include <omnetpp.h>
#include <cstring>
#include <string>
#include <algorithm>

/**
 * Structure of an IEEE 802.15.4 packet
 */
class RadioMessage : public omnetpp::cPacket
{
public:
    /**
     * Constructor
     * \param packet pointer to the packet payload bytes (including CRC if enabled)
     * \param size payload size (including CRC if enabled)
     * \param name packet name (for omnet++ GUI visualization)
     */
    RadioMessage(const void *packet, int size, const std::string& name) : cPacket(name.c_str()), length(size)
    {
        using namespace std;
        if(size>dataSize) throw runtime_error(string("Packet too long ")+to_string(size));
        memcpy(data, packet, length);
        memset(data+length, 0, dataSize-length);
        setByteLength(length);
        setContextPointer(data);
    }

    /**
     * \param size payload size in bytes (including CRC if enabled)
     * \return the time in ns required to send the data part of the packet
     */
    static long long getPSDUDuration(int size)
    {
        return size * 32000;
    }

    /**
     * \param size payload size in bytes (including CRC if enabled)
     * \return the time in ns required to send the packet
     */
    static long long getPPDUDuration(int size)
    {
        return getPSDUDuration(size) + preambleSfdTimeNs + lenTimeNs;
    }

    static const int lenTimeNs = 32000;
    static const int preambleSfdTimeNs = 160000;
    static const int constructiveInterferenceTimeNs = 500;
    static const int dataSize=127;

    //Note: preamble and SFD are not part of the packet because
    //they are constants. When sending packets, the time for
    //sending them is however accounted for.
    //Also the CRC is not simulated, except for considering that
    //when enabled, it occupies two bytes of data
    int length;
    unsigned char data[dataSize];
};
