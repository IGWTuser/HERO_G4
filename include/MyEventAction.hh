#ifndef MyEventAction_h
#define MyEventAction_h 1

#include "G4UserEventAction.hh"
#include "MySteppingAction.hh"
#include "G4String.hh"
#include "G4SystemOfUnits.hh"
#include <vector>
#include <utility>

class CSVWriter;

// Класс для обработки каждого события (одного выстрела частицы)
// Собирает статистику по нейтронам и записывает в CSV
class MyEventAction : public G4UserEventAction {
public:
    MyEventAction(MySteppingAction* steppingAction,
                  const G4String& dataDir = "../data");
    virtual ~MyEventAction();

    // Вызывается в начале каждого события
    virtual void BeginOfEventAction(const G4Event* event) override;
    
    // Вызывается в конце события — здесь пишем данные в CSV
    virtual void EndOfEventAction(const G4Event* event) override;

    void SaveSummaryData();
    
    // Методы для передачи параметров из main
    void SetCSVWriter(CSVWriter* writer) { fCSVWriter = writer; }
    void SetCurrentParticle(const G4String& name) { fCurrentParticleName = name; }
    void SetCurrentEnergy(G4double energy) { fCurrentEnergy = energy; }
    void SetGlobalEventNumber(int num) { fGlobalEventNumber = num; }
    
    // Подсчитывает нейтроны с энергией ниже порога для всех задержек
    std::vector<int> CountLowEnergyNeutrons(G4double energyThreshold = 1.0 * eV) const;

private:
    // Вспомогательные методы для форматирования имён файлов
    G4String BuildBaseNameFromPrimary(const G4Event* event) const;
    static G4String FormatEnergy(G4double e);
    static G4String FormatDelay(G4double t);
    static G4String Sanitize(const G4String& s);

private:
    MySteppingAction* fSteppingAction;  // ссылка на SteppingAction для доступа к данным
    G4String          fDataDirectory;    // папка для выходных файлов
    
    // Параметры для записи в CSV
    CSVWriter*        fCSVWriter;
    G4String          fCurrentParticleName;
    G4double          fCurrentEnergy;
    int               fGlobalEventNumber;
};

#endif
