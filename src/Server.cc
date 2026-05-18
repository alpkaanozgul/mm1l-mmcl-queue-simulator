#include "Server.h"

Define_Module(Server);

void Server::initialize()
{
    capacity      = par("capacity");
    mu            = par("mu").doubleValue();
    serverBusy    = false;
    jobInService  = nullptr;
    serviceEndEvent = new cMessage("serviceEnd");

    resetCounters();

    waitingTimeSignal  = registerSignal("waitingTime");
    queueLengthSignal  = registerSignal("queueLength");
    utilisationSignal  = registerSignal("utilisation");
    systemLengthSignal = registerSignal("systemLength");

    emit(queueLengthSignal,  0L);
    emit(utilisationSignal,  0.0);
    emit(systemLengthSignal, 0L);

    // Schedule counter reset at the end of the warmup period so that
    // manually accumulated scalars cover only the post-warmup window.
    SimTime wp = getSimulation()->getWarmupPeriod();
    if (wp > SimTime::ZERO) {
        warmupResetEvent = new cMessage("warmupReset");
        scheduleAt(wp, warmupResetEvent);
    } else {
        warmupResetEvent = nullptr;
    }

    EV << "[Server] initialized. capacity=" << capacity
       << " mu=" << mu << "/s" << endl;
}

void Server::resetCounters()
{
    totalArrived = 0;
    totalDropped = 0;
    totalServed  = 0;
    countWaited  = 0;
    sumWaitGT0   = 0.0;
}

void Server::handleMessage(cMessage *msg)
{
    if (msg == warmupResetEvent) {
        resetCounters();
        EV << "[Server] Warmup ended. Counters reset." << endl;
        delete warmupResetEvent;
        warmupResetEvent = nullptr;
        return;
    }

    if (msg == serviceEndEvent) {
        endService();
        return;
    }

    Job *job = check_and_cast<Job *>(msg);
    totalArrived++;

    if (systemSize() >= capacity) {
        totalDropped++;
        EV << "[Server] DROP (system full, L=" << capacity
           << "). drops=" << totalDropped << endl;
        delete job;
        return;
    }

    if (!serverBusy) {
        startService(job);
    } else {
        jobQueue.push(job);
        emit(queueLengthSignal,  (long)jobQueue.size());
        emit(systemLengthSignal, (long)systemSize());
        EV << "[Server] Queued. queue=" << jobQueue.size() << endl;
    }
}

void Server::startService(Job *job)
{
    serverBusy   = true;
    jobInService = job;

    double waitTime = simTime().dbl() - job->getArrivalTime();
    emit(waitingTimeSignal,  waitTime);
    emit(utilisationSignal,  1.0);
    emit(systemLengthSignal, (long)systemSize());

    if (waitTime > 0.0) {
        countWaited++;
        sumWaitGT0 += waitTime;
    }

    double svcTime = -std::log(dblrand()) / mu;
    scheduleAt(simTime() + svcTime, serviceEndEvent);

    EV << "[Server] Service started. wait=" << waitTime
       << "s svc=" << svcTime << "s" << endl;
}

void Server::endService()
{
    ASSERT(jobInService != nullptr);
    Job *job  = jobInService;
    jobInService = nullptr;

    send(job, "out");
    totalServed++;

    EV << "[Server] Service done. total served=" << totalServed << endl;

    if (!jobQueue.empty()) {
        Job *next = jobQueue.front();
        jobQueue.pop();
        emit(queueLengthSignal, (long)jobQueue.size());
        startService(next);
    } else {
        serverBusy = false;
        emit(utilisationSignal,  0.0);
        emit(queueLengthSignal,  0L);
        emit(systemLengthSignal, 0L);
    }
}

void Server::finish()
{
    SimTime wp          = getSimulation()->getWarmupPeriod();
    double  simDuration = (simTime() - wp).dbl();

    recordScalar("totalArrived", totalArrived);
    recordScalar("totalDropped", totalDropped);
    recordScalar("totalServed",  totalServed);

    double lambdaEff    = (simDuration > 0) ? (double)totalServed / simDuration : 0.0;
    double blockingProb = (totalArrived > 0) ? (double)totalDropped / totalArrived : 0.0;
    double waitGT0Mean  = (countWaited > 0)  ? sumWaitGT0 / countWaited : 0.0;

    recordScalar("lambda:eff",         lambdaEff);
    recordScalar("blocking:prob",      blockingProb);
    recordScalar("waitingTimeGT0:mean", waitGT0Mean);

    EV << "[Server] finish(): arrived=" << totalArrived
       << " served=" << totalServed << " dropped=" << totalDropped
       << " lambdaEff=" << lambdaEff
       << " E[W|W>0]=" << waitGT0Mean << endl;

    cancelAndDelete(serviceEndEvent);
    serviceEndEvent = nullptr;

    if (warmupResetEvent) {
        cancelAndDelete(warmupResetEvent);
        warmupResetEvent = nullptr;
    }

    if (jobInService) {
        delete jobInService;
        jobInService = nullptr;
    }
    while (!jobQueue.empty()) {
        delete jobQueue.front();
        jobQueue.pop();
    }
}
