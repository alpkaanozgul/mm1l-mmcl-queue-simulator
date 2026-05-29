// message that carries one customer through the system.
// we keep the moment the customer arrived so we can later
// figure out how long it waited and how long it stayed.

namespace mmclqueueingsimulation;

packet CustomerPacket {
    simtime_t arrivalInstant;
}
