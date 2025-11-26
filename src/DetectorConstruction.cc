#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"

DetectorConstruction::DetectorConstruction() : G4VUserDetectorConstruction() {}

DetectorConstruction::~DetectorConstruction() {}

G4VPhysicalVolume* DetectorConstruction::Construct() {
    // Получаем менеджер материалов NIST
    G4NistManager* nist = G4NistManager::Instance();
    
    // === Мир (World) ===
    // Размеры: 10×10×10 метров
    G4double worldSize = 10*m;
    G4Box* solidWorld = new G4Box("World", worldSize/2, worldSize/2, worldSize/2);
    
    // Материал мира: вакуум (галактический)
    G4Material* worldMat = nist->FindOrBuildMaterial("G4_Galactic");
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, worldMat, "World");
    
    // Физический объём мира (корневой, без матери)
    G4VPhysicalVolume* physWorld = 
        new G4PVPlacement(0,                    // без вращения
                         G4ThreeVector(),       // в начале координат
                         logicWorld,            // логический объём
                         "World",               // имя
                         0,                     // нет материнского объёма
                         false,                 // не булев объём
                         0);                    // номер копии
    
    // === Детектор ===
    // Размеры: 1×1×1 метр
    G4double detectorSize = 1*m;
    G4Box* solidDetector = new G4Box("Detector", 
                                     detectorSize/2, 
                                     detectorSize/2, 
                                     detectorSize/2);
    
    // Материал детектора: свинец (хорошо замедляет нейтроны)
    G4Material* detectorMat = nist->FindOrBuildMaterial("G4_Pb");
    G4LogicalVolume* logicDetector = new G4LogicalVolume(solidDetector, 
                                                         detectorMat, 
                                                         "Detector");
    
    // Размещаем детектор в центре мира
    new G4PVPlacement(0,                    // без вращения
                     G4ThreeVector(),       // в центре мира
                     logicDetector,         // логический объём
                     "Detector",            // имя (важно — по нему ищем в SteppingAction)
                     logicWorld,            // внутри мира
                     false,                 // не булев объём
                     0);                    // номер копии
    
    // Возвращаем физический объём мира (всё остальное внутри него)
    return physWorld;
}
