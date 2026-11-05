#!/bin/bash
#SBATCH --job-name=nps-6887-cut-scan
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --array=0-1
#SBATCH --time=00:30:00
#SBATCH --mem=3G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/nps_6887_cut_scan_%A_%a.out
#SBATCH --error=/farm_out/brash/nps_6887_cut_scan_%A_%a.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04

work=/work/hallc/gep/brash/gep4_nps_geant4/target_phase_space
cuts=(0.5 0.25)
labels=(0p5 0p25)
cut=${cuts[$SLURM_ARRAY_TASK_ID]}
label=${labels[$SLURM_ARRAY_TASK_ID]}
events=1000000
bias=300000
seed=$((6887500 + SLURM_ARRAY_TASK_ID))
output=$work/run6887_xsbias3e5_${events}_cut${label}.root

/usr/bin/time -v "$work/build/gep_target_phase_space" \
  --bias-electronuclear "$bias" "$output" "$events" 8.456 "$cut" "$seed"
