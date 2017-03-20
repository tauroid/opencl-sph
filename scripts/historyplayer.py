from PyQt4 import QtCore
import pyqtgraph.opengl as gl
from PyQt4 import QtGui, QtCore
import numpy as np

class HistoryPlayer:
    def __init__(self, history):
        self.history = history

    def play(self, **kwargs):
        if not "position" in self.history:
            raise Exception("No position information stored.")

        if not hasattr(self, "view_app"):
            self.view_app = QtGui.QApplication([])

        app = self.view_app

        view = gl.GLViewWidget()
        view.show()

        colors = np.tile(np.array([1.0,1.0,1.0,0.7]), (self.history['position'].shape[1], 1))

        if 'body_colours' in kwargs and 'body_numbers' in kwargs:
            for i, colour in enumerate(kwargs['body_colours']):
                colors[np.where(kwargs['body_numbers'] == i)] = kwargs['body_colours'][i]

        if 'fail_particles' in kwargs:
            print(kwargs['fail_particles'])
            colors *= 0.6
            colors[kwargs['fail_particles']] = [1.0, 0.2, 0.2, 1.0]

        scatter = gl.GLScatterPlotItem(pos=self.history['position'][0], size=6, color=colors)
        axes = gl.GLAxisItem()

        view.addItem(scatter)
        view.addItem(axes)

        self.current_history_step = 0

        frame_interval = 10 if not "frame_interval" in kwargs else kwargs["frame_interval"]
        loop_before = self.history['position'].shape[0] if not "loop_before" in kwargs else kwargs["loop_before"]

        def update_scatter():
            scatter.setData(pos=self.history['position'][self.current_history_step])
            app.processEvents()

            self.current_history_step = (self.current_history_step + 1) % loop_before

        timer = QtCore.QTimer()
        timer.timeout.connect(update_scatter)
        timer.start(frame_interval)

        app.exit(app.exec_())


