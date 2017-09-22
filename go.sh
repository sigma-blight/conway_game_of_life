#!/bin/bash
#SBATCH --partition=coursework
#SBATCH --job-name=game_of_life
#SBATCH --nodes=2
#SBATCH --ntasks=2
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --time=00:00:10

export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}

DATE=$(date +"%Y%m%d%H%M")
echo "time started  "$DATE
echo "This is job '$SLURM_JOB_NAME' (id: $SLURM_JOB_ID) running on the following nodes:"
echo $SLURM_NODELIST
echo "running with OMP_NUM_THREADS= $OMP_NUM_THREADS "
echo "running with SLURM_TASKS_PER_NODE= $SLURM_TASKS_PER_NODE "
echo "Now we start the show:"
export TIMEFORMAT="%E sec"

module load mpi
#time mpirun -n ${SLURM_TASKS_PER_NODE} ./game_of_life
time mpirun ./game_of_life -n ${SLURM_TASKS_PER_NODE}

DATE=$(date +"%Y%m%d%H%M")
echo "time finished "$DATE

