/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   NetworkManager.h
 * Author: paolo
 *
 * Created on November 30, 2017, 9:51 PM
 */

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

namespace miosix {
    class MACRoundFactory;
    class MACContext;
    class MediumAccessController {
    public:
        MediumAccessController() = delete;
        MediumAccessController(const MediumAccessController& orig) = delete;
        static MediumAccessController& instance(const MACRoundFactory *const roundFactory, unsigned short panId, short txPower, unsigned int radioFrequency, bool debug = true);
        unsigned int getRadioFrequency() const { return radioFrequency; }
        short getTxPower() const { return txPower; }
        unsigned short getPanId() const { return panId; }
        void run();
    private:
        MediumAccessController(const MACRoundFactory* const roundFactory, unsigned short panId, short txPower, unsigned int radioFrequency, bool debug);
        virtual ~MediumAccessController();
        unsigned short panId;
        short txPower;
        unsigned int radioFrequency;
        bool debug;
        MACContext* ctx;
    };
}

#endif /* NETWORKMANAGER_H */

