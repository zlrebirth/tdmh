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

#include "NodeBase.h"
#include "DisconnectMessage.h"
#include "interfaces-impl/power_manager.h"
#include "interfaces-impl/transceiver.h"
#include "interfaces-impl/virtual_clock.h"

using namespace std;

NodeBase::NodeBase() : cSimpleModule(coroutineStack) {};

void NodeBase::waitAndDeletePackets(simtime_t timeDelta)
{
    cQueue queue;
    waitAndEnqueue(timeDelta,&queue);
    //TODO: is this efficient? And most important, why can't they use std::list?
    while(!queue.isEmpty())
    {
        auto *message = queue.pop();
        bool isDisconnectMessage = dynamic_cast<DisconnectMessage*>(message) != nullptr;
        delete message;
        if(isDisconnectMessage) throw DisconnectException();
    }
}

NodeBase::~NodeBase() {
    using namespace miosix;
    PowerManager::deinstance(this);
    VirtualClock::deinstance(this);
    Transceiver::deinstance(this);
}

void NodeBase::initialize() {
    address = static_cast<unsigned char>(par("address").intValue());
    nodes = static_cast<unsigned char>(par("nodes").intValue());
    hops = static_cast<unsigned char>(par("hops").intValue());
    openStream = static_cast<unsigned char>(par("open_stream").boolValue());
}
