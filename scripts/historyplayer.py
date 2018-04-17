import os
import re

from PyQt4 import QtCore
import pyqtgraph.opengl as gl
from PyQt4 import QtGui, QtCore
import numpy as np

class HistoryPlayer:
    def play(self, history, **kwargs):
        if isinstance(history, str):
            files = os.listdir(history)

            history = { re.match("(\S+)_history", fname).group(1): np.lib.format.open_memmap(history+"/"+fname)
                                 for fname in filter(lambda f: "_history" in f, files) }

        if not "position" in history:
            raise Exception("No position information stored.")

        if not hasattr(self, "view_app"):
            self.view_app = QtGui.QApplication([])

        app = self.view_app

        view = gl.GLViewWidget()
        view.show()

        view.opts['distance'] = 2000
        view.opts['fov'] = 1

        colors = np.tile(np.array([1.0,1.0,1.0,0.7]), (history['position'].shape[1], 1))

        if 'body_colours' in kwargs and 'body_numbers' in kwargs:
            for i, colour in enumerate(kwargs['body_colours']):
                colors[np.where(kwargs['body_numbers'] == i)] = kwargs['body_colours'][i]

        if 'fail_particles' in kwargs:
            print(kwargs['fail_particles'])
            colors *= 0.6
            colors[kwargs['fail_particles']] = [1.0, 0.2, 0.2, 1.0]
        elif 'density' in history:
            colorangle = np.pi/3
            color1 = np.concatenate((0.5*np.cos([colorangle, colorangle + np.pi*2/3, colorangle + np.pi*4/3]) + 0.5, [0.7]))
            color2 = np.concatenate((0.5*np.cos([colorangle + np.pi, colorangle + np.pi*5/3, colorangle + np.pi/3]) + 0.5, [0.7]))
            max_density = np.max(history['density'][0])
            min_density = np.min(history['density'][0])
            colors = color1 + np.outer((history['density'][0] - min_density) / (max_density - min_density), (color2 - color1))

        if 'force_elastic' in history:
            lines_shape = np.copy(history['position'].shape)
            lines_shape[1] += history['force_elastic'].shape[1]
            elastic_lines = np.empty(lines_shape)
            elastic_lines[:,::2,:] = history['position']
            elastic_lines[:,1::2,:] = history['position'] + history['force_elastic']
            elastic_lines_plot = gl.GLLinePlotItem(pos=elastic_lines[0], color=[1.0, 0.0, 0.0, 1.0], mode='lines')

            view.addItem(elastic_lines_plot)

        if 'force_viscous' in history:
            lines_shape = np.copy(history['position'].shape)
            lines_shape[1] += history['force_viscous'].shape[1]
            viscous_lines = np.empty(lines_shape)
            viscous_lines[:,::2,:] = history['position']
            viscous_lines[:,1::2,:] = history['position'] + history['force_viscous']
            viscous_lines_plot = gl.GLLinePlotItem(pos=viscous_lines[0], color=[0.0, 1.0, 0.0, 1.0], mode='lines')

            view.addItem(viscous_lines_plot)

        """
        if 'strain' in history:
            lines_shape = np.copy(history['position'].shape)
            lines_shape[1] += history['strain'].shape[1]
            strain_lines = np.empty(lines_shape)
            strain_lines[:,::2,:] = history['position']
            strain_lines[:,1::2,:] = history['position'] + history['strain'][:,:,0:3] / 4
            strain_lines_plot = gl.GLLinePlotItem(pos=strain_lines[0], color=[0.0, 0.0, 1.0, 1.0], mode='lines')

            view.addItem(strain_lines_plot)

        if 'apq_temp' in history:
            lines_shape = np.copy(history['position'].shape)
            lines_shape[1] += history['apq_temp'].shape[1]
            apq_temp_lines = np.empty(lines_shape)
            apq_temp_lines[:,::2,:] = history['position']
            apq_temp_lines[:,1::2,:] = history['position'] + np.diagonal(history['apq_temp'], axis1=2, axis2=3) / 200
            apq_temp_lines_plot = gl.GLLinePlotItem(pos=apq_temp_lines[0], color=[0.0, 0.0, 1.0, 1.0], mode='lines')

            view.addItem(apq_temp_lines_plot)
        """

        if not "loop_before" in kwargs:
            nans = np.where(np.any(np.isnan(history['position']), axis=(1,2)))[0]
            if nans.size > 0:
                loop_before = nans[0]
                colors[np.where(np.any(np.isnan(history['position'][nans[0]]), axis=1))[0]] = [1.0, 0.0, 0.0, 1.0]
            else:
                loop_before = history['position'].shape[0]
        else:
            loop_before = kwargs['loop_before']

        scatter = gl.GLScatterPlotItem(pos=history['position'][0], size=6, color=colors)
        axes = gl.GLAxisItem()

        view.addItem(scatter)
        view.addItem(axes)

        self.current_history_step = 0

        frame_interval = 10 if not "frame_interval" in kwargs else kwargs["frame_interval"]

        def update_scatter():
            scatter.setData(pos=history['position'][self.current_history_step])

            if 'force_elastic' in history:
                elastic_lines_plot.setData(pos=elastic_lines[self.current_history_step])

            if 'force_viscous' in history:
                viscous_lines_plot.setData(pos=viscous_lines[self.current_history_step])

            """
            if 'strain' in history:
                strain_lines_plot.setData(pos=strain_lines[self.current_history_step])

            if 'apq_temp' in history:
                apq_temp_lines_plot.setData(pos=apq_temp_lines[self.current_history_step])
            """

            app.processEvents()

            self.current_history_step = (self.current_history_step + 2) % loop_before

        timer = QtCore.QTimer()
        timer.timeout.connect(update_scatter)
        timer.start(frame_interval)

        app.exit(app.exec_())


