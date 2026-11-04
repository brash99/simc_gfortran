#!/bin/bash
#SBATCH --job-name=nps-6887-direct-enuc
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --time=00:20:00
#SBATCH --mem=4G
#SBATCH --cpus-per-task=1
#SBATCH --array=0-3
#SBATCH --output=/farm_out/brash/nps_6887_direct_enuc_%A_%a.out
#SBATCH --error=/farm_out/brash/nps_6887_direct_enuc_%A_%a.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04
export DVCS_GEN_DIR=/group/nps/hhuang/software/dvcs_gen
export NPS_SOFT_DIR=/group/nps/hhuang/software/NPS_SOFT

work=/work/hallc/gep/brash/gep4_nps_geant4
build="$work/DVCS_evt_gen/build_gep"
macro="$work/DVCS_evt_gen/DVCS/macros/electronuclear_weight_smoke.mac"
cuts=(0 0.25 0.5 1.0)
labels=(none 025 050 100)
export GEP_TARGET_SECONDARY_CUT_GEV="${cuts[$SLURM_ARRAY_TASK_ID]}"
output="$work/electronuclear_direct_cut_${labels[$SLURM_ARRAY_TASK_ID]}.root"

cd "$build"
printf '%s\n%s\n%s\n%s\n' "$output" 34 5196152 5291502 | \
  ./DVCS 6887 "$macro" 0 0 --electronuclear-events 8.456 3.57
