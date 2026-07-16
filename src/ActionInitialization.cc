#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"

ActionInitialization::ActionInitialization(DetectorConstruction* det)
 : G4VUserActionInitialization(), fDetector(det)
{}

ActionInitialization::~ActionInitialization()
{}

void ActionInitialization::BuildForMaster() const
{
  SetUserAction(new RunAction(fDetector, nullptr));
}

void ActionInitialization::Build() const
{
  PrimaryGeneratorAction* prim = new PrimaryGeneratorAction(fDetector);
  SetUserAction(prim);
  
  RunAction* run = new RunAction(fDetector, prim);
  SetUserAction(run);
  
  SetUserAction(new EventAction(run));
  
  SetUserAction(new SteppingAction(fDetector, prim, run));
}
