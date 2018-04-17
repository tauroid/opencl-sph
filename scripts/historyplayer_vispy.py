# !/usr/bin/env python
# -*- coding: utf-8 -*-
""" 3D Scatter plotter
"""

import os
import re

from enum import Enum

from vispy import gloo
from vispy import app
from vispy import visuals
import vispy.visuals.transforms as transforms
from vispy.app.application import Application
import numpy as np

import pickle

VERT_SHADER = """
attribute vec3  a_position;
attribute vec3  a_color;
attribute float a_size;

uniform mat4 u_view;
uniform vec3 u_position;

varying vec4 v_fg_color;
varying vec4 v_bg_color;
varying float v_radius;
varying float v_linewidth;
varying float v_antialias;

void main (void) {
    v_radius = a_size;
    v_linewidth = 1.0;
    v_antialias = 1.0;
    v_fg_color  = vec4(0.0,0.0,0.0,1.0);
    v_bg_color  = vec4(a_color,    1.0);

    gl_Position = u_view*vec4(a_position, 1.0) + vec4(u_position, 0.0);
    gl_PointSize = 2.0*(v_radius + v_linewidth + 1.5*v_antialias);
}
"""

FRAG_SHADER = """
#version 120

varying vec4 v_fg_color;
varying vec4 v_bg_color;
varying float v_radius;
varying float v_linewidth;
varying float v_antialias;
void main()
{
    float size = 2.0*(v_radius + v_linewidth + 1.5*v_antialias);
    float t = v_linewidth/2.0-v_antialias;
    float r = length((gl_PointCoord.xy - vec2(0.5,0.5))*size);
    float d = abs(r - v_radius) - t;
    if( d < 0.0 )
        gl_FragColor = v_fg_color;
    else
    {
        float alpha = d/v_antialias;
        alpha = exp(-alpha*alpha);
        if (r > v_radius)
            gl_FragColor = vec4(v_fg_color.rgb, alpha*v_fg_color.a);
        else
            gl_FragColor = mix(v_bg_color, v_fg_color, alpha);
    }
}
"""

def get_actual_history(history):
    if isinstance(history, str):
        files = os.listdir(history)

        history = { re.match("(\S+)_history", fname).group(1): np.lib.format.open_memmap(history+"/"+fname)
                    for fname in filter(lambda f: "_history" in f, files) }

    return history

class HistoryPlayer(app.Canvas):

    InteractState = Enum('InteractState', 'SLIDER VIEW NONE')

    def __init__(self, history, **kwargs):
        self.history = get_actual_history(history)

        if not 'position' in self.history:
            raise Exception("No position information stored.")

        app.Canvas.__init__(self, keys='interactive')
        ps = self.pixel_scale

        self.frame_number = 0

        # Create vertices
        v_position = self.history['position'][self.frame_number].astype(np.float32)
        v_color = np.tile(np.array([1.0,1.0,1.0]), (self.history['position'].shape[1], 1)).astype(np.float32)
        v_size = np.tile(2.0*ps, self.history['position'].shape[1]).astype(np.float32)

        self.program = gloo.Program(VERT_SHADER, FRAG_SHADER)
        # Set uniform and attribute
        self.program['a_color'] = gloo.VertexBuffer(v_color)
        self.program['a_position'] = gloo.VertexBuffer(v_position)
        self.program['a_size'] = gloo.VertexBuffer(v_size)

        self.program['u_view'] = np.eye(4, dtype=np.float32)
        self.program['u_position'] = np.array([0, 0, 0], dtype=np.float32)
        self.reset_gloo_state()

        self.mouse_button = None
        self.interact_state = HistoryPlayer.InteractState.NONE

        self.slider = visuals.CompoundVisual([
            visuals.RectangleVisual(center=[0, 50], width=self.size[0]*0.8, height=50, radius=20, color=[0.9,0.9,0.9,1]),
            visuals.line.LineVisual(pos=np.array([[-self.size[0]*0.35, 50], [self.size[0]*0.35, 50]]))
        ])

        self.slider_peg = visuals.EllipseVisual(radius=10, center=[-self.size[0]*0.35, 50], color=[0.2,0.2,0.2,1.0])

        screentransform = transforms.STTransform(scale=[2.0/self.size[0], 2.0/self.size[1]], translate=[0, -1])

        self.slider.transform = screentransform
        self.slider_peg.transform = screentransform

        self.show()

    def reset_gloo_state(self):
        gloo.set_state('opaque', clear_color='white', blend=False, depth_test=False,
                       blend_func=('src_alpha', 'one_minus_src_alpha'))

    def updateProgramPositions(self):
        self.program['a_position'] = \
            gloo.VertexBuffer(self.history['position'][self.frame_number])
        self.update()

    def increment_frame(self):
        self.frame_number = min(self.history['position'].shape[0]-1,
                                self.frame_number + 1)

        self.updateProgramPositions()

    def decrement_frame(self):
        self.frame_number = max(0, self.frame_number - 1)

        self.updateProgramPositions()

    def rotate(self, mousediff):
        mousediff = mousediff / 100
        self.program['u_view'] = np.dot(self.program['u_view'].reshape((4,4)), np.dot(
                np.array([[ np.cos(mousediff[0]), 0, np.sin(mousediff[0]), 0],
                          [ 0,                    1, 0,                     0],
                          [ -np.sin(mousediff[0]), 0, np.cos(mousediff[0]),  0],
                          [ 0,                     0, 0,                     1]])
                ,
                np.array([[ 1, 0,                     0,                     0],
                          [ 0, np.cos(mousediff[1]),  -np.sin(mousediff[1]),  0],
                          [ 0, np.sin(mousediff[1]), np.cos(mousediff[1]),  0],
                          [ 0, 0,                     0,                     1]])
            ))

    def move(self, mousediff):
        mousediff = mousediff / 300
        self.program['u_position'] += np.array([mousediff[0], -mousediff[1], 0])

    def on_resize(self, event):
        gloo.set_viewport(0, 0, *event.physical_size)

    def on_draw(self, event):
        gloo.clear(color=True, depth=True)
        self.program.draw('points')
        self.slider.draw()
        self.reset_gloo_state()
        self.slider_peg.draw()

    def point_in_slider(self, point):
        return point[0] >= self.size[0]*0.1 and point[0] < self.size[0]*0.9 and\
               point[1] >= self.size[1]-75  and point[1] < self.size[1] - 25

    def update_slider(self, mouse_pos):
        self.slider_peg.center = [max(-self.size[0]*0.35, min(self.size[0]*0.35, mouse_pos[0] - self.size[0]/2)), 50]

        slider_position = (float(self.slider_peg.center[0]) + self.size[0]*0.35)/self.size[0]*0.7
        self.frame_number = int(slider_position * (self.history['position'].shape[0]-1))

        self.updateProgramPositions()

    def print_particle_info(self, particle, step):
        if particle is None: return

        print("Particle {} at frame {}:".format(particle, step))

        for k, v in self.history.items():
            print("{}:{}{}".format(k, '\n' if len(v.shape) > 3 else ' ', v[step][particle]))

    def get_particle_from_pos(self, pos):
        box_side = 0.05

        for i, p_pos in enumerate(self.history['position'][self.frame_number]):
            screen_pos = np.dot(self.program['u_view'].reshape((4,4)).T, np.append(p_pos, 1)) + np.append(self.program['u_position'], 0)
            if abs(pos[0] - screen_pos[0]) < box_side/2 and abs(pos[1] - screen_pos[1]) < box_side/2:
                return i

        return None

    def mouse_pos_to_view_pos(self, mouse_pos):
        return [mouse_pos[0]*2.0/self.size[0] - 1, 1 - mouse_pos[1]*2.0/self.size[1]]

    def on_mouse_press(self, event):
        self.mouse_button = event.button

        if self.point_in_slider(event.pos):
            self.interact_state = HistoryPlayer.InteractState.SLIDER
        else:
            self.interact_state = HistoryPlayer.InteractState.VIEW

        if self.interact_state is HistoryPlayer.InteractState.VIEW:
            self.last_mouse = np.array(event.pos)
            self.print_particle_info(self.get_particle_from_pos(self.mouse_pos_to_view_pos(event.pos)), self.frame_number)
        elif self.interact_state is HistoryPlayer.InteractState.SLIDER:
            if self.mouse_button == 1:
                self.update_slider(event.pos)

    def on_mouse_release(self, event):
        self.mouse_button = None
        self.interact_state = HistoryPlayer.InteractState.NONE

    def on_key_press(self, event):
        if event.key.name == 'Left':
            self.decrement_frame()
        elif event.key.name == 'Right':
            self.increment_frame()

        print(self.frame_number)

    def on_mouse_move(self, event):
        if self.mouse_button is None or self.interact_state is HistoryPlayer.InteractState.NONE: return

        if self.interact_state is HistoryPlayer.InteractState.VIEW:
            new_mouse = np.array(event.pos)
            diff = new_mouse - self.last_mouse

            self.last_mouse = new_mouse
            if self.mouse_button == 1:
                self.rotate(diff)
            else:
                self.move(diff)

        elif self.interact_state is HistoryPlayer.InteractState.SLIDER:
            if self.mouse_button == 1:
                self.update_slider(event.pos)

        self.update()

def play_history(history):
    h = HistoryPlayer(history)
    a = Application()
    a.run(allow_interactive=False)

if __name__ == "__main__":
    play_history('results/whole_4')
