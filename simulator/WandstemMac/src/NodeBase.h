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
#include <algorithm>
#include "miosix_utils_sim.h"

using namespace omnetpp;

/**
 * Utility code that is common to all nodes and can only be implemented
 * by calling cSimpleModule member functions has been factored here.
 * As a consequence, all nodes have to derive from NodeBase and not
 * cSimpleModule.
 * Note that this also selects the coroutine model
 */
class NodeBase : public cSimpleModule
{
public:
    NodeBase();
    virtual ~NodeBase();

    /**
     * Wait for a given simulation time, and deletes all received packets
     * while waiting
     * \param timeDelta time to wait
     */
    void waitAndDeletePackets(simtime_t timeDelta);

    unsigned char getAddress() const { return address; }

protected:
    unsigned char address;
    unsigned short nodes;
    unsigned short hops;
    bool openStream;
    virtual void initialize();

private:
    ///< Stack size for coroutines
    static const int coroutineStack=32*1024;
};

inline int guaranteedTopologies(int maxNumNodes, bool useWeakTopologies)
{
    /*
     * Dynamic Uplink allocation implemented
     * the parameter topologySMERatio configures the fraction of uplink packet
     * reserved for topology messages
     * UplinkMessagePkt   { hop, assignee, numTopology, numSME } : 4
     * NeighborTable      { bitmask }                            : bitmaskSize
     * TopologyElement(s) { nodeId, bitmask } * n                : (1+bitmaskSize) * n
     */
    int bitmaskSize = useWeakTopologies ? maxNumNodes/4 : maxNumNodes/8;
    const float topologySMERatio = 0.5;
    int packetCapacity = (125 - 4 - bitmaskSize) / (1 + bitmaskSize);
    return std::min<int>(packetCapacity * topologySMERatio, maxNumNodes - 2);
}

