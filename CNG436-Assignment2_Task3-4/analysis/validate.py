#!/usr/bin/env python3
"""validate.py

Analytical M/M/c/L closed-form solution and side-by-side comparison
against the OMNeT++ simulation scalars. The .sca files produced by the
two configurations (MMC2Sweep and MMC4Sweep) are read with
``opp_scavetool``, then each measure is paired with its analytical
counterpart and the percentage deviation is printed.

Variable naming used inside this script and in the report (Sec. 5.2):

    rho       = lam / mu             offered load in Erlangs
    u         = rho / c              offered load per server
    P_L                               blocking probability (state-L prob.)
    lambda_eff= lam * (1 - P_L)      effective arrival rate (throughput)
    U         = lambda_eff / (c*mu)   utilisation per server
    E_m       = sum_(n=c+1..L) (n-c) * P_n     mean number in queue
    E_w       = E_m / lambda_eff               mean waiting time (Little)
    E_t       = E_w + 1/mu                     mean response time
    E_w|wait  = E_w / p_wait                   mean wait of those who waited

Fixed parameters for the assignment: mu = 1 customer/s, L = 8.
"""

import csv
import glob
import os
import sys


# --- fixed parameters of the assignment ---
MU = 1.0
L = 8


# --- scalar name (.sca) -> internal key used inside this script ---
SCALAR_TO_KEY = {
    'blockingProbability_PL':                  'P_L',
    'throughput':                              'lambda_eff',
    'utilizationPerServer_rho':                'U',
    'meanQueueLength_Lq':                      'E_m',       # was L_q
    'averageWaitingTime_Wq':                   'E_w',       # was W_q
    'responseTime_W':                          'E_t',       # was W
    'averageWaitingTimeOfWaiters_WqGivenWait': 'E_w|wait',  # was W_q|wait
}


def mmcl_analytical(lam, mu, c, L):
    """Closed-form M/M/c/L performance measures.

    State space n = 0..L. Birth rate is lam for n<L (zero at the cap).
    Death rate is min(n, c)*mu. Returns a dict with the same keys used
    by the report (5.2).
    """
    rho = lam / mu                       # offered load in Erlangs
    u = rho / c                          # offered load per server (not used
                                         # directly in the formulas below,
                                         # kept for the comment column)

    # un-normalised state weights w_n via the standard birth-death recurrence
    w = [0.0] * (L + 1)
    w[0] = 1.0
    for n in range(1, L + 1):
        if n <= c:
            w[n] = w[n - 1] * rho / n        # mu_n = n * mu
        else:
            w[n] = w[n - 1] * rho / c        # mu_n = c * mu (all servers busy)
    S = sum(w)
    p = [wn / S for wn in w]

    P_L_val = p[L]                            # blocking probability
    lambda_eff = lam * (1.0 - P_L_val)        # effective arrival rate
    U = lambda_eff / (c * mu)                 # utilisation per server

    # mean number of customers waiting in the queue (not in service)
    E_m = sum((n - c) * p[n] for n in range(c + 1, L + 1))

    # Little's law on the queue
    E_w = E_m / lambda_eff if lambda_eff > 0 else 0.0

    # response time = wait + service
    E_t = E_w + 1.0 / mu

    # probability that an arrival has to wait, conditional on being accepted
    if (1.0 - P_L_val) > 0:
        p_wait = sum(p[n] for n in range(c, L)) / (1.0 - P_L_val)
    else:
        p_wait = 0.0

    E_w_given_wait = E_w / p_wait if p_wait > 0 else 0.0

    return {
        'P_L':        P_L_val,
        'lambda_eff': lambda_eff,
        'U':          U,
        'E_m':        E_m,
        'E_w':        E_w,
        'E_t':        E_t,
        'E_w|wait':   E_w_given_wait,
    }


# --- I/O helpers ---

def _run_scavetool(sca_glob, out_csv):
    """Invoke opp_scavetool to dump matching .sca files into a flat CSV."""
    cmd = f'opp_scavetool x "{sca_glob}" -o "{out_csv}"'
    rc = os.system(cmd + ' > /dev/null 2>&1')
    if rc != 0:
        raise RuntimeError("opp_scavetool failed for glob " + sca_glob +
                           " (is the OMNeT++ environment sourced?)")


def load_sca_runs(sca_glob):
    """Read all .sca files matching *sca_glob* and return:

        { meanIaT_value(str) : { internal_key : value(float) } }

    where the inner dict contains the seven measures defined in
    SCALAR_TO_KEY.
    """
    tmp_csv = '/tmp/_validate_extract.csv'
    _run_scavetool(sca_glob, tmp_csv)

    rows = list(csv.DictReader(open(tmp_csv)))

    # opp_scavetool stores the iteration variable (meanIaT) on a row of
    # type 'itervar' with attrname=meanIaT, attrvalue=<value>. Build a
    # mapping run -> meanIaT first, then collect scalars.
    iat_for_run = {}
    for r in rows:
        if r.get('type') == 'itervar' and r.get('attrname') == 'meanIaT':
            iat_for_run[r['run']] = r['attrvalue']

    grouped = {}
    for r in rows:
        if r.get('type') != 'scalar':
            continue
        internal_key = SCALAR_TO_KEY.get(r['name'])
        if internal_key is None:
            continue
        iat = iat_for_run.get(r['run'])
        if iat is None:
            continue
        try:
            v = float(r['value'])
        except ValueError:
            continue
        grouped.setdefault(iat, {})[internal_key] = v
    return grouped


# --- reporting ---

ORDER = ['P_L', 'lambda_eff', 'U', 'E_m', 'E_w', 'E_t', 'E_w|wait']

LABELS = {
    'P_L':        'P_L',
    'lambda_eff': 'lam_eff',
    'U':          'U',
    'E_m':        'E_m',
    'E_w':        'E_w',
    'E_t':        'E_t',
    'E_w|wait':   'E_w|wait',
}


NEAR_ZERO_THRESHOLD = 1e-3  # below this the relative percent is unreliable


def fmt_pct(sim, ana):
    """Return the percentage deviation as a string. When the analytical
    value is essentially zero, switch to an absolute error so the figure
    stays meaningful."""
    if abs(ana) < NEAR_ZERO_THRESHOLD:
        return f"({abs(sim - ana):.1e} abs)"
    return f"{abs(sim - ana) / abs(ana) * 100:.2f}%"


def print_table(c, runs_by_iat):
    iats = sorted(runs_by_iat.keys(),
                  key=lambda s: 1.0 / float(s.replace('s', '')))

    print()
    print(f"Config: MMC{c}Sweep   |   mu = {MU}, L = {L}, c = {c}")
    print(f"{'lam':>5} {'u':>5}  {'measure':>9} {'analytical':>12} "
          f"{'simulation':>12}   diff")
    print("-" * 64)

    worst = 0.0
    worst_cell = ''
    for iat in iats:
        lam = 1.0 / float(iat.replace('s', ''))
        u = lam / (c * MU)
        ana = mmcl_analytical(lam, MU, c, L)
        sim = runs_by_iat[iat]
        for key in ORDER:
            a = ana[key]
            s = sim.get(key, float('nan'))
            pct_str = fmt_pct(s, a)
            print(f"{lam:>5.2f} {u:>5.2f}  {LABELS[key]:>9} "
                  f"{a:>12.5f} {s:>12.5f}   {pct_str}")

            # track the worst non-near-zero deviation
            if abs(a) >= NEAR_ZERO_THRESHOLD:
                pct = abs(s - a) / abs(a) * 100
                if pct > worst:
                    worst = pct
                    worst_cell = f"lam={lam:.2f}  {key}"
        print()

    print(f"worst non-near-zero deviation: {worst:.2f}%  at  {worst_cell}")
    print("(5% threshold required by the assignment)")


# --- entry point ---

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    sims_dir = os.path.normpath(os.path.join(
        here, '..', 'mmclQueueingSimulation', 'simulations', 'results'))
    if not os.path.isdir(sims_dir):
        sys.stderr.write(
            "results folder not found at:\n"
            f"  {sims_dir}\n"
            "Build and run the simulation first, e.g.\n"
            "  cd ../mmclQueueingSimulation && make\n"
            "  cd simulations && \\\n"
            "  ../src/out/clang-release/mmclQueueingSimulation"
            " -u Cmdenv -c MMC2Sweep -n ../src\n"
            "  ../src/out/clang-release/mmclQueueingSimulation"
            " -u Cmdenv -c MMC4Sweep -n ../src\n")
        sys.exit(1)

    for c, sweep in [(2, 'MMC2Sweep'), (4, 'MMC4Sweep')]:
        glob_pat = os.path.join(sims_dir, f'{sweep}-*.sca')
        files = sorted(glob.glob(glob_pat))
        if not files:
            print(f"no .sca files for {sweep}, skipping")
            continue
        runs = load_sca_runs(glob_pat)
        print_table(c, runs)


if __name__ == '__main__':
    main()
