#pragma once

#include <filesystem>
#include <string>

#include "utils/file_writer.h"

namespace wevoaweb {

class ProjectCreator {
  public:
    explicit ProjectCreator(std::filesystem::path runtimeRoot = {}, FileWriter writer = {});

    std::filesystem::path createProject(const std::string& templateName, const std::string& projectName) const;

  private:
    std::filesystem::path resolveTargetPath(const std::string& projectName) const;
    std::filesystem::path resolveTemplateRoot() const;

    std::filesystem::path runtimeRoot_;
    FileWriter writer_;
};

}  // namespace wevoaweb
