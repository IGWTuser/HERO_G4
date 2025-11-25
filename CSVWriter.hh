#ifndef CSVWriter_h
#define CSVWriter_h 1

#include "G4String.hh"
#include "globals.hh"
#include <fstream>
#include <vector>
#include <string>

class CSVWriter {
public:
    CSVWriter(const G4String& filename, bool append = true);
    ~CSVWriter();
    
    // Запись заголовка CSV (только если файл новый или пустой)
    void WriteHeader(const std::vector<G4double>& delayTimes);
    
    // Запись одной строки данных
    void WriteRow(int runNumber, 
                  const G4String& particleName, 
                  G4double energyGeV,
                  const std::vector<int>& neutronCounts);
    
    bool IsOpen() const { return fFile.is_open(); }
    
    // Получить последний номер рана из файла
    static int GetLastRunNumber(const G4String& filename);
    
private:
    std::ofstream fFile;
    G4String fFilename;
    bool fFileExisted;
};

#endif