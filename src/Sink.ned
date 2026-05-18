//
// Sink: absorbs completed jobs and records end-to-end response time E[T].
//
simple Sink
{
    parameters:
        @display("i=block/sink");

        @signal[responseTime](type=double);

        @statistic[responseTime](title="response time"; source=responseTime; unit=s; record=mean,vector; checkSignals=false);

    gates:
        input in;
}
