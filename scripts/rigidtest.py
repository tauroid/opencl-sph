import numpy as np
import argparse

class RigidTest:
    def __init__(self, **params):
        required = ["soft_particles", "rigid_bodies",
                    "total_mass", "strain_solid",
                    "history_particles", "history_fields", "run_step_count"]

        missing = [key for key in required if not key in params]

        if len(missing) > 0:
            raise Exception("Missing required fields: {}".format(missing))

        self.params = argparse.Namespace(**params)

        self.rigid_body_start_indices = np.zeros(len(self.params.rigid_bodies), dtype=np.dtype('uint32'))

        if len(self.params.rigid_bodies) > 0:
            self.rigid_body_start_indices[0] = self.params.soft_particles.shape[0]
        
        for i, body in enumerate(self.params.rigid_bodies):
            if i >= len(self.params.rigid_bodies)-1:
                break

            self.rigid_body_start_indices[i+1] = self.params.soft_particles.shape[0] + body.particles.shape[0]

        self.positions = np.concatenate((self.params.soft_particles,)+tuple(body.particles for body in self.params.rigid_bodies))
        rotations = np.tile(np.eye(3), (self.positions.shape[0], 1, 1))

        self.body_numbers = np.zeros(self.positions.shape[0], dtype=np.dtype('uint32'))

        for i in range(len(self.rigid_body_start_indices)):
            start_index = self.rigid_body_start_indices[i]
            stop_index = self.rigid_body_start_indices[i+1] if i < len(self.rigid_body_start_indices) - 1 else None

            self.body_numbers[start_index:stop_index] = i+1

        self.params.strain_solid.setup(position=self.positions,
                                       mass=self.params.total_mass/self.positions.shape[0],
                                       n=self.positions.shape[0],
                                       rotation=rotations,
                                       body_number=self.body_numbers,
                                       body_position=np.array([body.position for body in self.params.rigid_bodies]),
                                       body_euler_rotation=np.array([body.euler_angles for body in self.params.rigid_bodies]),
                                       gravity=np.array([0, 0, -0]))

        self.__init_output_and_history_indices()
        self.__init_history_arrays()

        self.params.strain_solid.sync_host_to_device()

        self.body_positions = self.params.strain_solid.get_fields("body_position")

    def __init_output_and_history_indices(self):
        if self.params.history_particles == "all":
            self.history_particle_indices = np.arange(self.positions.shape[0])
        else:
            self.history_particle_indices = column_intersection_indices(indices, self.history_particles)

    def __init_history_arrays(self):
        p = self.params

        prototype_history_arrays = p.strain_solid.get_fields(*p.history_fields)

        self.history = { p.history_fields[i]: np.zeros((self.params.run_step_count, self.history_particle_indices.size) + prototype_history_arrays[i].shape[1:])
                         for i in range(len(p.history_fields)) }

        self.history_counter = 0

    def detect_failure(self, **data):
        return np.any(np.isnan(data['position']))

    def record_history(self):
        if (self.history_counter >= self.params.run_step_count):
            raise Exception("History is full")

        p = self.params

        for key in self.history:
            np.copyto(self.history[key][self.history_counter], p.strain_solid.get_fields(key)[0][self.history_particle_indices])

        self.history_counter += 1

    def run(self):
        ps = self.params.strain_solid

        for i in range(self.params.run_step_count):
            for j, body in enumerate(self.params.rigid_bodies):
                if hasattr(body, "pos_animation") and i < body.pos_animation.shape[0]:
                    ps.set_body_position(j, body.pos_animation[i])

                if hasattr(body, "angle_animation") and i < body.angle_animation.shape[0]:
                    ps.set_body_rotation(j, body.angle_animation[i])

            ps.update()

            ps.sync_fields_device_to_host(*self.params.history_fields)

            self.record_history()

            if self.detect_failure(position=self.history['position'][self.history_counter-1]):
                ps.sync_fields_device_to_host()

                print("Something exploded or got set to nan: stopping run")
                print("The fields from the last frame are available in params['strain_solid']")

                return "failure"
        
        return "success"
