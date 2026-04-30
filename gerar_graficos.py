#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Gera gráficos a partir dos resultados dos testes do TP.

Entrada principal esperada:
  test_results/summary.csv

Também aceita, em fallback:
  test_results/resultados.csv

Saída:
  test_results/gráficos/  (ou graficos, conforme o nome da pasta usado no sistema)

Gráficos gerados:
  - turnaround_medio.png
  - response_medio.png
  - justiça_jain.png
  - resumo_metricas.csv
"""

from __future__ import annotations

import csv
import math
import os
from collections import defaultdict
from typing import Dict, Iterable, List, Optional, Tuple

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "test_results")

# O enunciado usa “gráficos”; em Linux isto costuma funcionar sem problemas.
# Se preferires ASCII puro, troca para "graficos".
OUTPUT_DIR = os.path.join(DATA_DIR, "gráficos")

PREFERRED_INPUT_FILES = ["summary.csv", "resultados.csv"]


def pick_input_file() -> str:
    for name in PREFERRED_INPUT_FILES:
        path = os.path.join(DATA_DIR, name)
        if os.path.isfile(path):
            return path
    raise FileNotFoundError(
        "Não encontrei summary.csv nem resultados.csv dentro de test_results/."
    )


def normalize_key(name: str) -> str:
    return name.strip().lower()


def to_float(value: str) -> float:
    if value is None:
        raise ValueError("Valor numérico em falta")
    s = str(value).strip().replace(",", ".")
    if s == "":
        raise ValueError("Valor numérico vazio")
    return float(s)


def to_int(value: str) -> int:
    return int(round(to_float(value)))


def read_rows(csv_path: str) -> List[Dict[str, str]]:
    with open(csv_path, "r", encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)
        rows = []
        for row in reader:
            if not row:
                continue
            cleaned = {normalize_key(k): (v.strip() if isinstance(v, str) else v) for k, v in row.items() if k is not None}
            rows.append(cleaned)
        return rows


def get_first_present(row: Dict[str, str], candidates: Iterable[str], default: Optional[str] = None) -> Optional[str]:
    for key in candidates:
        if key in row and row[key] not in (None, ""):
            return row[key]
    return default


def parse_records(rows: List[Dict[str, str]]) -> List[Dict[str, object]]:
    records: List[Dict[str, object]] = []

    for row in rows:
        policy = get_first_present(row, ["politica", "policy", "sched_policy"])
        parallelism = get_first_present(row, ["paralelismo", "parallelism", "parallel_commands", "parallel-commands"])
        scenario = get_first_present(row, ["cenario", "scenario"])
        user = get_first_present(row, ["user", "user_id", "userid"])
        turnaround = get_first_present(row, ["avg_turnaround_s", "turnaround_time", "turnaround", "avg_turnaround", "turnaround_s"])
        response = get_first_present(row, ["avg_response_s", "response_time", "response", "avg_response", "response_s"])

        if policy is None or parallelism is None or user is None:
            continue

        rec = {
            "policy": str(policy).strip(),
            "parallelism": to_int(str(parallelism)),
            "scenario": str(scenario).strip() if scenario is not None else "",
            "user": str(user).strip(),
            "turnaround": to_float(str(turnaround)) if turnaround is not None else math.nan,
            "response": to_float(str(response)) if response is not None else math.nan,
        }
        records.append(rec)

    if not records:
        raise ValueError(
            "Não consegui extrair registos válidos do CSV. Verifica os nomes das colunas."
        )

    return records


def mean(values: Iterable[float]) -> float:
    vals = [v for v in values if not math.isnan(v)]
    if not vals:
        return math.nan
    return sum(vals) / len(vals)


def aggregate_by(records: List[Dict[str, object]], keys: Tuple[str, ...], value_key: str) -> Dict[Tuple[object, ...], float]:
    buckets: Dict[Tuple[object, ...], List[float]] = defaultdict(list)
    for rec in records:
        value = rec.get(value_key)
        if value is None or (isinstance(value, float) and math.isnan(value)):
            continue
        group = tuple(rec[k] for k in keys)
        buckets[group].append(float(value))
    return {k: mean(v) for k, v in buckets.items()}


def jain_index(values: List[float]) -> float:
    vals = [v for v in values if v > 0 and not math.isnan(v)]
    if not vals:
        return math.nan
    s = sum(vals)
    ss = sum(v * v for v in vals)
    if ss == 0:
        return math.nan
    return (s * s) / (len(vals) * ss)


def safe_filename(name: str) -> str:
    return name.replace("/", "_").replace(" ", "_")


def ensure_output_dir() -> None:
    os.makedirs(OUTPUT_DIR, exist_ok=True)


def policies_in_order(records: List[Dict[str, object]]) -> List[str]:
    preferred = ["fcfs", "rr"]
    seen = []
    all_policies = []
    for rec in records:
        p = str(rec["policy"])
        if p not in all_policies:
            all_policies.append(p)
    ordered = [p for p in preferred if p in all_policies]
    ordered.extend([p for p in all_policies if p not in ordered])
    return ordered


def plot_metric_by_parallelism(records: List[Dict[str, object]], metric: str, ylabel: str, title: str, output_name: str) -> None:
    policies = policies_in_order(records)
    parallelisms = sorted({int(rec["parallelism"]) for rec in records})

    series: Dict[str, List[float]] = {p: [] for p in policies}
    for p in policies:
        for par in parallelisms:
            vals = [float(rec[metric]) for rec in records if str(rec["policy"]) == p and int(rec["parallelism"]) == par and not math.isnan(float(rec[metric]))]
            series[p].append(mean(vals))

    plt.figure(figsize=(9, 5))
    x = list(range(len(parallelisms)))
    width = 0.35 if len(policies) == 2 else max(0.2, 0.8 / max(1, len(policies)))

    for i, p in enumerate(policies):
        offset = (i - (len(policies) - 1) / 2) * width
        plt.bar([xi + offset for xi in x], series[p], width=width, label=p.upper())

    plt.xticks(x, [str(p) for p in parallelisms])
    plt.xlabel("Paralelismo máximo do controller")
    plt.ylabel(ylabel)
    plt.title(title)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, output_name), dpi=200)
    plt.close()


def plot_fairness(records: List[Dict[str, object]], output_name: str) -> None:
    policies = policies_in_order(records)
    parallelisms = sorted({int(rec["parallelism"]) for rec in records})

    fairness_by_policy: Dict[str, List[float]] = {p: [] for p in policies}

    for p in policies:
        for par in parallelisms:
            subset = [rec for rec in records if str(rec["policy"]) == p and int(rec["parallelism"]) == par]
            if not subset:
                fairness_by_policy[p].append(math.nan)
                continue

            # Média de turnaround por utilizador; justiça medida com Jain sobre 1/turnaround.
            per_user: Dict[str, List[float]] = defaultdict(list)
            for rec in subset:
                t = float(rec["turnaround"])
                if not math.isnan(t) and t > 0:
                    per_user[str(rec["user"])].append(t)

            user_means = []
            for user, vals in per_user.items():
                avg_t = mean(vals)
                if avg_t > 0 and not math.isnan(avg_t):
                    user_means.append(1.0 / avg_t)

            fairness_by_policy[p].append(jain_index(user_means))

    plt.figure(figsize=(9, 5))
    x = list(range(len(parallelisms)))
    width = 0.35 if len(policies) == 2 else max(0.2, 0.8 / max(1, len(policies)))

    for i, p in enumerate(policies):
        offset = (i - (len(policies) - 1) / 2) * width
        plt.bar([xi + offset for xi in x], fairness_by_policy[p], width=width, label=p.upper())

    plt.xticks(x, [str(p) for p in parallelisms])
    plt.ylim(0, 1.05)
    plt.xlabel("Paralelismo máximo do controller")
    plt.ylabel("Índice de justiça de Jain")
    plt.title("Justiça entre utilizadores")
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, output_name), dpi=200)
    plt.close()


def write_summary_csv(records: List[Dict[str, object]], output_name: str) -> None:
    policies = policies_in_order(records)
    parallelisms = sorted({int(rec["parallelism"]) for rec in records})

    out_path = os.path.join(OUTPUT_DIR, output_name)
    with open(out_path, "w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            "politica",
            "paralelismo",
            "avg_turnaround_s",
            "avg_response_s",
            "fairness_jain",
            "num_users",
            "num_registos",
        ])

        for p in policies:
            for par in parallelisms:
                subset = [rec for rec in records if str(rec["policy"]) == p and int(rec["parallelism"]) == par]
                if not subset:
                    continue

                avg_turnaround = mean(float(rec["turnaround"]) for rec in subset)
                avg_response = mean(float(rec["response"]) for rec in subset)

                per_user: Dict[str, List[float]] = defaultdict(list)
                for rec in subset:
                    t = float(rec["turnaround"])
                    if not math.isnan(t) and t > 0:
                        per_user[str(rec["user"])].append(t)
                user_means = []
                for vals in per_user.values():
                    avg_t = mean(vals)
                    if avg_t > 0 and not math.isnan(avg_t):
                        user_means.append(1.0 / avg_t)
                fairness = jain_index(user_means)

                writer.writerow([
                    p,
                    par,
                    f"{avg_turnaround:.6f}" if not math.isnan(avg_turnaround) else "",
                    f"{avg_response:.6f}" if not math.isnan(avg_response) else "",
                    f"{fairness:.6f}" if not math.isnan(fairness) else "",
                    len(per_user),
                    len(subset),
                ])


def main() -> None:
    ensure_output_dir()
    input_file = pick_input_file()
    rows = read_rows(input_file)
    records = parse_records(rows)

    plot_metric_by_parallelism(
        records,
        metric="turnaround",
        ylabel="Tempo médio de turnaround (s)",
        title="Comparação do turnaround time: RR vs FCFS",
        output_name="turnaround_medio.png",
    )

    plot_metric_by_parallelism(
        records,
        metric="response",
        ylabel="Tempo médio de resposta (s)",
        title="Comparação do response time médio: RR vs FCFS",
        output_name="response_medio.png",
    )

    plot_fairness(
        records,
        output_name="justica_jain.png",
    )

    write_summary_csv(records, "resumo_metricas.csv")

    print(f"OK: gráficos guardados em {OUTPUT_DIR}")
    print("Ficheiros gerados:")
    print(" - turnaround_medio.png")
    print(" - response_medio.png")
    print(" - justica_jain.png")
    print(" - resumo_metricas.csv")


if __name__ == "__main__":
    main()