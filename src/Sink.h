#ifndef SINK_H
#define SINK_H

#include <omnetpp.h>
#include "../msg/Job_m.h"

using namespace omnetpp;

//
// Sink: absorbs completed jobs, records response time E[T] = sojourn time.
//
class Sink : public cSimpleModule
{
private:
    long        totalReceived;
    simsignal_t responseTimeSignal;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif
