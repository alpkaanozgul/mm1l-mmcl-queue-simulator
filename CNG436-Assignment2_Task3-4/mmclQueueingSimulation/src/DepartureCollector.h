#ifndef DEPARTURE_COLLECTOR_H_
#define DEPARTURE_COLLECTOR_H_

#include <omnetpp.h>

using namespace omnetpp;

namespace mmclqueueingsimulation {

class DepartureCollector : public cSimpleModule
{
  private:
    long countOfDepartedCustomers;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

}

#endif
