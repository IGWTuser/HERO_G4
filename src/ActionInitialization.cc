#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "MySteppingAction.hh"
#include "MyEventAction.hh"
#include "CSVWriter.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"

ActionInitialization::ActionInitialization(const std::vector<G4double>& delays,
                                           CSVWriter* csvWriter,
                                           const G4String& particleName,
                                           G4double energy)
    : G4VUserActionInitialization(),
      fDelays(delays),
      fCSVWriter(csvWriter),
      fParticleName(particleName),
      fEnergy(energy)
{
}

ActionInitialization::~ActionInitialization()
{
}

void ActionInitialization::BuildForMaster() const
{
    // Master не создаёт Actions
}

void ActionInitialization::Build() const
{
    // Каждый worker создаёт свои Action-объекты
    
    PrimaryGeneratorAction* primaryGen = new PrimaryGeneratorAction();
    
    G4ParticleGun* gun = primaryGen->GetParticleGun();
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* particle = particleTable->FindParticle(fParticleName);
    
    // Устанавливаем частицу и энергию для worker-потока
    gun->SetParticleDefinition(particle);
    gun->SetParticleEnergy(fEnergy);
    
    SetUserAction(primaryGen);
    
    // Отслеживание нейтронов
    MySteppingAction* steppingAction = new MySteppingAction(fDelays);
    SetUserAction(steppingAction);
    
    // Обработка событий и запись в CSV
    MyEventAction* eventAction = new MyEventAction(steppingAction, "../data");
    eventAction->SetCSVWriter(fCSVWriter);
    eventAction->SetCurrentParticle(fParticleName);
    eventAction->SetCurrentEnergy(fEnergy);
    SetUserAction(eventAction);
}
