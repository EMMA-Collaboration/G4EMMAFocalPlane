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
// $Id$
// 
/// \file B4DetectorConstruction.cc
/// \brief Implementation of the B4DetectorConstruction class

#include "B4DetectorConstruction.hh"

#include "G4Material.hh"
#include "G4NistManager.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4PVReplica.hh"
#include "G4UniformMagField.hh"

#include "G4GeometryManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4GenericMessenger.hh"

#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"

#include <stdio.h>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

B4DetectorConstruction::B4DetectorConstruction()
 : G4VUserDetectorConstruction(),
   fMessenger(0),
   fMagField(0),
   fAbsorberPV(0),
   fGapPV(0),
   fCheckOverlaps(true)
{
  // Define /B4/det commands using generic messenger class
  fMessenger 
    = new G4GenericMessenger(this, "/B4/det/", "Detector construction control");

  // Define /B4/det/setMagField command
  G4GenericMessenger::Command& setMagFieldCmd
    = fMessenger->DeclareMethod("setMagField", 
                                &B4DetectorConstruction::SetMagField, 
                                "Define magnetic field value (in X direction");
  setMagFieldCmd.SetUnitCategory("Magnetic flux density");                                
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

B4DetectorConstruction::~B4DetectorConstruction()
{ 
  delete fMagField;
  delete fMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* B4DetectorConstruction::Construct()
{
  // Define materials 
  DefineMaterials();
  
  // Define volumes
  return DefineVolumes();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void B4DetectorConstruction::DefineMaterials()
{ 
  // Lead material defined using NIST Manager
  G4NistManager* nistManager = G4NistManager::Instance();
  G4bool fromIsotopes = false;
  nistManager->FindOrBuildMaterial("G4_Pb", fromIsotopes);
  nistManager->FindOrBuildMaterial("G4_Au", fromIsotopes);
  
  // Liquid argon material
  G4double a;  // mass of a mole;
  G4double z;  // z=mean number of protons;  
  G4double density;
  G4double PGACdensity = 0; 

  nistManager->BuildMaterialWithNewDensity("Butane","G4_BUTANE",PGACdensity);

  // Vacuum
  new G4Material("Galactic", z=1., a=1.01*g/mole,density= universe_mean_density,
                  kStateGas, 2.73*kelvin, 3.e-18*pascal);

  // Print materials
  G4cout << *(G4Material::GetMaterialTable()) << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* B4DetectorConstruction::DefineVolumes()
{
  // Geometry parameters
  G4double PGAC_Thickness = 20.*mm;
  G4double PGAC_XY  = 10.*cm;

  G4double worldSizeXY = 0.5*m;
  G4double worldSizeZ  = 0.5*m;

  G4double TargetXY = 3.*cm;
  G4double TargetThickness = 10*um; 

  G4double PGAC_dist = 20*cm;
  G4doubel PGAC_sep = 5*cm
  
  // Get materials
  G4Material* defaultMaterial = G4Material::GetMaterial("Galactic");
  G4Material* TargetMaterial = G4Materal::GetMaterial("G4_Au");
  G4Material* PGAC_Material = G4Material::GetMaterial("G4_BUTANE");
  
  if ( ! defaultMaterial || ! TargetMaterial || ! PGAC_Material ) {
    G4cerr << "Cannot retrieve materials already defined. " << G4endl;
    G4cerr << "Exiting application " << G4endl;
    exit(1);
  }  
   
  //     
  // World
  //
  G4VSolid* worldS 
    = new G4Box("World",           // its name
                 worldSizeXY/2, worldSizeXY/2, worldSizeZ/2); // its size
                         
  G4LogicalVolume* worldLV
    = new G4LogicalVolume(
                 worldS,           // its solid
                 defaultMaterial,  // its material
                 "World");         // its name
                                   
  G4VPhysicalVolume* worldPV
    = new G4PVPlacement(
                 0,                // no rotation
                 G4ThreeVector(),  // at (0,0,0)
                 worldLV,          // its logical volume                         
                 "World",          // its name
                 0,                // its mother  volume
                 false,            // no boolean operation
                 0,                // copy number
                 fCheckOverlaps);  // checking overlaps 
  
  // Target

  G4VSolid* TargetS
	= new G4Box("Target", TargetXY/2, TargetXY/2, TargetThickness/2);

  G4LogicalVolume* TargetLV
	= new G4LogicalVolume(worldS, TargetMaterial, "Target");

  G4VPhysicalVolume* TargetPV
	= new G4PVPlacement(0,G4ThreeVector(),TargetLV,"Target",0,false,0,fChackOverlaps); 


  //                               
  // PGAC(s)
  //  
  G4VSolid* PGAC_S
    = new G4Box("PGAC",     // its name
                 PGAC_XY/2, PGAC_XY/2, PGAC_Thickness/2); // its size
                         
  G4LogicalVolume* PGAC_LV
    = new G4LogicalVolume(
                  PGAC_S,     // its solid
                 defaultMaterial,  // its material
                 "Calorimeter");   // its name

  // Place PGAC 1                                 
  new G4PVPlacement(
                 Rotate1,                // no rotation
                 G4ThreeVector(PGAC_dist,0,0),  // at (0,0,0)
                 PGAC_LV,          // its logical volume                         
                 "PGAC(1)",    // its name
                 worldLV,          // its mother  volume
                 false,            // no boolean operation
                 0,                // copy number
                 fCheckOverlaps);  // checking overlaps

  // Place PGAC 2
  new G4PVPlacement(
                 Rotate1,                // no rotation
                 G4ThreeVector(PGAC_dist + PGAC_sep,0,0),  // at (0,0,0)
                 PGAC_LV,          // its logical volume                         
                 "PGAC(2)",    // its name
                 worldLV,          // its mother  volume
                 false,            // no boolean operation
                 0,                // copy number
                 fCheckOverlaps);  // checking overlaps

 
/*  
  //                                 
  // Layer
  //
  G4VSolid* layerS 
    = new G4Box("Layer",           // its name
                 calorSizeXY/2, calorSizeXY/2, layerThickness/2); // its size
                         
  G4LogicalVolume* layerLV
    = new G4LogicalVolume(
                 layerS,           // its solid
                 defaultMaterial,  // its material
                 "Layer");         // its name

  new G4PVReplica(
                 "Layer",          // its name
                 layerLV,          // its logical volume
                 calorLV,          // its mother
                 kZAxis,           // axis of replication
                 nofLayers,        // number of replica
                 layerThickness);  // witdth of replica
  */
  //                               
  // Absorber
  //
  G4VSolid* absorberS 
    = new G4Box("Abso",            // its name
                 calorSizeXY/2, calorSizeXY/2, absoThickness/2); // its size
                         
  G4LogicalVolume* absorberLV
    = new G4LogicalVolume(
                 absorberS,        // its solid
                 absorberMaterial, // its material
                 "Abso");          // its name
                                   
  fAbsorberPV
    = new G4PVPlacement(
                 0,                // no rotation
                 G4ThreeVector(0., 0., -gapThickness/2), // its position
                 absorberLV,       // its logical volume                         
                 "Abso",           // its name
                 worldLV,          // its mother  volume
                 false,            // no boolean operation
                 0,                // copy number
                 fCheckOverlaps);  // checking overlaps 

  //                               
  // Gap
  //
  G4VSolid* gapS 
    = new G4Box("Gap",             // its name
                 calorSizeXY/2, calorSizeXY/2, gapThickness/2); // its size
                         
  G4LogicalVolume* gapLV
    = new G4LogicalVolume(
                 gapS,             // its solid
                 gapMaterial,      // its material
                 "Gap");           // its name
                                   
  fGapPV
    = new G4PVPlacement(
                 0,                // no rotation
                 G4ThreeVector(0., 0., absoThickness/2), // its position
                 gapLV,            // its logical volume                         
                 "Gap",            // its name
                 worldLV,          // its mother  volume
                 false,            // no boolean operation
                 0,                // copy number
                 fCheckOverlaps);  // checking overlaps 
  
  //
  // print parameters
  //
  G4cout << "\n------------------------------------------------------------"
         << "\n---> The calorimeter is " << nofLayers << " layers of: [ "
         << absoThickness/mm << "mm of " << absorberMaterial->GetName() 
         << " + "
         << gapThickness/mm << "mm of " << gapMaterial->GetName() << " ] " 
         << "\n------------------------------------------------------------\n";
  
  //                                        
  // Visualization attributes
  //
  worldLV->SetVisAttributes (G4VisAttributes::Invisible);

  G4VisAttributes* simpleBoxVisAtt= new G4VisAttributes(G4Colour(1.0,1.0,1.0));
  simpleBoxVisAtt->SetVisibility(true);
  calorLV->SetVisAttributes(simpleBoxVisAtt);

  //
  // Always return the physical World
  //
  return worldPV;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void B4DetectorConstruction::SetMagField(G4double fieldValue)
{
  // Apply a global uniform magnetic field along X axis
  G4FieldManager* fieldManager
    = G4TransportationManager::GetTransportationManager()->GetFieldManager();

  // Delete the existing magnetic field
  if ( fMagField )  delete fMagField; 

  if ( fieldValue != 0. ) {
    // create a new one if not null
    fMagField 
      = new G4UniformMagField(G4ThreeVector(fieldValue, 0., 0.));
      
    fieldManager->SetDetectorField(fMagField);
    fieldManager->CreateChordFinder(fMagField);
  } 
  else {
    fMagField = 0;
    fieldManager->SetDetectorField(fMagField);
  }
}
