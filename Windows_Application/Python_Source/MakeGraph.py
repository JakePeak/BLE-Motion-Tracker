import pandas as pd
import matplotlib.pyplot as plt
from matplotlib import rcParams as rcp
from itertools import count
from functools import partial

# Callback function to produce the animation
def Redraw(time, axes, name, graph):
    if (CheckExists(graph) == 0):
        return 0
    else:
        data = pd.read_csv(name, header=None, names=['x','y','z'], engine="c", lineterminator="\n")
        x = data['x']
        y = data['y']
        z = data['z']
        axes.clear()
        axes.plot(x, y, z, '-c')
        plt.draw()
        plt.pause(time)
        return 1

# Builds the graph alongside its animation
def MakeGraph(name, time):
    
    rcp["figure.raise_window"] = True; 

    plt.style.use("seaborn-v0_8-whitegrid")

    graph = plt.figure(clear=True, layout='tight')

    axes = plt.axes(projection='3d')

    plt.show(block=False)

    rcp["figure.raise_window"] = False; 
    
    return graph, axes 

# Deletes the graph
def DeleteGraph():
    plt.close('all')

# Checks if user has closed the graph window
def CheckExists(graph):
    if(plt.fignum_exists(graph.number)):  #returns True if a window is active
        return 1
    else:
        return 0
