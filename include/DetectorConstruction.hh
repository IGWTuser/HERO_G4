#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"

// Класс для построения геометрии детектора
// Создаёт мир (World) и сам детектор с материалами
class DetectorConstruction : public G4VUserDetectorConstruction {
public:
    DetectorConstruction();
    virtual ~DetectorConstruction();

    // Вызывается для построения геометрии
    virtual G4VPhysicalVolume* Construct() override;
};

#endif
