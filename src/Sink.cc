#include "Sink.h"

Define_Module(Sink);

void Sink::initialize()
{
    totalReceived     = 0;
    responseTimeSignal = registerSignal("responseTime");

    EV << "[Sink] initialized." << endl;
}

void Sink::handleMessage(cMessage *msg)
{
    Job *job = check_and_cast<Job *>(msg);

    double responseTime = simTime().dbl() - job->getArrivalTime();
    emit(responseTimeSignal, responseTime);
    totalReceived++;

    EV << "[Sink] Job received. responseTime=" << responseTime
       << "s  total=" << totalReceived << endl;

    delete job;
}

void Sink::finish()
{
    recordScalar("totalReceived", totalReceived);
    EV << "[Sink] finish(): totalReceived=" << totalReceived << endl;
}
