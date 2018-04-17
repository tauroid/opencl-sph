import os
import re

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

table = None

def display_particle_system(axes, history, step):
    pos = history['position'][step]

    axes.scatter(pos[:,0], pos[:,1], pos[:,2]).set_picker(True)

    l = pos.shape[0]

    if 'force_elastic' in history:
        fe = history['force_elastic'][step]*10
        axes.quiver(pos[:,0], pos[:,1], pos[:,2], fe[:,0], fe[:,1], fe[:,2], pivot='tail')

    """
    if 'deformation' in history:
        deformation = history['deformation'][step]*0.05

        x = np.array([[1.0,0.0,0.0],]*deformation.shape[0])
        y = np.array([[0.0,1.0,0.0],]*deformation.shape[0])
        z = np.array([[0.0,0.0,1.0],]*deformation.shape[0])

        for i in range(deformation.shape[0]):
            df = deformation[i]

            x[i] = np.dot(df, x[i])
            y[i] = np.dot(df, y[i])
            z[i] = np.dot(df, z[i])

        axes.quiver(pos[:,0], pos[:,1], pos[:,2], x[:,0], x[:,1], x[:,2], pivot='tail', color='r')
        axes.quiver(pos[:,0], pos[:,1], pos[:,2], y[:,0], y[:,1], y[:,2], pivot='tail', color='g')
        axes.quiver(pos[:,0], pos[:,1], pos[:,2], z[:,0], z[:,1], z[:,2], pivot='tail', color='b')
    """

    """
    if 'force_viscous' in history:
        fv = history['force_viscous'][step]
        axes.quiver(pos[:,0], pos[:,1], pos[:,2], fv[:,0], fv[:,1], fv[:,2])
    """

def get_history_structure(history):
    if isinstance(history, str):
        files = os.listdir(history)

        history = { re.match("(\S+)_history", fname).group(1): np.lib.format.open_memmap(history+"/"+fname)
                    for fname in filter(lambda f: "_history" in f, files) }

    return history

def create_history_figure(history, step):
    history = get_history_structure(history)

    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    display_particle_system(ax, history, step)

    def spit_event(event):
        global table
        if table is not None: table.remove()
        table = ax.table(
                    cellText = [["Particle number: {}".format(event.ind)]]
                    #+ [ [ "{}: ".format(field), "{}".format(data[step][event.ind[0]]) ] for field, data in history.items() ]
                )
        print("\n".join(["{}: {}".format(field, data[step][event.ind[0]]) for field, data in history.items()]))
        print("hi")
        fig.canvas.draw_idle()

    def onclick(event):
        print("hi")

    fig.canvas.mpl_connect('button_press_event', onclick)
    fig.canvas.mpl_connect('pick_event', spit_event)

    plt.show()

def print_particle_info(history, particle, step):
    history = get_history_structure(history)

    print("\n".join(["{}: {}".format(field, data[step][particle]) for field, data in history.items()]))
