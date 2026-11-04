#!/bin/bash
#SBATCH --job-name=nps-6887-pi0-target
#SBATCH --partition=production
#SBATCH --account=halla
#SBATCH --time=00:30:00
#SBATCH --mem=1G
#SBATCH --cpus-per-task=1
#SBATCH --output=/farm_out/brash/nps_6887_pi0_target_%j.out
#SBATCH --error=/farm_out/brash/nps_6887_pi0_target_%j.err

set -euo pipefail
source /etc/profile.d/modules.sh
module purge
module use /group/halla/modulefiles
module load geant4/11.2.1
module load root/6.30.04

work=/work/hallc/gep/brash/gep4_nps_geant4
app=$work/target_phase_space
source_events=$work/validation/nps_run6887_wiser_pi0_5m.hybrid.csv
output=$app/run6887_wiser_pi0_target_exit_cut1.root

/usr/bin/time -v "$app/build/gep_target_phase_space" \
  --source-events "$source_events" "$output" 1.0 6887002
