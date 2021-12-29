
#include "../../rdp/flavours/DumbRdp.h"

#include "../Rdp.h"

namespace inet {

namespace rdp {

Register_Class(DumbRdp);

#define REXMIT_TIMEOUT    2 // Just a dummy value

DumbRdp::DumbRdp() :
        RdpAlgorithm(), state((DumbRdpStateVariables*&) RdpAlgorithm::state)
{
    rexmitTimer = nullptr;
}

DumbRdp::~DumbRdp()
{
    // cancel and delete timers
    if (rexmitTimer)
        delete conn->getRDPMain()->cancelEvent(rexmitTimer);
}

void DumbRdp::initialize()
{
    RdpAlgorithm::initialize();

    rexmitTimer = new cMessage("DumbRdp-REXMIT");
    rexmitTimer->setContextPointer(conn);
}

void DumbRdp::connectionClosed()
{
    conn->getRDPMain()->cancelEvent(rexmitTimer);
}

void DumbRdp::processTimer(cMessage *timer, RdpEventCode &event)
{
    if (timer != rexmitTimer)
        throw cRuntimeError(timer, "unrecognized timer");
    conn->scheduleTimeout(rexmitTimer, REXMIT_TIMEOUT);
}

void DumbRdp::dataSent(uint32 fromseq)
{
    if (rexmitTimer->isScheduled()){
        conn->getRDPMain()->cancelEvent(rexmitTimer);
    }
    conn->scheduleTimeout(rexmitTimer, REXMIT_TIMEOUT);
}

} // namespace RDP

} // namespace inet

