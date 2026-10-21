#!/usr/bin/env bash
set -euo pipefail

# Branches
STEP5_BRANCH="feature/phase3-optimisation1"
STEP4_BRANCH="feature/phase2-matching-logic"

# Relative paths
BUILD_DIR="build"
SUMMARY_FILE="benchmark_summary.txt"
TEST_FILTER="OrderCacheTest.Benchmark_MatchingEngineLatency"

echo "=========================================================="
echo " Starting Matching Engine Latency Comparison Benchmark"
echo " Step 5: $STEP5_BRANCH (O(1) Aggregates)"
echo " Step 4: $STEP4_BRANCH (Naive Greedy Loops)"
echo "=========================================================="

# 1. Ensure clean working tree for OrderCache source files
if ! git diff --quiet OrderCache.h OrderCache.cpp; then
    echo "Error: Uncommitted changes detected in OrderCache.h or OrderCache.cpp."
    echo "Please commit or stash your changes before running this script."
    exit 1
fi

mkdir -p "$BUILD_DIR"

# ---------------------------------------------------------
# Phase A: Benchmark Step 5
# ---------------------------------------------------------
echo ""
echo ">>> [1/4] Preparing Step 5 (feature/phase3-optimisation1)..."
git checkout "$STEP5_BRANCH" -- OrderCache.h OrderCache.cpp

echo ">>> Building Step 5..."
cmake -B "$BUILD_DIR" -S . -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR"

echo ">>> Executing benchmark on Step 5..."
OUTPUT_STEP5=$("$BUILD_DIR/OrderCacheTest" --gtest_filter="$TEST_FILTER")
echo "$OUTPUT_STEP5"

# Extract microseconds from: "Query Execution Time: <number> us"
TIME_STEP5_US=$(echo "$OUTPUT_STEP5" | awk -F'Query Execution Time:[[:space:]]*' '/Query Execution Time:/ {split($2, a, " "); print a[1]; exit}')

if [[ -z "$TIME_STEP5_US" ]]; then
    echo "Error: Could not parse execution time for Step 5."
    exit 1
fi

# ---------------------------------------------------------
# Phase B: Benchmark Step 4
# ---------------------------------------------------------
echo ""
echo ">>> [2/4] Restoring Step 4 files ($STEP4_BRANCH)..."
git checkout "$STEP4_BRANCH" -- OrderCache.h OrderCache.cpp

echo ">>> Building Step 4..."
cmake --build "$BUILD_DIR"

echo ">>> Executing benchmark on Step 4..."
OUTPUT_STEP4=$("$BUILD_DIR/OrderCacheTest" --gtest_filter="$TEST_FILTER")
echo "$OUTPUT_STEP4"

# Extract microseconds from Step 4 output
TIME_STEP4_US=$(echo "$OUTPUT_STEP4" | awk -F'Query Execution Time:[[:space:]]*' '/Query Execution Time:/ {split($2, a, " "); print a[1]; exit}')

if [[ -z "$TIME_STEP4_US" ]]; then
    echo "Error: Could not parse execution time for Step 4."
    # Restore Step 5 files before exiting on error
    git checkout "$STEP5_BRANCH" -- OrderCache.h OrderCache.cpp
    exit 1
fi

# ---------------------------------------------------------
# Phase C: Restore Step 5 Source State
# ---------------------------------------------------------
echo ""
echo ">>> [3/4] Restoring working tree back to Step 5..."
git checkout "$STEP5_BRANCH" -- OrderCache.h OrderCache.cpp
cmake --build "$BUILD_DIR"

# ---------------------------------------------------------
# Phase D: Compute Metrics and Generate Summary
# ---------------------------------------------------------
echo ""
echo ">>> [4/4] Writing summary to $SUMMARY_FILE..."

# Calculate speedup ratio and percentage reduction using awk
SPEEDUP=$(awk -v s4="$TIME_STEP4_US" -v s5="$TIME_STEP5_US" 'BEGIN {
    if (s5 == 0) s5 = 1; # avoid division by zero if query is sub-microsecond
    printf "%.2f", s4 / s5
}')

REDUCTION=$(awk -v s4="$TIME_STEP4_US" -v s5="$TIME_STEP5_US" 'BEGIN {
    if (s4 == 0) printf "0.00";
    else printf "%.2f", ((s4 - s5) / s4) * 100
}')

cat << EOF > "$SUMMARY_FILE"
======================================================================
               MATCHING ENGINE LATENCY BENCHMARK REPORT
======================================================================
Generated: $(date)
Target Test: $TEST_FILTER
Orders in Book: 4,000

1. RAW LATENCY METRICS:
   - Step 4 (Naive Greedy O(N + B*S)):  $TIME_STEP4_US us ($(( TIME_STEP4_US / 1000 )) ms)
   - Step 5 (O(1) Company Aggregates):  $TIME_STEP5_US us ($(( TIME_STEP5_US / 1000 )) ms)

2. PERFORMANCE GAINS:
   - Speedup Factor:     ${SPEEDUP}x faster
   - Latency Reduction:  ${REDUCTION}% decrease in query time

3. ARCHITECTURAL ANALYSIS:
   - Step 4 relies on linear candidate collection and a nested double-loop
     evaluating cross-company pairings across individual orders.
   - Step 5 tracks security and company volume totals in real-time, reducing
     getMatchingSizeForSecurity to an O(K) closed-form calculation where K
     is the small number of distinct companies trading that security.
======================================================================
EOF

cat "$SUMMARY_FILE"
echo ""
echo "Benchmark run completed successfully. Report saved to $SUMMARY_FILE."