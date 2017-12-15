/***************************************************************************
 *   Copyright (C)  2017 by Terraneo Federico, Polidori Paolo              *
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

#ifndef DYNAMICMACROUND_H
#define DYNAMICMACROUND_H

#include "macround.h"
#include "macroundfactory.h"
#include "../flooding/hookingfloodingphase.h"
#include "../roundtrip/listeningroundtripphase.h"
#include "../roundtrip/askingroundtripphase.h"

namespace miosix {
    class DynamicMACRound : public MACRound {
    public:
        DynamicMACRound(const DynamicMACRound& orig) = delete;
        explicit DynamicMACRound(const MediumAccessController& mac, bool debug = true) :
                MACRound(
                    new HookingFloodingPhase(mac, debug),
                    new ListeningRoundtripPhase(
                        mac, getTime() + FloodingPhase::phaseDuration + FloodingPhase::syncNodeWakeupAdvance, debug)),
                debug(debug) {}
        virtual void run(MACContext& ctx) override;
        virtual ~DynamicMACRound();

        class DynamicMACRoundFactory : public MACRoundFactory {
        public:
            DynamicMACRoundFactory() {};
            MACRound* create(MACContext& ctx, bool debug = true) const override;
            virtual ~DynamicMACRoundFactory() {};
        };

    protected:
        DynamicMACRound() {};
    private:
        bool debug;
    };
}

#endif /* DYNAMICMACROUND_H */
