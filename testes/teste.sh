#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
# teste.sh  —  Testa FCFS vs RR em dois cenários de carga
#
#   Cenário A (homogéneo):   3 utilizadores com jobs de duração semelhante
#   Cenário B (heterogéneo): utilizador 1 = jobs longos
#                            utilizador 2 = jobs médios
#                            utilizador 3 = jobs curtos
#
#   Paralelismo testado: 1, 2, 10
#   Políticas testadas:  fcfs, rr
# ─────────────────────────────────────────────────────────────────────────────

# ─── ir sempre para a raiz do projeto ────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR/.."

CONTROLLER="./controller"
RUNNER="./runner"
LOG="log.txt"
DADOS_DIR="testes/dados"
SERVER_FIFO="tmp/controller_fifo"

PARALELOS=(1 2 10)
POLITICAS=("fcfs" "rr")

# ─── verificações iniciais ────────────────────────────────────────────────────
if [ ! -x "$CONTROLLER" ]; then
    echo "[ERRO] $CONTROLLER não encontrado. Corre 'make' primeiro."
    exit 1
fi
if [ ! -x "$RUNNER" ]; then
    echo "[ERRO] $RUNNER não encontrado."
    exit 1
fi

mkdir -p tmp "$DADOS_DIR"

# ─────────────────────────────────────────────────────────────────────────────
# CENÁRIO A — Homogéneo (todos os utilizadores com jobs ~2s)
# 3 utilizadores × 10 jobs = 30 jobs
# ─────────────────────────────────────────────────────────────────────────────
JOBS_A=(
    # utilizador 1 — 10 jobs de ~2s
    "1 sleep 2"
    "1 sleep 2"
    "1 sleep 2"
    "1 sleep 2"
    "1 sleep 2"
    "1 sleep 2"
    "1 sleep 2"
    "1 sleep 2"
    "1 sleep 2"
    "1 sleep 2"
    # utilizador 2 — 10 jobs de ~2s
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    # utilizador 3 — 10 jobs de ~2s
    "3 sleep 2"
    "3 sleep 2"
    "3 sleep 2"
    "3 sleep 2"
    "3 sleep 2"
    "3 sleep 2"
    "3 sleep 2"
    "3 sleep 2"
    "3 sleep 2"
    "3 sleep 2"
)

# ─────────────────────────────────────────────────────────────────────────────
# CENÁRIO B — Heterogéneo
#   utilizador 1 = jobs longos  (~5s) × 10
#   utilizador 2 = jobs médios  (~2s) × 10
#   utilizador 3 = jobs curtos  (~0.2s) × 10
# ─────────────────────────────────────────────────────────────────────────────
JOBS_B=(
    # utilizador 1 — jobs longos
    "1 sleep 5"
    "1 sleep 5"
    "1 sleep 5"
    "1 sleep 5"
    "1 sleep 5"
    "1 sleep 5"
    "1 sleep 5"
    "1 sleep 5"
    "1 sleep 5"
    "1 sleep 5"
    # utilizador 2 — jobs médios
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    "2 sleep 2"
    # utilizador 3 — jobs curtos
    "3 sleep 0.2"
    "3 sleep 0.2"
    "3 sleep 0.2"
    "3 sleep 0.2"
    "3 sleep 0.2"
    "3 sleep 0.2"
    "3 sleep 0.2"
    "3 sleep 0.2"
    "3 sleep 0.2"
    "3 sleep 0.2"
)

# ─── esperar que o FIFO do controller apareça ─────────────────────────────────
wait_for_fifo() {
    local retries=30
    while [ ! -p "$SERVER_FIFO" ] && [ $retries -gt 0 ]; do
        sleep 0.2
        retries=$((retries - 1))
    done
    if [ ! -p "$SERVER_FIFO" ]; then
        echo "[ERRO] Controller não criou o FIFO a tempo."
        kill "$CTRL_PID" 2>/dev/null
        exit 1
    fi
}

# ─── correr um cenário ────────────────────────────────────────────────────────
# Argumentos: parallel  policy  label  jobs_array_name
run_scenario() {
    local parallel=$1
    local policy=$2
    local label=$3
    shift 3
    local jobs=("$@")

    echo ""
    echo "════════════════════════════════════════════════════"
    echo "  $label  |  paralelo=$parallel  política=$policy"
    echo "════════════════════════════════════════════════════"

    rm -f "$LOG" "$SERVER_FIFO"

    # 1. Arrancar controller
    $CONTROLLER "$parallel" "$policy" &
    CTRL_PID=$!

    # 2. Esperar FIFO
    wait_for_fifo
    echo "[teste] Controller arrancou (PID=$CTRL_PID)"

    # 3. Submeter jobs — intercalados por utilizador para maior realismo
    RUNNER_PIDS=()
    for job in "${jobs[@]}"; do
        uid=$(echo "$job" | awk '{print $1}')
        cmd=$(echo "$job" | cut -d' ' -f2-)
        $RUNNER -e "$uid" "$cmd" &
        RUNNER_PIDS+=($!)
        sleep 0.05   # intervalo pequeno entre submissões
    done

    # 4. Esperar apenas pelos runners
    for pid in "${RUNNER_PIDS[@]}"; do
        wait "$pid" 2>/dev/null
    done
    echo "[teste] Todos os runners terminaram"

    # 5. Shutdown
    $RUNNER -s
    wait "$CTRL_PID" 2>/dev/null
    echo "[teste] Controller terminou"

    # 6. Guardar log
    local logfile="${DADOS_DIR}/${label}_p${parallel}_${policy}.log"
    if [ -f "$LOG" ]; then
        cp "$LOG" "$logfile"
        echo "[teste] Log: $logfile"
    else
        echo "[AVISO] log.txt não encontrado para $label p$parallel $policy"
    fi
}

# ─── executar todos os cenários ──────────────────────────────────────────────
for pol in "${POLITICAS[@]}"; do
    for par in "${PARALELOS[@]}"; do

        # Cenário A — Homogéneo
        run_scenario "$par" "$pol" "A_homogeneo" "${JOBS_A[@]}"
        sleep 1

        # Cenário B — Heterogéneo
        run_scenario "$par" "$pol" "B_heterogeneo" "${JOBS_B[@]}"
        sleep 1

    done
done

echo ""
echo "════════════════════════════════════════════════════"
echo "  Todos os cenários terminados."
echo "  Logs em: $DADOS_DIR/"
echo "  Corre: python3 testes/graficos.py"
echo "════════════════════════════════════════════════════"