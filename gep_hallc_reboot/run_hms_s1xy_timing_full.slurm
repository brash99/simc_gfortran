#!/bin/bash
#SBATCH --job-name=gep-s1xy-timing
#SBATCH --partition=production
#SBATCH --time=00:20:00
#SBATCH --mem=4G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/gep_s1xy_timing_%j.out
#SBATCH --error=/farm_out/brash/gep_s1xy_timing_%j.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load root/6.30.04

work=/work/hallc/gep/brash/gep4_nps_run6151_private
cd "$work"
root -l -b -q \
  'analyze_hms_s1xy_timing.C+("nps_replay_develop/ROOTfiles/COIN/PRODUCTION/nps_hms_coin_6151_1_100000.root","hms_s1xy_timing_efficiency_full.csv",-1)'
