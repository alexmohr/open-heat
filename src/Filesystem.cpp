//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//

#include "Filesystem.hpp"
#include "Logger.hpp"

namespace open_heat {
void Filesystem::setup() {
  // Format FileFS if not yet
#ifdef ESP32
  if (!FileFS.begin(true))
#else
  if (!FileFS.begin())
#endif
  {
    Logger::log(Logger::WARNING, "%s begin failed, formatting...", FS_Name);
#ifdef ESP8266
    if (!filesystem->format()) {
      Logger::log(Logger::ERROR, "Format of fs failed!");
    }
#endif
  }

  listFiles();
  Logger::log(Logger::DEBUG, "FS setup done");
}

void Filesystem::listFiles() {
  Dir dir = filesystem->openDir("/");
  Logger::log(Logger::DEBUG, "Opening / directory");

  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Logger::log(Logger::DEBUG, "FS File: %s, size: %s", fileName.c_str(),
                Logger::formatBytes(fileSize).c_str());
  }
}
} // namespace open_heat
