package mmclqueueingsimulation;

//
// Poisson arrival source. Sends one CustomerPacket out each
// time the next arrival fires. The inter-arrival times come
// from an exponential distribution with mean = meanInterArrivalTime.
//
simple ArrivalEventGenerator
{
    parameters:
        double meanInterArrivalTime @unit(s) = default(1s);
        @display("i=block/source");
    gates:
        output out;
}
