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
// $Id: B3StackingAction.cc 66536 2012-12-19 14:32:36Z ihrivnac $
// 
/// \file B3StackingAction.cc
/// \brief Implementation of the B3StackingAction class

#include "B3StackingAction.hh"
#include "dvcsGlobals.hh"

#include "G4Track.hh"
#include "G4NeutrinoE.hh"
#include "G4SystemOfUnits.hh"
#include "G4VPhysicalVolume.hh"

#include <cerrno>
#include <cstdlib>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

B3StackingAction::B3StackingAction()
  : fTargetSecondaryCut(0.), fDiscardedTracks(0),
    fDiscardedKineticEnergy(0.), fDiscardedWeightedKineticEnergy(0.)
{
  const char* value = std::getenv("GEP_TARGET_SECONDARY_CUT_GEV");
  if (value && *value) {
    char* end = 0;
    errno = 0;
    const G4double parsed = std::strtod(value, &end);
    if (errno || end == value || *end != '\0' || parsed < 0.) {
      G4ExceptionDescription message;
      message << "Invalid GEP_TARGET_SECONDARY_CUT_GEV=" << value;
      G4Exception("B3StackingAction::B3StackingAction",
                  "GEPELECTRONUCLEAR004", FatalException, message);
    }
    fTargetSecondaryCut = parsed * CLHEP::GeV;
  }
  G4cout << "TARGET_SECONDARY_CUT_GEV="
         << fTargetSecondaryCut/CLHEP::GeV << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

B3StackingAction::~B3StackingAction()
{
  G4cout << "TARGET_SECONDARY_DISCARD count=" << fDiscardedTracks
         << " kinetic_GeV=" << fDiscardedKineticEnergy/CLHEP::GeV
         << " weighted_kinetic_GeV="
         << fDiscardedWeightedKineticEnergy/CLHEP::GeV << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4ClassificationOfNewTrack
B3StackingAction::ClassifyNewTrack(const G4Track* track)
{

  if (dvcsGlobals::primary_mode != 4 && fTargetSecondaryCut > 0. &&
      track->GetParentID() > 0 &&
      track->GetKineticEnergy() < fTargetSecondaryCut) {
    const G4VPhysicalVolume* volume = track->GetVolume();
    const G4String name = volume ? volume->GetName() : "";
    if (name == "Target_pos" || name == "TargetSphere_pos") {
      ++fDiscardedTracks;
      fDiscardedKineticEnergy += track->GetKineticEnergy();
      fDiscardedWeightedKineticEnergy +=
          track->GetWeight() * track->GetKineticEnergy();
      return fKill;
    }
  }

  // //keep primary particle
  // if (track->GetParentID() == 0) return fUrgent;

  //remove the original
  // //kill secondary neutrino
  // if (track->GetDefinition() == G4NeutrinoE::NeutrinoE()) return fKill;
  // else return fUrgent;

  return fUrgent;//always return fUrgent when nothing is to be done to continue tracking, when no conditions are defined in this class.

  // fUrgent - track is placed in the urgent stack
  // fWaiting - track is placed in the waiting stack, and will not be simulated until the urgent stack is empty
  // fPostpone - track is postponed to the next event
  // fKill - the track is deleted immediately and not stored in any stack.

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
