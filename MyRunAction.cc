#include "MyRunAction.hh"
#include "CSVWriter.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"

G4int MyRunAction::sGlobalRunNumber = 0;

MyRunAction::MyRunAction(MyEventAction* eventAction, CSVWriter* csvWriter, const G4String& csvFilename)
    : G4UserRunAction(),
      fEventAction(eventAction),
      fCSVWriter(csvWriter),
      fCurrentParticleName("unknown"),
      fCurrentEnergy(0.0),
      fRunNumber(0)
{
    // Инициализируем sGlobalRunNumber из существующего файла
    if (sGlobalRunNumber == 0) {
        int lastRunNumber = CSVWriter::GetLastRunNumber(csvFilename);
        if (lastRunNumber >= 0) {
            sGlobalRunNumber = lastRunNumber + 1;
            G4cout << "Continuing from run number: " << sGlobalRunNumber 
                   << " (last run in file was " << lastRunNumber << ")" << G4endl;
        } else {
            G4cout << "Starting new CSV file from run number 0" << G4endl;
        }
    }
}

MyRunAction::~MyRunAction() {
}

void MyRunAction::BeginOfRunAction(const G4Run* run) {
    fRunNumber = sGlobalRunNumber;
    
    G4cout << "=== Run " << fRunNumber << " started ===" << G4endl;
    G4cout << "Particle: " << fCurrentParticleName << G4endl;
    G4cout << "Energy: " << fCurrentEnergy / GeV << " GeV" << G4endl;
    
    // Сброс статистики события
    if (fEventAction) {
        fEventAction->ResetRunStatistics();
    }
}

void MyRunAction::EndOfRunAction(const G4Run* run) {
    G4cout << "=== Run " << fRunNumber << " ended ===" << G4endl;
    
    // Получаем накопленные данные и записываем в CSV
    if (fEventAction && fCSVWriter && fCSVWriter->IsOpen()) {
        const std::vector<int>& neutronCounts = fEventAction->GetTotalNeutronCounts();
        
        fCSVWriter->WriteRow(
            fRunNumber,
            fCurrentParticleName,
            fCurrentEnergy / GeV,
            neutronCounts
        );
        
        G4cout << "Data written to CSV for run " << fRunNumber << G4endl;
    }
    
    sGlobalRunNumber++;
}