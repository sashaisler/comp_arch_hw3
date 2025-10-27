#!/usr/bin/env bash
set -euo pipefail

# Usage: ./run.sh [digits] [runs]
digits=${1:-10000}
runs=${2:-10}

# Generate N-digit random decimal (no leading zeros)
make_rand() {
  python3 - "$1" <<'PY'
import sys,random
n=int(sys.argv[1])
print(str(random.randint(1,9)) + ''.join(str(random.randint(0,9)) for _ in range(n-1)))
PY
}

A=$(make_rand "$digits")
B=$(make_rand "$digits")

echo "Digits: $digits"
echo "Runs:   $runs"

echo "Correctness check..."
./simple "$A" "$B" > o_simple.txt
./vector "$A" "$B" > o_vector.txt
diff o_simple.txt o_vector.txt && echo "OK: outputs match"

echo
echo "Timing simple ($runs runs):"
/usr/bin/time -p bash -c 'A='"$A"' B='"$B"'; for i in $(seq 1 '"$runs"'); do ./simple "$A" "$B" >/dev/null; done'

echo
echo "Timing vector ($runs runs):"
/usr/bin/time -p bash -c 'A='"$A"' B='"$B"'; for i in $(seq 1 '"$runs"'); do ./vector "$A" "$B" >/dev/null; done'
