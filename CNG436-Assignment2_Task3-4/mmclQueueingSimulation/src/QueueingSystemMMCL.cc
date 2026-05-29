#include "QueueingSystemMMCL.h"
#include <cstdio>

namespace mmclqueueingsimulation {

Define_Module(QueueingSystemMMCL);

QueueingSystemMMCL::QueueingSystemMMCL()
{
    warmupEndedMarker = nullptr;
}

QueueingSystemMMCL::~QueueingSystemMMCL()
{
    while (!waitingLine.empty()) {
        delete waitingLine.front();
        waitingLine.pop();
    }
    for (int i = 0; i < (int)serverFinishEvents.size(); i++) {
        cancelAndDelete(serverFinishEvents[i]);
        if (packetInService[i]) {
            delete packetInService[i];
        }
    }
    cancelAndDelete(warmupEndedMarker);
}

void QueueingSystemMMCL::initialize()
{
    numberOfServers = par("numberOfServers");
    maxSystemCapacity = par("maxSystemCapacity");
    meanServiceTime = par("meanServiceTime").doubleValue();

    if (numberOfServers <= 0) {
        throw cRuntimeError("numberOfServers must be at least 1");
    }
    if (maxSystemCapacity < numberOfServers) {
        throw cRuntimeError("maxSystemCapacity must be at least numberOfServers");
    }

    serverIsBusy.assign(numberOfServers, false);
    serverFinishEvents.assign(numberOfServers, nullptr);
    packetInService.assign(numberOfServers, nullptr);
    serverBusySince.assign(numberOfServers, SIMTIME_ZERO);

    // one self-message per server so we can tell who finished by
    // checking which message pointer fired.
    for (int i = 0; i < numberOfServers; i++) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "serviceDone_%d", i);
        serverFinishEvents[i] = new cMessage(buf);
    }

    totalArrivals = 0;
    arrivalsRejectedDueToFullSystem = 0;
    customersServedCompletely = 0;
    countOfCustomersThatHadToWait = 0;
    sumOfWaitingTimes = 0.0;
    sumOfWaitingTimesAmongThoseWhoActuallyWaited = 0.0;
    cumulativeBusyServerTime = 0.0;
    cumulativeQueueAreaUnderCurve = 0.0;
    lastQueueChangeInstant = SIMTIME_ZERO;
    measurementStartInstant = SIMTIME_ZERO;

    // figure out the warm-up. when the marker fires we will wipe
    // every counter and start fresh.
    simtime_t warmupPeriod = getSimulation()->getWarmupPeriod();
    if (warmupPeriod > SIMTIME_ZERO) {
        warmupEndedMarker = new cMessage("warmupEnded");
        scheduleAt(warmupPeriod, warmupEndedMarker);
        measurementWindowStarted = false;
    } else {
        measurementWindowStarted = true;
    }
}

void QueueingSystemMMCL::rollQueueAreaForward()
{
    // add the rectangle (queueLength) x (time since last change)
    // to the running integral, then move the time marker.
    simtime_t elapsedChunk = simTime() - lastQueueChangeInstant;
    cumulativeQueueAreaUnderCurve += waitingLine.size() * elapsedChunk.dbl();
    lastQueueChangeInstant = simTime();
}

int QueueingSystemMMCL::findOneFreeServer()
{
    for (int i = 0; i < numberOfServers; i++) {
        if (!serverIsBusy[i]) {
            return i;
        }
    }
    return -1;
}

void QueueingSystemMMCL::startServiceOnServer(int serverIndex, CustomerPacket *customer)
{
    serverIsBusy[serverIndex] = true;
    serverBusySince[serverIndex] = simTime();
    packetInService[serverIndex] = customer;

    simtime_t howLongThisServiceTakes = exponential(meanServiceTime);
    scheduleAt(simTime() + howLongThisServiceTakes, serverFinishEvents[serverIndex]);
}

void QueueingSystemMMCL::handleMessage(cMessage *msg)
{
    // warm-up boundary: forget everything that happened before now
    if (msg == warmupEndedMarker) {
        measurementWindowStarted = true;
        measurementStartInstant = simTime();

        totalArrivals = 0;
        arrivalsRejectedDueToFullSystem = 0;
        customersServedCompletely = 0;
        countOfCustomersThatHadToWait = 0;
        sumOfWaitingTimes = 0.0;
        sumOfWaitingTimesAmongThoseWhoActuallyWaited = 0.0;
        cumulativeBusyServerTime = 0.0;
        cumulativeQueueAreaUnderCurve = 0.0;
        lastQueueChangeInstant = simTime();

        // throw out customers that were still in the queue from the
        // warm-up phase so the measurement window starts from a clean
        // queue.
        while (!waitingLine.empty()) {
            delete waitingLine.front();
            waitingLine.pop();
        }

        // servers that are still busy with a warm-up customer keep going,
        // but we only count their busy time from this moment on.
        for (int i = 0; i < numberOfServers; i++) {
            if (serverIsBusy[i]) {
                serverBusySince[i] = simTime();
            }
        }
        return;
    }

    // service-done event for one of the servers?
    for (int i = 0; i < numberOfServers; i++) {
        if (msg == serverFinishEvents[i]) {
            // log the busy time for this last service spell
            simtime_t busyChunk = simTime() - serverBusySince[i];
            cumulativeBusyServerTime += busyChunk.dbl();

            CustomerPacket *done = packetInService[i];
            packetInService[i] = nullptr;
            send(done, "out");

            // only customers that arrived in the measurement window
            // count for the wait-time averages. warm-up leftovers do
            // not.
            if (measurementWindowStarted &&
                done->getArrivalInstant() >= measurementStartInstant) {
                customersServedCompletely++;
            }

            if (!waitingLine.empty()) {
                rollQueueAreaForward();
                CustomerPacket *nextOne = waitingLine.front();
                waitingLine.pop();

                // a customer that was in the line has by definition
                // waited a positive amount of time.
                simtime_t waitedFor = simTime() - nextOne->getArrivalInstant();
                if (measurementWindowStarted) {
                    sumOfWaitingTimes += waitedFor.dbl();
                    sumOfWaitingTimesAmongThoseWhoActuallyWaited += waitedFor.dbl();
                    countOfCustomersThatHadToWait++;
                }
                startServiceOnServer(i, nextOne);
            } else {
                serverIsBusy[i] = false;
            }
            return;
        }
    }

    // anything left here is a new arrival packet from the source
    CustomerPacket *arriving = check_and_cast<CustomerPacket *>(msg);

    if (measurementWindowStarted) {
        totalArrivals++;
    }

    // count everyone currently inside the system (queued + in service)
    int customersInSystem = (int)waitingLine.size();
    for (int i = 0; i < numberOfServers; i++) {
        if (serverIsBusy[i]) customersInSystem++;
    }

    if (customersInSystem >= maxSystemCapacity) {
        // system is full to the brim, drop this one
        if (measurementWindowStarted) {
            arrivalsRejectedDueToFullSystem++;
        }
        delete arriving;
        return;
    }

    int freeServerIdx = findOneFreeServer();
    if (freeServerIdx >= 0) {
        // a server is free, this customer does not have to wait
        startServiceOnServer(freeServerIdx, arriving);
    } else {
        // all servers busy, customer joins the line
        rollQueueAreaForward();
        waitingLine.push(arriving);
    }
}

void QueueingSystemMMCL::finish()
{
    if (!measurementWindowStarted) {
        EV_WARN << "the simulation finished before the warm-up did, "
                << "no statistics were collected.\n";
        return;
    }

    // close out the queue-area integral up to the very end
    rollQueueAreaForward();

    // any server still mid-service at the end of the run also has
    // some unbooked busy time from its last start.
    for (int i = 0; i < numberOfServers; i++) {
        if (serverIsBusy[i]) {
            simtime_t leftoverBusy = simTime() - serverBusySince[i];
            cumulativeBusyServerTime += leftoverBusy.dbl();
        }
    }

    simtime_t windowDuration = simTime() - measurementStartInstant;
    if (windowDuration <= SIMTIME_ZERO) {
        EV_WARN << "measurement window has zero length, nothing to report.\n";
        return;
    }
    double windowSeconds = windowDuration.dbl();

    double meanQueueLength = cumulativeQueueAreaUnderCurve / windowSeconds;
    double utilizationPerServer = cumulativeBusyServerTime / (numberOfServers * windowSeconds);

    double averageWaitingTime = 0.0;
    if (customersServedCompletely > 0) {
        averageWaitingTime = sumOfWaitingTimes / (double)customersServedCompletely;
    }

    double averageWaitingTimeOfWaiters = 0.0;
    if (countOfCustomersThatHadToWait > 0) {
        averageWaitingTimeOfWaiters =
            sumOfWaitingTimesAmongThoseWhoActuallyWaited / (double)countOfCustomersThatHadToWait;
    }

    double responseTime = averageWaitingTime + meanServiceTime;

    // throughput = effective arrival rate = arrivals that got in / window
    long acceptedArrivals = totalArrivals - arrivalsRejectedDueToFullSystem;
    double throughput = acceptedArrivals / windowSeconds;

    double blockingProbability = 0.0;
    if (totalArrivals > 0) {
        blockingProbability = (double)arrivalsRejectedDueToFullSystem / (double)totalArrivals;
    }

    recordScalar("meanQueueLength_Lq", meanQueueLength);
    recordScalar("throughput", throughput);
    recordScalar("utilizationPerServer_rho", utilizationPerServer);
    recordScalar("averageWaitingTime_Wq", averageWaitingTime);
    recordScalar("averageWaitingTimeOfWaiters_WqGivenWait", averageWaitingTimeOfWaiters);
    recordScalar("responseTime_W", responseTime);
    recordScalar("blockingProbability_PL", blockingProbability);
    recordScalar("totalArrivals", (double)totalArrivals);
    recordScalar("acceptedArrivals", (double)acceptedArrivals);
    recordScalar("arrivalsRejected", (double)arrivalsRejectedDueToFullSystem);
    recordScalar("customersUsedInWaitStats", (double)customersServedCompletely);
    recordScalar("measurementWindowSeconds", windowSeconds);
}

}
