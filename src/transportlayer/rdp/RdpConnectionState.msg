//
// Copyright (C) 2022 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

import inet.common.INETDefs;
import rdp_common.RdpHeader;

cplusplus {{
#inlude <common/utils/Estimator.h>
}}

class Estimator
{
   @existingClass;
   //FIXME tostring, ...
}


namespace inet::rdp;

cplusplus {{
typedef std::map<unsigned int, simtime_t> PullTimeMap;
}}

class PullTimeMap
{
   @existingClass;
   //FIXME tostring, ...
}

enum RdpState
{
    RDP_S_INIT = 0;
    RDP_S_CLOSED = 1;
    RDP_S_LISTEN = 2; 
    RDP_S_SYN_SENT = 3; 
    RDP_S_SYN_RCVD = 4;
    RDP_S_ESTABLISHED = 5;
};

//
// Event, strictly for the FSM state transition purposes.
// DO NOT USE outside performStateTransition()!
//
enum RdpEventCode
{
    RDP_E_IGNORE = 0;
    RDP_E_OPEN_ACTIVE = 1;
    RDP_E_OPEN_PASSIVE = 2;
    RDP_E_RCV_DATA = 3;
    RDP_E_RCV_SYN = 4;
};

// Contains state variables ("TCB") for RDP.
//
// RdpStateVariables is effectively a "struct" -- it only contains
// public data members. (Only declared as a class so that we can use
// cObject as base class and make it possible to inspect
// it in Tkenv.)
//
// RdpStateVariables only contains variables needed to implement
// the "base" (RFC 793) RDP. More advanced RDP variants are encapsulated
// into RdpAlgorithm subclasses which can have their own state blocks,
// subclassed from RdpStateVariables. See RdpAlgorithm::createStateVariables().
//
struct RdpStateVariables
{
    @implements(cObject);
    @descriptor(readonly);
    
    bool active = false;    // set if the connection was initiated by an active open
    uint32_t delayedNackNo = 0;
    uint32_t request_id = 0;
    uint32_t internal_request_id = 0;
    
    double pacingTime = 0;
    double lastPullTime = 0;
    
    uint32_t IW = 0;  //initial window size
    uint32_t cwnd = 0;
    uint32_t ssthresh = 0;
    uint32_t receivedPacketsInWindow = 0;
    uint32_t sentPullsInWindow = 0;
    uint32_t additiveIncreasePackets = 1;
    int outOfWindowPackets = 0;
    uint32_t numPacketsToGet = 0;
    uint32_t numPacketsToSend = 0;
    
    bool congestionInWindow = false;
    bool slowStartState = true;
    bool waitToStart = false;
    uint32_t numRcvdPkt = 0;
    uint32_t numRcvTrimmedHeader = 0;

    uint32_t numberReceivedPackets = 0;
    uint32_t numberSentPackets = 0;

    bool connNotAddedYet = true;
    bool justReduced = false;

    uint32_t packetsInFlight = 0;

    //RTT - Data
    simtime_t sRtt;
    simtime_t minRtt;
    simtime_t latestRtt;
    simtime_t rttvar;
    //RTT Step - Data
    simtime_t sRttStep;
    simtime_t minRttStep;
    simtime_t rttvarStep;

    //RTT - Header
    simtime_t sRttHeader;
    simtime_t minRttHeader;
    simtime_t latestRttHeader;
    simtime_t rttvarHeader;
    //RTT Step - Header
    simtime_t sRttStepHeader;
    simtime_t minRttStepHeader;
    simtime_t rttvarStepHeader;

    PullTimeMap pullRequestsTransmissionTimes;
    
    uint32_t sentPackets = 0;
    
    uint32_t observeWindEnd = 0; // used by RdpMark flavour.
    uint32_t observeWindSize = 0; // used by RdpMark flavour.

    Estimator bandwidthEstimator;
    Estimator rttPropEstimator;

    bool isfinalReceivedPrintedOut = false;
    
};

cplusplus(RdpStateVariables) {{
  public:
    virtual ~RdpStateVariables();
    virtual std::string str() const override;
    virtual std::string detailedInfo() const;
}}
