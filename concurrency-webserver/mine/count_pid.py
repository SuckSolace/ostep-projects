#!/usr/bin/env python3

from collections import defaultdict

count = defaultdict(int)

with open("log", "r") as f:
    for i, line in enumerate(f):
        if i < 100:
            count[line.split(" ")[3][2:-1]] += 1

count = dict(sorted(count.items(), key=lambda item: item[1]))
for k,v in count.items():
    print(f"{k}: {v}")

