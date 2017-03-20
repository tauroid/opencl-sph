from . import sph
from .sph import systems
from .straintest import StrainTest
from .historyplayer import HistoryPlayer
import numpy as np

ss = systems.StrainSolid()

"""
particles_on_edge = 10

prange = np.arange(particles_on_edge)
"""

axis = 0

x, y, z = np.meshgrid(np.arange(15), np.arange(8), np.arange(4), indexing='ij')

history_particles = np.vstack((x.flatten(), y.flatten(), z.flatten())).T
output_particles = history_particles[history_particles[:, axis] == 0, :]

straintest_args = {
    "strain_range": np.linspace(-0.5, 0.5, num=10),
    "axis": axis,
    "strain_rate": 0.1,
    "stabilisation_time": 3.5,
    "initial_dimensions": np.array([1.5, 0.8, 0.4]),
    "particle_dimensions": np.array([15, 8, 4]),
    "total_mass": 10.0,
    "strain_solid": ss,
    "output_particles": output_particles,
    "output_fields": ["position", "force"],
    "history_particles": "all",
    "history_fields": ["position"]
}

sph.init_opencl()

st = StrainTest(**straintest_args)

st.run()

h = HistoryPlayer(st.history)

h.play()
