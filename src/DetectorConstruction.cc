#include "DetectorConstruction.hh"

#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4Polyhedra.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

DetectorConstruction::DetectorConstruction() : G4VUserDetectorConstruction() { }

DetectorConstruction::~DetectorConstruction() { }

G4VPhysicalVolume* DetectorConstruction::Construct() {
    // Получаем менеджер материалов
    G4NistManager* nist = G4NistManager::Instance();
    
    // Получаем материалы из базы NIST:
    G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    
    // Создаем объем мира (world)
    G4double worldSize = 5.0 * m;
    G4Box* solidWorld = new G4Box("World", 0.5 * worldSize, 0.5 * worldSize, 0.5 * worldSize);
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, vacuum, "World");
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0, true);

    // Создаем композиционный материал (пример, здесь можно оставить ваш код для материала)
    // Например, материал уже создан ранее, или создайте его здесь.
    // Для примера создадим простой материал с плотностью 4.1 г/см³:
    G4Material* polystyrene = nist->FindOrBuildMaterial("G4_POLYSTYRENE");
    G4Material* tungsten = nist->FindOrBuildMaterial("G4_W");
    G4double compositeDensity = 4.1 * g/cm3;
    G4Material* compMaterial = new G4Material("CompMat", compositeDensity, 2);
    compMaterial->AddMaterial(polystyrene, 0.215);
    compMaterial->AddMaterial(tungsten, 0.785);
    
    // Параметры детектора: призма с правильным шестиугольным основанием
    G4double height = 1470 * mm;
    G4double zLow = -0.5 * height;
    G4double zHigh =  0.5 * height;
    G4int numSides = 6;
    G4double rInner = 0;          // Внутренний радиус (нет выреза)
    G4double rOuter = 800 * mm;     // Радиус описанной окружности шестиугольника

    // Для конструктора G4Polyhedra задаем число плоскостей и массивы значений
    G4int numZPlanes = 2;
    G4double zPlane[2]   = { zLow, zHigh };
    G4double rInnerArr[2] = { rInner, rInner };
    G4double rOuterArr[2] = { rOuter, rOuter };

    G4Polyhedra* solidDetector = new G4Polyhedra("Detector",
                                                  0.*deg,
                                                  360.*deg,
                                                  numSides,
                                                  numZPlanes,
                                                  zPlane,
                                                  rInnerArr,
                                                  rOuterArr);

    G4LogicalVolume* logicDetector = new G4LogicalVolume(solidDetector, compMaterial, "Detector");

    // Размещаем детектор в центре мира
    new G4PVPlacement(0, G4ThreeVector(), logicDetector, "Detector", logicWorld, false, 0, true);

    // Настройка визуальных атрибутов для детектора
    G4VisAttributes* visAttr = new G4VisAttributes(G4Colour(0.5, 0.5, 1.0));
    visAttr->SetForceSolid(true);
    logicDetector->SetVisAttributes(visAttr);

    return physWorld;
}
