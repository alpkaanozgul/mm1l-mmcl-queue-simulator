//
// Server: M/M/1/L single-server FIFO queue with finite system capacity L.
// Arrivals when the system holds L customers are blocked and dropped.
// Service times: Exp(mu) via inverse transform: -ln(U)/mu.
//
simple Server
{
    parameters:
        int    capacity = default(10);  // L: max customers in system (queue + server)
        double mu       = default(3.0); // service rate (jobs/s)

        @display("i=block/queue");

        @signal[waitingTime](type=double);
        @signal[queueLength](type=long);
        @signal[utilisation](type=double);
        @signal[systemLength](type=long);

        @statistic[waitingTime](title="waiting time";   source=waitingTime;  unit=s; record=mean,vector;       checkSignals=false);
        @statistic[queueLength](title="queue length";   source=queueLength;          record=mean,timeavg,vector; checkSignals=false);
        @statistic[utilisation](title="utilisation";    source=utilisation;          record=mean,timeavg;       checkSignals=false);
        @statistic[systemLength](title="system length"; source=systemLength;         record=mean,timeavg,vector; checkSignals=false);

    gates:
        input  in;
        output out;
}
