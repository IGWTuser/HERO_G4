#include "MyEventAction.hh"
#include "MySteppingAction.hh"

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
    fDataDirectory(dataDir)
{ }

MyEventAction::~MyEventAction() {
    SaveSummaryData();
    G4cout << "Summary data saved." << G4endl;
}

void MyEventAction::BeginOfEventAction(const G4Event*) {
    fSteppingAction->Reset();
}

// ---------- Форматирование ----------
G4String MyEventAction::FormatEnergy(G4double e) {
    // Энергию всегда печатаем в TeV с тремя знаками
    std::ostringstream os;
    os << std::fixed << std::setprecision(3) << (e / TeV);
    return os.str() + "TeV";
}

G4String MyEventAction::FormatDelay(G4double t) {
    // Всегда в микросекундах с точностью до тысячных: n.000us
    double usVal = t / us;
    // Избавляемся от артефактов двоичной арифметики (например, 0.099999...)
    usVal = std::round(usVal * 1000.0) / 1000.0;

    std::ostringstream os;
    os << std::fixed << std::setprecision(3) << usVal;
    return os.str() + "us";
}

G4String MyEventAction::Sanitize(const G4String& in) {
    G4String s = in;
    for (auto& ch : s) {
        if (!(std::isalnum(ch) || ch=='_' || ch=='-' )) ch = '_';
    }
    return s;
}

G4String MyEventAction::BuildBaseNameFromPrimary(const G4Event* event) const {
    const G4PrimaryVertex* vtx  = event ? event->GetPrimaryVertex() : nullptr;
    const G4PrimaryParticle* p0 = vtx ? vtx->GetPrimary() : nullptr;

    G4String pname = "unknown";
    G4double kinE  = 0.0;

    if (p0) {
        if (const G4ParticleDefinition* def = p0->GetG4code()) {
            pname = def->GetParticleName();
        }
        kinE = p0->GetKineticEnergy();
    }

    // <particle>_<energyTeV>
    return Sanitize(pname) + "_" + FormatEnergy(kinE);
}
// -----------------------------------

void MyEventAction::EndOfEventAction(const G4Event* event) {
    G4int eventID = event->GetEventID();
    const G4String baseName = BuildBaseNameFromPrimary(event);

    // 1) Начальные энергии вторичных нейтронов — задержка 0.000us
    {
        const G4String fname   = baseName + "_0.000us.txt";
        const std::string path = (fDataDirectory + "/" + fname).c_str();

        std::ofstream out(path.c_str(), std::ios::app);
        if (!out.is_open()) {
            G4cerr << "Error: cannot open " << path << " for writing!" << G4endl;
        } else {
            out << "Event " << eventID << "\nNeutronID\tInitialEnergy(eV)\n";
            for (auto const& entry : fSteppingAction->GetSecondaryNeutrons()) {
                out << entry.first << "\t"
                    << std::fixed << std::setprecision(6)
                    << entry.second / eV << "\n";
            }
            out << "----------------------------------------\n";
        }
    }

    // 2) Энергии для каждого порога задержки — <base>_<n.000us>.txt
    auto const& thresholds = fSteppingAction->GetThresholds();
    auto const& maps       = fSteppingAction->GetDelayedNeutronsMaps();

    for (size_t i = 0; i < thresholds.size(); ++i) {
        const G4double t      = thresholds[i];
        const G4String dStr   = FormatDelay(t);
        const G4String fname  = baseName + "_" + dStr + ".txt";
        const std::string path = (fDataDirectory + "/" + fname).c_str();

        std::ofstream out(path.c_str(), std::ios::app);
        if (!out.is_open()) {
            G4cerr << "Error: cannot open " << path << " for writing!" << G4endl;
            continue;
        }

        out << "Event " << eventID
            << "\nNeutronID\tEnergy(eV) at " << dStr << "\n";

        for (auto const& entry : maps[i]) {
            out << entry.first << "\t"
                << std::fixed << std::setprecision(6)
                << entry.second / eV << "\n";
        }
        out << "----------------------------------------\n";
    }
}

void MyEventAction::SaveSummaryData() {
    // Здесь можно объединить и сохранить сводные данные
}
