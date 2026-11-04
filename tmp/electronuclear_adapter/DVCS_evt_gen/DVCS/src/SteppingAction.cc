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
/// \file analysis/shared/src/SteppingAction.cc
/// \brief Implementation of the SteppingAction class
//
//
// $Id: SteppingAction.cc 67226 2013-02-08 12:07:18Z ihrivnac $
//
// 
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "SteppingAction.hh"

#include "DetectorConstruction.hh"
#include "EventAction.hh"

#include "G4Step.hh"

#include "G4TouchableHistory.hh"
#include "HistoManager.hh"
#include "dvcsGlobals.hh"
#include "G4VProcess.hh"

#include <vector>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SteppingAction::SteppingAction(DetectorConstruction* det,
			       EventAction* evt,
			       HistoManager* histo)
  : G4UserSteppingAction(), 
    fDetector(det), fEventAction(evt),
    fHistoManager(histo), fEne(), fPID(),
    fTime_opening(-999.), fID_opening(-999)
{
  fEne.resize(1080,0.), fPID.resize(1080,0);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SteppingAction::~SteppingAction()
{ }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SteppingAction::UserSteppingAction(const G4Step* aStep)
{
  G4Track* track = aStep->GetTrack();

  // NPS trigger studies concern prompt particles reaching a calorimeter only
  // 3.57 m from the target.  Charged hadronic secondaries can otherwise curl
  // indefinitely in the sweeper field or become trapped at passive-geometry
  // boundaries.  These limits are deliberately far outside the prompt flight
  // envelope, but prevent non-contributing tracks from exhausting memory.
  if (track->GetGlobalTime() > 100.*CLHEP::ns ||
      track->GetTrackLength() > 20.*CLHEP::m ||
      track->GetCurrentStepNumber() > 100000) {
    track->SetTrackStatus(fStopAndKill);
    return;
  }

  // get volume of the current step
  G4VPhysicalVolume* volume_pre 
    = aStep->GetPreStepPoint()->GetTouchableHandle()->GetVolume();

  G4VPhysicalVolume* volume_post
    = aStep->GetPostStepPoint()->GetTouchableHandle()->GetVolume();

  if (dvcsGlobals::primary_mode == 3) {
    const G4VProcess* process =
      aStep->GetPostStepPoint()->GetProcessDefinedStep();
    if (process && (process->GetProcessName() == "electronNuclear" ||
                    process->GetProcessName() == "photonNuclear")) {
      const std::vector<const G4Track*>* secondaries =
        aStep->GetSecondaryInCurrentStep();
      G4cout << "BEAM_INTERACTION event=" << fEventAction->GetEventNb()
             << " process=" << process->GetProcessName()
             << " parent_pid=" << track->GetParticleDefinition()->GetPDGEncoding()
             << " parent_weight=" << track->GetWeight()
             << " parent_E_GeV=" << track->GetTotalEnergy()/CLHEP::GeV
             << " z_cm=" << track->GetPosition().z()/CLHEP::cm
             << " nsecondary=" << secondaries->size();
      for (std::size_t i = 0; i < secondaries->size(); ++i) {
        const G4Track* secondary = (*secondaries)[i];
        G4cout << " secondary" << i << "_pid="
               << secondary->GetParticleDefinition()->GetPDGEncoding()
               << " secondary" << i << "_weight=" << secondary->GetWeight()
               << " secondary" << i << "_E_GeV="
               << secondary->GetTotalEnergy()/CLHEP::GeV;
      }
      G4cout << G4endl;
    }
  }

  // Record the first active PbWO4 crystal reached by the primary electron.
  // This separates an upstream/front-wrapper loss or miss from shower leakage.
  if (track->GetTrackID() == 1
      && track->GetDynamicParticle()->GetParticleDefinition()->GetPDGEncoding() == 11
      && volume_pre->GetLogicalVolume()->GetName() == "Crystal_log"
      && aStep->GetPreStepPoint()->GetStepStatus() == fGeomBoundary) {
    G4TouchableHistory* touchable =
      (G4TouchableHistory*)(aStep->GetPreStepPoint()->GetTouchable());
    G4VPhysicalVolume* cellPhysical = touchable->GetVolume(2);
    G4VPhysicalVolume* columnPhysical = touchable->GetVolume(1);
    G4int rowNo = cellPhysical->GetCopyNo();
    G4int columnNo = columnPhysical->GetCopyNo();
    G4int hitID = columnNo + 36*rowNo;
    fHistoManager->SetNPSEntry(hitID,
                               track->GetTotalEnergy()/CLHEP::GeV,
                               track->GetPosition()/CLHEP::mm,
                               track->GetMomentum()/CLHEP::GeV);
  }

  /*if(
     volume_pre->GetLogicalVolume()->GetName() == "HMS_opening_log"
     && aStep->GetPreStepPoint()->GetStepStatus() == fGeomBoundary
     && track->GetDynamicParticle()->GetParticleDefinition()->GetPDGEncoding() == 11 //electron
     ){
    fTime_opening = track->GetProperTime();
    fID_opening = track->GetTrackID();
  }*/
  if(
     /*volume_pre->GetLogicalVolume()->GetName() == "HMS_exit_log"
     && aStep->GetPreStepPoint()->GetStepStatus() == fGeomBoundary
     && */track->GetDynamicParticle()->GetParticleDefinition()->GetPDGEncoding() == 11 //electron
    //  &&track->GetProperTime() > fTime_opening
    //  &&track->GetTrackID() == fID_opening
     ){
    G4double energy = track->GetTotalEnergy()/CLHEP::GeV;//MeV to GeV
    G4ThreeVector mom = track->GetMomentum()/CLHEP::GeV;//MeV to GeV
    fEventAction->DefineHMS_elec(mom, energy);
  } // HMS acceptance in G4 are not required (done by MC-single arm for 2023-2024 DVCS experiment in Hall C)

  //All of these are temporary
  if(
     volume_pre->GetLogicalVolume()->GetName() == "FluxOptFiber_log"
     // Now, going to use this to compare the # of particles hitting the NPS and the # of particles reconstructed
     && volume_post->GetLogicalVolume()->GetName() == "Mother_log"
     ){
    G4int PID = track->GetDynamicParticle()->GetParticleDefinition()->GetPDGEncoding();
    G4double ene = track->GetTotalEnergy();
    G4ThreeVector position = track->GetPosition();
    G4ThreeVector mom = track->GetMomentum();

    // getting a position of particle in local coordinate.
    const G4AffineTransform transformation = aStep->GetPreStepPoint()->GetTouchable()->GetHistory()->GetTopTransform();
    G4ThreeVector localPosition = transformation.TransformPoint(position);
    G4int evtNb = fEventAction->GetEventNb();
    fHistoManager->SetFluxOptFiberEnergyandPID(evtNb, PID, ene, mom, position, localPosition);
    fHistoManager->FillNtupleFluxOptFiber(PID);
  }

  // collect energy and track length step by step
  G4double edep = aStep->GetTotalEnergyDeposit();

  if(// This does not work that well.
     volume_pre->GetLogicalVolume()->GetName() == "WrapFront_log"
     //     && volume_post->GetLogicalVolume()->GetName() == "Crystal_log"
     ){

    G4TouchableHistory* touchable
      = (G4TouchableHistory*)(aStep->GetPreStepPoint()->GetTouchable());

    //from B5HadCalorimeterSD.cc
    //crystal numbering. column by column!
    G4VPhysicalVolume* cellPhysical = touchable->GetVolume(2);
    G4int rowNo = cellPhysical->GetCopyNo();//0~29(30) in total
    G4VPhysicalVolume* columnPhysical = touchable->GetVolume(1);
    G4int columnNo = columnPhysical->GetCopyNo();//0~35(36) in total
    G4int hitID = columnNo+36*rowNo;//0~1079(1080)
    
    G4Track* track = aStep->GetTrack();
    G4int PID = track->GetDynamicParticle()->GetParticleDefinition()->GetPDGEncoding();
    G4double ene = track->GetTotalEnergy();

    fHistoManager->SetFluxEnergyandPID(hitID, PID, ene);
    fHistoManager->FillNtuple2();

  }

  if(
     volume_pre->GetLogicalVolume()->GetName() == "Flux_log"
     // && volume_post->GetLogicalVolume()->GetName() != "Flux_log"
     ){
    G4Track* track = aStep->GetTrack();
    G4int PID = track->GetDynamicParticle()->GetParticleDefinition()->GetPDGEncoding();
    G4double ene = track->GetTotalEnergy();
    G4ThreeVector position = track->GetPosition();

    fHistoManager->SetFluxSphereEnergyandPID(PID, ene, position);
    fHistoManager->FillNtupleFluxSphere(PID);//Changed its name.
  }

  G4double stepl = 0.;
  if (aStep->GetTrack()->GetDefinition()->GetPDGCharge() != 0.)
    stepl = aStep->GetStepLength();
      
  if (volume_pre->GetName() == "Target_log") fEventAction->AddAbs(edep,stepl);
  if (volume_pre->GetName() == "Crystal_log")      fEventAction->AddGap(edep,stepl);
}

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
