#include "ArrivalEventGenerator.h"
#include "CustomerPacket_m.h"

namespace mmclqueueingsimulation {

Define_Module(ArrivalEventGenerator);

ArrivalEventGenerator::ArrivalEventGenerator()
{
    nextArrivalReminder = nullptr;
}

ArrivalEventGenerator::~ArrivalEventGenerator()
{
    cancelAndDelete(nextArrivalReminder);
}

void ArrivalEventGenerator::initialize()
{
    meanInterArrivalTime = par("meanInterArrivalTime").doubleValue();

    // line up the first arrival. we wake up after an exponential gap
    // and from then on the handler keeps the chain going.
    nextArrivalReminder = new cMessage("nextArrivalReminder");
    scheduleAt(simTime() + exponential(meanInterArrivalTime), nextArrivalReminder);
}

void ArrivalEventGenerator::handleMessage(cMessage *msg)
{
    // this module only ever receives its own wake-up message
    if (msg == nextArrivalReminder) {
        CustomerPacket *newCustomer = new CustomerPacket("customer");
        newCustomer->setArrivalInstant(simTime());
        send(newCustomer, "out");

        // book the next arrival
        scheduleAt(simTime() + exponential(meanInterArrivalTime), nextArrivalReminder);
    }
}

void ArrivalEventGenerator::finish()
{
    // nothing to summarise here, the queueing system writes the scalars
}

}
