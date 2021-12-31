#include <string.h>

#include <inet/applications/common/SocketTag_m.h>
#include <inet/common/TimeTag_m.h>
#include "../contract/rdp/RdpCommand_m.h"
#include "../../application/rdpapp/GenericAppMsgRdp_m.h"
#include "../rdp/rdp_common/RdpHeader.h"
#include "Rdp.h"
#include "RdpAlgorithm.h"
#include "RdpConnection.h"
#include "RdpSendQueue.h"

namespace inet {
namespace rdp {

void RdpConnection::sendInitialWindow()
{

    // TODO  we don't do any checking about the received request segment, e.g. check if it's  a request nothing else
    // fetch the next Packet from the encodingPackets list
    EV_TRACE << "RdpConnection::sendInitialWindow";
    std::list<PacketsToSend>::iterator itt;
    if (state->IW > state->numPacketsToSend) {
        state->IW = state->numPacketsToSend;
    }
    for (int i = 1; i <= state->IW; i++) {
        std::tuple<Ptr<RdpHeader>, Packet*> packSeg = sendQueue->getRdpHeader();
        auto rdpseg = std::get<0>(packSeg);
        auto fp = std::get<1>(packSeg);
        if (rdpseg) {
            EV_INFO << "Sending IW packet " << rdpseg->getDataSequenceNumber() << endl;
            rdpseg->setIsDataPacket(true);
            rdpseg->setIsPullPacket(false);
            rdpseg->setIsHeader(false);
            rdpseg->setSynBit(true);
            rdpseg->setAckBit(false);
            rdpseg->setNackBit(false);
            rdpseg->setNumPacketsToSend(state->numPacketsToSend);
            sendToIP(fp, rdpseg);
        }
    }
}

RdpEventCode RdpConnection::process_RCV_SEGMENT(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address src, L3Address dest)
{
    EV_TRACE << "RdpConnection::process_RCV_SEGMENT" << endl;
    //EV_INFO << "Seg arrived: ";
    //printSegmentBrief(packet, rdpseg);
    EV_DETAIL << "TCB: " << state->str() << "\n";
    RdpEventCode event;
    if (fsm.getState() == RDP_S_LISTEN) {
        EV_INFO << "RDP_S_LISTEN processing the segment in listen state" << endl;
        event = processSegmentInListen(packet, rdpseg, src, dest);
        if (event == RDP_E_RCV_SYN) {
            EV_INFO << "RDP_E_RCV_SYN received syn. Changing state to Established" << endl;
            FSM_Goto(fsm, RDP_S_ESTABLISHED);
            EV_INFO << "Processing Segment" << endl;
            event = processSegment1stThru8th(packet, rdpseg);
        }
    }
    else {
        rdpMain->updateSockPair(this, dest, src, rdpseg->getDestPort(), rdpseg->getSrcPort());
        event = processSegment1stThru8th(packet, rdpseg);
    }
    delete packet;
    return event;
}

RdpEventCode RdpConnection::processSegment1stThru8th(Packet *packet, const Ptr<const RdpHeader> &rdpseg)
{
    EV_TRACE << "RdpConnection::processSegment1stThru8th" << endl;
    EV_INFO << "_________________________________________" << endl;
    RdpEventCode event = RDP_E_IGNORE;
    // (S.1)   at the sender: NACK Arrived at the sender, then prepare the trimmed pkt for retranmission
    //        (not to transmit yet just make it to be the first one to transmit upon getting a pull pkt later)
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    // ££££££££££££££££££££££££ NACK Arrived at the sender £££££££££££££££££££ Tx
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    ASSERT(fsm.getState() == RDP_S_ESTABLISHED);
    if (rdpseg->getNackBit() == true) {
        EV_INFO << "Nack arrived at the sender - move data packet to front" << endl;
        sendQueue->nackArrived(rdpseg->getNackNo());
    }

    // (S.2)  at the sender:  ACK arrived, so free the acknowledged pkt from the buffer.
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    // ££££££££££££££££££££££££ ACK Arrived at the sender £££££££££££££££££££ Tx
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    if (rdpseg->getAckBit() == true) {
        EV_INFO << "Ack arrived at the sender - free ack buffer" << endl;
        sendQueue->ackArrived(rdpseg->getAckNo());
    }

    // (S.3)  at the sender: PULL pkt arrived, this pkt triggers either retransmission of trimmed pkt or sending a new data pkt.
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    // ££££££££££££££££££££££££ REQUEST Arrived at the sender £££££££££££££££
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    if (rdpseg->isPullPacket() == true || ((rdpseg->getNackBit() == true) && (state->delayedNackNo > 0))) {
        int requestsGap = rdpseg->getPullSequenceNumber() - state->internal_request_id;
        EV_INFO << "Pull packet arrived at the sender - request gap " << requestsGap << endl;
        if(state->delayedNackNo > 0){
            requestsGap = 1;
            --state->delayedNackNo;
        }
        if (requestsGap >= 1) {
            //  we send Packets  based on requestsGap value
            // if the requestsGap is smaller than 1 that means we received a delayed request which we need to  ignore
            // as we have assumed it was lost and we send extra Packets previously
            for (int i = 1; i <= requestsGap; i++) {
                ++state->internal_request_id;
                std::tuple<Ptr<RdpHeader>, Packet*> headPack = sendQueue->getRdpHeader();
                const auto &rdpseg = std::get<0>(headPack);
                Packet *fp = std::get<1>(headPack);
                if (rdpseg) {
                    EV_INFO << "Sending data packet - " << rdpseg->getDataSequenceNumber() << endl;
                    rdpseg->setIsDataPacket(true);
                    rdpseg->setIsPullPacket(false);
                    rdpseg->setIsHeader(false);
                    rdpseg->setAckBit(false);
                    rdpseg->setNackBit(false);
                    rdpseg->setSynBit(false);
                    rdpseg->setNumPacketsToSend(state->numPacketsToSend);
                    sendToIP(fp, rdpseg);
                }
                else {
                    EV_WARN << "No Rdp header within the send queue!" << endl;
                    ++state->delayedNackNo;
                    --state->internal_request_id;
                    //EV_INFO << "No Rdp header within the send queue!" << endl;
                    //--state->internal_request_id;
                    //state->internal_request_id = state->internal_request_id - requestsGap;
                    //--state->request_id;

                }
            }
        }
        else if (requestsGap < 1) {
            EV_INFO << "Delayed pull arrived --> ignore it" << endl;
        }
    }
    // (R.1)  at the receiver
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$  HEADER arrived   $$$$$$$$$$$$$$$$   Rx
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // header arrived at the receiver==> send new request with pacing (fixed pacing: MTU/1Gbps)
    if (rdpseg->isHeader() == true && rdpseg->isDataPacket() == false) { // 1 read, 2 write
        EV_INFO << "Header arrived at the receiver" << endl;
        sendNackRdp(rdpseg->getDataSequenceNumber());
        //state->receivedPacketsInWindow++;
        state->numRcvTrimmedHeader++;
        //state->sentPullsInWindow--;
        bool noPacketsInFlight = false;
        if(state->outOfWindowPackets > 0){
            state->outOfWindowPackets--;
            if(state->outOfWindowPackets <= 0){
                noPacketsInFlight = true;
            }
        }else{
            state->sentPullsInWindow--;              //SEND PULLS FOR ALL CWND IF TWO HEADERS IN A ROW (FOR LOOP)
            state->receivedPacketsInWindow++;
        }
        if(!state->congestionInWindow && state->outOfWindowPackets <= 0 && !noPacketsInFlight){
            state->congestionInWindow = true;  //may have to change so certain amount of headers before multiplicative decrease
            //state->ssthresh = state->cwnd/2;
            state->ssthresh = 0;
            //if(state->ssthresh == 0) state->ssthresh = 1;
            state->cwnd = state->cwnd/2;
            if(state->cwnd == 0) state->cwnd = 1;
            emit(cwndSignal, state->cwnd);
            //if(state->receivedPacketsInWindow >= state->cwnd){
                state->outOfWindowPackets = state->sentPullsInWindow;// - state->cwnd;
                state->receivedPacketsInWindow = 0;
                state->sentPullsInWindow = 0;//state->cwnd;
                state->waitToStart = true;
            //}
        }
//        if(state->outOfWindowPackets <= 0){
//            state->congestionInWindow = false;
//            bool firstPull = true;
//            for(int i = 0; i < state->cwnd; i++){
//                if(firstPull){
//                    addRequestToPullsQueue(true);
//                    firstPull = false;
//                }
//                else{
//                    addRequestToPullsQueue(false);
//                }
//
//            }
//        }
        //else{
        //   addRequestToPullsQueue(true);
        //}

        if (state->numberReceivedPackets == 0 && state->connNotAddedYet == true) {
            getRDPMain()->requestCONNMap[getRDPMain()->connIndex] = this; // moh added
            state->connNotAddedYet = false;
            ++getRDPMain()->connIndex;
            EV_INFO << "sending first request" << endl;
            getRDPMain()->sendFirstRequest();
        }
        bool firstPull = true;
        if(noPacketsInFlight || state->outOfWindowPackets == 0){
            state->congestionInWindow = false;
            for(int i = 0; i < state->cwnd; i++){
                if(firstPull){
                    addRequestToPullsQueue(true, true);
                    firstPull = false;
                }
                else{
                    addRequestToPullsQueue(false, true);
                }

            }
            noPacketsInFlight = false;

        }
//        if(firstPull == false){
//            addRequestToPullsQueue(false);
//        }
//        else{
//           addRequestToPullsQueue(true);
//        }

    }
    // (R.2) at the receiver
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$  data pkt arrived at the receiver  $$$$$$$$$$$$$$$$
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    if (rdpseg->isDataPacket() == true && rdpseg->isHeader() == false) {
        bool windowIncreased = false;
        bool noPacketsInFlight = false;
        if(state->outOfWindowPackets > 0){
            state->outOfWindowPackets--;
            if(state->outOfWindowPackets <= 0){
                noPacketsInFlight = true;
            }
        }
        else{
            state->receivedPacketsInWindow++;
            state->sentPullsInWindow--;
        }
        EV_INFO << "Data packet arrived at the receiver - seq num " << rdpseg->getDataSequenceNumber() << endl;
        unsigned int arrivedPktSeqNo = rdpseg->getDataSequenceNumber();
        sendAckRdp(arrivedPktSeqNo);
        unsigned int seqNo = rdpseg->getDataSequenceNumber();
        ++state->numberReceivedPackets;
        //state->receivedPacketsInWindow++;
        //state->sentPullsInWindow--;
        bool onePullNeeded = false;
        if(state->sendPulls && state->outOfWindowPackets <= 0 && !noPacketsInFlight){
            int packetsNeeded = state->numPacketsToGet;
            //if(state->numRcvTrimmedHeader > 0){
            //    packetsNeeded = packetsNeeded + 2;
            //}
//            if(state->cwnd < state->ssthresh) {
//                //state->cwnd + state->additiveIncreasePackets;
//                state->slowStartState = true;
//                state->slowStartPacketsToSend++;
//                //state->cwnd++;
//                //emit(cwndSignal, state->cwnd);
//                //for(int i = 0; i < state->additiveIncreasePackets; i++){
//                //    addRequestToPullsQueue(false);
//                //}
//            }
            //else{
                state->slowStartState = false;
            //}
            if(state->numberReceivedPackets + state->sentPullsInWindow + 1 >= packetsNeeded){
                onePullNeeded = true;
            }
            else if(state->numberReceivedPackets + state->sentPullsInWindow + 2 >= packetsNeeded){
                windowIncreased = true;
                addRequestToPullsQueue(true, false);
                onePullNeeded = true;
                state->sendPulls = false;
                goto jmp;
            }
            else if(state->cwnd < state->ssthresh) {
                windowIncreased = true;
                //state->cwnd + state->additiveIncreasePackets;
                //state->slowStartState = true;
                //state->slowStartPacketsToSend++;
                state->cwnd++;
                emit(cwndSignal, state->cwnd);
                //for(int i = 0; i < state->additiveIncreasePackets; i++){
                addRequestToPullsQueue(true, false);
                //}
                goto jmp;
            }
            else if((state->receivedPacketsInWindow % state->cwnd) == 0){ //maybe do first?
                //if(!state->slowStartState){
                    windowIncreased = true;
                    state->cwnd = state->cwnd + state->additiveIncreasePackets;
                    emit(cwndSignal, state->cwnd);
                    state->receivedPacketsInWindow = 0;
                    state->congestionInWindow = false;
                    bool firstPull = true;
                    for(int i = 0; i < state->additiveIncreasePackets; i++){
                        if(firstPull){
                            addRequestToPullsQueue(true, true);
                            firstPull = false;
                        }
                        else{
                            addRequestToPullsQueue(false, true);
                        }

                    }
                    goto jmp;
                //}
//                else{
//                    state->receivedPacketsInWindow = 0;
//                    state->congestionInWindow = false;
//                    for(int i = 0; i < state->slowStartPacketsToSend; i++){
//                        state->cwnd++;
//                        emit(cwndSignal, state->cwnd);
//                        addRequestToPullsQueue(false);
//                    }
//                    state->slowStartPacketsToSend = 0;
//                }
            }
        }
        jmp:
        int numberReceivedPackets = state->numberReceivedPackets;
        int initialSentPackets = state->IW;
        int wantedPackets = state->numPacketsToGet;
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        // %%%%%%%%%%%%%%%%%%%%%%%% 1   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        if (numberReceivedPackets > wantedPackets) {
            EV_INFO << "All packets received - finish all connections!" << endl;
            state->connFinished = true;
            getRDPMain()->allConnFinished();
            goto ll;
        }
        else {
            if(state->sendPulls && !noPacketsInFlight && state->outOfWindowPackets <= 0 && state->sentPullsInWindow < state->cwnd || onePullNeeded){
                if (numberReceivedPackets <= (wantedPackets) && state->connFinished == false) {
                    EV_INFO << "Adding pull request to pull queue!" << endl;
                    if(windowIncreased || !onePullNeeded){
                        addRequestToPullsQueue(true, false);
                    }
                    else{
                        addRequestToPullsQueue(true, false);
                    }

                }
                else{
                    std::cout << "Found!" << endl;
                }
            }
            if (numberReceivedPackets == 1 && state->connNotAddedYet == true) {
                getRDPMain()->requestCONNMap[getRDPMain()->connIndex] = this; // moh added
                ++getRDPMain()->connIndex;
                state->connNotAddedYet = false;
                EV << "Requesting Pull Timer" << endl;
                getRDPMain()->sendFirstRequest();
            }
            // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
            // %%%%%%%%%%%%%%%%%%%%%%%%%  4  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
            // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
            //  send any received Packet to the app
            auto tag = rdpseg->getTag(0);
            const CreationTimeTag *timeTag = dynamic_cast<const CreationTimeTag *>(tag);
            //packet->setFrontOffset(B(0));
            //packet->setBackOffset(B(1500));
            Ptr<Chunk> msgRx;
            msgRx = packet->removeAll();
            if (state->connFinished == false) {
                EV_INFO << "Sending Data Packet to Application" << endl;
                // buffer the received Packet segment
                std::list<PacketsToSend>::iterator itR;  // received iterator
                itR = receivedPacketsList.begin();
                std::advance(itR, seqNo); // increment the iterator by esi
                // MOH: Send any received Packet to the app, just for now to test the Incast example, this shouldn't be the normal case
                //Packet *newPacket = packet->dup();
                std::string packetName = "DATAPKT-" + std::to_string(seqNo);
                Packet *newPacket = new Packet(packetName.c_str(), msgRx);
                newPacket->addTag<CreationTimeTag>()->setCreationTime(timeTag->getCreationTime());
                PacketsToSend receivedPkts;
                receivedPkts.pktId = seqNo;
                receivedPkts.msg = newPacket;
                receivedPacketsList.push_back(receivedPkts);
                newPacket->setKind(RDP_I_DATA); // TBD currently we never send RDP_I_URGENT_DATA
                newPacket->addTag<SocketInd>()->setSocketId(socketId);
                EV_INFO << "Sending to App packet: " << newPacket->str() << endl;
                sendToApp(newPacket);
            }
            // All the Packets have been received
            if (state->isfinalReceivedPrintedOut == false) {
                EV_INFO << "Total Received Packets: " << numberReceivedPackets << endl;
                EV_INFO << "Total Wanted Packets: " << wantedPackets << endl;
                if (numberReceivedPackets == wantedPackets || state->connFinished == true) {
                    std::list<PacketsToSend>::iterator iter; // received iterator
                    iter = receivedPacketsList.begin();
                    while (iter != receivedPacketsList.end()) {
                        iter++;
                    }
                    EV_INFO << " numRcvTrimmedHeader:    " << state->numRcvTrimmedHeader << endl;
                    EV_INFO << "CONNECTION FINISHED!" << endl;
                    sendIndicationToApp(RDP_I_PEER_CLOSED); // this is ok if the sinkApp is used by one conn
                    state->isfinalReceivedPrintedOut = true;
                }
            }
            if(noPacketsInFlight){
                bool firstPull = true;
                for(int i = 0; i < state->cwnd; i++){
                    if(firstPull){
                        addRequestToPullsQueue(true, true);
                        firstPull = false;
                    }
                    else{
                        addRequestToPullsQueue(false, true);
                    }

                }
                noPacketsInFlight = false;

            }
        }
    }
    ll: return event;
}

void RdpConnection::addRequestToPullsQueue(bool isFirstPull, bool pacePacket)
{
    EV_TRACE << "RdpConnection::addRequestToPullsQueue" << endl;
    ++state->request_id;
    char msgname[16];
    sprintf(msgname, "PULL-%d", state->request_id);
    Packet *rdppack = new Packet(msgname);

    const auto &rdpseg = makeShared<RdpHeader>();
    rdpseg->setIsDataPacket(false);
    rdpseg->setIsPullPacket(true);
    rdpseg->setIsHeader(false);
    rdpseg->setSynBit(false);
    rdpseg->setAckBit(false);
    rdpseg->setNackBit(false);
    rdpseg->setPullSequenceNumber(state->request_id);
    rdppack->insertAtFront(rdpseg);
    pullQueue.insert(rdppack);
    pullQueuePacing.push(pacePacket);
    EV_INFO << "Adding new request to the pull queue -- pullsQueue length now = " << pullQueue.getLength() << endl;
    bool napState = getRDPMain()->getNapState();
    if (napState == true && isFirstPull) {
        EV_INFO << "Requesting Pull Timer (12 microseconds)" << endl;
        getRDPMain()->requestTimer(pacePacket);
    }
}

void RdpConnection::sendRequestFromPullsQueue()
{
    EV_TRACE << "RdpConnection::sendRequestFromPullsQueue" << endl;
    if (pullQueue.getLength() > 0) {
        state->sentPullsInWindow++;
        Packet *fp = check_and_cast<Packet*>(pullQueue.pop());
        pullQueuePacing.pop();
        auto rdpseg = fp->removeAtFront<rdp::RdpHeader>();
        EV << "a request has been popped from the Pull queue, the new queue length  = " << pullQueue.getLength() << " \n\n";
        sendToIP(fp, rdpseg);
    }
}

int RdpConnection::getPullsQueueLength()
{
    int len = pullQueue.getLength();
    return len;
}

bool RdpConnection::isConnFinished()
{
    return state->connFinished;
}

int RdpConnection::getNumRcvdPackets()
{
    return state->numberReceivedPackets;
}

void RdpConnection::setConnFinished()
{
    state->connFinished = true;
}

RdpEventCode RdpConnection::processSegmentInListen(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address srcAddr, L3Address destAddr)
{
    EV_DETAIL << "Processing segment in LISTEN" << endl;

    if (rdpseg->getSynBit()) {
        EV_DETAIL << "SYN bit set: filling in foreign socket" << endl;
        rdpMain->updateSockPair(this, destAddr, srcAddr, rdpseg->getDestPort(), rdpseg->getSrcPort());
        // this is a receiver
        state->numPacketsToGet = rdpseg->getNumPacketsToSend();
        return RDP_E_RCV_SYN; // this will take us to SYN_RCVD
    }
    EV_WARN << "Unexpected segment: dropping it" << endl;
    return RDP_E_IGNORE;
}

void RdpConnection::segmentArrivalWhileClosed(Packet *packet, const Ptr<const RdpHeader> &rdpseg, L3Address srcAddr, L3Address destAddr)
{
    EV_TRACE << "RdpConnection::segmentArrivalWhileClosed" << endl;
    EV_INFO << "Seg arrived: " << endl;
    printSegmentBrief(packet, rdpseg);
    // This segment doesn't belong to any connection, so this object
    // must be a temp object created solely for the purpose of calling us
    ASSERT(state == nullptr);
    EV_INFO << "Segment doesn't belong to any existing connection" << endl;
    EV_FATAL << "RdpConnection::segmentArrivalWhileClosed should not be called!";
}

}    // namespace rdp

} // namespace inet

