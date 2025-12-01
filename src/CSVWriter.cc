/**
 * @file CSVWriter.cc
 * @brief Реализация класса для записи результатов симуляции в CSV файл
 *
 * Обеспечивает безопасную и надёжную запись данных событий в CSV файл.
 * Поддерживает как перезапись (режим WRITE), так и добавление (режим APPEND).
 *
 * **Основные функции:**
 * - WriteHeader() - запись заголовка с названиями колонок
 * - WriteRow() - запись одной строки с результатами события
 * - GetNextEventNumber() - получение уникального номера события
 * - GetLastRunNumber() - определение последнего номера для режима APPEND
 * - Синхронизация потоков (G4AutoLock, G4Mutex)
 *
 * **Особенности:**
 * - Потокобезопасность (многопоточный режим GEANT4)
 * - Режим APPEND (продолжение предыдущей симуляции)
 * - Автоматическое определение последнего номера события
 * - Буферизация с flushing для надёжности
 *
 * **Формат CSV:**
 * ```
 * Event,Particle,Energy(TeV),N<1eV@0.10us,N<1eV@0.20us,N<1eV@0.50us
 * 0,proton,10.000,1520,1634,1702
 * 1,proton,10.000,1485,1598,1668
 * ...
 * ```
 *
 * @date 2024
 * @version 1.0
 *
 * @note Потокобезопасен благодаря G4Mutex и G4AutoLock
 * @note Использует flushing для гарантии записи на диск
 * @see CSVWriter.hh для деклерации
 */

#include "CSVWriter.hh"
#include "G4SystemOfUnits.hh"
#include "G4AutoLock.hh"
#include <iomanip>
#include <fstream>
#include <sstream>

/**
 * @brief Глобальный мьютекс для синхронизации многопоточной записи
 *
 * Используется для защиты доступа к файлу из разных потоков.
 * Гарантирует, что одновременно только один поток может писать в файл.
 *
 * **Как это работает:**
 * - G4MUTEX_INITIALIZER - инициализация мьютекса
 * - G4AutoLock lock(&csvMutex) - автоматическое захватывание
 * - При выходе из области видимости мьютекс автоматически отпускается
 *
 * @note Глобальная переменная в анонимном namespace (видна только в этом файле)
 * @note Используется во всех методах, которые пишут в файл
 */
namespace {
    // Глобальный мьютекс для синхронизации записи в файл
    G4Mutex csvMutex = G4MUTEX_INITIALIZER;
}

/**
 * @brief Конструктор с параметрами
 *
 * Инициализирует CSVWriter и открывает файл в режиме WRITE или APPEND.
 *
 * **Алгоритм:**
 * 1. Проверить, существует ли уже файл
 * 2. Если append=true и файл существует:
 *    - Открыть файл в режиме APPEND (добавление в конец)
 *    - Найти последний номер события через GetLastRunNumber()
 * 3. Если append=false или файла нет:
 *    - Открыть файл в режиме WRITE (перезаписывание)
 *    - Установить счётчик на 0
 * 4. Проверить успешность открытия файла
 *
 * @param filename Путь к CSV файлу
 *                 Примеры: "results.csv", "../data/results.csv", "/home/user/sim_output.csv"
 *                 Должна быть указана корректная директория
 *
 * @param append Режим открытия файла (по умолчанию false)
 *               - true: APPEND режим (добавление в конец, если файл существует)
 *               - false: WRITE режим (перезапись, создание нового файла)
 *
 * **Примеры использования:**
 * ```cpp
 * // Режим перезаписи (новый файл)
 * CSVWriter writer1("results.csv", false);
 * writer1.WriteHeader(delays);
 * writer1.WriteRow(0, "proton", 10.0, counts);
 *
 * // Режим добавления (продолжение)
 * CSVWriter writer2("results.csv", true);  // открывает существующий файл
 * writer2.WriteRow(1000, "proton", 10.0, counts);  // продолжает нумерацию
 * ```
 *
 * **Определение последнего номера в режиме APPEND:**
 * ```
 * Если в файле уже есть события 0-999:
 * GetLastRunNumber() вернёт 999
 * fCurrentEventNumber установится на 1000
 * Следующее событие получит номер 1000 (GetNextEventNumber())
 * ```
 *
 * @note После конструктора нужно вызвать WriteHeader()
 * @see WriteHeader() для записи заголовка
 * @see GetLastRunNumber() для определения последнего номера
 */
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

/**
 * @brief Деструктор
 *
 * Закрывает файл при удалении объекта.
 * Вызывается автоматически при завершении программы.
 *
 * @note Не удаляет файл - только закрывает его
 * @note Все данные должны быть на диске благодаря flushing в WriteRow()
 */
CSVWriter::~CSVWriter() {
    if (fFile.is_open()) {
        fFile.close();
        G4cout << "CSV file closed: " << fFilename << G4endl;
    }
}

/**
 * @brief Записать заголовок CSV файла
 *
 * Пишет строку с названиями колонок.
 * Должна быть вызвана один раз перед записью данных (обычно в BeginOfRunAction).
 *
 * **Формат заголовка:**
 * ```
 * Event,Particle,Energy(TeV),N<1eV@0.10us,N<1eV@0.20us,N<1eV@0.50us
 * ```
 *
 * **Алгоритм:**
 * 1. Захватить мьютекс (G4AutoLock) для потокобезопасности
 * 2. Проверить, открыт ли файл
 * 3. Проверить, новый ли файл (fFileExisted == false)
 * 4. Если новый:
 *    - Записать стандартные колонки: Event, Particle, Energy(TeV)
 *    - Для каждой временной задержки:
 *      - Добавить колонку: N<1eV@{delay}us
 *    - Завершить строку символом новой строки
 *    - Вызвать flush() для сбрасывания на диск
 *
 * **Колонки с временными задержками:**
 * ```cpp
 * for (size_t i = 0; i < delayTimes.size(); ++i) {
 *     G4double timeInMicroseconds = delayTimes[i] / microsecond;
 *     fFile << ",N<1eV@" << std::fixed << std::setprecision(2) 
 *           << timeInMicroseconds << "us";
 * }
 * ```
 *
 * **Результат для delays = {0.10µs, 0.20µs, 0.50µs}:**
 * ```
 * Event,Particle,Energy(TeV),N<1eV@0.10us,N<1eV@0.20us,N<1eV@0.50us
 * ```
 *
 * @param delayTimes Вектор временных задержек (в единицах GEANT4)
 *                   Пример: {0.10 * microsecond, 0.20 * microsecond, 0.50 * microsecond}
 *                   Каждая задержка добавляется как отдельная колонка
 *
 * @note Потокобезопасна благодаря G4AutoLock
 * @note Вызывается один раз при запуске симуляции
 * @note В режиме APPEND заголовок НЕ переписывается
 *
 * @example
 * ```cpp
 * CSVWriter writer("results.csv", false);
 * std::vector<G4double> delays = {0.10 * microsecond, 0.20 * microsecond};
 * writer.WriteHeader(delays);
 * // Результат в файле:
 * // Event,Particle,Energy(TeV),N<1eV@0.10us,N<1eV@0.20us
 * ```
 *
 * @see WriteRow() для записи данных событий
 * @see G4AutoLock для потокобезопасности
 */
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

/**
 * @brief Получить и увеличить счётчик номера события
 *
 * Возвращает уникальный номер для текущего события и увеличивает счётчик.
 * Используется для глобальной нумерации всех событий во всех Run-ах.
 *
 * **Поведение:**
 * - Первый вызов: возвращает 0, счётчик становится 1
 * - Второй вызов: возвращает 1, счётчик становится 2
 * - И т.д.
 *
 * **В режиме APPEND:**
 * - Счётчик инициализируется как GetLastRunNumber() + 1
 * - Например, если последнее событие было 999, счётчик = 1000
 * - Первый вызов вернёт 1000
 *
 * @return int Номер текущего события (начиная с 0 или после последнего события в файле)
 *
 * **Примеры:**
 * ```cpp
 * CSVWriter writer("results.csv", false);  // новый файл
 * int n1 = writer.GetNextEventNumber();     // возвращает 0
 * int n2 = writer.GetNextEventNumber();     // возвращает 1
 * int n3 = writer.GetNextEventNumber();     // возвращает 2
 *
 * // В режиме APPEND существующего файла с событиями 0-999:
 * CSVWriter writer2("results.csv", true);  // fCurrentEventNumber = 1000
 * int n1 = writer2.GetNextEventNumber();   // возвращает 1000
 * int n2 = writer2.GetNextEventNumber();   // возвращает 1001
 * ```
 *
 * @note Потокобезопасна благодаря G4AutoLock
 * @note Должна вызваться один раз для каждого события
 * @note Используется в MyEventAction::EndOfEventAction()
 *
 * @see WriteRow() - использует номер от GetNextEventNumber()
 * @see GetLastRunNumber() - для инициализации в конструкторе
 */
int CSVWriter::GetNextEventNumber() {
    // Атомарно увеличиваем счётчик и возвращаем номер
    G4AutoLock lock(&csvMutex);
    return fCurrentEventNumber++;
}

/**
 * @brief Записать одну строку данных события в CSV файл
 *
 * Пишет результаты одного события в формате: Event, Particle, Energy, Counts...
 * Вызывается один раз в конце каждого события (из MyEventAction::EndOfEventAction).
 *
 * **Формат строки:**
 * ```
 * {eventNumber},{particleName},{energyTeV},{count1},{count2},...
 * ```
 *
 * **Пример:**
 * ```
 * 0,proton,10.000,1520,1634,1702
 * 1,proton,10.000,1485,1598,1668
 * ```
 *
 * **Алгоритм:**
 * 1. Захватить мьютекс для потокобезопасности
 * 2. Проверить, открыт ли файл
 * 3. Записать: eventNumber, particleName, energyTeV (с точностью 3 знака)
 * 4. Для каждого элемента в neutronCounts:
 *    - Записать: comma + count
 * 5. Завершить строку символом новой строки
 * 6. Вызвать flush() для сбрасывания на диск
 *
 * **Параметры:**
 * @param eventNumber Номер события (обычно от GetNextEventNumber())
 *                    Значения: 0, 1, 2, ... (или продолжение в режиме APPEND)
 *
 * @param particleName Название первичной частицы (строка)
 *                     Примеры: "proton", "neutron", "pion0"
 *
 * @param energyTeV Энергия частицы в ТэВ (G4double)
 *                  Должна быть уже преобразована из ГэВ (энергия / TeV)
 *                  Пример: 10.0 * GeV / TeV = 10.0 (ТэВ)
 *
 * @param neutronCounts Вектор подсчётов нейтронов для каждой задержки
 *                      Пример: {1520, 1634, 1702}
 *                      - counts[0] = нейтроны до 1-й задержки
 *                      - counts[1] = нейтроны до 2-й задержки
 *                      - и т.д.
 *
 * **Примеры использования:**
 * ```cpp
 * // В MyEventAction::EndOfEventAction()
 * int eventID = fCSVWriter->GetNextEventNumber();
 * G4double energyTeV = fCurrentEnergy / TeV;
 * std::vector<int> counts = CountLowEnergyNeutrons(1.0 * eV);
 *
 * fCSVWriter->WriteRow(eventID, "proton", energyTeV, counts);
 * // Результат в CSV: 0,proton,10.000,1520,1634,1702
 * ```
 *
 * @note Потокобезопасна благодаря G4AutoLock
 * @note Вызывается один раз в конце каждого события
 * @note flush() гарантирует запись на диск (важно для надёжности)
 * @note В многопоточном режиме строки не смешиваются благодаря мьютексу
 *
 * @see GetNextEventNumber() для получения номера события
 * @see WriteHeader() для формата колонок
 * @see MyEventAction::EndOfEventAction() для примера использования
 */
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

/**
 * @brief Получить последний номер события из существующего файла
 *
 * Прочитывает CSV файл и находит максимальный номер события.
 * Используется в конструкторе для инициализации счётчика в режиме APPEND.
 *
 * **Алгоритм:**
 * 1. Открыть файл на чтение
 * 2. Пропустить заголовок (первая строка)
 * 3. Для каждой строки:
 *    - Извлечь первое поле (номер события)
 *    - Преобразовать в целое число
 *    - Отследить максимальное значение
 * 4. Вернуть максимальный номер (или -1, если файл пуст или не открывается)
 *
 * **Формат файла:**
 * ```
 * Event,Particle,Energy(TeV),N<1eV@0.10us
 * 0,proton,10.000,1520         ← строка 1
 * 1,proton,10.000,1485         ← строка 2
 * 999,proton,10.000,1502       ← строка 1000 (последний)
 * ```
 *
 * **Возвращаемое значение:**
 * - 999 (максимальный номер события)
 * - После этого fCurrentEventNumber = 1000 (в конструкторе)
 * - Следующее событие получит номер 1000
 *
 * **Обработка ошибок:**
 * - Если файл не существует: вернуть -1
 * - Если файл пуст: вернуть -1
 * - Если строка битая: пропустить (catch ...)
 *
 * @param filename Путь к CSV файлу
 *                 Должен быть в формате CSV с первым полем - номер события
 *
 * @return int Максимальный номер события из файла (-1, если ошибка)
 *
 * **Примеры:**
 * ```cpp
 * // Файл с событиями 0-999
 * int maxNum = GetLastRunNumber("results.csv");  // возвращает 999
 *
 * // Файл не существует
 * int maxNum = GetLastRunNumber("nonexistent.csv");  // возвращает -1
 *
 * // Файл пуст (только заголовок)
 * int maxNum = GetLastRunNumber("empty.csv");  // возвращает -1
 * ```
 *
 * @note Используется только в конструкторе при append=true
 * @note Медленнее для больших файлов (читает весь файл)
 * @note Обработка ошибок в try-catch блоке
 *
 * @see CSVWriter() конструктор использует эту функцию
 */
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