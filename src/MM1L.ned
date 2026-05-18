//
// MM1L: M/M/1/L finite-capacity queueing system.
// CNG 436 Wireless Communication and Networks
//
// Topology: Source  -->  Server (M/M/1/L queue)  -->  Sink
//
// Source generates Poisson arrivals (rate lambda).
// Server queues and serves jobs (rate mu), dropping arrivals when
// the system (queue + server) already holds L customers.
// Sink absorbs completed jobs and records response time.
//
network MM1L
{
    parameters:
        @display("bgb=600,300");

    submodules:
        source: Source {
            @display("p=100,150");
        }
        server: Server {
            @display("p=300,150");
        }
        sink: Sink {
            @display("p=500,150");
        }

    connections:
        source.out --> server.in;
        server.out --> sink.in;
}
