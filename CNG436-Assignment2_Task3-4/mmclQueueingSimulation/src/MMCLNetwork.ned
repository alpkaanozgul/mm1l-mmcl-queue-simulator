package mmclqueueingsimulation;

//
// Top-level network. Three pieces wired in a straight line:
//   arrivals --> queueing system --> sink.
// All the per-run knobs (c, L, lambda, mu) come from omnetpp.ini.
//
network MMCLNetwork
{
    submodules:
        arrivalEventGenerator: ArrivalEventGenerator;
        queueingSystem: QueueingSystemMMCL;
        departureCollector: DepartureCollector;
    connections:
        arrivalEventGenerator.out --> queueingSystem.in;
        queueingSystem.out --> departureCollector.in;
}
