#include "CSVWriter.hh"
#include "G4SystemOfUnits.hh"
#include "G4AutoLock.hh"
#include <iomanip>
#include <fstream>
#include <sstream>

namespace {
    // Глобальный мьютекс для синхронизации записи в файл
    G4Mutex csvMutex = G4MUTEX_INITIALIZER;
}

CSVWriter::CSVWriter(const G4String& filename, bool append)
    : fFilename(filename), fFileExisted(false), fCurrentEventNumber(0)
{
    // Проверяем, существует ли файл
    std::ifstream checkFile(filename.c_str());
    fFileExisted = checkFile.good();
    checkFile.close();
    
    if (append && fFileExisted) {
        // Режим добавления: открываем в конец файла
        fFile.open(filename.c_str(), std::ios::app);
        G4cout << "CSV file opened in APPEND mode: " << filename << G4endl;
        
        // Находим последний номер события, чтобы продолжить нумерацию
        fCurrentEventNumber = GetLastRunNumber(filename) + 1;
    } else {
        // Режим перезаписи: создаём новый файл
        fFile.open(filename.c_str(), std::ios::out);
        G4cout << "CSV file opened in WRITE mode: " << filename << G4endl;
        fCurrentEventNumber = 0;
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
    // Блокируем доступ других потоков на время записи
    G4AutoLock lock(&csvMutex);
    
    if (!fFile.is_open()) return;
    
    // Пишем заголовок только если файл новый
    if (!fFileExisted) {
        fFile << "Event,Particle,Energy(TeV)";
        
        // Добавляем колонку для каждой временной задержки
        for (size_t i = 0; i < delayTimes.size(); ++i) {
            G4double timeInMicroseconds = delayTimes[i] / microsecond;
            fFile << ",N<1eV@" << std::fixed << std::setprecision(2) 
                  << timeInMicroseconds << "us";
        }
        
        fFile << "\n";
        fFile.flush();  // сразу сбрасываем на диск
        G4cout << "CSV header written." << G4endl;
    }
}

int CSVWriter::GetNextEventNumber() {
    // Атомарно увеличиваем счётчик и возвращаем номер
    G4AutoLock lock(&csvMutex);
    return fCurrentEventNumber++;
}

void CSVWriter::WriteRow(int eventNumber, 
                         const G4String& particleName, 
                         G4double energyTeV,
                         const std::vector<int>& neutronCounts) {
    // Блокируем, чтобы строки не перемешались между потоками
    G4AutoLock lock(&csvMutex);
    
    if (!fFile.is_open()) return;
    
    // Номер события, название частицы, энергия
    fFile << eventNumber << "," 
          << particleName << "," 
          << std::fixed << std::setprecision(3) << energyTeV;
    
    // Количество нейтронов для каждой задержки
    for (const auto& count : neutronCounts) {
        fFile << "," << count;
    }
    
    fFile << "\n";
    fFile.flush();  // сразу на диск (для надёжности)
}

int CSVWriter::GetLastRunNumber(const G4String& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        return -1;  // файла нет
    }
    
    int lastEventNumber = -1;
    std::string line;
    
    // Пропускаем заголовок
    if (std::getline(file, line)) {
        // Читаем все строки и ищем максимальный номер события
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            // Берём первое поле (номер события)
            size_t commaPos = line.find(',');
            if (commaPos != std::string::npos) {
                std::string eventNumberStr = line.substr(0, commaPos);
                try {
                    int eventNum = std::stoi(eventNumberStr);
                    if (eventNum > lastEventNumber) {
                        lastEventNumber = eventNum;
                    }
                } catch (...) {
                    // игнорируем битые строки
                }
            }
        }
    }
    
    file.close();
    return lastEventNumber;
}
