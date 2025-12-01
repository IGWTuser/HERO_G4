/**
 * @file CSVWriter.hh
 * @brief Запись результатов симуляции в CSV файл
 *
 * CSV (Comma-Separated Values) - стандартный текстовый формат для данных.
 * Совместим со всеми электронными таблицами (Excel, LibreOffice) и Python (pandas).
 *
 * Класс поддерживает многопоточное написание:
 * - Использует std::mutex для синхронизации доступа из разных потоков
 * - Поддерживает режим добавления (append) к существующему файлу
 * - Автоматически нумерует события глобально (thread-safe)
 *
 * Формат файла:
 * ```
 * EventID,ParticleType,EnergyTeV,Delay_0.10us,Delay_0.20us,Delay_0.50us
 * 1,proton,10.0,1520,1634,1702
 * 2,proton,10.0,1540,1658,1725
 * ```
 *
 * @date 2024
 * @version 1.0
 *
 * @note Для многопоточного режима (G4MTRunManager) используется mutex для 
 *       безопасной записи из разных потоков
 * @see MyEventAction, simulation_results.csv
 */

#ifndef CSVWriter_h
#define CSVWriter_h 1

#include "G4String.hh"
#include "globals.hh"
#include <fstream>
#include <vector>
#include <string>
#include <mutex>

/**
 * @class CSVWriter
 * @brief Запись данных событий в CSV файл
 *
 * Предоставляет функции для записи результатов симуляции в CSV формат.
 * 
 * Особенности:
 * - **Многопоточность**: использует std::mutex для синхронизации
 * - **Режим добавления**: может продолжить запись в существующий файл
 * - **Автонумерация**: глобальный счётчик событий (thread-safe)
 * - **Гибкость**: поддерживает произвольное количество временных задержек
 *
 * Использование:
 * ```cpp
 * // Создание writer с автоматическим поиском последнего события
 * CSVWriter writer("results.csv", true);
 * 
 * // Запись заголовка (один раз в начале)
 * std::vector<G4double> delays = {0.10, 0.20, 0.50};  // микросекунды
 * writer.WriteHeader(delays);
 * 
 * // Запись данных события (из разных потоков)
 * std::vector<int> counts = {1520, 1634, 1702};
 * writer.WriteRow(event_id, "proton", 10.0, counts);
 * ```
 *
 * @note ВНИМАНИЕ: Все методы (кроме конструктора/деструктора) безопасны для 
 *       многопоточного использования благодаря std::mutex
 *
 * @see MyEventAction для примера вызова WriteRow()
 * @see GetNextEventNumber() для получения автоматического номера события
 */
class CSVWriter {
public:
    /**
     * @brief Конструктор с открытием файла
     *
     * Открывает CSV файл для записи. Если файл существует и append=true,
     * определяет последний номер события для продолжения нумерации.
     *
     * @param filename Путь к файлу для записи (например, "results.csv")
     *
     * @param append Если true (по умолчанию):
     *               - Если файл существует, продолжит запись (append mode)
     *               - Нумерация событий начнится с последнего номера + 1
     *               Если false:
     *               - Перезаписывает файл (truncate mode)
     *               - Нумерация начнится с 1
     *
     * @example
     * ```cpp
     * // Создание нового файла (перезапишет старый)
     * CSVWriter writer1("new_results.csv", false);
     * 
     * // Добавление к существующему файлу (продолжит нумерацию)
     * CSVWriter writer2("results.csv", true);
     * ```
     *
     * @note Если файл не может быть открыт, IsOpen() вернёт false
     * @see IsOpen(), GetLastRunNumber()
     */
    CSVWriter(const G4String& filename, bool append = true);
    
    /**
     * @brief Деструктор
     *
     * Автоматически закрывает файл при удалении объекта.
     * Гарантирует, что все данные записаны на диск.
     */
    ~CSVWriter();
    
    /**
     * @brief Записать заголовок таблицы CSV
     *
     * Записывает первую строку с названиями колонок.
     * Должна быть вызвана один раз в начале (обычно в BeginOfRunAction).
     *
     * Формируемый заголовок зависит от количества задержек:
     * - 3 задержки: EventID,ParticleType,EnergyTeV,Delay_0.10us,Delay_0.20us,Delay_0.50us
     * - N задержек: ... Delay_[delayTimes[0]],Delay_[delayTimes[1]],... Delay_[delayTimes[N-1]]
     *
     * @param delayTimes Вектор временных задержек (в микросекундах или других единицах)
     *                   Используется для генерации названий колонок
     *                   Пример: {0.10, 0.20, 0.50}
     *
     * @example
     * ```cpp
     * std::vector<G4double> delays = {0.10, 0.20, 0.50};
     * csvWriter.WriteHeader(delays);
     * // Результат: EventID,ParticleType,EnergyTeV,Delay_0.10us,Delay_0.20us,Delay_0.50us
     * ```
     *
     * @note Эта функция thread-safe благодаря mutex
     * @see WriteRow()
     */
    void WriteHeader(const std::vector<G4double>& delayTimes);
    
    /**
     * @brief Записать одну строку данных события
     *
     * Записывает результаты одного события в CSV формат.
     * Все значения разделены запятыми и находятся на одной строке.
     *
     * Записываемая строка:
     * ```
     * eventNumber,particleName,energyTeV,neutronCounts[0],neutronCounts[1],...
     * ```
     *
     * Пример:
     * ```
     * 42,proton,10.0,1520,1634,1702
     * ```
     *
     * @param eventNumber Порядковый номер события (обычно от GetNextEventNumber())
     *                    Может быть получен через GetNextEventNumber() для автоматической нумерации
     *
     * @param particleName Название типа частицы (например, "proton", "neutron", "pion")
     *                     Обычно "proton" для вашего проекта
     *
     * @param energyTeV Кинетическая энергия первичной частицы в ТеВ (0.001 ТеВ = 1 ГеВ)
     *                  Обычно 10.0 для 10 ГеВ протонов
     *
     * @param neutronCounts Вектор количеств нейтронов (или других частиц) в разных временных окнах
     *                      Размер должен совпадать с количеством задержек в WriteHeader()
     *                      Пример: {1520, 1634, 1702} - 3 временных окна
     *
     * @example
     * ```cpp
     * int eventID = csvWriter.GetNextEventNumber();
     * std::vector<int> counts = {1520, 1634, 1702};
     * csvWriter.WriteRow(eventID, "proton", 10.0, counts);
     * ```
     *
     * @note **THREAD-SAFE**: Может вызываться одновременно из разных потоков
     *       Mutex автоматически синхронизирует доступ к файлу
     *
     * @warning neutronCounts.size() должен равняться количеству задержек!
     * @see GetNextEventNumber(), WriteHeader()
     */
    void WriteRow(int eventNumber, 
                  const G4String& particleName, 
                  G4double energyTeV,
                  const std::vector<int>& neutronCounts);
    
    /**
     * @brief Проверить, успешно ли открыт файл
     *
     * Возвращает состояние файлового потока.
     *
     * @return true - файл открыт и готов к записи
     *         false - файл не открыт (ошибка при открытии)
     *
     * @example
     * ```cpp
     * CSVWriter writer("results.csv");
     * if (!writer.IsOpen()) {
     *     G4cerr << "Ошибка: не удалось открыть CSV файл\n";
     *     return;
     * }
     * ```
     *
     * @see CSVWriter(const G4String&, bool)
     */
    bool IsOpen() const { return fFile.is_open(); }
    
    /**
     * @brief Найти последний номер события в существующем файле
     *
     * Статический метод для определения, с какого номера начинать нумерацию
     * при добавлении данных в существующий файл (режим append).
     *
     * Алгоритм:
     * 1. Открыть файл в режиме чтения
     * 2. Пропустить заголовок (первую строку)
     * 3. Прочитать первое число (EventID) из каждой строки
     * 4. Вернуть максимальное найденное значение
     *
     * @param filename Путь к файлу для проверки
     *
     * @return Последний номер события в файле (или 0, если файл пустой/не существует)
     *
     * @example
     * ```cpp
     * int lastEvent = CSVWriter::GetLastRunNumber("results.csv");
     * G4cout << "Последнее событие: " << lastEvent << "\n";
     * // Вывод: Последнее событие: 1000
     * ```
     *
     * @note Используется конструктором для определения начального номера при append=true
     * @see CSVWriter(const G4String&, bool)
     */
    static int GetLastRunNumber(const G4String& filename);
    
    /**
     * @brief Получить следующий свободный номер события
     *
     * Возвращает уникальный номер для текущего события и увеличивает внутренний счётчик.
     * THREAD-SAFE для многопоточного режима благодаря mutex.
     *
     * Поведение:
     * 1. Первый вызов вернёт fCurrentEventNumber + 1
     * 2. Каждый последующий вызов вернёт предыдущее + 1
     * 3. Номера гарантированно уникальны даже при одновременных вызовах
     *
     * @return Уникальный номер события (начиная с 1 или последнего + 1 при append)
     *
     * @example
     * ```cpp
     * int event1 = writer.GetNextEventNumber();  // вернёт 1
     * int event2 = writer.GetNextEventNumber();  // вернёт 2
     * int event3 = writer.GetNextEventNumber();  // вернёт 3
     * 
     * // При append к файлу с 1000 событиями:
     * int event_next = writer.GetNextEventNumber();  // вернёт 1001
     * ```
     *
     * @note **THREAD-SAFE**: Защищен mutex от race conditions
     *       Может вызываться одновременно из разных потоков
     *
     * @see WriteRow(int eventNumber, ...) для использования в записи
     */
    int GetNextEventNumber();

private:
    /// @brief Файловый поток для записи CSV данных
    ///
    /// Открывается в конструкторе и закрывается в деструкторе.
    /// В режиме append добавляет данные в конец существующего файла.
    std::ofstream fFile;
    
    /// @brief Путь к CSV файлу
    ///
    /// Сохраняется для использования в GetLastRunNumber()
    /// и в целях отладки
    G4String fFilename;
    
    /// @brief Флаг: существовал ли файл до открытия конструктором
    ///
    /// true - если файл существовал (был открыт в режиме append)
    /// false - если файл новый (был создан или перезаписан)
    /// Используется для определения, нужно ли записывать заголовок
    bool fFileExisted;
    
    /// @brief Mutex для синхронизации многопоточного доступа
    ///
    /// Гарантирует, что:
    /// - Только один поток пишет в файл одновременно
    /// - Номера событий не дублируются
    /// - Данные записываются в целостности
    ///
    /// Используется в WriteRow() и GetNextEventNumber()
    mutable std::mutex fMutex;
    
    /// @brief Текущий счётчик событий (для GetNextEventNumber)
    ///
    /// Инициализируется в конструкторе:
    /// - Обнуляется (0) для нового файла
    /// - Устанавливается на GetLastRunNumber() для существующего файла в режиме append
    ///
    /// Каждый вызов GetNextEventNumber() увеличивает это значение на 1
    int fCurrentEventNumber;
};

#endif