#ifndef ActionInitialization_h
#define ActionInitialization_h 1

#include "G4VUserActionInitialization.hh"
#include "globals.hh"
#include <vector>

class CSVWriter;

// Этот класс нужен для многопоточного режима Geant4.
// Он создаёт отдельные экземпляры Action-классов для каждого потока.
class ActionInitialization : public G4VUserActionInitialization {
public:
    // Конструктор принимает параметры, которые будут использоваться
    // всеми worker-потоками: задержки, CSV writer, тип частицы и энергию
    ActionInitialization(const std::vector<G4double>& delays, 
                        CSVWriter* csvWriter,
                        const G4String& particleName,
                        G4double energy);
    virtual ~ActionInitialization();

    // BuildForMaster вызывается только в master-потоке (оставляем пустым)
    virtual void BuildForMaster() const override;
    
    // Build вызывается в каждом worker-потоке для создания своих Actions
    virtual void Build() const override;

private:
    std::vector<G4double> fDelays;     // временные задержки для подсчёта
    CSVWriter* fCSVWriter;              // общий для всех потоков
    G4String fParticleName;             // текущая частица
    G4double fEnergy;                   // текущая энергия
};

#endif
