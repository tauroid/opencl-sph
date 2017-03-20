import sph
from sph import bindings as bnd
from sph import systems
from rigidtest import RigidTest
from historyplayer import HistoryPlayer
import numpy as np
import time

startt = time.clock()

bnd.set_log_level(2)

ss = systems.StrainSolid()

class RigidBody:
    def __init__(self, **kwargs):
        if not 'particles' in kwargs:
            raise Exception("You need to supply particle positions")

        self.particles = kwargs['particles']

        self.position = kwargs['position'] if 'position' in kwargs else np.zeros(3)
        self.euler_angles = kwargs['euler_angles'] if 'euler_angles' in kwargs else np.zeros(3)

        if 'pos_animation' in kwargs:
            self.pos_animation = kwargs['pos_animation']

        if 'angle_animation' in kwargs:
            self.angle_animation = kwargs['angle_animation']

"""
particles_on_edge = 10

prange = np.arange(particles_on_edge)
"""

x, y, z = np.meshgrid(np.linspace(-0.75, 0.75, num=30),
                      np.linspace(-0.4, 0.4, num=16),
                      np.linspace(-0.2, 0.2, num=8), indexing='ij')

all_particles = np.vstack((x.flatten(), y.flatten(), z.flatten())).T

soft_particles = np.array(all_particles[
    np.where(np.logical_and(all_particles[:,0] > -0.4, all_particles[:,0] < 0.4))])

rigid_body_1 = np.array(all_particles[np.where(all_particles[:,0] <= -0.4)])
rigid_body_2 = np.array(all_particles[np.where(all_particles[:,0] >= 0.4)])

rigid_body_1_animation_x = np.concatenate((np.linspace(0, -0.35, num=200), np.ones(400) * -0.35, np.linspace(-0.35, 0, num=200), np.array([0])))
rigid_body_1_animation_z = np.concatenate((np.zeros(250), np.linspace(0, -0.1, num=50), np.ones(50) * -0.1, np.linspace(-0.1, 0.1, num=100), np.ones(50) * 0.1, np.linspace(0.1, 0, num=50), np.zeros(251)))
rb1a_zeros = np.zeros(rigid_body_1_animation_x.shape)
rigid_body_1_animation = np.vstack((rigid_body_1_animation_x, rb1a_zeros, rigid_body_1_animation_z)).T

rigidtest_args = {
    "soft_particles": soft_particles,
    "rigid_bodies": [RigidBody(particles=rigid_body_1, pos_animation=rigid_body_1_animation), RigidBody(particles=rigid_body_2)],
    "total_mass": 10.0,
    "strain_solid": ss,
    "history_particles": "all",
    "history_fields": ["position"],
    "run_step_count": 900
}

sph.init_opencl()

rt = RigidTest(**rigidtest_args)

h = HistoryPlayer(rt.history)

bodynumbers = rt.body_numbers
bodycolours = [[1.0, 1.0, 1.0, 0.7], [0.4, 0.4, 1.0, 0.7], [0.4, 0.4, 1.0, 0.7]]

condition = rt.run()

print("time elapsed: {}".format(time.clock() - startt))

if condition == "failure":
    print("Marking culprit particles")
    num_body_particles = np.where(rt.body_numbers == 0)[0].size
    culprits = np.where(np.isnan(np.sum(ss.get_fields('rotation')[0][:num_body_particles], axis=(1,2))))[0]
    h.play(fail_particles=culprits, loop_before=rt.history_counter)
else:
    h.play(body_numbers=bodynumbers, body_colours=bodycolours)
