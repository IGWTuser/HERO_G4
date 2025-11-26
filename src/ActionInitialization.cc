#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "MySteppingAction.hh"
#include "MyEventAction.hh"
#include "CSVWriter.hh"

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
    // В master-потоке ничего не делаем, он только управляет workers
}

void ActionInitialization::Build() const
{
    // Каждый worker создаёт свои собственные Action-объекты
    
    // Генератор первичных частиц
    SetUserAction(new PrimaryGeneratorAction());
    
    // SteppingAction отслеживает нейтроны на каждом шаге
    MySteppingAction* steppingAction = new MySteppingAction(fDelays);
    SetUserAction(steppingAction);
    
    // EventAction собирает статистику по событию и пишет в CSV
    MyEventAction* eventAction = new MyEventAction(steppingAction, "../data");
    eventAction->SetCSVWriter(fCSVWriter);       // передаём общий CSV writer
    eventAction->SetCurrentParticle(fParticleName);
    eventAction->SetCurrentEnergy(fEnergy);
    SetUserAction(eventAction);
}
