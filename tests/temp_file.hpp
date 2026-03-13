/**
 * @file temp_file.hpp
 * @brief RAII temporary file helper for test cases.
 *
 * Creates a uniquely-named file in /tmp (or %TEMP% on Windows).
 * The file is deleted automatically when the object goes out of scope.
 */

#pragma once
#include <string>
#include <fstream>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

class TempFile {
public:
    explicit TempFile(const std::string& content = "") {
#ifdef _WIN32
        char tmp_dir[MAX_PATH];
        GetTempPathA(MAX_PATH, tmp_dir);
        char tmp_path[MAX_PATH];
        GetTempFileNameA(tmp_dir, "ss_", 0, tmp_path);
        m_path = tmp_path;
#else
        m_path = "/tmp/safesocket_test_XXXXXX";
        int fd = mkstemp(&m_path[0]);
        if (fd >= 0) ::close(fd);
#endif
        if (!content.empty()) {
            std::ofstream f(m_path, std::ios::binary);
            f.write(content.data(), (std::streamsize)content.size());
        }
    }

    ~TempFile() {
        if (!m_path.empty())
            std::remove(m_path.c_str());
    }

    const std::string& path() const { return m_path; }

    /// Read entire file contents back as a string.
    std::string read_all() const {
        std::ifstream f(m_path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(f),
                           std::istreambuf_iterator<char>());
    }

    bool exists() const {
        std::ifstream f(m_path);
        return f.good();
    }

    // Non-copyable
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

private:
    std::string m_path;
};
