#ifndef SOURCE_H
#define SOURCE_H

#include <omnetpp.h>

using namespace omnetpp;

//
// Source: generates Poisson arrivals (exponential inter-arrival times).
// Inter-arrival drawn via inverse transform: -ln(U) / lambda.
//
class Source : public cSimpleModule
{
private:
    double   lambda;
    cMessage *arrivalEvent;
    long     totalGenerated;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif
