#include <string.h>
#include <algorithm>
#include <inet/networklayer/contract/IL3AddressType.h>
#include <inet/networklayer/common/IpProtocolId_m.h>
#include <inet/applications/common/SocketTag_m.h>
#include <inet/common/INETUtils.h>
#include <inet/common/packet/Message.h>
#include <inet/networklayer/common/EcnTag_m.h>
#include <inet/networklayer/common/IpProtocolId_m.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/common/L3AddressTag_m.h>
#include <inet/networklayer/common/HopLimitTag_m.h>
#include <inet/common/Protocol.h>
#include <inet/common/TimeTag_m.h>

#include "../../application/rdpapp/GenericAppMsgRdp_m.h"
#include "../common/L4ToolsRdp.h"
#include "../contract/rdp/RdpCommand_m.h"
#include "../rdp/rdp_common/RdpHeader.h"
#include "Rdp.h"
#include "RdpAlgorithm.h"
#include "RdpConnection.h"
#include "RdpSendQueue.h"
namespace inet {

namespace rdp {

//
// helper functions
//

const char* RdpConnection::stateName(int state)
{
#define CASE(x)    case x: \
        s = #x + 5; break
    const char *s = "unknown";
    switch (state) {
        CASE(RDP_S_INIT)
;            CASE(RDP_S_CLOSED);
            CASE(RDP_S_LISTEN);
            CASE(RDP_S_ESTABLISHED);
        }
    return s;
#undef CASE
}

const char* RdpConnection::eventName(int event)
{
#define CASE(x)    case x: \
        s = #x + 5; break
    const char *s = "unknown";
    switch (event) {
        CASE(RDP_E_IGNORE)
;            CASE(RDP_E_OPEN_ACTIVE);
            CASE(RDP_E_OPEN_PASSIVE);
        }
    return s;
#undef CASE
}

const char* RdpConnection::indicationName(int code)
{
#define CASE(x)    case x: \
        s = #x + 5; break
    const char *s = "unknown";
    switch (code) {
        CASE(RDP_I_DATA);
        CASE(RDP_I_ESTABLISHED);
        CASE(RDP_I_PEER_CLOSED);

        }
    return s;
#undef CASE
}

void RdpConnection::sendToIP(Packet *packet, const Ptr<RdpHeader> &rdpseg)
{
    EV_TRACE << "RdpConnection::sendToIP" << endl;
    rdpseg->setSrcPort(localPort);
    rdpseg->setDestPort(remotePort);
    //EV_INFO << "Sending: " << endl;
    //printSegmentBrief(packet, rdpseg);
    IL3AddressType *addressType = remoteAddr.getAddressType();
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(addressType->getNetworkProtocol());
    auto addresses = packet->addTagIfAbsent<L3AddressReq>();
    addresses->setSrcAddress(localAddr);
    addresses->setDestAddress(remoteAddr);
    insertTransportProtocolHeader(packet, Protocol::rdp, rdpseg);
    rdpMain->sendFromConn(packet, "ipOut");
}

void RdpConnection::sendToIP(Packet *packet, const Ptr<RdpHeader> &rdpseg, L3Address src, L3Address dest)
{
    //EV_INFO << "Sending: ";
    //printSegmentBrief(packet, rdpseg);
    IL3AddressType *addressType = dest.getAddressType();
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(addressType->getNetworkProtocol());
    auto addresses = packet->addTagIfAbsent<L3AddressReq>();
    addresses->setSrcAddress(src);
    addresses->setDestAddress(dest);

    insertTransportProtocolHeader(packet, Protocol::rdp, rdpseg);
    rdpMain->sendFromConn(packet, "ipOut");
}

void RdpConnection::sendIndicationToApp(int code, const int id)
{
    EV_INFO << "Notifying app: " << indicationName(code) << endl;
    auto indication = new Indication(indicationName(code), code);
    RdpCommand *ind = new RdpCommand();
    ind->setNumRcvTrimmedHeader(state->numRcvTrimmedHeader);
    ind->setUserId(id);
    indication->addTag<SocketInd>()->setSocketId(socketId);
    indication->setControlInfo(ind);
    sendToApp(indication);
}

void RdpConnection::sendEstabIndicationToApp()
{
    EV_INFO << "Notifying app: " << indicationName(RDP_I_ESTABLISHED) << endl;
    auto indication = new Indication(indicationName(RDP_I_ESTABLISHED), RDP_I_ESTABLISHED);
    RdpConnectInfo *ind = new RdpConnectInfo();
    ind->setLocalAddr(localAddr);
    ind->setRemoteAddr(remoteAddr);
    ind->setLocalPort(localPort);
    ind->setRemotePort(remotePort);
    indication->addTag<SocketInd>()->setSocketId(socketId);
    indication->setControlInfo(ind);
    sendToApp(indication);
}

void RdpConnection::sendToApp(cMessage *msg)
{
    rdpMain->sendFromConn(msg, "appOut");
}

void RdpConnection::initConnection(RdpOpenCommand *openCmd)
{
    sendQueue = rdpMain->createSendQueue();
    sendQueue->setConnection(this);
    //create algorithm
    const char *rdpAlgorithmClass = openCmd->getRdpAlgorithmClass();

    if (!rdpAlgorithmClass || !rdpAlgorithmClass[0])
        rdpAlgorithmClass = rdpMain->par("rdpAlgorithmClass");

    rdpAlgorithm = check_and_cast<RdpAlgorithm*>(inet::utils::createOne(rdpAlgorithmClass));
    rdpAlgorithm->setConnection(this);
    // create state block
    state = rdpAlgorithm->getStateVariables();
    configureStateVariables();
    rdpAlgorithm->initialize();
}

void RdpConnection::configureStateVariables()
{
    state->IW = rdpMain->par("initialWindow");
    state->ssthresh = rdpMain->par("ssthresh");
    state->cwnd = state->IW;
    state->slowStartState = true;
    state->slowStartPacketsToSend = 0;
    state->sentPullsInWindow = state->IW;
    state->additiveIncreasePackets = rdpMain->par("additiveIncreasePackets");
    rdpMain->recordScalar("initialWindow=", state->IW);

}

// the receiver sends NACK when receiving a header
void RdpConnection::sendNackRdp(unsigned int nackNum)
{
    EV_INFO << "Sending Nack! NackNum: " << nackNum << endl;
    const auto &rdpseg = makeShared<RdpHeader>();
    rdpseg->setAckBit(false);
    rdpseg->setNackBit(true);
    rdpseg->setNackNo(nackNum);
    rdpseg->setSynBit(false);
    rdpseg->setIsDataPacket(false);
    rdpseg->setIsPullPacket(false);
    rdpseg->setIsHeader(false);
    std::string packetName = "RdpNack-" + std::to_string(nackNum);
    Packet *fp = new Packet(packetName.c_str());
    // send it
    sendToIP(fp, rdpseg);
}

void RdpConnection::sendAckRdp(unsigned int AckNum)
{
    EV_INFO << "Sending Ack! AckNum: " << AckNum << endl;
    const auto &rdpseg = makeShared<RdpHeader>();
    rdpseg->setAckBit(true);
    rdpseg->setAckNo(AckNum);
    rdpseg->setNackBit(false);
    rdpseg->setSynBit(false);
    rdpseg->setIsDataPacket(false);
    rdpseg->setIsPullPacket(false);
    rdpseg->setIsHeader(false);
    std::string packetName = "RdpAck-" + std::to_string(AckNum);
    Packet *fp = new Packet(packetName.c_str());
    // send it
    sendToIP(fp, rdpseg);
}

void RdpConnection::printConnBrief() const
{
    EV_DETAIL << "Connection " << localAddr << ":" << localPort << " to " << remoteAddr << ":" << remotePort << "  on socketId=" << socketId << "  in " << stateName(fsm.getState()) << endl;
}

void RdpConnection::printSegmentBrief(Packet *packet, const Ptr<const RdpHeader> &rdpseg)
{
    EV_STATICCONTEXT
    ;
    EV_INFO << "." << rdpseg->getSrcPort() << " > ";
    EV_INFO << "." << rdpseg->getDestPort() << ": ";

    if (rdpseg->getSynBit())
        EV_INFO << (rdpseg->getAckBit() ? "SYN+ACK " : "SYN ");
    if (rdpseg->getRstBit())
        EV_INFO << (rdpseg->getAckBit() ? "RST+ACK " : "RST ");
    if (rdpseg->getAckBit())
        EV_INFO << "ack " << rdpseg->getAckNo() << " ";
    EV_INFO << endl;
}

uint32 RdpConnection::convertSimtimeToTS(simtime_t simtime)
{
    ASSERT(SimTime::getScaleExp() <= -3);
    uint32 timestamp = (uint32) (simtime.inUnit(SIMTIME_MS));
    return timestamp;
}

simtime_t RdpConnection::convertTSToSimtime(uint32 timestamp)
{
    ASSERT(SimTime::getScaleExp() <= -3);
    simtime_t simtime(timestamp, SIMTIME_MS);
    return simtime;
}

} // namespace rdp

} // namespace inet

