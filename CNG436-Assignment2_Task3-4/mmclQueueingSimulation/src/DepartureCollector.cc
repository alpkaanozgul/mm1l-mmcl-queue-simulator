#include "DepartureCollector.h"

namespace mmclqueueingsimulation {

Define_Module(DepartureCollector);

void DepartureCollector::initialize()
{
    countOfDepartedCustomers = 0;
}

void DepartureCollector::handleMessage(cMessage *msg)
{
    // we just take the customer off the wire and throw it away.
    // counts of who left are useful for a quick sanity-check.
    countOfDepartedCustomers++;
    delete msg;
}

void DepartureCollector::finish()
{
    recordScalar("totalDepartures", (double)countOfDepartedCustomers);
}

}
