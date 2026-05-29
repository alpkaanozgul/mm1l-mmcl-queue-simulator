package mmclqueueingsimulation;

//
// Sink at the end of the chain. Counts the customers that left and
// throws the packets away. The interesting numbers live in the
// QueueingSystemMMCL module, this one just gives the network a tail.
//
simple DepartureCollector
{
    parameters:
        @display("i=block/sink");
    gates:
        input in;
}
