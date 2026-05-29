package mmclqueueingsimulation;

//
// M/M/c/L core. Holds c parallel servers and a waiting line so that
// the total of (in-service + queued) customers never goes past L.
// New arrivals that find the system already full are dropped.
//
// All the performance numbers (E_w, E_w | wait, utilisation, E_m,
// throughput, response time) are collected here and written out as
// scalars when the run ends.
//
simple QueueingSystemMMCL
{
    parameters:
        int numberOfServers = default(1);
        int maxSystemCapacity = default(5);
        double meanServiceTime @unit(s) = default(1s);
        @display("i=block/server");
    gates:
        input in;
        output out;
}
