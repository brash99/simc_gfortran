//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file analysis/AnaEx02/AnaEx02.cc
/// \brief Main program of the analysis/AnaEx02 example
//
//
// $Id: AnaEx02.cc 81444 2014-05-28 14:28:20Z gcosmo $
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "PhysicsList.hh"

#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"
#include "HistoManager.hh"

#include "B3StackingAction.hh"

#include "dvcsGlobals.hh"
#include "TDVCSDB.h"
#include "TCaloEvent.h"

#include <ctime>//for cout time
#include <cstdlib>

// #ifdef G4VIS_USE
// #include "G4VisExecutive.hh"
// #endif

// #ifdef G4UI_USE
// #include "G4UIExecutive.hh"
// #endif

#include "G4UIExecutive.hh"
#include "G4VisManager.hh"
#include "G4VisExecutive.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int dvcsGlobals::run_number = 0;
double dvcsGlobals::Ebeam = 0;
double dvcsGlobals::HMS_angle = 0;
double dvcsGlobals::HMS_momentum = 0;
double dvcsGlobals::Calo_distance = 0;
double dvcsGlobals::Calo_angle = 0;
int dvcsGlobals::target_type = 0;
int dvcsGlobals::target_gen_proc_type = 0;
double dvcsGlobals::target_density = 0;
double dvcsGlobals::target_offset = 0;
double dvcsGlobals::target_length = 0;
bool dvcsGlobals::hit_HMS_CALO_flag = false;

int    dvcsGlobals::primary_mode = 0;
double dvcsGlobals::mono_energy = 0.;
double dvcsGlobals::mono_angle = 0.;
double dvcsGlobals::mono_vertex_z = 0.;
double dvcsGlobals::mono_local_x = 0.;
double dvcsGlobals::mono_local_y = 0.;
double dvcsGlobals::mono_nps_distance = 0.;
std::string dvcsGlobals::simc_event_file;
double dvcsGlobals::simc_nps_angle = 0.;
double dvcsGlobals::simc_nps_distance = 0.;
bool dvcsGlobals::simc_keep_target_material = false;

//bool   dvcsGlobals::SM_field_flag = true;
bool   dvcsGlobals::SM_field_flag = false;
double dvcsGlobals::SM_field_str  = 1.;

double dvcsGlobals::SM_angle = 0.;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc,char** argv)
{
if (argc != 5 && argc != 8 && argc != 9 && argc != 11 && argc != 12) {
  G4cerr << "Usage: " << argv[0]
         << " <run> <macro> <target> <process>"
         << " [--mono-electron <energy_GeV> <theta_deg> <vertex_z_cm>"
         << " [<nps_local_x_mm> <nps_local_y_mm> [<nps_distance_m>]]]"
         << "\n       " << argv[0]
         << " <run> <macro> <target> <process>"
         << " --simc-events <csv> <nps_angle_deg> <nps_distance_m>"
         << "\n       " << argv[0]
         << " <run> <macro> <target> <process>"
         << " --simc-events-target-on <csv> <nps_angle_deg> <nps_distance_m>"
         << "\n       " << argv[0]
         << " <run> <macro> <target> <process>"
         << " --simc-pi0-events <csv> <nps_angle_deg> <nps_distance_m>"
         << "\n       " << argv[0]
         << " <run> <macro> <target> <process>"
         << " --simc-hadron-events <csv> <nps_angle_deg> <nps_distance_m>"
         << "\n       " << argv[0]
         << " <run> <macro> <target> <process>"
         << " --phase-space-events <csv> <nps_angle_deg> <nps_distance_m>"
         << "\n       " << argv[0]
         << " <run> <macro> <target> <process>"
         << " --beam-electron <total_energy_GeV> <start_z_cm> <nps_distance_m>"
         << "\n       " << argv[0]
         << " <run> <macro> <target> <process>"
         << " --electronuclear-events <total_energy_GeV> <nps_distance_m>"
         << G4endl;
  return 2;
}

if (argc == 8 && G4String(argv[5]) == "--electronuclear-events") {
  dvcsGlobals::primary_mode = 4;
  dvcsGlobals::mono_energy = std::atof(argv[6]);
  dvcsGlobals::mono_nps_distance = 100. * std::atof(argv[7]);
  if (dvcsGlobals::mono_energy <= 0.510999e-3 ||
      dvcsGlobals::mono_nps_distance <= 0.) {
    G4cerr << "Electronuclear energy and NPS distance must be positive."
           << G4endl;
    return 2;
  }
} else if (argc == 9 && G4String(argv[5]) == "--beam-electron") {
  dvcsGlobals::primary_mode = 3;
  dvcsGlobals::mono_energy = std::atof(argv[6]);
  dvcsGlobals::mono_vertex_z = std::atof(argv[7]);
  dvcsGlobals::mono_nps_distance = 100. * std::atof(argv[8]);
  if (dvcsGlobals::mono_energy <= 0.510999e-3 ||
      dvcsGlobals::mono_nps_distance <= 0.) {
    G4cerr << "Beam-electron energy and NPS distance must be positive."
           << G4endl;
    return 2;
  }
} else if (argc == 9 && (G4String(argv[5]) == "--simc-events" ||
                  G4String(argv[5]) == "--simc-events-target-on" ||
                  G4String(argv[5]) == "--simc-pi0-events" ||
                  G4String(argv[5]) == "--simc-hadron-events" ||
                  G4String(argv[5]) == "--phase-space-events")) {
  dvcsGlobals::primary_mode = 2;
  dvcsGlobals::simc_keep_target_material =
      G4String(argv[5]) == "--simc-events-target-on" ||
      G4String(argv[5]) == "--simc-pi0-events" ||
      G4String(argv[5]) == "--simc-hadron-events";
  dvcsGlobals::simc_event_file = argv[6];
  dvcsGlobals::simc_nps_angle = std::atof(argv[7]) * CLHEP::deg;
  dvcsGlobals::simc_nps_distance = 100. * std::atof(argv[8]);
  if (dvcsGlobals::simc_event_file.empty() ||
      dvcsGlobals::simc_nps_distance <= 0.) {
    G4cerr << "SIMC event file must be nonempty and NPS distance positive."
           << G4endl;
    return 2;
  }
} else if (argc == 9 || argc == 11 || argc == 12) {
  if (G4String(argv[5]) != "--mono-electron") {
    G4cerr << "Unknown generator option: " << argv[5] << G4endl;
    return 2;
  }
  dvcsGlobals::primary_mode = 1;
  dvcsGlobals::mono_energy = std::atof(argv[6]);
  dvcsGlobals::mono_angle = std::atof(argv[7]) * CLHEP::deg;
  dvcsGlobals::mono_vertex_z = std::atof(argv[8]);
  if (argc >= 11) {
    dvcsGlobals::mono_local_x = std::atof(argv[9]);
    dvcsGlobals::mono_local_y = std::atof(argv[10]);
  }
  if (argc == 12) {
    dvcsGlobals::mono_nps_distance = 100. * std::atof(argv[11]);
    if (dvcsGlobals::mono_nps_distance <= 0.) {
      G4cerr << "Explicit NPS distance must be positive." << G4endl;
      return 2;
    }
  }
  if (dvcsGlobals::mono_energy <= 0.510999e-3) {
    G4cerr << "Mono-electron total energy must exceed the electron rest mass."
           << G4endl;
    return 2;
  }
}

// current date/time based on current system
time_t now = time(0);

TDVCSDB *db=new TDVCSDB("dvcs", "clrlpc", 3306, "munoz", "");

G4int run_number = atoi(argv[1]);
G4double *Calo_distance;
G4double *Calo_angle;
G4double *BeamEnergy;
G4double *HMSangle;
G4double *HMSmomentum;
G4double *SM_angle;

Calo_distance=db->GetEntry_d("CALO_geom_Dist", run_number);
Calo_angle=db->GetEntry_d("CALO_geom_Yaw", run_number);
BeamEnergy = db->GetEntry_d("BEAM_param_Energy", run_number);
HMSangle = db->GetEntry_d("SIMU_param_HMSangle", run_number);
HMSmomentum = db->GetEntry_d("SIMU_param_HMSmomentum", run_number);
SM_angle = db->GetEntry_d("SM_geom_Yaw", run_number);

dvcsGlobals::Ebeam = BeamEnergy[0];//[GeV]
dvcsGlobals::HMS_angle = HMSangle[0];//[rad]
dvcsGlobals::HMS_momentum = HMSmomentum[0];//[GeV]
dvcsGlobals::Calo_distance = Calo_distance[0];//[cm]
dvcsGlobals::Calo_angle = Calo_angle[0];//[rad]  

if (dvcsGlobals::primary_mode == 1 && dvcsGlobals::mono_nps_distance > 0.) {
  dvcsGlobals::Calo_distance = dvcsGlobals::mono_nps_distance;
  dvcsGlobals::Calo_angle = dvcsGlobals::mono_angle;
}
if (dvcsGlobals::primary_mode == 3 && dvcsGlobals::mono_nps_distance > 0.) {
  // Beam mode preserves the authoritative run-database calorimeter angle.
  dvcsGlobals::Calo_distance = dvcsGlobals::mono_nps_distance;
}
if (dvcsGlobals::primary_mode == 4 && dvcsGlobals::mono_nps_distance > 0.) {
  dvcsGlobals::Calo_distance = dvcsGlobals::mono_nps_distance;
}
if (dvcsGlobals::primary_mode == 2) {
  dvcsGlobals::Calo_distance = dvcsGlobals::simc_nps_distance;
  dvcsGlobals::Calo_angle = dvcsGlobals::simc_nps_angle;
}

dvcsGlobals::target_type = atoi(argv[3]); //"0" : hydrogen, "1" : deuterium
dvcsGlobals::target_gen_proc_type = atoi(argv[4]);//"0" : proton, "1" : neutron, "2" : deutron

dvcsGlobals::run_number = run_number;
dvcsGlobals::SM_angle = SM_angle[0];

int targ_type = dvcsGlobals::target_type;
cout<<"targ_type = "<<targ_type<<endl;
if( targ_type == 0 ) { dvcsGlobals::target_density = 0.0708; cout<<"Using Hydrogen Target"<<endl;} // Hydrogen
else if( targ_type == 1 ) { dvcsGlobals::target_density = 0.1638; cout<<"Using Deuterium Target"<<endl;}  // Deuterium
int target_gen_proc_type = dvcsGlobals::target_gen_proc_type;
if( target_gen_proc_type == 0 ) {cout<<"Process to be genarated is DVCS on proton"<<endl;}
else if ( target_gen_proc_type == 1 ) {cout<<"Process to be genarated is DVCS on neutron"<<endl;}
else if ( target_gen_proc_type == 2 ) {cout<<"Process to be genarated is DVCS on deutron"<<endl;}

dvcsGlobals::target_length = 10.; // [cm]

if (dvcsGlobals::primary_mode == 1) {
  G4cout << "Primary mode: mono electron" << G4endl
         << "  total energy [GeV]: " << dvcsGlobals::mono_energy << G4endl
         << "  Hall theta [deg]: " << dvcsGlobals::mono_angle / CLHEP::deg << G4endl
         << "  vertex z [cm]: " << dvcsGlobals::mono_vertex_z << G4endl
         << "  NPS local face x,y [mm]: " << dvcsGlobals::mono_local_x
         << ", " << dvcsGlobals::mono_local_y << G4endl
         << "  explicit NPS distance [m]: "
         << dvcsGlobals::mono_nps_distance / 100. << G4endl;
}
if (dvcsGlobals::primary_mode == 3) {
  G4cout << "Primary mode: beam electron through target" << G4endl
         << "  total energy [GeV]: " << dvcsGlobals::mono_energy << G4endl
         << "  start z [cm]: " << dvcsGlobals::mono_vertex_z << G4endl
         << "  direction: +z" << G4endl
         << "  target/cell material: ON" << G4endl
         << "  explicit NPS distance [m]: "
         << dvcsGlobals::mono_nps_distance / 100. << G4endl;
}
if (dvcsGlobals::primary_mode == 4) {
  G4cout << "Primary mode: direct conditional electronuclear events" << G4endl
         << "  total beam-electron energy [GeV]: "
         << dvcsGlobals::mono_energy << G4endl
         << "  target liquid path [cm]: 10.0" << G4endl
         << "  NPS angle: run-database setting" << G4endl
         << "  explicit NPS distance [m]: "
         << dvcsGlobals::mono_nps_distance / 100. << G4endl;
}
if (dvcsGlobals::primary_mode == 2) {
  G4cout << "Primary mode: SIMC event stream" << G4endl
         << "  input CSV: " << dvcsGlobals::simc_event_file << G4endl
         << "  NPS angle [deg]: "
         << dvcsGlobals::simc_nps_angle / CLHEP::deg << G4endl
         << "  NPS distance [m]: "
         << dvcsGlobals::simc_nps_distance / 100. << G4endl
         << "  target/cell material: "
         << (dvcsGlobals::simc_keep_target_material
                 ? "ON (diagnostic double-counting test)" : "vacuum")
         << G4endl;
}

G4cout<<"calo angle!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! : "<<dvcsGlobals::Calo_angle*TMath::RadToDeg()<<G4endl;
G4cout<<"calo dist!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! : "<<dvcsGlobals::Calo_distance<<G4endl;
G4cout<<"spectro angle!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! : "<<dvcsGlobals::HMS_angle*TMath::RadToDeg()<<G4endl;

delete Calo_distance;
delete Calo_angle;
delete BeamEnergy;
delete HMSangle;
delete HMSmomentum;

delete db;

// Choose the Random engine
G4Random::setTheEngine(new CLHEP::RanecuEngine);

G4RunManager* runManager = new G4RunManager;

// Set mandatory initialization classes
//
DetectorConstruction* detector = new DetectorConstruction(dvcsGlobals::Calo_distance*10./*cm to mm*/, dvcsGlobals::Calo_angle/*radian*/, dvcsGlobals::SM_field_flag, dvcsGlobals::SM_field_str);
runManager->SetUserInitialization(detector);

PhysicsList* phys = new PhysicsList;
runManager->SetUserInitialization(phys);          

// set an HistoManager
//
HistoManager*  histo = new HistoManager();

G4String fileNamee;
G4cout<<"Name of the output file?"<<G4endl;
if (!(G4cin>>fileNamee)) {
  G4cerr << "Missing output filename on standard input." << G4endl;
  return 3;
}

G4int index;
long seed1, seed2;
G4cout<<"index for Rancu seed table?"<<G4endl;
if (!(G4cin>>index)) {
  G4cerr << "Missing Ranecu seed-table index on standard input." << G4endl;
  return 3;
}
G4cout<<"seed1?"<<G4endl;
if (!(G4cin>>seed1)) {
  G4cerr << "Missing first Ranecu seed on standard input." << G4endl;
  return 3;
}
G4cout<<"seed2?"<<G4endl;
if (!(G4cin>>seed2)) {
  G4cerr << "Missing second Ranecu seed on standard input." << G4endl;
  return 3;
}
if (index < 0 || index > 214 || seed1 <= 0 || seed2 <= 0) {
  G4cerr << "Ranecu requires table index 0--214 and two positive seeds."
         << G4endl;
  return 3;
}

RunAction* run_action = new RunAction(histo, fileNamee, index, seed1, seed2);  
runManager->SetUserAction(run_action);

EventAction* event_action = new EventAction(run_action, histo);
runManager->SetUserAction(event_action);

PrimaryGeneratorAction* gen_action = 
    new PrimaryGeneratorAction(event_action, seed1, seed2);
runManager->SetUserAction(gen_action);

SteppingAction* stepping_action =
    new SteppingAction(detector, event_action, histo);
runManager->SetUserAction(stepping_action);

//To kill track such as secondary particle(ex, "cerenkov")
B3StackingAction* stack_action = new B3StackingAction;
runManager->SetUserAction(stack_action);          

G4UImanager* UImanager = G4UImanager::GetUIpointer();
  
if (argc == 5 || argc == 8 || argc == 9 || argc == 11 || argc == 12)   // batch mode
{
    G4String command = "/control/execute ";
    G4String fileName = argv[2];
    UImanager->ApplyCommand(command+fileName);   
}
else
{  // interactive mode : define visualization and UI terminal
    G4String command = "/control/execute ";
    G4String fileName = argv[2];
    // #ifdef G4VIS_USE
    G4VisManager* visManager = new G4VisExecutive;
    visManager->Initialize();
    // #endif
    // #ifdef G4UI_USE
    G4UIExecutive* ui = new G4UIExecutive(argc, argv);
    UImanager->ApplyCommand(command+fileName);
    ui->SessionStart();
    delete ui;
// #endif

// #ifdef G4VIS_USE
    delete visManager;
// #endif
}

// current date/time based on current system
time_t then = time(0);
time_t diff = then - now;
// convert now to string form
char* dt = ctime(&diff);
// convert now to tm struct for UTC
tm* gmtm = gmtime(&diff);
dt = asctime(gmtm);
G4cout << "It took :"<< dt <<"to finish the job." << G4endl;

// Job termination
delete runManager;

return 0;

}

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
