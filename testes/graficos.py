#!/usr/bin/env python3
"""
graficos.py  —  Analisa os logs do teste.sh e produz gráficos comparativos.
"""

import os
import re
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
from matplotlib import rcParams

# ─── tema global ──────────────────────────────────────────────────────────────
rcParams.update({
    "font.family"       : "DejaVu Sans",
    "font.size"         : 10,
    "axes.spines.top"   : False,
    "axes.spines.right" : False,
    "axes.grid"         : True,
    "axes.grid.axis"    : "y",
    "grid.alpha"        : 0.35,
    "grid.linestyle"    : "--",
    "figure.facecolor"  : "#F7F9FC",
    "axes.facecolor"    : "#F7F9FC",
    "axes.titlepad"     : 10,
})

# ─── paleta ───────────────────────────────────────────────────────────────────
COR        = {"fcfs": "#3A6EBF", "rr": "#E8714A"}
COR_LIGHT  = {"fcfs": "#A8BFE8", "rr": "#F5C3AE"}
USER_CORES = ["#E8714A", "#3A6EBF", "#44A869"]

# ─── configuração ─────────────────────────────────────────────────────────────
DADOS_DIR    = "testes/dados"
GRAFICOS_DIR = "testes/graficos"
PARALELOS    = [1, 2, 10]
POLITICAS    = ["fcfs", "rr"]
CENARIOS     = ["A_homogeneo", "B_heterogeneo"]

os.makedirs(GRAFICOS_DIR, exist_ok=True)

# ─── parser ───────────────────────────────────────────────────────────────────
LINE_RE = re.compile(
    r"job=(?P<job>\d+)\s+user=(?P<user>\d+)\s+cmd=(?P<cmd>.+?)\s+"
    r"arrival=(?P<arrival>[\d.]+)\s+first_run=(?P<first_run>[\d.]+)\s+"
    r"duration=(?P<duration>[\d.]+)s"
)

def parse_log(path):
    jobs = []
    if not os.path.exists(path):
        return jobs
    with open(path) as f:
        for line in f:
            m = LINE_RE.search(line)
            if m:
                jobs.append({
                    "job_id"    : int(m.group("job")),
                    "user_id"   : int(m.group("user")),
                    "arrival"   : float(m.group("arrival")),
                    "first_run" : float(m.group("first_run")),
                    "duration"  : float(m.group("duration")),
                })
    return jobs

def compute_metrics(jobs):
    if not jobs:
        return None
    response_times   = [j["first_run"] - j["arrival"] for j in jobs]
    turnaround_times = [j["duration"] for j in jobs]
    t_start    = min(j["arrival"] for j in jobs)
    t_end      = max(j["arrival"] + j["duration"] for j in jobs)
    total_time = max(t_end - t_start, 0.001)
    throughput = len(jobs) / total_time
    per_user   = defaultdict(list)
    for j in jobs:
        per_user[j["user_id"]].append(j["duration"])
    user_means      = [np.mean(v) for v in per_user.values()]
    fairness        = np.std(user_means) if len(user_means) > 1 else 0.0
    user_turnaround = {uid: np.mean(vals) for uid, vals in per_user.items()}
    return {
        "response_mean"   : np.mean(response_times),
        "turnaround_mean" : np.mean(turnaround_times),
        "throughput"      : throughput,
        "fairness"        : fairness,
        "user_turnaround" : user_turnaround,
        "n_jobs"          : len(jobs),
    }

# ─── carregar dados ───────────────────────────────────────────────────────────
data = {c: {p: {} for p in POLITICAS} for c in CENARIOS}

for cenario in CENARIOS:
    for pol in POLITICAS:
        for par in PARALELOS:
            path = os.path.join(DADOS_DIR, f"{cenario}_p{par}_{pol}.log")
            jobs = parse_log(path)
            m    = compute_metrics(jobs)
            if m:
                data[cenario][pol][par] = m
                print(f"[OK] {cenario} p{par} {pol}: {m['n_jobs']} jobs")
            else:
                print(f"[AVISO] sem dados: {path}")

# ─── utilitário: barras agrupadas estilizadas ─────────────────────────────────
def bar_chart(ax, cenario, metric_key, title, ylabel, note=None, lower_is_better=False):
    x     = np.arange(len(PARALELOS))
    width = 0.32

    for i, pol in enumerate(POLITICAS):
        vals = [data[cenario][pol].get(par, {}).get(metric_key, 0)
                for par in PARALELOS]
        xpos = x + i * width
        # barra com borda suave
        bars = ax.bar(xpos, vals, width,
                      label=pol.upper(),
                      color=COR[pol],
                      edgecolor="white",
                      linewidth=0.8,
                      zorder=3)
        # rótulos das barras
        for bar, v in zip(bars, vals):
            ax.text(bar.get_x() + bar.get_width() / 2,
                    bar.get_height() + max(vals) * 0.02,
                    f"{v:.2f}",
                    ha="center", va="bottom",
                    fontsize=7.5, fontweight="bold",
                    color="#333333")

    ax.set_title(title, fontsize=10.5, fontweight="bold", color="#222222")
    ax.set_xlabel("Paralelismo (slots)", fontsize=9, color="#555555")
    ax.set_ylabel(ylabel, fontsize=9, color="#555555")
    ax.set_xticks(x + width / 2)
    ax.set_xticklabels([f"{p} slot{'s' if p>1 else ''}" for p in PARALELOS], fontsize=9)
    ax.tick_params(axis="both", length=0)
    ax.set_ylim(0, ax.get_ylim()[1] * 1.18)

    if note:
        ax.text(0.98, 0.97, note, transform=ax.transAxes,
                fontsize=7.5, va="top", ha="right",
                color="#888888", style="italic")

    # legenda minimalista
    patches = [mpatches.Patch(color=COR[p], label=p.upper()) for p in POLITICAS]
    ax.legend(handles=patches, fontsize=8.5, frameon=False,
              loc="upper right", handlelength=1.2)

# ─── gráficos resumo 2×2 ─────────────────────────────────────────────────────
METRICAS = [
    ("response_mean",   "Response Time",      "segundos",   None,              False),
    ("turnaround_mean", "Turnaround Time",     "segundos",   None,              False),
    ("throughput",      "Throughput",          "jobs / s",   None,              False),
    ("fairness",        "Fairness",            "desvio-padrão (s)", "↓ melhor", True),
]

TITULO_CENARIO = {
    "A_homogeneo"  : "Cenário A — Homogéneo  (todos os utilizadores com jobs ~2 s)",
    "B_heterogeneo": "Cenário B — Heterogéneo  (utilizador 1 longo · 2 médio · 3 curto)",
}

for cenario in CENARIOS:
    fig, axes = plt.subplots(2, 2, figsize=(14, 9))
    fig.patch.set_facecolor("#F7F9FC")

    # título geral
    fig.suptitle(
        f"FCFS  vs  RR\n{TITULO_CENARIO[cenario]}",
        fontsize=13, fontweight="bold", color="#111111", y=1.01
    )

    for ax, (key, label, unit, note, lib) in zip(axes.flat, METRICAS):
        bar_chart(ax, cenario, key, label, unit, note, lib)

    # linha separadora decorativa entre os dois subplots de cima e de baixo
    fig.tight_layout(rect=[0, 0, 1, 1], h_pad=3.5, w_pad=2.5)
    out = os.path.join(GRAFICOS_DIR, f"resumo_{cenario}.png")
    fig.savefig(out, dpi=160, bbox_inches="tight", facecolor=fig.get_facecolor())
    plt.close(fig)
    print(f"[gráfico] {out}")

# ─── fairness por utilizador — Cenário B, paralelismo=1 ──────────────────────
cenario = "B_heterogeneo"
par     = 1

fig, axes = plt.subplots(1, 2, figsize=(13, 6))
fig.patch.set_facecolor("#F7F9FC")
fig.suptitle(
    "Turnaround médio por utilizador  —  Cenário B  (paralelismo = 1 slot)\n"
    "Utilizador 1 = jobs longos (5 s)   ·   Utilizador 2 = médios (2 s)   ·   Utilizador 3 = curtos (0.2 s)",
    fontsize=11, fontweight="bold", color="#111111", y=1.02
)

USER_LABELS = {1: "User 1\n(longo)", 2: "User 2\n(médio)", 3: "User 3\n(curto)"}

for ax, pol in zip(axes, POLITICAS):
    m = data[cenario][pol].get(par)
    if m is None:
        ax.set_title(f"{pol.upper()} — sem dados")
        continue

    ut     = m["user_turnaround"]
    users  = sorted(ut.keys())
    vals   = [ut[u] for u in users]
    labels = [USER_LABELS.get(u, f"User {u}") for u in users]

    bars = ax.bar(labels, vals,
                  color=USER_CORES[:len(users)],
                  edgecolor="white", linewidth=0.8,
                  width=0.45, zorder=3)

    for bar, v in zip(bars, vals):
        ax.text(bar.get_x() + bar.get_width() / 2,
                bar.get_height() + max(vals) * 0.02,
                f"{v:.2f} s",
                ha="center", va="bottom",
                fontsize=9, fontweight="bold", color="#333333")

    ax.set_title(pol.upper(), fontsize=13, fontweight="bold",
                 color=COR[pol], pad=10)
    ax.set_ylabel("Turnaround médio (s)", fontsize=9, color="#555555")
    ax.set_ylim(0, max(vals) * 1.28)
    ax.tick_params(axis="both", length=0)

    # desvio-padrão como anotação
    std = m["fairness"]
    ax.text(0.97, 0.95, f"std = {std:.2f} s",
            transform=ax.transAxes, fontsize=9,
            va="top", ha="right",
            color="#888888", style="italic",
            bbox=dict(boxstyle="round,pad=0.3", fc="white", ec="#cccccc", alpha=0.8))

fig.tight_layout(rect=[0, 0, 1, 1], w_pad=3)
out = os.path.join(GRAFICOS_DIR, "fairness_por_utilizador_B_p1.png")
fig.savefig(out, dpi=160, bbox_inches="tight", facecolor=fig.get_facecolor())
plt.close(fig)
print(f"[gráfico] {out}")

# ─── fairness comparativo A vs B ─────────────────────────────────────────────
fig, axes = plt.subplots(1, 2, figsize=(13, 6))
fig.patch.set_facecolor("#F7F9FC")
fig.suptitle(
    "Fairness — desvio-padrão do turnaround por utilizador  (↓ mais justo)\n"
    "Comparação entre os dois cenários",
    fontsize=12, fontweight="bold", color="#111111", y=1.02
)

TITULO_CURTO = {
    "A_homogeneo"  : "Cenário A — Homogéneo",
    "B_heterogeneo": "Cenário B — Heterogéneo",
}

for ax, cenario in zip(axes, CENARIOS):
    x     = np.arange(len(PARALELOS))
    width = 0.32

    for i, pol in enumerate(POLITICAS):
        vals = [data[cenario][pol].get(par, {}).get("fairness", 0)
                for par in PARALELOS]
        xpos = x + i * width
        bars = ax.bar(xpos, vals, width,
                      label=pol.upper(),
                      color=COR[pol],
                      edgecolor="white", linewidth=0.8,
                      zorder=3)
        for bar, v in zip(bars, vals):
            ax.text(bar.get_x() + bar.get_width() / 2,
                    bar.get_height() + max(vals) * 0.02,
                    f"{v:.2f}",
                    ha="center", va="bottom",
                    fontsize=7.5, fontweight="bold", color="#333333")

    ax.set_title(TITULO_CURTO[cenario], fontsize=11, fontweight="bold",
                 color="#222222")
    ax.set_xlabel("Paralelismo (slots)", fontsize=9, color="#555555")
    ax.set_ylabel("Desvio-padrão (s)", fontsize=9, color="#555555")
    ax.set_xticks(x + width / 2)
    ax.set_xticklabels([f"{p} slot{'s' if p>1 else ''}" for p in PARALELOS], fontsize=9)
    ax.tick_params(axis="both", length=0)
    ax.set_ylim(0, ax.get_ylim()[1] * 1.2)

    patches = [mpatches.Patch(color=COR[p], label=p.upper()) for p in POLITICAS]
    ax.legend(handles=patches, fontsize=8.5, frameon=False, loc="upper right")

fig.tight_layout(rect=[0, 0, 1, 1], w_pad=3)
out = os.path.join(GRAFICOS_DIR, "fairness_comparativo.png")
fig.savefig(out, dpi=160, bbox_inches="tight", facecolor=fig.get_facecolor())
plt.close(fig)
print(f"[gráfico] {out}")

print(f"\nConcluído. Gráficos em: {GRAFICOS_DIR}/")