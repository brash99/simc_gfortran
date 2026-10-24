#!/bin/bash
#SBATCH --job-name=gep-nps1918-scan
#SBATCH --partition=production
#SBATCH --time=00:20:00
#SBATCH --mem=6G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/gep_nps1918_scan_%j.out
#SBATCH --error=/farm_out/brash/gep_nps1918_scan_%j.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load root/6.30.04

work=/work/hallc/gep/brash/gep4_nps_run6151_private
cd "$work"
root -l -b -q \
  'analyze_nps_run6151.C("nps_replay_develop/ROOTfiles/COIN/PRODUCTION/nps_hms_coin_1918_1_100000.root","nps_run1918_100k")'
