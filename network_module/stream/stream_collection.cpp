/***************************************************************************
 *   Copyright (C) 2019 by Federico Amedeo Izzo                            *
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

#include "stream_collection.h"
#include "../util/debug_settings.h"
#include <algorithm>
#include <set>

namespace mxnet {

std::vector<MasterStreamInfo> StreamSnapshot::getStreams() const {
    std::vector<MasterStreamInfo> result;
    for(auto& stream : collection)
        result.push_back(stream.second);
    return result;
}

std::vector<MasterStreamInfo> StreamSnapshot::getStreamsWithStatus(MasterStreamStatus s) const {
    std::vector<MasterStreamInfo> result;
    for (auto& stream: collection) {
        if(stream.second.getStatus() == s)
            result.push_back(stream.second);
    }
    return result;
}

std::map<StreamId, StreamChange> StreamSnapshot::getStreamChanges(const std::list<ScheduleElement>& schedule) const {
/* NOTE: we need to compare the schedule with the streams in this StreamSnapshot
   to precompute 3 types of changes to apply to the StreamCollection:
   - ESTABLISH: For ACCEPTED streams in snapshot, present in new schedule 
   - REJECT: For ACCEPTED streams in snapshot, missing from new schedule 
   - CLOSE: For ESTABLISHED streams in snapshot, missing from new schedule */

    std::map<StreamId, StreamChange> result;

    // Create a copy of the collection keys,
    // to get the streams not present in schedule
    std::set<StreamId> streamsNotInSchedule;
    for(auto& pair : collection) {
        // Ignore servers, they are not affected by schedule
        if(pair.first.isServer())
            continue;
        streamsNotInSchedule.insert(pair.first);
    }
    // Cycle over schedule
    for(auto& el : schedule) {
        auto id = el.getStreamId();
        // Search stream in collection
        auto it = collection.find(id); 
        // If stream is present in collection
        if(it != collection.end()) {
            auto& streamInfo = it->second;
            if(streamInfo.getStatus() == MasterStreamStatus::ACCEPTED)
                result[id] = StreamChange::ESTABLISH;
            // If stream is ESTABLISHED and present in schedule, no action needed
            // Remove id from streamsNotInSchedule
            streamsNotInSchedule.erase(id);
        }
        // If stream is not present in collection, do nothing
    }
    // Cycle over streams not present in schedule
    for(auto& id : streamsNotInSchedule) {
        // Search stream in collection
        auto it = collection.find(id); 
        // If stream is present in collection
        if(it != collection.end()) {
            auto& streamInfo = it->second;
            if(streamInfo.getStatus() == MasterStreamStatus::ACCEPTED) {
                result[id] = StreamChange::REJECT;
            }
            else if(streamInfo.getStatus() == MasterStreamStatus::ESTABLISHED) {
                result[id] = StreamChange::CLOSE;
            }
        }
        // If stream is not present in collection, do nothing
    }
    return result;
}

void StreamCollection::receiveSMEs(UpdatableQueue<SMEKey,
                                   StreamManagementElement>& smes) {
#ifdef _MIOSIX
    miosix::Lock<miosix::Mutex> lck(coll_mutex);
#else
    std::unique_lock<std::mutex> lck(coll_mutex);
#endif
    auto numSME = smes.size();
    for(unsigned int i=0; i < numSME; i++) {
        auto sme = smes.dequeue();
        
#ifdef WITH_SME_SEQNO
        print_dbg("[SC] SME from (%d,%d,%d,%d) %s seq=%d\n",
                  sme.getSrc(),sme.getDst(),sme.getSrcPort(),sme.getDstPort(),
                  smeTypeToString(sme.getType()),sme.getSeqNo());
#endif //WITH_SME_SEQNO
        
        if(sme.getType() == SMEType::RESEND_SCHEDULE)
        {
            resend_flag = true;
            if(SCHEDULER_SUMMARY_DBG)
                print_dbg("[SC] schedule resend due to RESEND_SCHEDULE sme from (%d)\n",
                            sme.getSrc());
            continue;
        }
        
        StreamId id = sme.getStreamId();
        auto it = collection.find(id);
        // If stream/server is present in collection
        if(it != collection.end()) {
            auto& stream = it->second;
            // SME belongs to stream
            if(id.isStream())
                updateStream(stream, sme);
            // SME belongs to server
            else
                updateServer(stream, sme);
        }
        // If stream/server is not present in collection
        else {
            // SME belongs to stream
            if(id.isStream())
                createStream(sme);
            // SME belongs to server
            else
                createServer(sme);
        }
    }
}

void StreamCollection::applyChanges(const std::map<StreamId, StreamChange>& changes) {
#ifdef _MIOSIX
    miosix::Lock<miosix::Mutex> lck(coll_mutex);
#else
    std::unique_lock<std::mutex> lck(coll_mutex);
#endif
    // Cycle over changes
    for(auto& change : changes) {
        StreamId id = change.first;
        // Ignore servers, they are not affected by schedule
        if(id.isServer())
            continue;
        auto it = collection.find(id);
        // If stream is present in collection
        if(it != collection.end()) {
            auto& stream = it->second;
            switch(change.second) {
                case StreamChange::ESTABLISH:
                    if(stream.getStatus() == MasterStreamStatus::ACCEPTED)
                        stream.setStatus(MasterStreamStatus::ESTABLISHED);
                    break;
                case StreamChange::REJECT:
                    if(stream.getStatus() == MasterStreamStatus::ACCEPTED) {
                        collection.erase(id);
                        enqueueInfo(id, InfoType::STREAM_REJECT);
                    }
                    break;
                case StreamChange::CLOSE:
                    if(stream.getStatus() == MasterStreamStatus::ESTABLISHED)
                        collection.erase(id);
                    break;
            }
        }
        // If stream is not present in collection do nothing
    }
}

std::vector<MasterStreamInfo> StreamCollection::getStreams() {
#ifdef _MIOSIX
    miosix::Lock<miosix::Mutex> lck(coll_mutex);
#else
    std::unique_lock<std::mutex> lck(coll_mutex);
#endif
    std::vector<MasterStreamInfo> result;
    for(auto& stream : collection)
        result.push_back(stream.second);
    return result;
}


std::vector<InfoElement> StreamCollection::dequeueInfo(unsigned int num) {
#ifdef _MIOSIX
    miosix::Lock<miosix::Mutex> lck(coll_mutex);
#else
    std::unique_lock<std::mutex> lck(coll_mutex);
#endif
    std::vector<InfoElement> result;
    unsigned int i=0;
    while(i<num && !infoQueue.empty()) {
        result.push_back(infoQueue.dequeue());
        i++;
    }
    return result;
}

void StreamCollection::enqueueInfo(StreamId id, InfoType type) {
    InfoElement info(id, type);
    // Add to sending queue
    infoQueue.enqueue(id, info);
}

void StreamCollection::updateStream(MasterStreamInfo& stream, StreamManagementElement& sme) {
    StreamId id = sme.getStreamId();
    SMEType type = sme.getType();
    if(type == SMEType::LISTEN)
    {
        print_dbg("[SC] BUG! LISTEN sme for non-server stream (%d,%d)\n",id.src,id.srcPort);
        return;
    }
    auto status = stream.getStatus();
    switch(status)
    {
        case MasterStreamStatus::ACCEPTED:
            if(type == SMEType::CLOSED)
            {
                collection.erase(id);
                removed_flag = true;
                modified_flag = true;
            }
            //No action if SMEType::CONNECT
            break;
        case MasterStreamStatus::ESTABLISHED:
            switch(type)
            {
                case SMEType::CLOSED:
                    collection.erase(id);
                    removed_flag = true;
                    modified_flag = true;
                    break;
                case SMEType::CONNECT:
                    resend_flag = true;
                    if(SCHEDULER_SUMMARY_DBG)
                        print_dbg("[SC] schedule resend due to CONNECT while ESTABLISHED (%d,%d,%d,%d)\n",
                                  id.src,id.dst,id.srcPort,id.dstPort);
                    break;
                default:
                    break;
            }
            break;
        case MasterStreamStatus::LISTEN:
            print_dbg("[SC] BUG! LISTEN state for non-server stream (%d,%d)\n",id.src,id.srcPort);
            break;
    }
}

void StreamCollection::updateServer(MasterStreamInfo& server, StreamManagementElement& sme) {
    StreamId id = sme.getStreamId();
    SMEType type = sme.getType();
    auto status = server.getStatus();
    if(status == MasterStreamStatus::LISTEN) {
        if(type == SMEType::CLOSED) {
            if(SCHEDULER_SUMMARY_DBG)
                print_dbg("[SC] Server (%d,%d,%d,%d) Closed\n", id.src,id.dst,id.srcPort,id.dstPort);
            // Delete server because it has been closed by remote node
            collection.erase(id);
            // Enqueue SERVER_CLOSED info element
            enqueueInfo(id, InfoType::SERVER_CLOSED);
        }
        else if(type == SMEType::LISTEN) {
            // Enqueue again SERVER_OPENED info element
            enqueueInfo(id, InfoType::SERVER_OPENED);
        }
    }
}

void StreamCollection::createStream(StreamManagementElement& sme) {
    StreamId id = sme.getStreamId();
    SMEType type = sme.getType();
    if(type == SMEType::LISTEN)
    {
        print_dbg("[SC] BUG! LISTEN sme for non-server stream (%d,%d)\n",id.src,id.srcPort);
    } else if(type == SMEType::CLOSED) {
        resend_flag = true;
        if(SCHEDULER_SUMMARY_DBG)
            print_dbg("[SC] schedule resend due to CLOSED while NOEXIST (%d,%d,%d,%d)\n",
                        id.src,id.dst,id.srcPort,id.dstPort);
    } else if(type == SMEType::CONNECT) {
        // Check for corresponding Server
        auto serverId = id.getServerId();
        // Server present (can only be in LISTEN status)
        auto serverit = collection.find(serverId);
        if(serverit != collection.end()) {
            auto clientParams = sme.getParams();
            auto server = serverit->second;
            auto serverParams = server.getParams();
            // If the direction of client and server don't match, reject stream
            if(serverParams.direction != clientParams.direction) {
                if(SCHEDULER_SUMMARY_DBG)
                    print_dbg("[SC] Stream (%d,%d,%d,%d) Rejected: parameters don't match\n", id.src,id.dst,id.srcPort,id.dstPort);
                
                enqueueInfo(id, InfoType::STREAM_REJECT);
            } else {
                // Otherwise create new stream
                if(SCHEDULER_SUMMARY_DBG)
                    print_dbg("[SC] Stream (%d,%d,%d,%d) Accepted\n", id.src,id.dst,id.srcPort,id.dstPort);
                // Negotiate parameters between client and servers
                StreamParameters newParams = negotiateParameters(serverParams, clientParams);
                // Create ACCEPTED stream with new parameters
                collection[id] = MasterStreamInfo(id, newParams, MasterStreamStatus::ACCEPTED);
                // Set flags
                added_flag = true;
                modified_flag = true;
            } 
        } else {
            // Server absent
            if(SCHEDULER_SUMMARY_DBG)
                print_dbg("[SC] Stream (%d,%d,%d,%d) Rejected: server missing\n", id.src,id.dst,id.srcPort,id.dstPort);
            
            enqueueInfo(id, InfoType::STREAM_REJECT);
        }
    }
}

void StreamCollection::createServer(StreamManagementElement& sme) {
    StreamId id = sme.getStreamId();
    SMEType type = sme.getType();
    StreamParameters params = sme.getParams();
    if(type == SMEType::LISTEN) {
        if(SCHEDULER_SUMMARY_DBG)
            print_dbg("[SC] Server (%d,%d,%d,%d) Accepted\n", id.src,id.dst,id.srcPort,id.dstPort);
        // Create server
        collection[id] = MasterStreamInfo(id, params, MasterStreamStatus::LISTEN);
        // Enqueue SERVER_OPENED info element
        enqueueInfo(id, InfoType::SERVER_OPENED);
    }
    if(type == SMEType::CLOSED) {
        // Enqueue again SERVER_CLOSED info element
        enqueueInfo(id, InfoType::SERVER_CLOSED);
    }
}

StreamParameters StreamCollection::negotiateParameters(StreamParameters& serverParams,
                                                       StreamParameters& clientParams) {
    /* NOTE: during the negotiation we compare the "unsigned int" parameters
     * only converting the resulting parameters, this to avoid double conversions. */
    // Pick the lowest redundancy level between Client and Server
    Redundancy redundancy = static_cast<Redundancy>(std::min(serverParams.redundancy,
                                                             clientParams.redundancy));
    // Pick the highest period between Client and Server
    Period period = static_cast<Period>(std::max(serverParams.period,
                                                 clientParams.period));
    // Pick the lowest payloadSize between Client and Server
    unsigned short payloadSize = std::min(serverParams.payloadSize,
                                          clientParams.payloadSize);
    // We assume that direction is the same between Client and Server,
    // Because we checked it in createStream(). So just copy it from one of them
    Direction direction = clientParams.getDirection();
    // Create resulting StreamParameters struct
    StreamParameters newParams(redundancy, period, payloadSize, direction);
    return newParams;
}

} // namespace mxnet
