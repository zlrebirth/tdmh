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

#ifndef MASTERRESERVATIONPHASE_H
#define MASTERRESERVATIONPHASE_H

#include "reservationphase.h"
#include <vector>

namespace miosix {
class MasterReservationPhase : public ReservationPhase {
public:
    MasterReservationPhase(long long roundtripEndTime, unsigned char maxHops) : ReservationPhase(roundtripEndTime, 0, maxHops) {}
    MasterReservationPhase() = delete;
    MasterReservationPhase(const MasterReservationPhase& orig) = delete;
    virtual ~MasterReservationPhase();
    void execute(MACContext& ctx) override;
    std::vector<unsigned char>* getUpstreamReservations();
private:

};
}

#endif /* MASTERRESERVATIONPHASE_H */
