
#include "RdpMarking.h"
#include "../Rdp.h"

namespace inet {
namespace rdp {

Register_Class(RdpMarking);

simsignal_t RdpMarking::cwndSignal = cComponent::registerSignal("cwnd");    // will record changes to snd_cwnd
simsignal_t RdpMarking::ssthreshSignal = cComponent::registerSignal("ssthresh");    // will record changes to ssthresh

RdpMarkingStateVariables::RdpMarkingStateVariables()
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
    alpha = 0;
    //gamma = 0.0625; // 1/16 (backup 0.16) TODO make it NED parameter;
    gamma = 0.0625;
    numOfMarkedPackets = 0;
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
}

RdpMarking::RdpMarking() :
        RdpAlgorithm(), state((RdpMarkingStateVariables*&) RdpAlgorithm::state)
{

}

RdpMarking::~RdpMarking()
{

}

void RdpMarking::initialize()
{

}

void RdpMarking::connectionClosed()
{

}

void RdpMarking::processTimer(cMessage *timer, RdpEventCode &event)
{

}

void RdpMarking::dataSent(uint32_t fromseq)
{

}

void RdpMarking::ackSent()
{

}

void RdpMarking::receivedHeader(unsigned int seqNum)
{
    EV_INFO << "Header arrived at the receiver" << endl;
    conn->sendNackRdp(seqNum);
    if(state->outOfWindowPackets > 0){
        state->outOfWindowPackets--;
    }
    else{
        state->sentPullsInWindow--;              //SEND PULLS FOR ALL CWND IF TWO HEADERS IN A ROW (FOR LOOP)
        state->receivedPacketsInWindow++;
    }

    if(state->outOfWindowPackets <= 0){
        state->congestionInWindow = true;  //may have to change so certain amount of headers before decrease
        state->ssthresh = state->cwnd/2;  //ssthresh, half of cwnd at loss event
        conn->emit(ssthreshSignal, state->ssthresh);
        state->cwnd = state->cwnd/2;
        if(state->cwnd == 0) state->cwnd = 1;
        //if(state->receivedPacketsInWindow >= state->cwnd){
        state->outOfWindowPackets = state->sentPullsInWindow - state->cwnd; //print this
        std::cout << "\n Out of Window packets: " << state->outOfWindowPackets << endl;
        std::cout << "\n Sent pulls in window: " << state->sentPullsInWindow << endl;
        std::cout << "\n New cwnd: " << state->cwnd << "\n" << endl;
        state->receivedPacketsInWindow = 0;
        state->numOfMarkedPackets = 0;
        state->sentPullsInWindow = state->cwnd;
        state->waitToStart = true;
            //}
    }

    if (state->numberReceivedPackets == 0 && state->connNotAddedYet == true) { //TODO never going to be called - IW is prioritised REMOVE
        conn->prepareInitialRequest();
        conn->addRequestToPullsQueue();
    }
    if(state->outOfWindowPackets < 0){
        state->congestionInWindow = false;
        int packetsMissing = state->outOfWindowPackets * -1;
        state->sentPullsInWindow = state->cwnd - packetsMissing;
        for(int i = 0; i < state->outOfWindowPackets * -1; i++){
            conn->addRequestToPullsQueue();

        }
        state->outOfWindowPackets = 0;
        //conn->paceChanged(state->sRtt.dbl()/(double(state->cwnd)));
        state->pacingTime = state->sRtt.dbl()/(double(state->cwnd));
        if(conn->getPullsQueueLength() > 0){
            conn->schedulePullTimer(); //should check if timer is scheduled, if is do nothing. Otherwise, schedule new timer based on previous time step.
        }
    }
    conn->emit(cwndSignal, state->cwnd);
}

void RdpMarking::receivedData(unsigned int seqNum, bool isMarked)
{   //make it so once the window after out of order packets has reached the new window, waits till end of this window to perform next action - being either additive increase or marked/mulitplicative decrease
    int pullPacketsToSend = 0;
    bool windowIncreased = false;
    //If there are some packets still in the network following a trimmed header arrival
    if(state->outOfWindowPackets > 0){
        state->outOfWindowPackets--;
    }
    else{
        state->receivedPacketsInWindow++;
        state->sentPullsInWindow--;
        if(isMarked && !state->justReduced){
            state->numOfMarkedPackets++;
            //std::cout << "Marked packet received! Current cwnd: " << state->cwnd << endl;
        }
    }
    EV_INFO << "Data packet arrived at the receiver - seq num " << seqNum << endl;
    unsigned int arrivedPktSeqNo = seqNum;
    conn->sendAckRdp(arrivedPktSeqNo); //TODO rename method to sendAck
    unsigned int seqNo = seqNum;
    state->numberReceivedPackets++;

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
        state->slowStartState = true;
        pullPacketsToSend++;
        state->cwnd++;
    }
    else{
        state->slowStartState = false;
    }

    bool cwndReduced = false;
    if(state->outOfWindowPackets <= 0){
//        if(!isMarked){
        if(state->receivedPacketsInWindow == state->cwnd){ //Congestion Avoidance - Linear Increase
            if(state->slowStartState){
                //pullPacketsToSend = state->cwnd;
                state->receivedPacketsInWindow = 0;
                state->numOfMarkedPackets = 0;
                windowIncreased = true;
                //state->cwnd = state->cwnd + state->cwnd;
                state->congestionInWindow = false;
            }
            else{
                if(state->numOfMarkedPackets > 0){
                    double ratio = (double) state->numOfMarkedPackets / (double)state->cwnd;
                    std::cout << "\n\n _____________" << endl;
                    std::cout << "\n Old Alpha Value: " << state->alpha << endl;
                    state->alpha = state->alpha * (1 - state->gamma) + state->gamma * ratio;
                    std::cout << "\n Marked Packets: " << state->numOfMarkedPackets << endl;
                    std::cout << "\n Alpha Value: " << state->alpha << endl;
                    std::cout << "\n Gamma Value: " << state->gamma << endl; //see reasoning in paper
                    std::cout << "\n Ratio Value: " << ratio << endl;
                    std::cout << "\n Old CWND Value: " << state->cwnd << endl;
                    state->cwnd = state->cwnd * (1 - state->alpha / 2);
                    state->justReduced = false;
                    state->ssthresh = state->cwnd;
                    cwndReduced = true;
                    std::cout << "\n New CWND Value: " << state->cwnd << endl;
                    state->numOfMarkedPackets = 0;
                    std::cout << "\n sentPullsInWindow" << state->sentPullsInWindow << endl;
                    state->outOfWindowPackets = state->sentPullsInWindow - state->cwnd;
                    std::cout << "\n Out of Window Packets: " << state->outOfWindowPackets << endl;
                    state->receivedPacketsInWindow = 0;
                    state->sentPullsInWindow = state->cwnd;
                    if(state->outOfWindowPackets < 0){
                        state->congestionInWindow = false;
                        int packetsMissing = state->outOfWindowPackets * -1;
                        state->sentPullsInWindow = state->cwnd - packetsMissing;
                        for(int i = 0; i < state->outOfWindowPackets * -1; i++){
                            conn->addRequestToPullsQueue();

                        }
                        state->outOfWindowPackets = 0;
                        state->pacingTime = state->sRtt.dbl()/(double(state->cwnd));
                        //state->pacingTime = state->sRtt.dbl()/(double(125));
                        if(conn->getPullsQueueLength() > 0){
                            conn->schedulePullTimer(); //should check if timer is scheduled, if is do nothing. Otherwise, schedule new timer based on previous time step.
                        }
                    }
                }
                else{
                    double ratio = 0;
                    state->alpha = state->alpha * (1 - state->gamma) + state->gamma * ratio;
                    std::cout << "\n\n _____________" << endl;
                    std::cout << "\n Alpha Value (no markings): " << state->alpha << endl;
                    state->numOfMarkedPackets = 0;
                    windowIncreased = true;
                    state->cwnd = state->cwnd + state->additiveIncreasePackets;
                    pullPacketsToSend = pullPacketsToSend + state->additiveIncreasePackets;
                    state->receivedPacketsInWindow = 0;
                    state->congestionInWindow = false;
                    state->justReduced = false;
                }
            }
        }
//        }
//        else{
//            std::cout << "\n Marked Packet arrival!" << endl;
//            std::cout << "\n CWND CHANGED FROM " << state->cwnd << endl;
//            state->cwnd = state->cwnd * (1 - state->alpha / 2);
//            std::cout << "\n TO  " << state->cwnd << endl;
//            state->outOfWindowPackets = state->sentPullsInWindow - state->cwnd;
//            state->receivedPacketsInWindow = 0;
//            state->sentPullsInWindow = state->cwnd;
//            if(state->outOfWindowPackets < 0){
//                state->congestionInWindow = false;
//                int packetsMissing = state->outOfWindowPackets * -1;
//                state->sentPullsInWindow = state->cwnd - packetsMissing;
//                for(int i = 0; i < state->outOfWindowPackets * -1; i++){
//                    conn->addRequestToPullsQueue();
//
//                }
//                state->outOfWindowPackets = 0;
//                state->pacingTime = state->sRtt.dbl()/(double(state->cwnd));
//                if(conn->getPullsQueueLength() > 0){
//                    conn->schedulePullTimer(); //should check if timer is scheduled, if is do nothing. Otherwise, schedule new timer based on previous time step.
//                }
//            }
//        }
    }

    if (state->numberReceivedPackets == 1 && state->connNotAddedYet == true && state->outOfWindowPackets <= 0 && !cwndReduced) {
        conn->prepareInitialRequest();
        conn->addRequestToPullsQueue();
        for(int pullNum = 0; pullNum < pullPacketsToSend; pullNum++){
            conn->addRequestToPullsQueue();
        }
    }
    else if((state->outOfWindowPackets <= 0) && (state->sentPullsInWindow < state->cwnd) && !cwndReduced){
        if (state->numberReceivedPackets <= state->numPacketsToGet) {
            //for loop sending pull for each pullPacketToSend;
            pullPacketsToSend++; //Pull Request to maintain current flow

            if(pullPacketsToSend > 0 && ((state->numberReceivedPackets + state->sentPullsInWindow + pullPacketsToSend) >= state->numPacketsToGet)){
                int newPullPacketsToSend = (state->numPacketsToGet - (state->numberReceivedPackets + state->sentPullsInWindow));
                if(newPullPacketsToSend < pullPacketsToSend) pullPacketsToSend = newPullPacketsToSend;
            } //maybe add this if statement to first window instance (see previous if block)

            for(int pullNum = 0; pullNum < pullPacketsToSend; pullNum++){
                conn->addRequestToPullsQueue();
            }
        }
    }

    EV_INFO << "Sending Data Packet to Application" << endl;
    conn->sendPacketToApp(seqNum);

    //conn->paceChanged(state->sRtt.dbl()/(double(state->cwnd)));
    state->pacingTime = state->sRtt.dbl()/(double(state->cwnd));
    if(conn->getPullsQueueLength() > 0){
        conn->schedulePullTimer(); //should check if timer is scheduled, if is do nothing. Otherwise, schedule new timer based on previous time step.
    }
    conn->emit(cwndSignal, state->cwnd);
}

} // namespace RDP

} // namespace inet

