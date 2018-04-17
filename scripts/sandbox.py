from . import sph
from .sph import systems
from .straintest import StrainTestSymPadding
from .historyplayer import HistoryPlayer
import numpy as np

sph.init_opencl()

ss = systems.StrainSolid()

"""
particles_on_edge = 10

prange = np.arange(particles_on_edge)
"""

axis = 0

x, y, z = np.meshgrid(np.arange(8), np.arange(4), np.arange(2), indexing='ij')

history_particles = np.vstack((x.flatten(), y.flatten(), z.flatten())).T
output_particles = history_particles[history_particles[:, axis] == 0, :]

straintest_args = {
    "strain_range": np.linspace(-0.5, 0.5, num=10),
    "axis": axis,
    "strain_rate": 0.1,
    "stabilisation_time": 3.5,
    "initial_dimensions": np.array([0.8, 0.4, 0.2]),
    "particle_dimensions": np.array([8, 4, 2]),
    "total_mass": 10.0,
    "strain_solid": ss,
    "output_particles": output_particles,
    "output_fields": ["position", "force"],
    "history_particles": "all",
    "history_fields": ["position"]
}

st = StrainTestSymPadding(**straintest_args)

condition = st.run()

h = HistoryPlayer(st.history)

if condition == "failure":
    print("Marking culprit particles")
    culprits = np.where(np.isnan(np.sum(ss.get_fields('rotation')[0], axis=(1,2))))[0]
    h.play(fail_particles=culprits, loop_before=st.history_counter)
else:
    h.play()
