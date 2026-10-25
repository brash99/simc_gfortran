#!/bin/bash
#SBATCH --job-name=gep-g4-6151-actual
#SBATCH --partition=production
#SBATCH --time=00:30:00
#SBATCH --mem=24G
#SBATCH --cpus-per-task=1
#SBATCH --exclude=farm180207
#SBATCH --output=/farm_out/brash/nps_run6151_actual_2335_%j.out
#SBATCH --error=/farm_out/brash/nps_run6151_actual_2335_%j.err

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
  6151 validation/beam_on_2335.mac 0 0 \
  --simc-events validation/nps_run6151_elastic_actual_geometry.hybrid.csv 21.17 9.5 \
  < validation/nps_run6151_elastic_actual_geometry_2335.input
