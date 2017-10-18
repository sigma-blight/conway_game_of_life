#!/bin/bash
#SBATCH −−partition=coursework
#SBATCH −−job−name=yo1x4x4x1
#SBATCH −−nodes=1
#SBATCH −−ntasks=4
#SBATCH −−ntasks−per−node=4
#SBATCH −−cpus−per−task=1

export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}

DATE=$(date +"%Y%m%d%H%M")
echo "time started  "$DATE
echo "This is job ’$SLURM_JOB_NAME’ (id: $SLURM_JOB_ID) running on the following nodes:"
echo $SLURM_NODELIST
echo "running with OMP_NUM_THREADS= $OMP_NUM_THREADS "
echo "running with SLURM_TASKS_PER_NODE= $SLURM_TASKS_PER_NODE "
echo "Now we start the show:"
export TIMEFORMAT="%E sec"

module load mpi
time mpirun ./game -n ${SLURM_TASKS_PER_NODE}
DATE=$(date +"%Y%m%d%H%M")

echo "time finished "$DATE
