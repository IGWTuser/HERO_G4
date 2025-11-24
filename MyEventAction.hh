#ifndef MyEventAction_h
#define MyEventAction_h 1

#include "G4UserEventAction.hh"
#include "MySteppingAction.hh"
#include "G4String.hh"
#include <vector>
#include <utility>

/**
 * Класс для записи данных по событиям.
 * Имена файлов формируются автоматически как:
 *   <particle>_<energyTeV>_<delay_us>.txt
 * где:
 *   energyTeV  — энергия первичной частицы в TeV с 3 знаками (например, 1.000TeV)
 *   delay_us   — задержка в микросекундах с 3 знаками (например, 0.100us, 8.000us)
 * Для начального распределения используется задержка 0.000us:
 *   <particle>_<energyTeV>_0.000us.txt
 */
class MyEventAction : public G4UserEventAction {
public:
    // dataDir — относительный путь от build/ до папки CW/data
    MyEventAction(MySteppingAction* steppingAction,
                  const G4String& dataDir = "../data");
    virtual ~MyEventAction();

    virtual void BeginOfEventAction(const G4Event* event) override;
    virtual void EndOfEventAction(const G4Event* event) override;

    // Функция для сохранения итоговых данных по всем событиям
    void SaveSummaryData();

private:
    // Формирование базового имени: <particle>_<energyTeV>
    G4String BuildBaseNameFromPrimary(const G4Event* event) const;

    // Форматирование величин в имени файла
    static G4String FormatEnergy(G4double e); // всегда TeV, 3 знака
    static G4String FormatDelay(G4double t);  // всегда us, формат n.000us
    static G4String Sanitize(const G4String& s);

private:
    MySteppingAction* fSteppingAction;
    G4String          fDataDirectory;
};

#endif // MyEventAction_h
