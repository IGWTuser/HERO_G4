#include "CSVWriter.hh"
#include "G4SystemOfUnits.hh"
#include <iomanip>
#include <fstream>
#include <sstream>

CSVWriter::CSVWriter(const G4String& filename, bool append)
    : fFilename(filename), fFileExisted(false)
{
    // Проверяем, существует ли файл
    std::ifstream checkFile(filename.c_str());
    fFileExisted = checkFile.good();
    checkFile.close();
    
    // Открываем файл в режиме добавления или перезаписи
    if (append && fFileExisted) {
        fFile.open(filename.c_str(), std::ios::app);
        G4cout << "CSV file opened in APPEND mode: " << filename << G4endl;
    } else {
        fFile.open(filename.c_str(), std::ios::out);
        G4cout << "CSV file opened in WRITE mode: " << filename << G4endl;
    }
    
    if (!fFile.is_open()) {
        G4cerr << "Error: Cannot open CSV file " << filename << G4endl;
    }
}

CSVWriter::~CSVWriter() {
    if (fFile.is_open()) {
        fFile.close();
        G4cout << "CSV file closed: " << fFilename << G4endl;
    }
}

void CSVWriter::WriteHeader(const std::vector<G4double>& delayTimes) {
    if (!fFile.is_open()) return;
    
    // Записываем заголовок только если файл новый или был пустой
    if (!fFileExisted) {
        fFile << "Event,Particle,Energy(TeV)";
        
        // Добавляем колонки для каждого времени задержки
        for (size_t i = 0; i < delayTimes.size(); ++i) {
            G4double timeInMicroseconds = delayTimes[i] / microsecond;
            fFile << ",N<1eV@" << std::fixed << std::setprecision(2) 
                  << timeInMicroseconds << "us";
        }
        
        fFile << "\n";
        fFile.flush();
        G4cout << "CSV header written." << G4endl;
    } else {
        G4cout << "CSV file already exists, header not written." << G4endl;
    }
}

void CSVWriter::WriteRow(int eventNumber, 
                         const G4String& particleName, 
                         G4double energyTeV,
                         const std::vector<int>& neutronCounts) {
    if (!fFile.is_open()) return;
    
    // Записываем номер события, название частицы и энергию в TeV
    fFile << eventNumber << "," 
          << particleName << "," 
          << std::fixed << std::setprecision(3) << energyTeV;
    
    // Записываем количество нейтронов для каждой задержки
    for (const auto& count : neutronCounts) {
        fFile << "," << count;
    }
    
    fFile << "\n";
    fFile.flush();
}

int CSVWriter::GetLastRunNumber(const G4String& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        return -1; // Файл не существует
    }
    
    int lastEventNumber = -1;
    std::string line;
    
    // Пропускаем заголовок
    if (std::getline(file, line)) {
        // Читаем все строки и берём первое поле (номер события)
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            // Находим первую запятую
            size_t commaPos = line.find(',');
            if (commaPos != std::string::npos) {
                std::string eventNumberStr = line.substr(0, commaPos);
                try {
                    int eventNum = std::stoi(eventNumberStr);
                    if (eventNum > lastEventNumber) {
                        lastEventNumber = eventNum;
                    }
                } catch (...) {
                    // Игнорируем некорректные строки
                }
            }
        }
    }
    
    file.close();
    return lastEventNumber;
}
