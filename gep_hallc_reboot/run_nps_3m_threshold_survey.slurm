#!/bin/bash
#SBATCH --job-name=nps-3m-thr-survey
#SBATCH --partition=production
#SBATCH --time=02:00:00
#SBATCH --mem=8G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/nps_3m_threshold_survey_%j.out
#SBATCH --error=/farm_out/brash/nps_3m_threshold_survey_%j.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load hcana/1.1

private=/work/hallc/gep/brash/gep4_nps_run6151_private
export LD_LIBRARY_PATH="$private/NPSlib_current/BUILD/src${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export DB_DIR=DBASE

replay="$private/nps_replay_develop"
cd "$replay"

for run in 1909 1916 1917 1919 1920; do
  hcana -q "SCRIPTS/NPS/replay_production_coin_NPS_HMS.C(${run},10000,1,0,0)"
done
