# UTEC Video Downloader - C++ Edition

A modern C++17 parallel video downloader for UTEC class recordings.

## Features

- **Parallel Downloads**: Configurable worker threads for concurrent downloads
- **Automatic Organization**: Files organized by year/semester/subject
- **Semester Classification**: Automatic detection of academic semester from dates
- **Duplicate Detection**: Prevents re-downloading existing files
- **Progress Tracking**: Real-time download progress reporting
- **Retry Logic**: Automatic retry with exponential backoff

## Project Structure

```
.
├── CMakeLists.txt              # Build configuration
├── external/                   # Git submodules (dependencies)
│   ├── fmt/                    # {fmt} formatting library
│   ├── googletest/             # Google Test framework
│   └── json/                   # nlohmann/json library
├── include/utec_downloader/    # Public headers
│   ├── core/
│   │   ├── ClassInfo.hpp
│   │   ├── ConfigManager.hpp
│   │   ├── DataParser.hpp
│   │   ├── DownloadManager.hpp
│   │   ├── FileOrganizer.hpp
│   │   ├── IYtdlpDownloader.hpp
│   │   ├── SemesterClassifier.hpp
│   │   └── SubprocessYtdlp.hpp
│   └── utils/
│       └── Logger.hpp
├── src/                        # Implementation files
│   ├── core/
│   ├── utils/
│   └── main.cpp
├── tests/                      # Unit tests
└── config/                     # Configuration files
```

## Dependencies

All dependencies are included as git submodules (no system libraries required):

- **nlohmann/json**: JSON parsing
- **fmt**: Modern C++ formatting
- **GoogleTest**: Testing framework

Optional system dependencies:
- **libcurl**: HTTP downloads (optional, subprocess mode available)

## Building

```bash
# Clone with submodules
git clone --recursive https://github.com/minervarr/Link_Downloader_CPP.git
cd Link_Downloader_CPP

# Or initialize submodules if already cloned
git submodule update --init --recursive

# Build
mkdir build && cd build
cmake ..
cmake --build .

# Run tests
ctest --output-on-failure
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | ON | Build unit tests |
| `BUILD_SHARED_LIBS` | OFF | Build shared libraries |

```bash
cmake .. -DBUILD_TESTS=OFF  # Disable tests
```

## Usage

```bash
# Basic usage
./utec_downloader

# Custom configuration
./utec_downloader -c /path/to/config.txt

# Specify data directory
./utec_downloader -d /path/to/json_files

# Set output directory
./utec_downloader -o /path/to/downloads

# Set worker count
./utec_downloader -w 8

# Verbose logging
./utec_downloader -v

# Help
./utec_downloader -h
```

## Configuration

Create a `config/config.txt` file:

```ini
# Download settings
download_path=downloads
max_workers=4
max_retries=3
quality=best

# yt-dlp settings
ytdlp_path=yt-dlp
cookies_file=

# Logging
log_level=INFO
log_file=logs/downloader.log
```

## JSON Data Format

Place JSON files in `data/json_files/`:

```json
[
  {
    "subject": "Matemáticas I",
    "fecha": "2024-04-15",
    "url": "https://zoom.us/rec/...",
    "weekNumber": 5,
    "seccion": "TEORÍA",
    "modalidad": "PRESENCIAL"
  }
]
```

Alternative field names are also supported: `materia`, `date`, `link`, `semana`.

## Architecture

The project follows a clean architecture pattern:

- **Core Layer**: Business logic (ClassInfo, SemesterClassifier, DataParser)
- **Infrastructure Layer**: External integrations (DownloadManager, SubprocessYtdlp)
- **Utilities**: Cross-cutting concerns (Logger, ConfigManager)

### Key Components

| Component | Responsibility |
|-----------|---------------|
| `ClassInfo` | Data model for class recordings |
| `ConfigManager` | Configuration loading and validation |
| `DataParser` | JSON file parsing and deduplication |
| `SemesterClassifier` | Date-based semester classification |
| `FileOrganizer` | Directory structure and filename generation |
| `DownloadManager` | Parallel download orchestration |
| `IYtdlpDownloader` | Download backend interface |
| `SubprocessYtdlp` | yt-dlp subprocess implementation |

## Testing

```bash
cd build
ctest --output-on-failure

# Or run directly
./utec_downloader_tests
```

## License

MIT License
