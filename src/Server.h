#ifndef SERVER_H
#define SERVER_H

#include <omnetpp.h>
#include <queue>
#include "../msg/Job_m.h"

using namespace omnetpp;

//
// Server: M/M/1/L single-server FIFO queue with finite system capacity L.
//
// Arrivals: accepted if systemSize() < L, else blocked and dropped.
// Service:  Exp(mu) via inverse transform: -ln(U)/mu.
// Stats:    collects E[W], E[W|W>0], U, E[Nq], E[N], lambda_eff.
//
class Server : public cSimpleModule
{
private:
    int    capacity;        // L: max customers in system (queue + server)
    double mu;              // service rate

    std::queue<Job *> jobQueue;
    bool              serverBusy;
    Job              *jobInService;
    cMessage         *serviceEndEvent;

    // Accumulated post-warmup counters (reset by warmupResetEvent)
    long   totalArrived;
    long   totalDropped;
    long   totalServed;
    long   countWaited;    // jobs that had to wait (W > 0)
    double sumWaitGT0;     // sum of positive wait times

    cMessage *warmupResetEvent;

    simsignal_t waitingTimeSignal;
    simsignal_t queueLengthSignal;
    simsignal_t utilisationSignal;
    simsignal_t systemLengthSignal;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

private:
    void startService(Job *job);
    void endService();
    int  systemSize() const { return (int)jobQueue.size() + (serverBusy ? 1 : 0); }
    void resetCounters();
};

#endif
