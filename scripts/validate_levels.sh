#!/usr/bin/env bash
# validate_levels.sh — Validate all implemented levels with multiple seeds

set -euo pipefail
cd "$(dirname "$0")/.."

RUNNER="./build/sim_runner"

if [ ! -x "$RUNNER" ]; then
    echo "sim_runner not found. Building..."
    ./scripts/build.sh
fi

COUNT="${1:-10}"
BOT="${2:-cautious}"
TICKS="${3:-72000}"

echo "=== SkyRoads Level Validator (All Levels) ==="
echo "Seeds per level:  $COUNT"
echo "Bot:              $BOT"
echo "Ticks:            $TICKS ($(echo "scale=1; $TICKS / 120" | bc)s)"
echo "=============================================="
echo ""

for LEVEL in 1 2 3 4 5 6; do
    echo "=== Level $LEVEL ==="
    
    SURVIVED=0
    DIED=0
    TOTAL_SCORE=0
    TOTAL_DIST=0
    
    for i in $(seq 1 "$COUNT"); do
        SEED=$(printf "0x%08X" $((0xC0FFEE + i * 7919)))
        LINE=$($RUNNER --level "$LEVEL" --seed "$SEED" --ticks "$TICKS" --bot "$BOT" --quiet 2>/dev/null || true)
        
        STATUS=$(echo "$LINE" | sed -n 's/.*status=\([^ ]*\).*/\1/p')
        SCORE=$(echo "$LINE" | sed -n 's/.*score=\([^ ]*\).*/\1/p' | tr -d ' ')
        DIST=$(echo "$LINE" | sed -n 's/.*dist=\([^ ]*\).*/\1/p' | tr -d ' ')
        
        if [ "$STATUS" = "SURVIVED" ]; then
            SURVIVED=$((SURVIVED + 1))
        else
            DIED=$((DIED + 1))
        fi
        
        SCORE_INT=$(printf "%.0f" "$SCORE" 2>/dev/null || echo "0")
        DIST_INT=$(printf "%.0f" "$DIST" 2>/dev/null || echo "0")
        TOTAL_SCORE=$((TOTAL_SCORE + SCORE_INT))
        TOTAL_DIST=$((TOTAL_DIST + DIST_INT))
    done
    
    echo "Survived: $SURVIVED / $COUNT"
    echo "Died:    $DIED / $COUNT"
    if [ "$COUNT" -gt 0 ]; then
        echo "Avg score: $((TOTAL_SCORE / COUNT))"
        echo "Avg dist:  $((TOTAL_DIST / COUNT))"
    fi
    echo ""
done
