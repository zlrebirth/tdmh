/***************************************************************************
 *   Copyright (C)  2018 by Polidori Paolo                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "miosix_utils_sim.h"
#include "NodeBase.h"
#include <string>
#include <cstdarg>
#include <vector>

using namespace omnetpp;

namespace mxnet {

std::string vformat (const char *fmt, va_list ap)
{
    // Allocate a buffer on the stack that's big enough for us almost
    // all the time.
    size_t size = 1024;
    char buf[size];

    // Try to vsnprintf into our buffer.
    va_list apcopy;
    va_copy (apcopy, ap);
    int needed = vsnprintf (&buf[0], size, fmt, ap);
    // NB. On Windows, vsnprintf returns -1 if the string didn't fit the
    // buffer.  On Linux & OSX, it returns the length it would have needed.

    if (needed <= static_cast<int>(size) && needed >= 0) {
        // It fit fine the first time, we're done.
        return std::string (&buf[0]);
    } else {
        // vsnprintf reported that it wanted to write more characters
        // than we allotted.  So do a malloc of the right size and try again.
        // This doesn't happen very often if we chose our initial size
        // well.
        std::vector <char> vlbuf;
        size = needed;
        vlbuf.resize (size);
        needed = vsnprintf (&vlbuf[0], size, fmt, apcopy);
        return std::string (&vlbuf[0]);
    }
}

void print_dbg_(const char *fmt, ...) {
    va_list ap;
    va_start (ap, fmt);
    std::string buf = vformat (fmt, ap);
    va_end (ap);
    EV_INFO << buf;
    printf("%s", buf.c_str());
}
}
namespace miosix {

void ledOn() {
    Leds::redOn = true;
    Leds::print();
}
void ledOff() {
    Leds::redOn = false;
    Leds::print();
}
long long getTime() { return simTime().inUnit(SIMTIME_NS); }

void Thread::nanoSleep(long long delta) {
    getNode()->waitAndDeletePackets(SimTime(delta, SIMTIME_NS));
}
void Thread::nanoSleepUntil(long long when) {
    getNode()->waitAndDeletePackets(SimTime(when, SIMTIME_NS) - simTime());
}

bool Leds::greenOn = false;
bool Leds::redOn = false;

void Leds::print(){
    using namespace std;
    EV_INFO << std::string(Leds::redOn? "\U0001F534": "\U000026AA") << " " << std::string(Leds::greenOn? "\U0001F535": "\U000026AA") << endl;
}

void greenLed::high() {
    Leds::greenOn = true;
    Leds::print();
}

void greenLed::low() {
    Leds::greenOn = false;
    Leds::print();
}

static void memPrint(const char *data, char len)
{
    printf("0x%08x | ",reinterpret_cast<const unsigned char*>(data)[0]);
    for(int i=0;i<len;i++) printf("%02x ",static_cast<unsigned char>(data[i]));
    for(int i=0;i<(16-len);i++) printf("   ");
    printf("| ");
    for(int i=0;i<len;i++)
    {
        if((data[i]>=32)&&(data[i]<127)) printf("%c",data[i]);
        else printf(".");
    }
    printf("\n");
}

void memDump(const void *start, int len)
{
    const char *data=reinterpret_cast<const char*>(start);
    while(len>16)
    {
        memPrint(data,16);
        len-=16;
        data+=16;
    }
    if(len>0) memPrint(data,len);
}

}
