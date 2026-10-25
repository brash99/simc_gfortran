#!/bin/bash
#SBATCH --job-name=nps2875-full-count
#SBATCH --partition=production
#SBATCH --time=00:30:00
#SBATCH --mem=8G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/nps2875_full_count_%j.out
#SBATCH --error=/farm_out/brash/nps2875_full_count_%j.err

set -euo pipefail

source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load hcana/1.1

repo=/work/hallc/gep/brash/gep4_nps_run6151_private/nps_replay_develop
export LD_LIBRARY_PATH=/work/hallc/gep/brash/gep4_nps_run6151_private/NPSlib_current/BUILD/src:${LD_LIBRARY_PATH:-}
cd "$repo"
source setup.sh

hcana -l -q 'SCRIPTS/NPS/replay_production_coin_NPS_HMS.C(2875,-1)'

root -l -b -q -e '
  TFile f("ROOTfiles/COIN/PRODUCTION/nps_hms_coin_2875_1_-1.root");
  TTree *t=(TTree*)f.Get("T");
  std::cout << "FULL_RUN_2875_TREE_ENTRIES="
            << (t ? t->GetEntries() : -1) << std::endl;
'
