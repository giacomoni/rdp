
#include "HrdpMark.h"
#include "../Rdp.h"

namespace inet {
namespace rdp {

Register_Class(HrdpMark);

simsignal_t HrdpMark::cwndSignal = cComponent::registerSignal("cwnd");    // will record changes to snd_cwnd
simsignal_t HrdpMark::ssthreshSignal = cComponent::registerSignal("ssthresh");    // will record changes to ssthresh

HrdpMarkStateVariables::HrdpMarkStateVariables()
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

    alpha = 0;
    gamma = 0.0625;
    numOfMarkedPackets = 0;
    observeWindEnd = 0;

    increaseFactor = 1;
    lastCongestionEvent = 0;
}

HrdpMark::HrdpMark() :
        RdpAlgorithm(), state((HrdpMarkStateVariables*&) RdpAlgorithm::state)
{

}

HrdpMark::~HrdpMark()
{

}

void HrdpMark::initialize()
{

}

void HrdpMark::connectionClosed()
{

}

void HrdpMark::processTimer(cMessage *timer, RdpEventCode &event)
{

}

void HrdpMark::dataSent(uint32_t fromseq)
{

}

void HrdpMark::ackSent()
{

}

void HrdpMark::receivedHeader(unsigned int seqNum)
{
    EV_INFO << "Header arrived at the receiver" << endl;
    conn->sendNackRdp(seqNum);
    state->lastCongestionEvent = simTime();
    if(state->outOfWindowPackets > 0){
        state->outOfWindowPackets--;
    }
    else{
        state->sentPullsInWindow--;              //SEND PULLS FOR ALL CWND IF TWO HEADERS IN A ROW (FOR LOOP)
        state->receivedPacketsInWindow++;
    }

    if(state->outOfWindowPackets == 0){
        state->congestionInWindow = true;  //may have to change so certain amount of headers before multiplicative decrease
        state->ssthresh = state->cwnd/2;  //ssthresh, half of cwnd at loss event
        conn->emit(ssthreshSignal, state->ssthresh);
        state->cwnd = state->cwnd/2;
        if(state->cwnd == 0) state->cwnd = 1;
        state->outOfWindowPackets = state->sentPullsInWindow - state->cwnd;
        state->receivedPacketsInWindow = 0;
        state->sentPullsInWindow = state->cwnd;
        state->waitToStart = true;
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

void HrdpMark::receivedData(unsigned int seqNum, bool isMarked)
{
    EV_INFO << "\nRdpMark ___________________________________________" << endl;
    EV_INFO << "\nRdpMark - Received Data" << endl;
    int pullPacketsToSend = 0;
    //If there are some packets still in the network following a trimmed header arrival
    bool isOutOfWindow = false;
    if(state->outOfWindowPackets > 0){
        state->outOfWindowPackets--;
        isOutOfWindow = true;
        EV_INFO << "\nRdpMark - Out of Window Packet received. Remaining: " << state->outOfWindowPackets << endl;
    }
    else{
        state->receivedPacketsInWindow++;
        state->sentPullsInWindow--;
    }

    if(isMarked){
        EV_INFO << "\nRdpMark - Marked data packet arrived" << endl;
        state->numOfMarkedPackets++;
        state->lastCongestionEvent = simTime();
    }
    else{
        simtime_t delta = simTime() - state->lastCongestionEvent;
        if (delta <= SimTime(1, SIMTIME_S))
        {
            state->increaseFactor = 1;
        }
        else{
            simtime_t diff = delta - SimTime(1, SIMTIME_S);
            state->increaseFactor = (1 + 10 * diff.dbl() + 0.25 * (diff.dbl() * diff.dbl()));
        }
        state->increaseFactor = 2 * (1 - (state->alpha / 2)) * state->increaseFactor;
    }

    EV_INFO << "\nRdpMark - Data packet arrived at the receiver - seq num " << seqNum << endl;
    conn->sendAckRdp(seqNum); //TODO rename method to sendAck
    state->numberReceivedPackets++;

    if (state->numberReceivedPackets >= state->numPacketsToGet) {
        EV_INFO << "\nRdpMark - All packets received - finish all connections!" << endl;
        conn->emit(cwndSignal, state->cwnd);
        EV_INFO << "Total Received Packets: " << state->numberReceivedPackets << endl;
        EV_INFO << "Total Wanted Packets: " << state->numPacketsToGet << endl;
        conn->sendPacketToApp(seqNum);
        conn->closeConnection();
        return;
    }

    if(!isOutOfWindow){
        if(state->cwnd < state->ssthresh) { //Slow-Start - Exponential Increase
            EV_INFO << "\nRdpMark - Increasing CWND (Slow Start)" << endl;
            state->slowStartState = true;
            pullPacketsToSend++;
            state->cwnd++;
        }
        else{
            state->slowStartState = false;
        }

        if(seqNum >= state->observeWindEnd){ //Marking Observational Window
            int oldObserve = state->observeWindEnd;
            //state->observeWindEnd = state->observeWindEnd + conn->getPullsQueueLength() + state->sentPullsInWindow;
            state->observeWindEnd = state->observeWindEnd + state->cwnd;
            EV_INFO << "\nRdpMark - End of observation window, moving end from " << oldObserve << " to " << state->observeWindEnd << endl;
            EV_INFO << "\nRdpMark - Current Number of Received Packets " << state->numberReceivedPackets << endl;
            double ratio;
            if(state->numOfMarkedPackets > 0){
                ratio = (double) state->numOfMarkedPackets / (double)state->observeWindSize;
            }
            else{
                ratio = 0;
            }
            EV_INFO << "\nRdpMark - Old Alpha Value: " << state->alpha << endl;
            state->alpha = state->alpha * (1 - state->gamma) + state->gamma * ratio;
            EV_INFO << "\nRdpMark - Marked Packets: " << state->numOfMarkedPackets << endl;
            EV_INFO << "\nRdpMark - Alpha Value: " << state->alpha << endl;
            EV_INFO << "\nRdpMark - Ratio Value: " << ratio << endl;
            state->numOfMarkedPackets = 0;
            state->justReduced = false;
            state->observeWindSize = state->observeWindEnd - oldObserve;
        }

        bool newWindow = false;
        if(state->numOfMarkedPackets > 0 && !state->justReduced){ //Marked packet found (no previously marked packets in window)
            double prevCwnd = state->cwnd;
            state->cwnd = state->cwnd * (1 - state->alpha / 2);
            if(state->cwnd == 0) state->cwnd = 1;
            state->ssthresh = state->cwnd;
            EV_INFO << "\nRdpMark - Marked packet in window, reducing cwnd from " << prevCwnd << " to " << state->cwnd << endl;
            EV_INFO << "\nRdpMark - Current sent pulls in window:  " << state->sentPullsInWindow << endl;
            if(prevCwnd > state->cwnd){
                state->outOfWindowPackets = state->sentPullsInWindow - state->cwnd;
                state->receivedPacketsInWindow = 0;
                state->sentPullsInWindow = state->cwnd;
                newWindow = true;
                EV_INFO << "\nRdpMark - Total out of order packets  = " << state->outOfWindowPackets << endl;
            }
            state->justReduced = true;
            if(state->outOfWindowPackets < 0){
                int packetsMissing = state->outOfWindowPackets * -1;
                state->sentPullsInWindow = state->cwnd - packetsMissing;
                EV_INFO << "\nRdpMark - Reducing window, out of new window packets: " << packetsMissing << endl;
                for(int i = 0; i < state->outOfWindowPackets * -1; i++){
                    conn->addRequestToPullsQueue();
                }
                state->outOfWindowPackets = 0;
            }
        }

//        if(seqNum >= state->observeWindEnd){ //Marking Observational Window
//            int oldObserve = state->observeWindEnd;
//            state->observeWindEnd = state->observeWindEnd + conn->getPullsQueueLength() + state->sentPullsInWindow;
//            //state->observeWindEnd = state->observeWindEnd + state->sentPullsInWindow;
//            EV_INFO << "\nRdpMark - End of observation window, moving end from " << oldObserve << " to " << state->observeWindEnd << endl;
//            EV_INFO << "\nRdpMark - Current Number of Received Packets " << state->numberReceivedPackets << endl;
//            double ratio;
//            if(state->numOfMarkedPackets > 0){
//                ratio = (double) state->numOfMarkedPackets / (double)state->observeWindSize;
//            }
//            else{
//                ratio = 0;
//            }
//            EV_INFO << "\nRdpMark - Old Alpha Value: " << state->alpha << endl;
//            state->alpha = state->alpha * (1 - state->gamma) + state->gamma * ratio;
//            EV_INFO << "\nRdpMark - Marked Packets: " << state->numOfMarkedPackets << endl;
//            EV_INFO << "\nRdpMark - Alpha Value: " << state->alpha << endl;
//            EV_INFO << "\nRdpMark - Ratio Value: " << ratio << endl;
//            state->numOfMarkedPackets = 0;
//            state->justReduced = false;
//            state->observeWindSize = state->observeWindEnd - oldObserve;
//        }


        if(((state->receivedPacketsInWindow % state->cwnd) == 0) && !newWindow){ //Congestion Avoidance - Linear Increase
            EV_INFO << "\nRdpMark - End of Window." << endl;
            //if(state->slowStartState){
            //state->receivedPacketsInWindow = 0; //may not be needed
            state->congestionInWindow = false;
            //}
            //else{
//            double ratio;
//            if(state->numOfMarkedPackets > 0){
//                ratio = (double) state->numOfMarkedPackets / (double)state->cwnd;
//            }
//            else{
//                ratio = 0;
              state->cwnd = state->cwnd + state->increaseFactor;
              pullPacketsToSend = pullPacketsToSend + state->increaseFactor;
//            }
//            EV_INFO << "\nRdpMark - Old Alpha Value: " << state->alpha << endl;
//            state->alpha = state->alpha * (1 - state->gamma) + state->gamma * ratio;
//            EV_INFO << "\nRdpMark - Marked Packets: " << state->numOfMarkedPackets << endl;
//            EV_INFO << "\nRdpMark - Alpha Value: " << state->alpha << endl;
//            EV_INFO << "\nRdpMark - Ratio Value: " << ratio << endl;
            //state->numOfMarkedPackets = 0;
            state->receivedPacketsInWindow = 0;
            //state->justReduced = false;
            //}
        }

        if (state->numberReceivedPackets == 1 && state->connNotAddedYet == true) {
            conn->prepareInitialRequest();
            conn->addRequestToPullsQueue();
            for(int pullNum = 0; pullNum < pullPacketsToSend; pullNum++){
                conn->addRequestToPullsQueue();
            }
        }
        else if(state->sentPullsInWindow < state->cwnd){
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

