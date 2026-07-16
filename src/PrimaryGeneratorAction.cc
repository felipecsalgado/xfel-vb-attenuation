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
/// \file polarisation/Pol01/src/PrimaryGeneratorAction.cc
/// \brief Implementation of the PrimaryGeneratorAction class
//
// 
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "PrimaryGeneratorAction.hh"

#include "DetectorConstruction.hh"

#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"



#include "Randomize.hh"

// Read the file
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <math.h>       /* sin */

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::PrimaryGeneratorAction(DetectorConstruction* det)
: G4VUserPrimaryGeneratorAction(), 
  fParticleGun(0), fDetector(det)                                               
{
  fParticleGun  = new G4ParticleGun(1);
  G4ParticleDefinition* particle
           = G4ParticleTable::GetParticleTable()->FindParticle("e-");
  fParticleGun->SetParticleDefinition(particle);
  fParticleGun->SetParticleEnergy(10*MeV);    
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0.,0.,1.));
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  //this function is called at the begining of event
  //
  G4double z0 = -0.5 * fDetector->GetAirThickness() - fDetector->GetKaptonThickness() - 1.0 * cm;
  
  
  //this function is called at the begining of event
  //
  //distribution uniform in solid angle
  //
  //G4double theta_div_max = 10.0e-3; //1.1e-3;
  
  //G4double range_phi = theta_div_max*1e4 - theta_div_min*1e4 + 1;
  //G4double phi = G4UniformRand() * 3.141592653589793;
  //G4double theta = theta_div_max * (2*G4UniformRand() - 1);
  
  
  //G4double sinTheta = std::sin(theta);
  //G4double cosTheta = std::cos(theta);
  
  //G4double ux = sinTheta*std::cos(phi),
  //         uy = sinTheta*std::sin(phi),
  //         uz = cosTheta; 

  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0,0,1));
  fParticleGun->SetParticleEnergy(8.766*keV);
  
  fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., z0));
  fParticleGun->GeneratePrimaryVertex(anEvent);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
