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

#ifndef TRANSPORTLAYER_RDP_FLAVOURS_RdpMarking_H_
#define TRANSPORTLAYER_RDP_FLAVOURS_RdpMarking_H_

#include <inet/common/INETDefs.h>
#include "../RdpAlgorithm.h"

namespace inet {

namespace rdp {

/**
 * State variables for DumbRdp.
 */
class INET_API RdpMarkingStateVariables : public RdpStateVariables
{
  public:
    RdpMarkingStateVariables();
  public:
    double alpha;
    double gamma; //ALLOW ENTRY THROUGH NED
    int numOfMarkedPackets;
};

/**
 * A very-very basic RdpAlgorithm implementation, with hardcoded
 * retransmission timeout and no other sophistication. It can be
 * used to demonstrate what happened if there was no adaptive
 * timeout calculation, delayed acks, silly window avoidance,
 * congestion control, etc.
 */
class INET_API RdpMarking : public RdpAlgorithm
{
  protected:
    RdpMarkingStateVariables *& state;    // alias to TCLAlgorithm's 'state'

    static simsignal_t cwndSignal;    // will record changes to cwnd
    static simsignal_t ssthreshSignal;    // will record changes to ssthresh

  protected:
    /** Creates and returns a DumbRdpStateVariables object. */
    virtual RdpStateVariables *createStateVariables() override
    {
        return new RdpMarkingStateVariables();
    }

  public:
    /** Ctor */
    RdpMarking();

    virtual ~RdpMarking();

    virtual void initialize() override;

    virtual void connectionClosed() override;

    virtual void processTimer(cMessage *timer, RdpEventCode& event) override;

    virtual void dataSent(uint32_t fromseq) override;

    virtual void ackSent() override;

    virtual void receivedHeader(unsigned int seqNum) override;

    virtual void receivedData(unsigned int seqNum, bool isMarked) override;

};

} // namespace RDP

} // namespace inet

#endif // ifndef __INET_RdpMarking_H

