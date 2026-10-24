#!/bin/bash
#SBATCH --job-name=gep-inelastic6151
#SBATCH --partition=production
#SBATCH --time=00:20:00
#SBATCH --mem=4G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/gep_inelastic6151_%j.out
#SBATCH --error=/farm_out/brash/gep_inelastic6151_%j.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load root/6.30.04

work=/work/hallc/gep/brash/gep4_nps_run6151_private
export TMPDIR="$work/tmp"
export XDG_CACHE_HOME="$work/.cache"
mkdir -p "$TMPDIR" "$XDG_CACHE_HOME/tmp"
cd "$work"
root -l -b -q \
  'analyze_prompt_inelastic.C+("nps_replay_develop/ROOTfiles/COIN/PRODUCTION/nps_hms_coin_6151_1_100000.root","prompt_inelastic_run6151.csv")'
