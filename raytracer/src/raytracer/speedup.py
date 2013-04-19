from __future__ import division
import numpy as np
import matplotlib.pyplot as plt
import sys

def main():
    x = []
    y = []

    baseline = 0.0

    with open(sys.argv[1], 'rb') as f:
        lines = f.readlines()

        for l in lines:
            words = l.split()
            if len(words) < 3:
                continue

            if words[1] != "Total":
                continue

            nthreads = int(words[0])

            if nthreads == 1:
                baseline = float(words[3])

            if nthreads not in x:
                x.append(nthreads)
                y.append(baseline/float(words[3]))

    f.close()

    plt.plot(x, y)

    plt.title(sys.argv[1])
    plt.xlabel("Number of cores")
    plt.ylabel("Run time")
    plt.show()

if __name__ == "__main__":
    main()
