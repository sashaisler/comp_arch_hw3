#!/usr/bin/env bash
set -euo pipefail

# -------- Build --------
# 1) Baseline: turn OFF auto-vectorizers (clean reference)
gcc -O3 -std=c11 -fno-vectorize -fno-slp-vectorize simple.c -o simple

# 2) Compiler auto-vectorized Part 1
gcc -O3 -std=c11 -fvectorize -fslp-vectorize simple.c -o simple_autovec

# 3) NEON intrinsics (Apple Silicon / ARM64)
gcc -O3 -std=c11 -march=armv8-a+simd -DUSE_NEON vector.c -o vector

# -------- Params --------
# Usage: ./run.sh [digits] [runs]
digits=${1:-10000}
runs=${2:-10}

make_rand() { python3 - "$1" <<'PY'
import sys,random
n=int(sys.argv[1])
print(str(random.randint(1,9)) + ''.join(str(random.randint(0,9)) for _ in range(n-1)))
PY
}
A=$(make_rand "$digits")
B=$(make_rand "$digits")

echo "Digits: $digits"
echo "Runs:   $runs"

time_real() { /usr/bin/time -p bash -c "$1" 2>&1 | awk '/^real/ {print $2}'; }

cmd_simple='A='"$A"' B='"$B"'; for i in $(seq 1 '"$runs"'); do ./simple "$A" "$B" >/dev/null; done'
cmd_auto='A='"$A"' B='"$B"'; for i in $(seq 1 '"$runs"'); do ./simple_autovec "$A" "$B" >/dev/null; done'
cmd_vector='A='"$A"' B='"$B"'; for i in $(seq 1 '"$runs"'); do ./vector "$A" "$B" >/dev/null; done'

t_simple=$(time_real "$cmd_simple")
t_auto=$(time_real "$cmd_auto")
t_vector=$(time_real "$cmd_vector")

echo "simple: $t_simple s"
echo "simple_autovec: $t_auto s"
echo "vector: $t_vector s"

python3 - <<PY
s=float("$t_simple"); a=float("$t_auto"); v=float("$t_vector")
print(f"speedup(auto vs simple):  {s/a:.3f}x")
print(f"speedup(vector vs simple): {s/v:.3f}x")
PY
