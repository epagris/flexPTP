import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys


def plot_data(d, name):
    global fi
    fig = plt.figure(fi, figsize=(5.2, 3))
    t_arr = np.arange(0, n)
    plt.plot(t_arr, np.array(d))
    plt.xlim([0, t_arr[-1]])
    plt.grid()
    plt.xlabel("Cycle")
    plt.ylabel("Time error [ns]")
    plt.show()
    fig.savefig(name + ".svg")
    fi = fi + 1


# initialize plot style
plt.rcParams["font.family"] = "Arial"
plt.rcParams["font.size"] = 8
plt.rcParams["lines.antialiased"] = True
plt.rcParams["lines.linewidth"] = 0.75
plt.rcParams["grid.color"] = "#262626"
plt.rcParams["grid.alpha"] = 0.15
plt.rcParams["grid.linewidth"] = 0.5
plt.rcParams["legend.loc"] = "upper right"

n = 1250
fi = 1

dump = pd.read_table(sys.argv[1], delimiter=' ', skipinitialspace=True)

plot_data(dump["dt_ns"][0:n], "ptp_plot")

exit(0)
