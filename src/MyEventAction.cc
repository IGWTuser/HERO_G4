/**
 * @file MyEventAction.cc
 * @brief Реализация обработчика событий симуляции
 *
 * Выполняется в начале и конце каждого события.
 * Главная обязанность: анализ события и запись результатов в CSV файл.
 *
 * **Основные функции:**
 * - BeginOfEventAction() - инициализация перед событием (очистка данных)
 * - EndOfEventAction() - финализация после события (подсчёт и запись)
 * - CountLowEnergyNeutrons() - подсчёт нейтронов по энергии
 * - Вспомогательные функции форматирования (FormatEnergy, FormatDelay, Sanitize)
 *
 * **Жизненный цикл события:**
 * ```
 * BeginOfEventAction()
 *     ↓ Очистить карты нейтронов
 * PrimaryGeneratorAction::GeneratePrimaries()
 *     ↓ Создать первичную частицу
 * MySteppingAction::UserSteppingAction()
 *     ↓ Для каждого шага регистрировать вторичные нейтроны
 * EndOfEventAction()
 *     ↓ Подсчитать нейтроны и записать в CSV
 * ```
 *
 * @date 2024
 * @version 1.0
 *
 * @note Работает в тесной связи с MySteppingAction для доступа к данным
 * @see G4UserEventAction, MySteppingAction, CSVWriter
 */

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

/**
 * @brief Конструктор с параметрами
 *
 * Инициализирует MyEventAction и связывает его со SteppingAction.
 *
 * @param steppingAction Указатель на MySteppingAction для доступа к данным шагов
 *                       Содержит карты нейтронов для разных задержек
 *                       @warning НЕ ДОЛЖЕН быть nullptr
 *
 * @param dataDir Путь к папке для сохранения выходных файлов (по умолчанию "../data")
 *                Если папка не существует, её нужно создать в main
 *                Пример: "../data" или "/home/user/results"
 *
 * @example
 * ```cpp
 * MySteppingAction* stepping = new MySteppingAction(delays);
 * MyEventAction* eventAction = new MyEventAction(stepping, "../data");
 * eventAction->SetCSVWriter(&csvWriter);
 * ```
 *
 * @see SetCSVWriter(), SetCurrentParticle(), SetCurrentEnergy()
 */
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

/**
 * @brief Деструктор
 *
 * Освобождает ресурсы при удалении объекта.
 * Вызывает SaveSummaryData() для сохранения итоговой статистики.
 *
 * @note Вызывается при завершении симуляции (обычно автоматически)
 */
MyEventAction::~MyEventAction() {
    SaveSummaryData();
    G4cout << "Summary data saved." << G4endl;
}

/**
 * @brief Инициализация события
 *
 * Вызывается в начале каждого события, ДО генерации первичной частицы.
 * Используется для обнуления счетчиков и очистки контейнеров.
 *
 * **Основная операция:**
 * - Вызвать fSteppingAction->Reset() для очистки карт нейтронов
 *
 * Это КРИТИЧНО! Без Reset() данные из разных событий смешаются.
 *
 * @param event Указатель на G4Event (может быть nullptr, но обычно не используется)
 *
 * @note Вызывается один раз в начале каждого события
 * @note **ВАЖНО**: очищаем карты нейтронов перед новым событием
 *
 * @see MySteppingAction::Reset() для очистки данных
 * @see EndOfEventAction() для финализации
 */
void MyEventAction::BeginOfEventAction(const G4Event*) {
    // Очищаем карты нейтронов перед новым событием
    fSteppingAction->Reset();
}

/**
 * @brief Форматировать энергию в строку
 *
 * Преобразует численное значение энергии в строку с единицами.
 * Используется для построения имён файлов и логирования.
 *
 * @param e Энергия (в единицах GEANT4, обычно ГэВ)
 *          Пример: 10.0 * GeV
 *
 * @return G4String Форматированная строка
 *                  Пример: "10.000TeV"
 *
 * **Алгоритм:**
 * 1. Преобразовать энергию в ТеВ (e / TeV)
 * 2. Форматировать с 3 знаками после запятой (fixed, precision(3))
 * 3. Добавить единицу "TeV"
 *
 * @example
 * ```cpp
 * G4String str = FormatEnergy(10.0 * GeV);  // "10.000TeV"
 * G4String str = FormatEnergy(5.5 * GeV);   // "5.500TeV"
 * ```
 *
 * @see FormatDelay() для форматирования времени
 * @see BuildBaseNameFromPrimary() где используется
 */
G4String MyEventAction::FormatEnergy(G4double e) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(3) << (e / TeV);
    return os.str() + "TeV";
}

/**
 * @brief Форматировать время/задержку в строку
 *
 * Преобразует численное значение времени в строку с единицами.
 * Используется для построения имён файлов и логирования.
 *
 * @param t Время (в единицах GEANT4, обычно микросекунды)
 *          Пример: 0.10 * microsecond
 *
 * @return G4String Форматированная строка
 *                  Пример: "0.100us"
 *
 * **Алгоритм:**
 * 1. Преобразовать время в микросекунды (t / microsecond)
 * 2. Форматировать с 3 знаками после запятой
 * 3. Добавить единицу "us"
 *
 * @example
 * ```cpp
 * G4String str = FormatDelay(0.10 * microsecond);  // "0.100us"
 * G4String str = FormatDelay(0.25 * microsecond);  // "0.250us"
 * ```
 *
 * @see FormatEnergy() для форматирования энергии
 */
G4String MyEventAction::FormatDelay(G4double t) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(3) << (t / microsecond);
    return os.str() + "us";
}

/**
 * @brief Очистить строку от недопустимых символов для имени файла
 *
 * Удаляет или заменяет символы, которые недопустимы в имени файла.
 * Например: пробелы → подчеркивание, "/" → "_", и т.д.
 *
 * @param s Исходная строка
 *          Пример: "proton 10 GeV"
 *
 * @return G4String Очищенная строка, безопасная для использования в имени файла
 *                  Пример: "proton_10_GeV"
 *
 * **Алгоритм:**
 * 1. Создать копию исходной строки
 * 2. Заменить '/' на '_'
 * 3. Заменить ' ' (пробел) на '_'
 * 4. Вернуть результат
 *
 * @example
 * ```cpp
 * G4String safe = Sanitize("proton 10 GeV");  // "proton_10_GeV"
 * G4String safe = Sanitize("pion/anti");      // "pion_anti"
 * ```
 *
 * @see BuildBaseNameFromPrimary() где используется
 */
G4String MyEventAction::Sanitize(const G4String& s) {
    G4String result = s;
    std::replace(result.begin(), result.end(), '/', '_');
    std::replace(result.begin(), result.end(), ' ', '_');
    return result;
}

/**
 * @brief Построить базовое имя файла из параметров первичной частицы
 *
 * Создает строку для имени файла на основе типа частицы и энергии.
 * Используется для сохранения детальных данных события (если требуется).
 *
 * **Формат:**
 * ```
 * {particle_name}_{energy_TeV}
 * ```
 *
 * **Примеры:**
 * ```
 * "proton_10.000TeV"
 * "neutron_5.500TeV"
 * "pion0_3.000TeV"
 * ```
 *
 * **Алгоритм:**
 * 1. Получить первичную вершину события
 * 2. Получить первичную частицу
 * 3. Получить определение частицы
 * 4. Получить имя и энергию
 * 5. Форматировать и очистить
 * 6. Вернуть результат
 *
 * @param event Указатель на G4Event
 *
 * @return G4String Базовое имя вида "proton_10TeV"
 *                  Или "unknown" при ошибке
 *
 * @example
 * ```cpp
 * G4String baseName = BuildBaseNameFromPrimary(event);
 * // Возвращает: "proton_10.000TeV"
 * ```
 *
 * @see FormatEnergy(), Sanitize()
 */
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

/**
 * @brief Подсчитать вторичные нейтроны с энергией ниже порога
 *
 * Подсчитывает количество нейтронов (вторичных частиц)
 * с энергией ниже заданного порога для каждой временной задержки.
 *
 * **Алгоритм:**
 * 1. Получить карты нейтронов из MySteppingAction для всех задержек
 * 2. Для каждой задержки:
 *    - Итерировать по всем нейтронам
 *    - Если энергия < порог: увеличить счётчик
 * 3. Вернуть вектор подсчётов
 *
 * **Возвращаемое значение:**
 * ```
 * counts[0] - нейтроны до 1-й задержки с энергией < порога
 * counts[1] - нейтроны до 2-й задержки с энергией < порога
 * counts[n] - нейтроны до n-й задержки с энергией < порога
 * ```
 *
 * @param energyThreshold Пороговая энергия для отсчета нейтронов (по умолчанию 1 эВ)
 *                        Нейтроны с энергией < этого значения учитываются
 *                        Пример: 1.0 * eV, 10.0 * keV, 1.0 * MeV
 *
 * @return std::vector<int> Вектор подсчётов нейтронов для каждой задержки
 *                          Размер = количество задержек в MySteppingAction
 *
 * **Примеры:**
 * ```cpp
 * // Подсчитать нейтроны ниже 1 эВ для каждой задержки
 * std::vector<int> counts = CountLowEnergyNeutrons(1.0 * eV);
 * // counts[0] = 1520 нейтронов до 0.10 µs
 * // counts[1] = 1634 нейтронов до 0.20 µs
 * // counts[2] = 1702 нейтронов до 0.50 µs
 *
 * // Запись в CSV
 * fCSVWriter->WriteRow(eventID, "proton", 10.0, counts);
 * ```
 *
 * @note **THREAD-SAFE**: может вызваться из разных потоков
 * @note Результат зависит от данных в MySteppingAction
 * @note Использует GetDelayedNeutronsMaps() для доступа к картам
 *
 * @see MySteppingAction::GetDelayedNeutronsMaps()
 * @see CSVWriter::WriteRow() для использования результата
 * @see EndOfEventAction() где используется
 */
std::vector<int> MyEventAction::CountLowEnergyNeutrons(G4double energyThreshold) const {
    std::vector<int> counts;
    
    if (!fSteppingAction) {
        return counts;
    }
    
    // Получаем карты нейтронов для всех задержек
    auto const& maps = fSteppingAction->GetDelayedNeutronsMaps();
    counts.resize(maps.size(), 0);
    
    // Для каждой задержки подсчитываем нейтроны с энергией < порога
    for (size_t i = 0; i < maps.size(); ++i) {
        for (auto const& entry : maps[i]) {
            if (entry.second < energyThreshold) {
                counts[i]++;
            }
        }
    }
    
    return counts;
}

/**
 * @brief Финализация события
 *
 * Вызывается в конце события, после завершения отслеживания всех треков.
 * Используется для анализа собранных данных и сохранения результатов.
 *
 * **Алгоритм:**
 * 1. Проверить, открыт ли CSV файл
 * 2. Подсчитать нейтроны с энергией < 1 эВ для каждой задержки
 * 3. Получить уникальный номер события через GetNextEventNumber()
 * 4. Записать строку в CSV: Event, Particle, Energy, Counts...
 *
 * **Процесс записи:**
 * ```cpp
 * if (fCSVWriter && fCSVWriter->IsOpen()) {
 *     // 1. Подсчитать нейтроны ниже 1 эВ
 *     std::vector<int> neutronCounts = CountLowEnergyNeutrons(1.0 * eV);
 *
 *     // 2. Получить номер события (thread-safe)
 *     int eventNum = fCSVWriter->GetNextEventNumber();
 *
 *     // 3. Записать в CSV
 *     fCSVWriter->WriteRow(eventNum, fCurrentParticleName, fCurrentEnergy / TeV, neutronCounts);
 * }
 * ```
 *
 * **Результат в CSV:**
 * ```
 * 0,proton,10.000,1520,1634,1702
 * 1,proton,10.000,1485,1598,1668
 * ...
 * ```
 *
 * @param event Указатель на G4Event (содержит информацию о событии)
 *
 * @note Вызывается один раз в конце каждого события
 * @note **КРИТИЧНО**: здесь должна быть запись в CSV!
 * @note Может быть вызвано из разных потоков (многопоточный режим)
 * @note GetNextEventNumber() обеспечивает уникальность номеров
 *
 * @see BeginOfEventAction() - начало события
 * @see CountLowEnergyNeutrons() для подсчёта нейтронов
 * @see CSVWriter::WriteRow() для записи в CSV
 * @see CSVWriter::GetNextEventNumber() для получения номера события
 */
void MyEventAction::EndOfEventAction(const G4Event* event) {
    // Текстовые файлы убрали — они замедляют многопоточность
    // и создают кучу мелких файлов
    
    // Записываем только в CSV
    if (fCSVWriter && fCSVWriter->IsOpen()) {
        // Считаем нейтроны с энергией < 1 эВ для всех задержек
        std::vector<int> neutronCounts = CountLowEnergyNeutrons(1.0 * eV);
        
        // Получаем уникальный номер события (thread-safe)
        int eventNum = fCSVWriter->GetNextEventNumber();
        
        // Записываем строку в CSV
        fCSVWriter->WriteRow(
            eventNum,
            fCurrentParticleName,
            fCurrentEnergy / TeV,
            neutronCounts
        );
    }
}

/**
 * @brief Сохранить общую статистику всех событий
 *
 * Сохраняет итоговую статистику по завершении всего Run.
 * Обычно вызывается из MyRunAction::EndOfRunAction().
 *
 * **Может включать:**
 * - Общее количество событий
 * - Среднюю энергию нейтронов
 * - Статистику по типам событий
 * - Информацию о параметрах симуляции
 *
 * @example
 * ```cpp
 * // В MyRunAction::EndOfRunAction()
 * eventAction->SaveSummaryData();
 * ```
 *
 * @note Вызывается в деструкторе
 * @note Может быть пуста, если статистика не требуется
 *
 * @see MyRunAction::EndOfRunAction()
 */
void MyEventAction::SaveSummaryData() {
    // Здесь можно добавить сохранение summary-статистики, если нужно
}