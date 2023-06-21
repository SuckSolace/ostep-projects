#!/usr/bin/env python3

from collections import defaultdict

count = defaultdict(int)

with open("log", "r") as f:
    for line in f:
        if line.startswith("pid"):
            count["".join(filter(str.isdigit, line))] += 1

count = dict(sorted(count.items(), key=lambda item: item[1]))
for k,v in count.items():
    print(f"{k}: {v}")

print(sum(count.values()))

