#!/bin/bash
#SBATCH --job-name=nps-edtm-scaler
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --time=00:30:00
#SBATCH --mem=4G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/nps_edtm_scaler_%j.out
#SBATCH --error=/farm_out/brash/nps_edtm_scaler_%j.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load root/6.30.04

private=/work/hallc/gep/brash/gep4_nps_run6151_private
roots="$private/nps_replay_develop/ROOTfiles/COIN/PRODUCTION"
macro="$private/analyze_nps_electron_edtm_scaler_closure.C"

for spec in "6158 21.17" "6165 19.87" "6168 18.56"; do
  read -r run angle <<< "$spec"
  root -l -b -q "$macro(\"$roots/nps_hms_coin_${run}_1_100000.root\",\"$private/nps_run${run}_100k_edtm_scaler_closure.csv\",${run},${angle},20.0,1.6)"
done
