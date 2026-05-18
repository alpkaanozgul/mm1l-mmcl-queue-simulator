#include "Source.h"
#include "../msg/Job_m.h"

Define_Module(Source);

void Source::initialize()
{
    lambda         = par("lambda").doubleValue();
    totalGenerated = 0;

    arrivalEvent = new cMessage("arrival");
    double ia = -std::log(dblrand()) / lambda;
    scheduleAt(simTime() + ia, arrivalEvent);

    EV << "[Source] initialized. lambda=" << lambda << "/s" << endl;
}

void Source::handleMessage(cMessage *msg)
{
    ASSERT(msg == arrivalEvent);

    Job *job = new Job("job");
    job->setArrivalTime(simTime().dbl());
    send(job, "out");
    totalGenerated++;

    double ia = -std::log(dblrand()) / lambda;
    scheduleAt(simTime() + ia, arrivalEvent);
}

void Source::finish()
{
    recordScalar("totalGenerated", totalGenerated);

    cancelAndDelete(arrivalEvent);
    arrivalEvent = nullptr;

    EV << "[Source] finish(): totalGenerated=" << totalGenerated << endl;
}
