#ifndef CSVWriter_h
#define CSVWriter_h 1

#include "G4String.hh"
#include "globals.hh"
#include <fstream>
#include <vector>
#include <string>
#include <mutex>

class CSVWriter {
public:
    CSVWriter(const G4String& filename, bool append = true);
    ~CSVWriter();
    
    // Записывает заголовок таблицы
    void WriteHeader(const std::vector<G4double>& delayTimes);
    
    // Записывает одну строку данных
    void WriteRow(int eventNumber, 
                  const G4String& particleName, 
                  G4double energyTeV,
                  const std::vector<int>& neutronCounts);
    
    bool IsOpen() const { return fFile.is_open(); }
    
    // Находит последний номер события в файле (для режима append)
    static int GetLastRunNumber(const G4String& filename);
    
    // Возвращает следующий свободный номер события (thread-safe)
    int GetNextEventNumber();

private:
    std::ofstream fFile;
    G4String fFilename;
    bool fFileExisted;           // был ли файл до открытия
    
    mutable std::mutex fMutex;   // для безопасной работы из разных потоков
    int fCurrentEventNumber;     // счётчик событий
};

#endif
