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
/// \file polarisation/Pol01/src/DetectorConstruction.cc
/// \brief Implementation of the DetectorConstruction class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "DetectorConstruction.hh"
#include "DetectorMessenger.hh"

#include "G4Material.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"

#include "G4GeometryManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"

#include "G4UnitsTable.hh"

#include "G4PolarizationManager.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"

#include "G4Element.hh"
#include <cmath>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction(),
  fWorld(0), fBox(0), fTargetMaterial(0), fWorldMaterial(0),
  fKaptonThickness(50*um), fAirThickness(1*m)
{
  fBoxSizeXY = 1*m;
  fBoxSizeZ = 50*um;
  fWorldSize = 3.*m;
  SetTargetMaterial("G4_KAPTON");  
  SetWorldMaterial("G4_Galactic");  
  fMessenger = new DetectorMessenger(this);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::~DetectorConstruction()
{ 
  delete fMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4VPhysicalVolume* DetectorConstruction::Construct()
{
  // Cleanup old geometry
  G4GeometryManager::GetInstance()->OpenGeometry();
  G4PhysicalVolumeStore::GetInstance()->Clean();
  G4LogicalVolumeStore::GetInstance()->Clean();
  G4SolidStore::GetInstance()->Clean();
  
  G4PolarizationManager::GetInstance()->Clean();
  
  
  /////////////////
     // Constants
    G4double T = 298 * kelvin;
    G4double RH = 0.25;
    
    G4double R = 8.314462618; // J/(mol·K)
    G4double p_total = 1.0 * atmosphere; // Pa

    // Partial pressure of water vapor
    G4double T_C = T - 273.15;  // convert to °C
    G4double p_sat = 6.1078 * std::pow(10.0, (7.5 * T_C) / (T_C + 237.3)) * 100.0;  // in Pa
    G4double p_H2O = RH * p_sat;                   // Pa
    G4double p_dry = p_total - p_H2O;

    // Molar masses (kg/mol)
    G4double M_H2O = 18.01528e-3;
    G4double M_dry = 28.96546e-3;

    // Molar fraction of water vapor
    G4double y_H2O = p_H2O / p_total;

    // Effective molar mass of humid air
    G4double M_humid = y_H2O * M_H2O + (1.0 - y_H2O) * M_dry;

    // Density from ideal gas law
    G4double rho = (p_total * M_humid) / (R * T);  // in kg/m3
    //rho *= g/cm3; // convert to Geant4 units
    
    // Define elements
    G4Element* C  = new G4Element("Carbon",   "C", 6., 12.011 * g/mole);
    G4Element* N  = new G4Element("Nitrogen", "N", 7., 14.007 * g/mole);
    G4Element* O  = new G4Element("Oxygen",   "O", 8., 15.999 * g/mole);
    G4Element* Ar = new G4Element("Argon",   "Ar", 18., 39.948 * g/mole);
    G4Element* H  = new G4Element("Hydrogen", "H", 1., 1.008 * g/mole);
    
    // Dry air composition (atomic fractions from G4_AIR)
    G4double frac_C = 0.000124 * (1.0 - y_H2O);
    G4double frac_N = 0.755268 * (1.0 - y_H2O);
    G4double frac_O = 0.231781 * (1.0 - y_H2O);
    G4double frac_Ar = 0.012827 * (1.0 - y_H2O);

    // Water vapor: H₂O = 2 H + 1 O → 3 atoms per molecule
    G4double frac_H = 2.0 / 3.0 * y_H2O;
    G4double frac_H2O_O = 1.0 / 3.0 * y_H2O;
    
   // Create humid air
   //    hAir = new G4Material("test;at",density=1.2*g/cm3,ncomponents=4, kStateGas, T, p_total);
   //G4Material* hAir = new G4Material("HumidsAir", density=rho * g/cm3, ncomponents=5);
   //humidAir->GetIonisation()->SetMeanExcitationEnergy(85.7 * eV);
 	G4double density = rho*g/cm3; //2.7 * g/cm3; // Define density with units
 	G4int ncomponents = 5; // Define density with units
 	G4cout << "Density = " << density << G4endl;
 G4Material* myMaterial = new G4Material("MyMaterial", density, ncomponents,kStateGas, T, p_total);
    // Total must sum to 1.0
    myMaterial->AddElement(C,  frac_C);
    myMaterial->AddElement(N,  frac_N);
    myMaterial->AddElement(O,  frac_O + frac_H2O_O); // O from dry air and H2O
    myMaterial->AddElement(Ar, frac_Ar);
    myMaterial->AddElement(H,  frac_H);
 
  
  /**************************************/	
  // World
  //
  G4Box*
  sWorld = new G4Box("World",                            //name
                   fWorldSize/2,fWorldSize/2,fWorldSize/2); //dimensions

  G4LogicalVolume*                                                                 
  lWorld = new G4LogicalVolume(sWorld,                   //shape
                               fWorldMaterial,           //material
                              "World");                  //name

  fWorld = new G4PVPlacement(0,                          //no rotation
                             G4ThreeVector(),            //at (0,0,0)
                             lWorld,                     //logical volume
                             "World",                    //name
                             0,                          //mother volume
                             false,                      //no boolean operation
                             0);                         //copy number
  G4double kaptonXY = 10*cm;
  G4double kaptonZ = fKaptonThickness;
  G4double GapZ = fAirThickness;
  G4double GapXY = 10*cm;

  G4LogicalVolume* lKapton = nullptr;
  if (kaptonZ > 0) {
    G4Box* sKapton = new G4Box("KaptonSolid",                            //name
                     kaptonXY/2, kaptonXY/2, kaptonZ/2); //dimensions

    lKapton = new G4LogicalVolume(sKapton,                        //its shape
                                G4NistManager::Instance()->FindOrBuildMaterial("G4_KAPTON"), //its material
                                "KaptonVolume");                   //its name

    new G4PVPlacement(0,                             //no rotation
                            G4ThreeVector(0, 0, -GapZ/2 - kaptonZ/2), //location
                            lKapton,                          //its logical volume
                            "KaptonPlacement",    //its name
                            lWorld,                        //its mother  volume
                            false,                         //no boolean operation
                            0);                            //copy number
  }
                         

  
  
  G4Box*
  sGap = new G4Box("GapSolid",                            //name
                   GapXY/2, GapXY/2, GapZ/2); //dimensions

  //G4Material* myHumidAir = CreateHumidAir(298.15 * kelvin, 0.25);
        
  G4LogicalVolume*
  lGap = new G4LogicalVolume(sGap,                        //its shape
                              G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR"), //its material
                              //myMaterial, // custom Humid air
                              "GapVolume");                   //its name

  G4PVPlacement* fGap = new G4PVPlacement(0,                             //no rotation
                            		G4ThreeVector(0, 0, 0), //location
                            		lGap,                          //its logical volume
                           		"GapPlacement",    //its name
 		                        lWorld,                        //its mother  volume
                 		        false,                         //no boolean operation
                            		0);                            //copy number
                            		
                           		
  // Screen Volume
  G4double ScreenXY = 10*cm;
  G4double ScreenZ = 10*um; 
  
  G4Box*
  sScreen = new G4Box("ScreenSolid",                            //name
                   ScreenXY/2, ScreenXY/2, ScreenZ/2); //dimensions

                   
  G4LogicalVolume*
  lScreen = new G4LogicalVolume(sScreen,                        //its shape
                              G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic"), //its material
                              "ScreenVolume");                   //its name

  G4PVPlacement* fScreen = new G4PVPlacement(0,                             //no rotation
                            		G4ThreeVector(0, 0, GapZ/2 + ScreenZ/2), //location
                            		lScreen,                          //its logical volume
                           		"ScreenPlacement",    //its name
 		                        lWorld,                        //its mother  volume
                 		        false,                         //no boolean operation
                            		0);                            //copy number

  // register logical Volume in PolarizationManager with zero polarization
  G4PolarizationManager * polMgr = G4PolarizationManager::GetInstance();
  polMgr->SetVolumePolarization(lGap,G4ThreeVector(0.,0.,0.));
                           
  PrintParameters();
  
  //always return the root volume
  //
  return fWorld;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::PrintParameters()
{
  G4cout << "\n The Box is " << G4BestUnit(fBoxSizeXY,"Length")
         << " x " << G4BestUnit(fBoxSizeXY,"Length")
         << " x " << G4BestUnit(fBoxSizeZ,"Length")
         << " of " << fTargetMaterial->GetName() << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetTargetMaterial(G4String materialChoice)
{
  // search the material by its name
  G4Material* mat =
    G4NistManager::Instance()->FindOrBuildMaterial(materialChoice);
  if (mat != fTargetMaterial) {
    if(mat) {
      fTargetMaterial = mat;
      UpdateGeometry();
    } else {
      G4cout << "### Warning!  Target material: <"
           << materialChoice << "> not found" << G4endl;  
    }     
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetWorldMaterial(G4String materialChoice)
{
  // search the material by its name
  G4Material* mat =
    G4NistManager::Instance()->FindOrBuildMaterial(materialChoice);
  if (mat != fWorldMaterial) {
    if(mat) {
      fWorldMaterial = mat;
      UpdateGeometry();
    } else {
      G4cout << "### Warning! World material: <"
           << materialChoice << "> not found" << G4endl;  
    }     
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetSizeXY(G4double value)
{
  fBoxSizeXY = value; 
  if (fWorldSize<fBoxSizeXY) fWorldSize = 1.5*fBoxSizeXY;
  UpdateGeometry();
}

void DetectorConstruction::SetSizeZ(G4double value)
{
  fBoxSizeZ = value; 
  if (fWorldSize<fBoxSizeZ) fWorldSize = 1.5*fBoxSizeZ;
  UpdateGeometry();
}

void DetectorConstruction::SetKaptonThickness(G4double val)
{
  fKaptonThickness = val;
  G4double requiredWorldSize = 2.0 * (fKaptonThickness + fAirThickness) + 20*cm;
  if (fWorldSize < requiredWorldSize) fWorldSize = requiredWorldSize;
  UpdateGeometry();
}

void DetectorConstruction::SetAirThickness(G4double val)
{
  fAirThickness = val;
  G4double requiredWorldSize = 2.0 * (fKaptonThickness + fAirThickness) + 20*cm;
  if (fWorldSize < requiredWorldSize) fWorldSize = requiredWorldSize;
  UpdateGeometry();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "G4RunManager.hh"

void DetectorConstruction::UpdateGeometry()
{
  if (fWorld) 
    G4RunManager::GetRunManager()->DefineWorldVolume(Construct());
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
