#!/bin/bash
#SBATCH --job-name=nps-6887-wiser-pi0
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --time=02:00:00
#SBATCH --mem=8G
#SBATCH --cpus-per-task=1
#SBATCH --exclude=farm180207
#SBATCH --output=/farm_out/brash/nps_run6887_wiser_pi0_g4_%j.out
#SBATCH --error=/farm_out/brash/nps_run6887_wiser_pi0_g4_%j.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04

export DVCS_GEN_DIR=/group/nps/hhuang/software/dvcs_gen
export NPS_SOFT_DIR=/group/nps/hhuang/software/NPS_SOFT

repo=/work/hallc/gep/brash/gep4_nps_geant4
cd "$repo"
/usr/bin/time -v ./DVCS_evt_gen/build_gep/DVCS \
  6151 validation/beam_on_88122.mac 0 0 \
  --simc-pi0-events validation/nps_run6887_wiser_pi0_5m.hybrid.csv \
  16.57 3.57 \
  < validation/nps_run6887_wiser_pi0.input

root -l -b -q \
  'validation/extract_nps_inclusive_geant4_response.C("6887101","validation/nps_run6887_wiser_pi0_geant4_response.csv")'
