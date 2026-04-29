#!/bin/bash
# avaliacao_escalonamento_adaptado.sh
# Versão adaptada: remove Jain, adiciona response time e cenário com chegadas espaçadas

set -euo pipefail

CONTROLLER="./controller"
RUNNER="./runner"
RESULTS_DIR="test_results"
CSV_FILE="${RESULTS_DIR}/resultados.csv"
LOG_FILE="log.txt"

CTRL_PID=""

cleanup() {
    if [ -n "$CTRL_PID" ] && kill -0 "$CTRL_PID" 2>/dev/null; then
        kill "$CTRL_PID" 2>/dev/null
        wait "$CTRL_PID" 2>/dev/null || true
    fi
    CTRL_PID=""
    rm -f tmp/controller_fifo tmp/runner_fifo_* 2>/dev/null || true
}
trap cleanup EXIT

check_binaries() {
    if [ ! -x "$CONTROLLER" ] || [ ! -x "$RUNNER" ]; then
        echo "[avaliacao] Binários não encontrados. A compilar..."
        make --silent
        if [ ! -x "$CONTROLLER" ] || [ ! -x "$RUNNER" ]; then
            echo "[avaliacao] ERRO: compilação falhou."
            exit 1
        fi
        echo "[avaliacao] Compilação concluída."
    fi
}

run_test() {
    local policy=$1
    local parallel=$2
    local scenario=$3

    local label="${policy}_p${parallel}_${scenario}"
    local runner_pids=()

    cleanup
    mkdir -p tmp

    rm -f "$LOG_FILE"

    $CONTROLLER "$parallel" "$policy" > /dev/null 2>&1 &
    CTRL_PID=$!

    local waited=0
    while [ ! -p "tmp/controller_fifo" ] && [ $waited -lt 20 ]; do
        sleep 0.1
        waited=$((waited + 1))
    done

    if [ ! -p "tmp/controller_fifo" ]; then
        echo "  ERRO: controller não arrancou (policy=$policy parallel=$parallel)"
        return 1
    fi

    case "$scenario" in
        "desigual")
            $RUNNER -e 1 "sleep 3" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
            $RUNNER -e 2 "sleep 1" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
            $RUNNER -e 2 "sleep 1" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
            $RUNNER -e 2 "sleep 1" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
            $RUNNER -e 3 "sleep 2" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
            $RUNNER -e 3 "sleep 2" > /dev/null 2>&1 & runner_pids+=($!)
            ;;
        "igual")
            for user in 1 2 3; do
                for i in 1 2 3; do
                    $RUNNER -e "$user" "sleep 1" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
                done
            done
            ;;
        "misto")
            $RUNNER -e 1 "sleep 1" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
            $RUNNER -e 2 "sleep 3" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
            $RUNNER -e 3 "sleep 1" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
            $RUNNER -e 1 "sleep 2" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
            $RUNNER -e 2 "sleep 1" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
            $RUNNER -e 3 "sleep 2" > /dev/null 2>&1 & runner_pids+=($!)
            ;;
        "espacado")
            # Chegadas espaçadas: alguns jobs chegam mais tarde para testar preempção/arrival effects
            $RUNNER -e 1 "sleep 3" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.1
            $RUNNER -e 2 "sleep 1" > /dev/null 2>&1 & runner_pids+=($!); sleep 1.0  # chegada atrasada
            $RUNNER -e 3 "sleep 2" > /dev/null 2>&1 & runner_pids+=($!); sleep 0.5
            $RUNNER -e 1 "sleep 1" > /dev/null 2>&1 & runner_pids+=($!); sleep 1.5  # chegada bem atrasada
            $RUNNER -e 2 "sleep 2" > /dev/null 2>&1 & runner_pids+=($!)
            ;;
        *)
            echo "Cenário desconhecido: $scenario"
            return 1
            ;;
    esac

    for pid in "${runner_pids[@]}"; do
        wait "$pid" 2>/dev/null || true
    done

    $RUNNER -s > /dev/null 2>&1
    wait "$CTRL_PID" 2>/dev/null || true
    CTRL_PID=""

    if [ -f "$LOG_FILE" ]; then
        cp "$LOG_FILE" "${RESULTS_DIR}/${label}.log"
    else
        echo "  AVISO: log.txt não encontrado para ${label}"
    fi
}

analyze_log() {
    local logfile=$1
    local policy=$2
    local parallel=$3
    local scenario=$4

    if [ ! -f "$logfile" ]; then
        printf "  %-5s  p=%-2d  %-8s  (sem dados)\n" "$policy" "$parallel" "$scenario"
        return
    fi

    awk \
        -v policy="$policy"     \
        -v parallel="$parallel" \
        -v scenario="$scenario" \
        -v csv="$CSV_FILE"      \
    '
    # Espera linhas do log com campos como:
    # job=N user=N cmd="..." arrival=TIMESTAMP first_run=TIMESTAMP duration=Ns
    {
        user = ""; dur = ""; arr = ""; first = ""
        for (i = 1; i <= NF; i++) {
            if ($i ~ /^user=/)     { split($i, a, "="); user = a[2] }
            if ($i ~ /^duration=/) { split($i, a, "="); dur  = a[2]; gsub(/s/, "", dur) }
            if ($i ~ /^arrival=/)  { split($i, a, "="); arr  = a[2] }
            if ($i ~ /^first_run=/){ split($i, a, "="); first= a[2] }
        }
        if (user == "" || dur == "") next

        user_sum[user]   += dur
        user_count[user]++
        if (dur > user_max[user]) user_max[user] = dur

        # turnaround contabilizado
        grand_total += dur
        grand_count++

        # response time se disponível
        if (arr != "" && first != "") {
            resp = first - arr
            user_resp_sum[user] += resp
            user_resp_count[user]++
        }
    }
    END {
        if (grand_count == 0) {
            printf "  %-5s  p=%-2d  %-8s  (0 jobs registados)\n",
                   policy, parallel, scenario
            exit
        }

        n_users  = 0
        sum_avgs = 0
        for (u in user_sum) {
            n_users++
            user_avg[u] = user_sum[u] / user_count[u]
            sum_avgs   += user_avg[u]
        }
        global_avg = sum_avgs / n_users

        # Desvio padrão das médias por utilizador (injustiça)
        var = 0
        for (u in user_avg) {
            diff = user_avg[u] - global_avg
            var += diff * diff
        }
        stddev = (n_users > 1) ? sqrt(var / n_users) : 0

        printf "  %-5s  p=%-2d  %-8s  jobs=%-3d  avg_turn=%.2fs  stddev=%.2fs\n",
               policy, parallel, scenario,
               grand_count, grand_total / grand_count, stddev

        for (u in user_avg) {
            # calcular avg response se disponível
            if (user_resp_count[u] > 0) {
                user_resp_avg = user_resp_sum[u] / user_resp_count[u]
            } else {
                user_resp_avg = -1
            }
            printf "               user %-2s : avg_turn=%.2fs  max=%.2fs  (%d jobs)  avg_resp=%s\n",
                   u, user_avg[u], user_max[u], user_count[u],
                   (user_resp_avg >= 0 ? sprintf("%.2f s", user_resp_avg) : "N/A")

            # Escrever uma linha CSV por utilizador
            printf "%s,%d,%s,%s,%.4f,%d,%.4f,%s\n",
                   policy, parallel, scenario, u,
                   user_avg[u], user_count[u], user_max[u],
                   (user_resp_avg >= 0 ? sprintf("%.4f", user_resp_avg) : "N/A") \
                   >> csv
        }
    }
    ' "$logfile"
}

print_comparison() {
    echo ""
    echo "════════════════════════════════════════════════════════════"
    echo "  Comparação FCFS vs RR  (stddev dos utilizadores)"
    echo "════════════════════════════════════════════════════════════"
    printf "  %-8s  p=%-2s  %-8s  %-10s  %-10s  Vencedor\n" \
           "Cenário" "P" "---" "FCFS stddev" "RR stddev"
    echo "  ──────────────────────────────────────────────────────────"

    local scenarios=("desigual" "igual" "misto" "espacado")
    local parallels=(1 2 4)

    for scenario in "${scenarios[@]}"; do
        for parallel in "${parallels[@]}"; do
            fcfs_stddev=$(grep "^fcfs,${parallel},${scenario}," "$CSV_FILE" 2>/dev/null \
                | awk -F',' 'NR==1{print $8}' || echo "?")
            rr_stddev=$(grep "^rr,${parallel},${scenario}," "$CSV_FILE" 2>/dev/null \
                | awk -F',' 'NR==1{print $8}' || echo "?")

            winner="="
            if [ "$fcfs_stddev" != "?" ] && [ "$rr_stddev" != "?" ]; then
                winner=$(awk -v f="$fcfs_stddev" -v r="$rr_stddev" \
                    'BEGIN{ if(f+0 < r+0) print "FCFS"; else if(r+0 < f+0) print "RR"; else print "=" }')
            fi

            printf "  %-8s  p=%-2d  %-8s  %-10s  %-10s  %s\n" \
                   "$scenario" "$parallel" "---" \
                   "${fcfs_stddev}" "${rr_stddev}" "$winner"
        done
    done
}

main() {
    check_binaries
    mkdir -p "$RESULTS_DIR" tmp
    rm -f "$CSV_FILE"

    echo "politica,paralelismo,cenario,user,avg_turnaround_s,num_jobs,max_turnaround_s,avg_response_s" \
        > "$CSV_FILE"

    local policies=("fcfs" "rr")
    local parallels=(1 2 4)
    local scenarios=("desigual" "igual" "misto" "espacado")

    echo ""
    echo "════════════════════════════════════════════════════════════"
    echo "  Avaliação de Políticas de Escalonamento (adaptado)"
    echo "  Políticas : FCFS, Round-Robin"
    echo "  Paralelismo: 1, 2, 4 comandos simultâneos"
    echo "════════════════════════════════════════════════════════════"

    for scenario in "${scenarios[@]}"; do
        echo ""
        echo "── Cenário: $scenario ─────────────────────────────────────"
        for policy in "${policies[@]}"; do
            for parallel in "${parallels[@]}"; do
                printf "  A correr %-5s p=%-2d %-8s ...\r" \
                       "$policy" "$parallel" "$scenario"
                run_test "$policy" "$parallel" "$scenario"
                analyze_log \
                    "${RESULTS_DIR}/${policy}_p${parallel}_${scenario}.log" \
                    "$policy" "$parallel" "$scenario"
                sleep 0.3
            done
        done
    done

    print_comparison

    echo ""
    printf "  Logs individuais : %s/*.log\n" "$RESULTS_DIR"
    printf "  CSV completo     : %s\n" "$CSV_FILE"
    echo ""
}

main
