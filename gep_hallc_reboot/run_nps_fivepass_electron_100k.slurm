#!/bin/bash
#SBATCH --job-name=nps-5pass-100k
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --array=0-2
#SBATCH --time=01:00:00
#SBATCH --mem=8G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/nps_5pass_100k_%A_%a.out
#SBATCH --error=/farm_out/brash/nps_5pass_100k_%A_%a.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load hcana/1.1

private=/work/hallc/gep/brash/gep4_nps_run6151_private
export LD_LIBRARY_PATH="$private/NPSlib_current/BUILD/src${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export DB_DIR=DBASE

runs=(2855 5183 3888)
run=${runs[$SLURM_ARRAY_TASK_ID]}

cd "$private/nps_replay_develop"
hcana -q "SCRIPTS/NPS/replay_production_coin_NPS_HMS.C(${run},100000,1,0,0)"
