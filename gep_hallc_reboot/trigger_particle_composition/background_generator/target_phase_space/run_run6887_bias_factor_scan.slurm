#!/bin/bash
#SBATCH --job-name=nps-6887-bias-scan
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --array=0-2
#SBATCH --time=00:30:00
#SBATCH --mem=2G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/nps_6887_bias_scan_%A_%a.out
#SBATCH --error=/farm_out/brash/nps_6887_bias_scan_%A_%a.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04

work=/work/hallc/gep/brash/gep4_nps_geant4/target_phase_space
factors=(30000 100000 300000)
labels=(3e4 1e5 3e5)
bias=${factors[$SLURM_ARRAY_TASK_ID]}
label=${labels[$SLURM_ARRAY_TASK_ID]}
events=${GEP_PHASE_SPACE_EVENTS:-20000}
cut=${GEP_PHASE_SPACE_CUT_GEV:-1.0}
seed=$((6887300 + SLURM_ARRAY_TASK_ID))
output=$work/run6887_xsbias${label}_${events}_cut${cut}.root

/usr/bin/time -v "$work/build/gep_target_phase_space" \
  --bias-electronuclear "$bias" "$output" "$events" 8.456 "$cut" "$seed"
