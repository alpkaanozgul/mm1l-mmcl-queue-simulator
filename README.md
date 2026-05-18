# M/M/1/L Queue Simulator

OMNeT++ 6.2 discrete-event simulation of an M/M/1/L finite-capacity queue.  
**CNG 436 Wireless Communication and Networks – METU Northern Cyprus Campus**

## Model

```
Source  ──►  Server (M/M/1/L)  ──►  Sink
```

| Parameter | Value |
|-----------|-------|
| Arrival process | Poisson (rate λ) |
| Service process | Exponential (rate μ = 3 jobs/s) |
| Servers | 1 |
| System capacity | L = 10 (queue + server) |
| Blocking policy | Arriving jobs dropped when system holds L customers |

## Build

```bash
# Source OMNeT++ environment first (if not already in .bashrc)
source /path/to/omnetpp-6.2.0/setenv

make MODE=release
```

## Run

```bash
# Single configuration
./mm1l-mmcl-queue-simulator simulations/omnetpp.ini -c MM1L_medium -u Cmdenv

# All four configurations
for cfg in MM1L_light MM1L_medium MM1L_heavy MM1L_overload; do
    ./mm1l-mmcl-queue-simulator simulations/omnetpp.ini -c $cfg -u Cmdenv
done
```

## Configurations

| Config | λ | ρ = λ/μ | Load |
|--------|---|---------|------|
| MM1L_light | 1.0 | 0.333 | Light |
| MM1L_medium | 2.0 | 0.667 | Medium |
| MM1L_heavy | 2.5 | 0.833 | Heavy |
| MM1L_overload | 4.0 | 1.333 | Overload |

Simulation time: 100 000 s | Warm-up: 1 000 s

## Validate

```bash
python3 analysis/validate.py
```

Reads `.sca` result files from `simulations/results/`, computes analytical
M/M/1/L values, and prints a comparison table. All differences should be < 5 %.

## Statistics Collected

| Metric | Source | OMNeT++ stat name |
|--------|--------|-------------------|
| E[W] | Server signal | `waitingTime:mean` |
| E[W\|W>0] | Server `recordScalar` | `waitingTimeGT0:mean` |
| U = 1 − P₀ | Server signal | `utilisation:timeavg` |
| E[Nq] | Server signal | `queueLength:timeavg` |
| λ_eff | Server `recordScalar` | `lambda:eff` |
| E[T] | Sink signal | `responseTime:mean` |

## Project Structure

```
mm1l-mmcl-queue-simulator/
├── src/          NED + C++ source files
├── msg/          Job.msg  (opp_msgc generates Job_m.cc / Job_m.h)
├── simulations/  omnetpp.ini + results/
├── analysis/     validate.py
├── handwritten/  scanned derivation notes
└── Makefile
```
