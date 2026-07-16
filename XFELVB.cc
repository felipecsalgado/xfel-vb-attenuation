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
/// \file polarisation/Pol01/Pol01.cc
/// \brief Main program of the polarisation/Pol01 example
//
// 
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo...... 

#include "G4Types.hh"

#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh"
#else
#include "G4RunManager.hh"
#endif
#include "G4UImanager.hh"
#include "Randomize.hh"

#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "PrimaryGeneratorAction.hh"

#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"
#include "ActionInitialization.hh"

#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
 
int main(int argc,char** argv) {
 
  G4int nThreads = 5;
  G4String macroFile = "";
  for (int i = 1; i < argc; ++i) {
    G4String arg = argv[i];
    if (arg == "-t" || arg == "--threads") {
      if (i + 1 < argc) {
        nThreads = std::atoi(argv[++i]);
      }
    } else if (macroFile == "") {
      macroFile = arg;
    } else {
      nThreads = std::atoi(arg);
    }
  }

  // Instantiate G4UIExecutive if interactive mode
  G4UIExecutive* ui = nullptr;
  if ( macroFile == "" ) {
    ui = new G4UIExecutive(argc, argv);
  }

  //choose the Random engine
  CLHEP::HepRandom::setTheEngine(new CLHEP::RanluxEngine());
  
  // Construct the run manager
#ifdef G4MULTITHREADED
  G4MTRunManager * runManager = new G4MTRunManager;
  runManager->SetNumberOfThreads(nThreads);
  G4cout << "Running in multithreaded mode with " << nThreads << " threads." << G4endl;
#else
  G4RunManager * runManager = new G4RunManager;
  G4cout << "Running in single-threaded mode." << G4endl;
#endif

  // set mandatory initialization classes
  DetectorConstruction* det = new DetectorConstruction;
  runManager->SetUserInitialization(det);
  runManager->SetUserInitialization(new PhysicsList);
  runManager->SetUserInitialization(new ActionInitialization(det));

  G4VisManager* visManager = new G4VisExecutive;
  visManager->Initialize();
   
  //get the pointer to the User Interface manager
  G4UImanager* UImanager = G4UImanager::GetUIpointer();

  if (ui) {
    //interactive mode
    UImanager->ApplyCommand("/control/execute vis.mac");
    if (ui->IsGUI()) {
      UImanager->ApplyCommand("/control/execute gui.mac");
    }
    ui->SessionStart();
    delete ui;
  } else {
    //batch mode
    G4String command = "/control/execute ";
    G4String fileName = macroFile;
    UImanager->ApplyCommand(command+fileName);
  }

  // job termination
  delete visManager;
  delete runManager;
  return 0;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo...... 
