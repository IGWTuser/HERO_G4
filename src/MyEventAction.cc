#include "MyEventAction.hh"
#include "MySteppingAction.hh"
#include "CSVWriter.hh"

#include "G4Event.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

MyEventAction::MyEventAction(MySteppingAction* steppingAction,
                             const G4String& dataDir)
  : G4UserEventAction(),
    fSteppingAction(steppingAction),
    fDataDirectory(dataDir),
    fCSVWriter(nullptr),
    fCurrentParticleName("unknown"),
    fCurrentEnergy(0.0),
    fGlobalEventNumber(0)
{ }

MyEventAction::~MyEventAction() {
    SaveSummaryData();
    G4cout << "Summary data saved." << G4endl;
}

void MyEventAction::BeginOfEventAction(const G4Event*) {
    fSteppingAction->Reset();
}

G4String MyEventAction::FormatEnergy(G4double e) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(3) << (e / TeV);
    return os.str() + "TeV";
}

G4String MyEventAction::FormatDelay(G4double t) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(3) << (t / microsecond);
    return os.str() + "us";
}

G4String MyEventAction::Sanitize(const G4String& s) {
    G4String result = s;
    std::replace(result.begin(), result.end(), '/', '_');
    std::replace(result.begin(), result.end(), ' ', '_');
    return result;
}

G4String MyEventAction::BuildBaseNameFromPrimary(const G4Event* event) const {
    auto vtx = event->GetPrimaryVertex(0);
    if (!vtx) return "unknown";
    auto prim = vtx->GetPrimary(0);
    if (!prim) return "unknown";

    auto def = prim->GetParticleDefinition();
    if (!def) return "unknown";

    G4String pName = Sanitize(def->GetParticleName());
    G4double pE    = prim->GetKineticEnergy();
    G4String eStr  = FormatEnergy(pE);
    return pName + "_" + eStr;
}

std::vector<int> MyEventAction::CountLowEnergyNeutrons(G4double energyThreshold) const {
    std::vector<int> counts;
    
    if (!fSteppingAction) {
        return counts;
    }
    
    auto const& maps = fSteppingAction->GetDelayedNeutronsMaps();
    counts.resize(maps.size(), 0);
    
    // Подсчитываем нейтроны с энергией < energyThreshold для каждой задержки
    for (size_t i = 0; i < maps.size(); ++i) {
        for (auto const& entry : maps[i]) {
            if (entry.second < energyThreshold) {
                counts[i]++;
            }
        }
    }
    
    return counts;
}

void MyEventAction::EndOfEventAction(const G4Event* event) {
    G4int eventID = event->GetEventID();
    const G4String baseName = BuildBaseNameFromPrimary(event);

    // Записываем в текстовые файлы (если нужно)
    auto const& maps       = fSteppingAction->GetDelayedNeutronsMaps();
    auto const& thresholds = fSteppingAction->GetThresholds();

    for (size_t i = 0; i < thresholds.size(); ++i) {
        G4String delayStr = FormatDelay(thresholds[i]);
        G4String fname = fDataDirectory + "/" + baseName + "_" + delayStr + ".txt";

        std::ofstream out(fname, std::ios::app);
        if (out.is_open()) {
            for (auto const& entry : maps[i]) {
                out << eventID << " " << entry.second / MeV << "\n";
            }
            out.close();
        }
    }

    // Записываем в CSV после каждого события
    if (fCSVWriter && fCSVWriter->IsOpen()) {
        std::vector<int> neutronCounts = CountLowEnergyNeutrons(1.0 * eV);
        
        fCSVWriter->WriteRow(
            fGlobalEventNumber,
            fCurrentParticleName,
            fCurrentEnergy / TeV,
            neutronCounts
        );
        
        fGlobalEventNumber++;
    }
}

void MyEventAction::SaveSummaryData() {
    // Ваш существующий код для summary (если нужно)
}
