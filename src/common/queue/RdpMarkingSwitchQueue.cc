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

#include <inet/common/INETDefs.h>
#include <inet/common/ModuleAccess.h>
#include <inet/common/Simsignals.h>
#include <inet/queueing/function/PacketComparatorFunction.h>
#include <inet/queueing/function/PacketDropperFunction.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/queueing/base/PacketQueueBase.h>

#include "../../application/rdpapp/GenericAppMsgRdp_m.h"
#include "../../transportlayer/rdp/rdp_common/RdpHeader.h"
#include "RdpMarkingSwitchQueue.h"

namespace inet {
namespace queueing {
Define_Module(RdpMarkingSwitchQueue);

void RdpMarkingSwitchQueue::initialize(int stage)
{
    RdpSwitchQueue::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        kthresh = par("kthresh");
    }
}

void RdpMarkingSwitchQueue::pushPacket(Packet *packet, cGate *gate)
{
    Enter_Method("pushPacket");
    take(packet);
    cNamedObject packetPushStartedDetails("atomicOperationStarted");
    emit(packetPushStartedSignal, packet, &packetPushStartedDetails);
    EV_INFO << "PACKET STRING" << packet->str() << endl;
    EV_INFO << "Pushing packet " << packet->getName() << " into the queue." << endl;
    const auto& ipv4Datagram = packet->peekAtFront<Ipv4Header>();
    const auto& rdpHeaderPeek = packet->peekDataAt<rdp::RdpHeader>(ipv4Datagram->getChunkLength());
    if ( rdpHeaderPeek->getAckBit()==true || rdpHeaderPeek->getSynBit()==true || rdpHeaderPeek->getNackBit()==true) {
       synAckQueue.insert(packet);
       //std::cout << "\nPushing packet " << packet->getName() << " into SYN ACK queue." << endl;
       synAckQueueLength=synAckQueue.getLength();
       goto pushEnded;
    }
    else if (rdpHeaderPeek->isHeader() == true ) {
        headersQueue.insert(packet);
        //std::cout << "\nPushing packet " << packet->getName() << " into Headers queue." << endl;
        headersQueueLength = headersQueue.getLength();
        goto pushEnded;
    }
    else if (rdpHeaderPeek->isPullPacket() == true ) {
        headersQueue.insert(packet);
        //std::cout << "\nPushing packet " << packet->getName() << " into the Pull queue." << endl;
        headersQueueLength = headersQueue.getLength();
        goto pushEnded;
    }
    else if (isOverloaded()) {
        std::string header="Header-";
        auto ipv4Header = packet->removeAtFront<Ipv4Header>();
        ASSERT(B(ipv4Header->getTotalLengthField()) >= ipv4Header->getChunkLength());
        if (ipv4Header->getTotalLengthField() < packet->getDataLength())
            packet->setBackOffset(B(ipv4Header->getTotalLengthField()) - ipv4Header->getChunkLength());
        auto rdpHeader = packet->removeAtFront<rdp::RdpHeader>();
        packet->removeAtFront<GenericAppMsgRdp>();
        if (rdpHeader != nullptr) {
            std::string name=packet->getName();
            std::string rename=header+name;
            packet->setName(rename.c_str());
            rdpHeader->setIsHeader(true);
            rdpHeader->setIsDataPacket(false);

            unsigned short srcPort = rdpHeader->getSrcPort();
            unsigned short destPort = rdpHeader->getDestPort();
            EV << "RdpSwitchQueue srcPort:" << srcPort << endl;
            EV << "RdpSwitchQueue destPort:" << destPort << endl;
            EV << "RdpSwitchQueue Header Full Name:" << rdpHeader->getFullName() << endl;
        }
        packet->insertAtFront(rdpHeader);
        ipv4Header->setTotalLengthField(ipv4Header->getChunkLength() + packet->getDataLength());
        packet->insertAtFront(ipv4Header);
        headersQueue.insert(packet);
        headersQueueLength=headersQueue.getLength();
        //emit(packetPushedHeadersQueueSignal, packet);
        ++numTrimmedPkt;
        numTrimmedPacketsVec.record(numTrimmedPkt);
        cSimpleModule::emit(numTrimmedPktSig, numTrimmedPkt);
        goto pushEnded;
    }
    else {
        if(dataQueue.getLength() >= kthresh){
            //std::cout << "\nPacket over threshold" << endl;
            auto ipv4Header = packet->removeAtFront<Ipv4Header>();
            ASSERT(B(ipv4Header->getTotalLengthField()) >= ipv4Header->getChunkLength());
            if (ipv4Header->getTotalLengthField() < packet->getDataLength())
                packet->setBackOffset(B(ipv4Header->getTotalLengthField()) - ipv4Header->getChunkLength());
            auto rdpHeader = packet->removeAtFront<rdp::RdpHeader>();
            rdpHeader->setMarkedBit(true);
            packet->insertAtFront(rdpHeader);
            ipv4Header->setTotalLengthField(ipv4Header->getChunkLength() + packet->getDataLength());
            packet->insertAtFront(ipv4Header);
        }
        dataQueue.insert(packet);
        dataQueueLength=dataQueue.getLength();
        goto pushEnded;
    }
    pushEnded:
        if (collector != nullptr && !isEmpty())
                collector->handleCanPullPacketChanged(outputGate->getPathEndGate());
    cNamedObject packetPushEndedDetails("atomicOperationEnded");
    emit(packetPushEndedSignal, nullptr, &packetPushEndedDetails);
    updateDisplayString();
    return;
}

}
} // namespace inet

