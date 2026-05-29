#ifndef QUEUEING_SYSTEM_MMCL_H_
#define QUEUEING_SYSTEM_MMCL_H_

#include <omnetpp.h>
#include <queue>
#include <vector>
#include "CustomerPacket_m.h"

using namespace omnetpp;

namespace mmclqueueingsimulation {

class QueueingSystemMMCL : public cSimpleModule
{
  private:
    // --- parameters read once from omnetpp.ini ---
    int numberOfServers;        // c
    int maxSystemCapacity;      // L  (total: in service + in queue)
    double meanServiceTime;     // 1/mu

    // --- live state ---
    std::queue<CustomerPacket*> waitingLine;
    std::vector<bool> serverIsBusy;
    std::vector<cMessage*> serverFinishEvents;
    // remember which packet is currently being served by each server,
    // so we can hand it on to the sink when service finishes
    std::vector<CustomerPacket*> packetInService;
    // when did this server start its current job? used for busy time
    std::vector<simtime_t> serverBusySince;

    // --- counters for the six measures ---
    long totalArrivals;
    long arrivalsRejectedDueToFullSystem;
    long customersServedCompletely;
    long countOfCustomersThatHadToWait;

    double sumOfWaitingTimes;
    double sumOfWaitingTimesAmongThoseWhoActuallyWaited;
    double cumulativeBusyServerTime;

    // time-weighted queue length accounting
    simtime_t lastQueueChangeInstant;
    double cumulativeQueueAreaUnderCurve;

    // start of the steady-state observation window
    simtime_t measurementStartInstant;
    bool measurementWindowStarted;
    // wakes us at the end of the warm-up so we can wipe the counters
    cMessage *warmupEndedMarker;

    // --- helpers ---
    void rollQueueAreaForward();
    int findOneFreeServer();
    void startServiceOnServer(int serverIndex, CustomerPacket *customer);

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

  public:
    QueueingSystemMMCL();
    virtual ~QueueingSystemMMCL();
};

}

#endif
