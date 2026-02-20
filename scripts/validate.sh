#!/usr/bin/env bash
# validate.sh — Run headless sim across multiple seeds and summarize results.
#
# Usage:
#   ./scripts/validate.sh                    # default: 20 seeds, cautious bot
#   ./scripts/validate.sh 50 aggressive      # 50 seeds, aggressive bot
#   ./scripts/validate.sh 10 random 72000    # 10 seeds, random bot, 10 min each

set -euo pipefail
cd "$(dirname "$0")/.."

RUNNER="./build/sim_runner"

if [ ! -x "$RUNNER" ]; then
    echo "sim_runner not found. Building..."
    ./scripts/build.sh
fi

COUNT="${1:-20}"
BOT="${2:-cautious}"
TICKS="${3:-36000}"

echo "=== SkyRoads Level Validator ==="
echo "Seeds:  $COUNT"
echo "Bot:    $BOT"
echo "Ticks:  $TICKS ($(echo "scale=1; $TICKS / 120" | bc)s)"
echo "================================"
echo ""

SURVIVED=0
DIED=0
TOTAL_SCORE=0
TOTAL_DIST=0
MIN_SCORE=999999999
MAX_SCORE=0

for i in $(seq 1 "$COUNT"); do
    # Generate a unique seed per run.
    SEED=$(printf "0x%08X" $((0xC0FFEE + i * 7919)))
    LINE=$($RUNNER --seed "$SEED" --ticks "$TICKS" --bot "$BOT" --quiet 2>/dev/null || true)
    echo "$LINE"

    STATUS=$(echo "$LINE" | sed -n 's/.*status=\([^ ]*\).*/\1/p')
    SCORE=$(echo "$LINE" | sed -n 's/.*score=\([^ ]*\).*/\1/p' | tr -d ' ')
    DIST=$(echo "$LINE" | sed -n 's/.*dist=\([^ ]*\).*/\1/p' | tr -d ' ')

    if [ "$STATUS" = "SURVIVED" ]; then
        SURVIVED=$((SURVIVED + 1))
    else
        DIED=$((DIED + 1))
    fi

    # Accumulate for averages (integer math is fine for summary).
    SCORE_INT=$(printf "%.0f" "$SCORE")
    DIST_INT=$(printf "%.0f" "$DIST")
    TOTAL_SCORE=$((TOTAL_SCORE + SCORE_INT))
    TOTAL_DIST=$((TOTAL_DIST + DIST_INT))

    if [ "$SCORE_INT" -lt "$MIN_SCORE" ]; then MIN_SCORE=$SCORE_INT; fi
    if [ "$SCORE_INT" -gt "$MAX_SCORE" ]; then MAX_SCORE=$SCORE_INT; fi
done

echo ""
echo "=== Summary ==="
echo "Survived: $SURVIVED / $COUNT"
echo "Died:     $DIED / $COUNT"
if [ "$COUNT" -gt 0 ]; then
    echo "Avg score: $((TOTAL_SCORE / COUNT))"
    echo "Min score: $MIN_SCORE"
    echo "Max score: $MAX_SCORE"
    echo "Avg dist:  $((TOTAL_DIST / COUNT))"
fi

# Exit non-zero if any run died (useful for CI).
if [ "$DIED" -gt 0 ]; then
    exit 1
fi
