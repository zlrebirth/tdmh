/***************************************************************************
 *   Copyright (C)  2019 by Polidori Paolo, Federico Amedeo Izzo,          *
 *                          Federico Terraneo                              *
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

#pragma once

#include "../mac_phase.h"
#include "../mac_context.h"
#include "../timesync/networktime.h"
#include "../stream_manager.h"

namespace mxnet {
class UplinkPhase : public MACPhase {
public:
    UplinkPhase() = delete;

    UplinkPhase(const UplinkPhase& orig) = delete;

    virtual ~UplinkPhase() {};

    //TODO: check the duration calculation, it is currently hardcoded
    static unsigned long long getDuration(unsigned char numUplinkPackets) {
        return (packetArrivalAndProcessingTime + transmissionInterval) * numUplinkPackets;
    }

    /**
     * Align uplink phase to the network time
     */
    void alignToNetworkTime(NetworkTime nt);

    void advance(long long slotStart) override;

    static const int transmissionInterval = 1000000; //1ms
    static const int packetArrivalAndProcessingTime = 5000000;//32 us * 127 B + tp = 5ms
    static const int packetTime = 4256000;//physical time for transmitting/receiving the packet: 4256us
protected:
    UplinkPhase(MACContext& ctx, StreamManager* const streamMgr) :
            MACPhase(ctx),
            streamMgr(streamMgr),
            myId(ctx.getNetworkId()),
            nodesCount(ctx.getNetworkConfig().getMaxNodes()),
            nextNode(nodesCount - 1) {};
    
    /**
     * updated the internal status of the class,
     * with the address of the node to listen
     */
    unsigned char getAndUpdateCurrentNode() {
        auto currentNode = nextNode;
        if (--nextNode <= 0)
            nextNode = nodesCount - 1;
        return currentNode;
    }
    /* Pointer to StreamManager class, used to get SMEs */
    StreamManager* const streamMgr;

    /* Cached NetworkId of this node */
    unsigned char myId;
    /* Cached NetworkConfiguration::getMaxNodes() */
    unsigned char nodesCount;
    /* Next node to talk in the round-robin */
    unsigned char nextNode;
};

}
