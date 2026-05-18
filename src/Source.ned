//
// Source: Poisson arrival process.
// Inter-arrival times are Exp(1/lambda) via inverse transform: -ln(U)/lambda.
//
simple Source
{
    parameters:
        double lambda = default(1.0);   // arrival rate (jobs/s)
        @display("i=block/source");
    gates:
        output out;
}
