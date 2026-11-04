#!/bin/bash
#SBATCH --job-name=nps-5pass-closure
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --array=0-2
#SBATCH --time=01:00:00
#SBATCH --mem=10G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/nps_5pass_closure_%A_%a.out
#SBATCH --error=/farm_out/brash/nps_5pass_closure_%A_%a.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load root/6.30.04

private=/work/hallc/gep/brash/gep4_nps_run6151_private
roots="$private/nps_replay_develop/ROOTfiles/COIN/PRODUCTION"
macro="$private/analyze_nps_electron_edtm_scaler_closure.C"

runs=(2855 5183 3888)
angles=(16.58 15.57 14.76)
thresholds=(1.4 1.6 1.4)
run=${runs[$SLURM_ARRAY_TASK_ID]}
angle=${angles[$SLURM_ARRAY_TASK_ID]}
threshold=${thresholds[$SLURM_ARRAY_TASK_ID]}

# Use the hardware threshold recorded in each run report. Run 5183 used
# 1.6 GeV; runs 2855 and 3888 used 1.4 GeV.
root -l -b -q "$macro(\"$roots/nps_hms_coin_${run}_1_100000.root\",\"$private/nps_run${run}_100k_edtm_scaler_closure.csv\",${run},${angle},20.0,${threshold})"
