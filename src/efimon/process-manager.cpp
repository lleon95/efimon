/**
 * @file process-manager.cpp
 * @author Luis G. Leon-Vega (luis.leon@ieee.org)
 * @brief A really basic process launcher
 *
 * @copyright Copyright (c) 20234. See License for Licensing
 */

#include <efimon/process-manager.hpp>

#include <third-party/pstream.hpp>

#include <iostream>

namespace efimon {

ProcessManager::ProcessManager(const std::string &cmd, const Mode mode,
                               std::ostream *stream)
    : ip_(), mode_{mode}, stream_{stream} {
  this->Open(cmd, mode);
}

ProcessManager::ProcessManager(const std::string &cmd,
                               const std::vector<std::string> &args,
                               const Mode mode, std::ostream *stream)
    : ip_(), mode_{mode}, stream_{stream} {
  this->Open(cmd, args, mode);
}

Status ProcessManager::Open(const std::string &cmd, const Mode mode,
                            std::ostream *stream) {
  Status ret{};
  auto pmode_ = redi::pstreambuf::pstdout | redi::pstreambuf::pstderr;

  switch (mode) {
    case STDOUT:
      pmode_ = redi::pstreambuf::pstdout;
      break;
    case STDERR:
      pmode_ = redi::pstreambuf::pstderr;
      break;
    default:
      break;
  }

  this->ip_.open(cmd, pmode_);
  this->mode_ = mode;
  this->stream_ = stream;

  if (!this->ip_.is_open()) {
    ret = Status{Status::CANNOT_OPEN, "Cannot open the process"};
  }
  return ret;
}

Status ProcessManager::Open(const std::string &cmd,
                            const std::vector<std::string> &args,
                            const Mode mode, std::ostream *stream) {
  Status ret{};
  auto pmode_ = redi::pstreambuf::pstdout | redi::pstreambuf::pstderr;

  switch (mode) {
    case STDOUT:
      pmode_ = redi::pstreambuf::pstdout;
      break;
    case STDERR:
      pmode_ = redi::pstreambuf::pstderr;
      break;
    default:
      break;
  }

  this->ip_.open(cmd, args, pmode_);
  this->mode_ = mode;
  this->stream_ = stream;

  if (!this->ip_.is_open()) {
    ret = Status{Status::CANNOT_OPEN, "Cannot open the process"};
  }
  return ret;
}

Status ProcessManager::Sync(const bool single_line) {
  Status ret{};

  if (!this->ip_.is_open()) {
    ret = Status{Status::FILE_ERROR, "Cannot access the process"};
  }

  std::string line;
  if (!single_line) {
    while (std::getline(this->ip_, line)) {
      if (Mode::SILENT != this->mode_ && this->stream_) {
        (*stream_) << line << std::endl;
      } else if (Mode::SILENT != this->mode_) {
        std::cerr << line << std::endl;
      }
    }
    this->Close();
  } else {
    bool ok = static_cast<bool>(std::getline(this->ip_, line));
    if (Mode::SILENT != this->mode_ && this->stream_) {
      (*stream_) << line << std::endl;
    } else if (Mode::SILENT != this->mode_) {
      std::cerr << line << std::endl;
    }
    if (!ok) {
      this->Close();
    }
  }

  return ret;
}

pid_t ProcessManager::GetPID() { return this->ip_.getpid(); }

Status ProcessManager::Close() {
  this->ip_.close();
  return Status{};
}

bool ProcessManager::IsRunning() {
  this->Sync(true);
  return this->ip_.is_open();
}

} /* namespace efimon */
