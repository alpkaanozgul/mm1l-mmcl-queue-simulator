#!/usr/bin/env python3
"""
validate.py  -  Compare M/M/1/L simulation results against analytical values.
CNG 436 Wireless Communication and Networks

Usage:
    python3 analysis/validate.py

Reads .sca files from simulations/results/, computes analytical M/M/1/L
performance metrics, and prints a comparison table for each configuration.
All differences should be under 5 % for a run of 100 000 s.
"""

import os
import sys
import glob

# ── M/M/1/L analytical solution ──────────────────────────────────────────────

def mm1l_analytical(lam, mu, L):
    """Return a dict of analytical M/M/1/L performance metrics."""
    rho = lam / mu

    if abs(rho - 1.0) < 1e-10:
        # Special case: rho = 1  →  uniform state probabilities
        P0 = 1.0 / (L + 1)
        P_L = P0
        E_N = L / 2.0
    else:
        denom = 1.0 - rho ** (L + 1)
        P0    = (1.0 - rho) / denom
        P_L   = P0 * rho ** L
        # E[N] = rho/(1-rho) - (L+1)*rho^(L+1)/(1-rho^(L+1))
        E_N   = rho / (1.0 - rho) - (L + 1) * rho ** (L + 1) / denom

    lam_eff = lam * (1.0 - P_L)
    U       = 1.0 - P0
    E_Nq    = E_N - U
    E_T     = E_N / lam_eff   if lam_eff > 0 else 0.0
    E_W     = E_Nq / lam_eff  if lam_eff > 0 else 0.0
    # E[W|W>0] = E[W] / P(W>0).
    # For M/M/1/L with PASTA, P(W>0) = 1 - P0/(1-P_L).
    # The textbook approximation P(W>0) ≈ 1-P0 (valid when P_L ≈ 0) gives:
    #   E[W|W>0] = E[W] / (1-P0)   (formula as specified in the assignment)
    E_W_gt0 = E_W / (1.0 - P0) if (1.0 - P0) > 0 else 0.0

    return {
        'rho':        rho,
        'P0':         P0,
        'P_L':        P_L,
        'lambda_eff': lam_eff,
        'U':          U,
        'E[N]':       E_N,
        'E[Nq]':      E_Nq,
        'E[T]':       E_T,
        'E[W]':       E_W,
        'E[W|W>0]':   E_W_gt0,
    }

# ── .sca file parser ──────────────────────────────────────────────────────────

def parse_sca(path):
    """Parse an OMNeT++ .sca file; return {(module, stat): value}."""
    results = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line.startswith('scalar '):
                parts = line.split(None, 3)
                if len(parts) == 4:
                    _, module, stat_name, value = parts
                    try:
                        results[(module, stat_name)] = float(value)
                    except ValueError:
                        pass
    return results


def find_scalar(data, module_suffix, stat):
    """Return the value whose module ends with *module_suffix* and stat matches."""
    for (mod, sname), v in data.items():
        if mod.endswith(module_suffix) and sname == stat:
            return v
    return None

# ── Configurations and statistic mapping ─────────────────────────────────────

CONFIGS = {
    'MM1L_light':    {'lambda': 1.0,  'mu': 3.0, 'L': 10},
    'MM1L_medium':   {'lambda': 2.0,  'mu': 3.0, 'L': 10},
    'MM1L_heavy':    {'lambda': 2.5,  'mu': 3.0, 'L': 10},
    'MM1L_overload': {'lambda': 4.0,  'mu': 3.0, 'L': 10},
}

# (display_name, module_suffix, sca_stat_name, analytical_key)
STAT_MAP = [
    ('E[W]',       '.server', 'waitingTime:mean',    'E[W]'),
    ('E[W|W>0]',   '.server', 'waitingTimeGT0:mean', 'E[W|W>0]'),
    ('U',          '.server', 'utilisation:timeavg', 'U'),
    ('E[Nq]',      '.server', 'queueLength:timeavg', 'E[Nq]'),
    ('lambda_eff', '.server', 'lambda:eff',           'lambda_eff'),
    ('E[T]',       '.sink',   'responseTime:mean',    'E[T]'),
]

# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    script_dir  = os.path.dirname(os.path.abspath(__file__))
    results_dir = os.path.join(script_dir, '..', 'simulations', 'results')
    sca_files   = glob.glob(os.path.join(results_dir, '*.sca'))

    if not sca_files:
        print(f"No .sca files found in: {os.path.realpath(results_dir)}")
        print("Run the simulations first:")
        print("  ./mm1l-mmcl-queue-simulator simulations/omnetpp.ini -c MM1L_medium -u Cmdenv")
        sys.exit(1)

    # Group .sca files by config name (most recent run per config)
    config_files = {}
    for f in sorted(sca_files):
        for cfg in CONFIGS:
            if os.path.basename(f).startswith(cfg):
                config_files[cfg] = f   # last sorted = most recent

    all_ok = True

    for cfg_name, params in CONFIGS.items():
        lam = params['lambda']
        mu  = params['mu']
        L   = params['L']
        ana = mm1l_analytical(lam, mu, L)

        print("=" * 74)
        print(f"  Config: {cfg_name:<16}  λ={lam}  μ={mu}  L={L}  ρ={ana['rho']:.4f}")
        print(f"  P0={ana['P0']:.6f}   P_L={ana['P_L']:.6f}   "
              f"λ_eff_analytical={ana['lambda_eff']:.4f}")
        print("=" * 74)

        if cfg_name not in config_files:
            print(f"  [MISSING] No .sca result file found for {cfg_name}\n")
            all_ok = False
            continue

        sim_data = parse_sca(config_files[cfg_name])

        hdr = f"  {'Metric':<12}  {'Analytical':>13}  {'Simulation':>13}  {'|Diff|%':>9}"
        sep = "  " + "-" * (len(hdr) - 2)
        print(hdr)
        print(sep)

        for (name, mod_suffix, stat_name, ana_key) in STAT_MAP:
            ana_val = ana[ana_key]
            sim_val = find_scalar(sim_data, mod_suffix, stat_name)

            if sim_val is None:
                print(f"  {name:<12}  {ana_val:>13.6f}  {'N/A':>13}  {'?':>9}")
                all_ok = False
                continue

            if abs(ana_val) > 1e-12:
                diff_pct = abs(sim_val - ana_val) / abs(ana_val) * 100.0
            else:
                diff_pct = abs(sim_val - ana_val) * 100.0

            flag = "  !!!" if diff_pct > 5.0 else ""
            print(f"  {name:<12}  {ana_val:>13.6f}  {sim_val:>13.6f}  "
                  f"{diff_pct:>8.2f}%{flag}")

            if diff_pct > 5.0:
                all_ok = False

        print()

    print("=" * 74)
    if all_ok:
        print("  Validation PASSED  –  all differences within 5 %.")
    else:
        print("  Validation FAILED  –  some differences exceed 5 % or data is missing.")
    print("=" * 74)

    sys.exit(0 if all_ok else 1)


if __name__ == '__main__':
    main()
