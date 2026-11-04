#!/bin/bash
#SBATCH --job-name=nps-6887-wiser-smoke
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --time=00:10:00
#SBATCH --mem=2G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/nps_run6887_wiser_pi0_smoke_%j.out
#SBATCH --error=/farm_out/brash/nps_run6887_wiser_pi0_smoke_%j.err

set -euo pipefail
source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04
export DVCS_GEN_DIR=/group/nps/hhuang/software/dvcs_gen
export NPS_SOFT_DIR=/group/nps/hhuang/software/NPS_SOFT

cd /work/hallc/gep/brash/gep4_nps_geant4
./DVCS_evt_gen/build_gep/DVCS \
  6151 beam_on_10.mac 0 0 \
  --simc-events validation/nps_run6887_wiser_pi0_5m.hybrid.csv \
  16.57 3.57 \
  < validation/nps_run6887_wiser_pi0_smoke.input
