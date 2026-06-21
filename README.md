# Qt Network Utils

[![Qt 6](https://img.shields.io/badge/Qt-6.5%2B-green)](https://www.qt.io/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)]()

Переиспользуемые сетевые утилиты для Qt 6 с современным C++17.

## Возможности

### QFileLogger
- Потокобезопасное логирование в файл (`std::mutex`)
- Автоматическая генерация уникальных имён файлов с таймстемпами
- Интеграция с Qt UI через сигналы
- RAII-управление ресурсами

### QFtpClient
- Полная реализация FTP-протокола с нуля (RFC 959)
- **Архитектура State Machine** с 10+ состояниями
- Пассивный режим (PASV)
- Асинхронная загрузка файлов с отслеживанием прогресса
- Неблокирующие операции (интеграция с Qt event loop)
- Подключаемое логирование через абстрактный интерфейс
- Обработка кодов ответа 125 и 150 (совместимость с разными FTP-серверами)

## Установка

### Через CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    qt_network_utils
    GIT_REPOSITORY https://github.com/Bo81K/qt-network-utils.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(qt_network_utils)

target_link_libraries(your_app 
    PRIVATE 
        QtNetworkUtils::FileLogger
        QtNetworkUtils::FtpClient
)
```

### Как Git submodule

```bash
git submodule add https://github.com/Bo81K/qt-network-utils.git third_party/qt-network-utils
```

```cmake
add_subdirectory(third_party/qt-network-utils)
target_link_libraries(your_app PRIVATE QtNetworkUtils::FtpClient)
```

## Quick Start

### Пример логгера

```cpp
#include "qfile_logger.h"

QNetUtils::QFileLogger logger;
logger.init("app.log");

// Подключаем к UI
QObject::connect(&logger, &QNetUtils::QFileLogger::newLogMessage,
                 ui->textEdit, &QTextEdit::append);

logger.log("Приложение запущено");
```

### Пример FTP-клиента

```cpp
#include "qftp_client.h"

QNetUtils::QFtpClient ftp;

QObject::connect(&ftp, &QNetUtils::QFtpClient::connected, [&]() {
    ftp.uploadFile("local.txt", "/remote/file.txt");
});

QObject::connect(&ftp, &QNetUtils::QFtpClient::uploadProgress,
    [](qint64 sent, qint64 total) {
        qDebug() << "Прогресс:" << sent << "/" << total;
    });

ftp.connectToServer("ftp.example.com", 21, "user", "pass");
```

## 🏗 Архитектура

### QFtpClient State Machine

```
Idle → Connecting → WaitingUser → WaitingPass → LoggedIn
                                              ↓
                                         WaitingPasv
                                              ↓
                                       WaitingTransfer
                                              ↓
                                          Transferring
                                              ↓
                                            LoggedIn
```

Все переходы состояний происходят в `processReply()` на основе кодов ответа FTP-сервера (220, 230, 227, 125, 150, 226 и т.д.).

## 🔧 Сборка

```bash
mkdir build && cd build
cmake .. -DBUILD_EXAMPLES=ON
cmake --build .
```

### Опции CMake

| Опция | По умолчанию | Описание |
|-------|--------------|----------|
| `BUILD_EXAMPLES` | ON | Собрать примеры приложений |
| `BUILD_SHARED_LIBS` | OFF | Собрать динамические библиотеки |

## 📂 Структура проекта

```
qt-network-utils/
├── CMakeLists.txt              # Главный CMake
├── README.md
├── LICENSE
├── libs/
│   ├── file_logger/            # Библиотека логгера
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   └── src/
│   └── ftp_client/             # Библиотека FTP-клиента
│       ├── CMakeLists.txt
│       ├── include/
│       └── src/
└── examples/
    ├── logger_demo/            # Пример использования логгера
    └── ftp_upload/             # Пример загрузки файла по FTP
```

## 📄 Лицензия

MIT License - см. [LICENSE](LICENSE)

