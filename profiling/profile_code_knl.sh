#!/bin/bash --login

#!/bin/bash --login
 
#SBATCH --nodes=1
#SBATCH --time=00:20:00
#SBATCH --account=gpuhack02

#SBATCH --reservation=gpu-hackathon

#SBATCH --partition=gpuq
#SBATCH --gres=gpu:1
 
# Launch a profiling run on 24 MPI tasks on one node
# Set OMP_NUM_THREADS=1 to prevent any inadvertant threading
# (MAP does not specifically support threaded models)

module load knl mvapich forge

map -profile /home/tpotter/Projects/opencl-sph/install/bin/opencl_sph
