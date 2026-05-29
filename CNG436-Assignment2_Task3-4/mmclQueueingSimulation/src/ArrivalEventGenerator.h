#ifndef ARRIVAL_EVENT_GENERATOR_H_
#define ARRIVAL_EVENT_GENERATOR_H_

#include <omnetpp.h>

using namespace omnetpp;

namespace mmclqueueingsimulation {

class ArrivalEventGenerator : public cSimpleModule
{
  private:
    // average gap between two arrivals, taken from the .ini file
    double meanInterArrivalTime;

    // self-message we use to wake ourselves up at the next arrival time
    cMessage *nextArrivalReminder;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

  public:
    ArrivalEventGenerator();
    virtual ~ArrivalEventGenerator();
};

}

#endif
