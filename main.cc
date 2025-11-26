#include "G4MTRunManager.hh"  // многопоточный менеджер вместо обычного G4RunManager
#include "G4UImanager.hh"

#include "G4PhysListFactory.hh"
#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"  // новый класс для MT режима
#include "G4NeutronTrackingCut.hh"
#include "G4SystemOfUnits.hh"

#include "CSVWriter.hh"

#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"

#include <vector>
#include <chrono>
#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
    // Создаём многопоточный run manager
    G4MTRunManager* runManager = new G4MTRunManager;
    
    // По умолчанию 4 потока (можно изменить через аргументы)
    int nThreads = 4;
    runManager->SetNumberOfThreads(nThreads);
    G4cout << "Running with " << nThreads << " threads" << G4endl;

    // Геометрия детектора (одна на все потоки)
    runManager->SetUserInitialization(new DetectorConstruction());

    // Физический лист (один на все потоки)
    G4PhysListFactory factory;
    G4VModularPhysicsList* physicsList = factory.GetReferencePhysList("FTFP_BERT_HP");
    runManager->SetUserInitialization(physicsList);

    // Ограничиваем время жизни нейтронов (чтобы симуляция не зависала)
    auto neutronCut = new G4NeutronTrackingCut();
    neutronCut->SetTimeLimit(1.*s);
    neutronCut->SetKineticEnergyLimit(0.*eV);
    physicsList->RegisterPhysics(neutronCut);

    // Массив временных задержек для подсчёта нейтронов
    std::vector<G4double> delays = { 
        100*ns, 250*ns, 750*ns,
        1*us, 2*us, 4*us, 8*us, 10*us,
        20*us, 35*us, 50*us, 100*us,
        125*us, 150*us, 200*us
    };
    
    // Разбираем аргументы командной строки
    bool appendMode = true;       // добавлять в существующий CSV или создать новый
    int nEventsPerRun = 100;      // сколько событий на каждую комбинацию частица+энергия
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--reset") {
            appendMode = false;
            G4cout << "Reset mode: Starting fresh CSV file" << G4endl;
        } else {
            int val = std::atoi(argv[i]);
            if (val > 0 && val < 1000) {
                // Если число маленькое — это количество потоков
                nThreads = val;
                runManager->SetNumberOfThreads(nThreads);
            } else if (val >= 1000) {
                // Если большое — это количество событий
                nEventsPerRun = val;
            }
        }
    }
    
    // Создаём один CSV writer для всех потоков
    G4String csvFilename = "../data/simulation_results.csv";
    CSVWriter* csvWriter = new CSVWriter(csvFilename, appendMode);
    csvWriter->WriteHeader(delays);

    // Список частиц и энергий для симуляции
    std::vector<G4String> particles = {"proton", "e-", "neutron", "gamma"};
    std::vector<G4double> energies = {0.1*TeV, 0.5*TeV, 1.0*TeV, 5.0*TeV, 10.0*TeV};
    
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    
    // Флаг, чтобы вызвать Initialize() только один раз
    bool isInitialized = false;
    
    // Засекаем время начала
    auto tStart = std::chrono::steady_clock::now();
    
    // Перебираем все комбинации частица × энергия
    for (const auto& particleName : particles) {
        G4ParticleDefinition* particle = particleTable->FindParticle(particleName);
        if (!particle) {
            G4cerr << "Particle " << particleName << " not found!" << G4endl;
            continue;
        }
        
        for (const auto& energy : energies) {
            G4cout << "\n========================================" << G4endl;
            G4cout << "Starting simulation:" << G4endl;
            G4cout << "  Particle: " << particleName << G4endl;
            G4cout << "  Energy: " << energy/TeV << " TeV" << G4endl;
            G4cout << "  Events: " << nEventsPerRun << G4endl;
            G4cout << "  Threads: " << nThreads << G4endl;
            G4cout << "========================================\n" << G4endl;
            
            // Создаём ActionInitialization с текущими параметрами
            // Он распределит Actions по всем worker-потокам
            ActionInitialization* actionInit = 
                new ActionInitialization(delays, csvWriter, particleName, energy);
            runManager->SetUserInitialization(actionInit);
            
            // Инициализируем только первый раз
            if (!isInitialized) {
                runManager->Initialize();
                isInitialized = true;
            }
            
            // Запускаем симуляцию (потоки работают параллельно)
            runManager->BeamOn(nEventsPerRun);
            
            // Показываем прошедшее время
            auto tNow = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(tNow - tStart).count();
            G4cout << "Total elapsed time: " << elapsed << " s\n" << G4endl;
        }
    }
    
    // Чистим память
    delete csvWriter;
    delete runManager;

    G4cout << "\nSimulation completed! Check ../data/simulation_results.csv" << G4endl;

    return 0;
}
