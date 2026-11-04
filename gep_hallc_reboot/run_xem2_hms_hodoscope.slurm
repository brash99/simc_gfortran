#!/bin/bash
#SBATCH --job-name=xem2_hms_hodo_edtm_17193
#SBATCH --account=halla
#SBATCH --partition=production
#SBATCH --time=02:00:00
#SBATCH --mem=4G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/xem2_hms_hodo_edtm_17193_%j.out
#SBATCH --error=/farm_out/brash/xem2_hms_hodo_edtm_17193_%j.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load hcana/1.1

cd /work/hallc/gep/brash/hallc_replay_XEM
hcana -l -b -q 'replay_xem2_hms_hodoscope_coin.C(17193,100000)'
