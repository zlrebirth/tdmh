/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AskingRoundtripPhase.h
 * Author: paolo
 *
 * Created on November 30, 2017, 10:12 PM
 */

#ifndef ASKINGROUNDTRIPPHASE_H
#define ASKINGROUNDTRIPPHASE_H

#include "../maccontext.h"
#include "roundtripphase.h"
#include <memory>

namespace miosix {
class AskingRoundtripPhase : public RoundtripPhase {
public:
    AskingRoundtripPhase(const MediumAccessController& mac, long long startTime, bool debug = true) :
            AskingRoundtripPhase(startTime, mac.getPanId(), debug) {};
    AskingRoundtripPhase(long long startTime, short panId, bool debug = true) :
            RoundtripPhase(startTime, panId, debug) {};
    AskingRoundtripPhase() = delete;
    AskingRoundtripPhase(const AskingRoundtripPhase& orig) = delete;
    virtual ~AskingRoundtripPhase();
    void execute(MACContext& ctx) override;
private:

};
}

#endif /* ASKINGROUNDTRIPPHASE_H */
