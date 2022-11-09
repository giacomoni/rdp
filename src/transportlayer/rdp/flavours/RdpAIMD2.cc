
#include "RdpAIMD2.h"
#include "../Rdp.h"

namespace inet {
namespace rdp {

Register_Class(RdpAIMD2);

simsignal_t RdpAIMD2::cwndSignal = cComponent::registerSignal("cwnd");    // will record changes to snd_cwnd
simsignal_t RdpAIMD2::ssthreshSignal = cComponent::registerSignal("ssthresh");    // will record changes to ssthresh

RdpAIMD2StateVariables::RdpAIMD2StateVariables()
{
    internal_request_id = 0;
    request_id = 0;  // source block number (8-bit unsigned integer)

    pacingTime = 1200/1000000;
    lastPullTime = 0;
    numPacketsToGet = 0;
    numPacketsToSend = 0;
    congestionInWindow = false;

    numRcvTrimmedHeader = 0;
    numberReceivedPackets = 0;
    numberSentPackets = 0;
    IW = 0; // send the initial window (12 Packets as in RDP) IWWWWWWWWWWWW
    receivedPacketsInWindow = 0;
    sentPullsInWindow = 0;
    additiveIncreasePackets = 1;
    slowStartState = true;
    outOfWindowPackets = 0;
    waitToStart = false;
    ssthresh = 0;
    numRcvdPkt = 0;
    delayedNackNo = 0;
    connNotAddedYet = true;
    cwnd = 0;
    active = false;

    sentPackets = 1;
}

RdpAIMD2::RdpAIMD2() :
        RdpAlgorithm(), state((RdpAIMD2StateVariables*&) RdpAlgorithm::state)
{

}

RdpAIMD2::~RdpAIMD2()
{

}

void RdpAIMD2::initialize()
{

}

void RdpAIMD2::connectionClosed()
{

}

void RdpAIMD2::processTimer(cMessage *timer, RdpEventCode &event)
{

}

void RdpAIMD2::dataSent(uint32_t fromseq)
{

}

void RdpAIMD2::ackSent()
{

}

void RdpAIMD2::receivedHeader(unsigned int seqNum)
{
    EV_INFO << "Header arrived at the receiver" << endl;
    state->sentPackets--;
    conn->sendNackRdp(seqNum);

    std::cout << "\nHeader Received" << endl;

    if(!state->congestionInWindow){
        state->ssthresh = state->cwnd/2;
        state->cwnd = state->cwnd/2;
        if(state->cwnd <= 0) state->cwnd = 1;
        state->congestionInWindow = true;
        if(state->sentPackets > state->cwnd){
            state->receivedPacketsInWindow = 0;
            state->outOfWindowPackets = state->sentPackets - state->cwnd;
        }
    }

    if(state->sentPackets < state->cwnd){
        for(int pullNum = state->sentPackets; pullNum < state->cwnd; pullNum++){
            conn->addRequestToPullsQueue();
            state->sentPackets++;
        }
    }


    state->pacingTime = state->sRtt.dbl()/(double(state->cwnd));
    if(conn->getPullsQueueLength() > 0){
        conn->schedulePullTimer(); //should check if timer is scheduled, if is do nothing. Otherwise, schedule new timer based on previous time step.
    }
//    if(state->outOfWindowPackets > 0){
//        state->outOfWindowPackets--;
//    }
//    else{
//        state->sentPullsInWindow--;              //SEND PULLS FOR ALL CWND IF TWO HEADERS IN A ROW (FOR LOOP)
//        state->receivedPacketsInWindow++;
//    }
//
//    if(state->outOfWindowPackets == 0){
//        state->congestionInWindow = true;  //may have to change so certain amount of headers before multiplicative decrease
//        state->ssthresh = state->cwnd/2;  //ssthresh, half of cwnd at loss event
//        conn->emit(ssthreshSignal, state->ssthresh);
//        //state->cwnd = state->cwnd/2;
//        //state->cwnd = state->cwnd/2;
//        state->cwnd = state->cwnd - 5;
//        if(state->cwnd == 0) state->cwnd = 1;
//        //if(state->receivedPacketsInWindow >= state->cwnd){
//            state->outOfWindowPackets = state->sentPullsInWindow - state->cwnd;
//            state->receivedPacketsInWindow = 0;
//            state->sentPullsInWindow = state->cwnd;
//            state->waitToStart = true;
//        //}
//    }
//
//    if (state->numberReceivedPackets == 0 && state->connNotAddedYet == true) { //TODO never going to be called - IW is prioritised REMOVE
//        conn->prepareInitialRequest();
//        conn->addRequestToPullsQueue();
//    }
//    if(state->outOfWindowPackets < 0){
//        state->congestionInWindow = false;
//        int packetsMissing = state->outOfWindowPackets * -1;
//        state->sentPullsInWindow = state->cwnd - packetsMissing;
//        for(int i = 0; i < state->outOfWindowPackets * -1; i++){
//            conn->addRequestToPullsQueue();
//
//        }
//        state->outOfWindowPackets = 0;
//        //conn->paceChanged(state->sRtt.dbl()/(double(state->cwnd)));
//        state->pacingTime = state->sRtt.dbl()/(double(state->cwnd));
//        if(conn->getPullsQueueLength() > 0){
//            conn->schedulePullTimer(); //should check if timer is scheduled, if is do nothing. Otherwise, schedule new timer based on previous time step.
//        }
//    }
//    conn->emit(cwndSignal, state->cwnd);
}

void RdpAIMD2::receivedData(unsigned int seqNum, bool isMarked)
{
    conn->sendAckRdp(seqNum); //TODO rename method to sendAck
    if(state->outOfWindowPackets <= 0){
        state->numberReceivedPackets++;
        state->receivedPacketsInWindow++;
        state->sentPackets--;
        if (state->numberReceivedPackets >= state->numPacketsToGet) {
            EV_INFO << "All packets received - finish all connections!" << endl;
            conn->emit(cwndSignal, state->cwnd);
            EV_INFO << "Total Received Packets: " << state->numberReceivedPackets << endl;
            EV_INFO << "Total Wanted Packets: " << state->numPacketsToGet << endl;
            conn->sendPacketToApp(seqNum);
            conn->closeConnection();
            return;
        }

        if(state->cwnd < state->ssthresh) { //Slow-Start - Exponential Increase
            state->cwnd++;
        }
        else{
            std::cout << "\n Sent Packets: " << state->sentPackets << endl;
            std::cout << "\n Current CWND: " << state->cwnd << endl;
            if(state->receivedPacketsInWindow == state->cwnd){
                state->cwnd = state->cwnd + state->additiveIncreasePackets;
                state->receivedPacketsInWindow = 0;
                state->congestionInWindow = false;
            }
        }

        if(state->sentPackets < state->cwnd){
            for(int pullNum = state->sentPackets; pullNum < state->cwnd; pullNum++){
                conn->addRequestToPullsQueue();
                state->sentPackets++;
            }
        }
    }
    else{
        state->outOfWindowPackets--;
    }

    conn->sendPacketToApp(seqNum);
    //    //conn->paceChanged(state->sRtt.dbl()/(double(state->cwnd)));
    state->pacingTime = state->sRtt.dbl()/(double(state->cwnd));
    if(conn->getPullsQueueLength() > 0){
        conn->schedulePullTimer(); //should check if timer is scheduled, if is do nothing. Otherwise, schedule new timer based on previous time step.
    }
    conn->emit(cwndSignal, state->cwnd);

//    int pullPacketsToSend = 0;
//    bool windowIncreased = false;
//    //If there are some packets still in the network following a trimmed header arrival
//    if(state->outOfWindowPackets > 0){
//        state->outOfWindowPackets--;
//    }
//    else{
//        state->receivedPacketsInWindow++;
//        state->sentPullsInWindow--;
//    }
//    EV_INFO << "Data packet arrived at the receiver - seq num " << seqNum << endl;
//    unsigned int arrivedPktSeqNo = seqNum;
//    conn->sendAckRdp(arrivedPktSeqNo); //TODO rename method to sendAck
//    unsigned int seqNo = seqNum;
//    state->numberReceivedPackets++;
//
//    if (state->numberReceivedPackets >= state->numPacketsToGet) {
//        EV_INFO << "All packets received - finish all connections!" << endl;
//        conn->emit(cwndSignal, state->cwnd);
//        EV_INFO << "Total Received Packets: " << state->numberReceivedPackets << endl;
//        EV_INFO << "Total Wanted Packets: " << state->numPacketsToGet << endl;
//        conn->sendPacketToApp(seqNum);
//        conn->closeConnection();
//        return;
//    }




//    if(state->cwnd < state->ssthresh) { //Slow-Start - Exponential Increase
//        state->slowStartState = true;
//        pullPacketsToSend++;
//        state->cwnd++;
//    }
//    else{
//    }
//
//    if(state->outOfWindowPackets <= 0){
//        if(((state->receivedPacketsInWindow % state->cwnd) == 0)){ //Congestion Avoidance - Linear Increase
//            if(state->slowStartState){
//                //pullPacketsToSend = state->cwnd;
//                state->receivedPacketsInWindow = 0;
//                windowIncreased = true;
//                //state->cwnd = state->cwnd + state->cwnd;
//                state->congestionInWindow = false;
//            }
//            else{
//                windowIncreased = true;
//                state->cwnd = state->cwnd + state->additiveIncreasePackets;
//                pullPacketsToSend = pullPacketsToSend + state->additiveIncreasePackets;
//                state->receivedPacketsInWindow = 0;
//                state->congestionInWindow = false;
//            }
//        }
//    }
//
//    if (state->numberReceivedPackets == 1 && state->connNotAddedYet == true && state->outOfWindowPackets <= 0) {
//        conn->prepareInitialRequest();
//        conn->addRequestToPullsQueue();
//        for(int pullNum = 0; pullNum < pullPacketsToSend; pullNum++){
//            conn->addRequestToPullsQueue();
//        }
//    }
//    else if((state->outOfWindowPackets <= 0) && (state->sentPullsInWindow < state->cwnd)){
//        if (state->numberReceivedPackets <= state->numPacketsToGet) {
//            //for loop sending pull for each pullPacketToSend;
//            pullPacketsToSend++; //Pull Request to maintain current flow
//
//            if(pullPacketsToSend > 0 && ((state->numberReceivedPackets + state->sentPullsInWindow + pullPacketsToSend) >= state->numPacketsToGet)){
//                int newPullPacketsToSend = (state->numPacketsToGet - (state->numberReceivedPackets + state->sentPullsInWindow));
//                if(newPullPacketsToSend < pullPacketsToSend) pullPacketsToSend = newPullPacketsToSend;
//            } //maybe add this if statement to first window instance (see previous if block)
//
//            for(int pullNum = 0; pullNum < pullPacketsToSend; pullNum++){
//                conn->addRequestToPullsQueue();
//            }
//        }
//    }
//
//    EV_INFO << "Sending Data Packet to Application" << endl;
//    conn->sendPacketToApp(seqNum);
//
//    //conn->paceChanged(state->sRtt.dbl()/(double(state->cwnd)));
//    state->pacingTime = state->sRtt.dbl()/(double(state->cwnd));
//    if(conn->getPullsQueueLength() > 0){
//        conn->schedulePullTimer(); //should check if timer is scheduled, if is do nothing. Otherwise, schedule new timer based on previous time step.
//    }
//    conn->emit(cwndSignal, state->cwnd);
}

} // namespace RDP

} // namespace inet

