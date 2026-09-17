#ifndef CAPTURE_VIDEO_WRITER_HPP
#define CAPTURE_VIDEO_WRITER_HPP

#include <string>
#include <stdexcept>
#include <vector>
#ifdef WIN32
#include <windows.h>
#endif

// One bounded OS pipe per stream. No shell and no intermediate image files.
class CaptureVideoWriter {
public:
  CaptureVideoWriter(const std::string &executable, const std::string &filename,
                     int width, int height, int fps) : width(width), height(height) {
#ifdef WIN32
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    HANDLE inputRead = NULL;
    if (!CreatePipe(&inputRead, &inputWrite, &sa, 65536)) Fail("create pipe");
    SetHandleInformation(inputWrite, HANDLE_FLAG_INHERIT, 0);
    HANDLE log = CreateFileA((filename + ".ffmpeg.log").c_str(), GENERIC_WRITE,
      FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (log == INVALID_HANDLE_VALUE) {
      CloseHandle(inputRead); CloseHandle(inputWrite); inputWrite = NULL;
      Fail("open encoder log");
    }
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = inputRead; startup.hStdOutput = log; startup.hStdError = log;
    PROCESS_INFORMATION info = {};
    std::string command = Quote(executable) + " -hide_banner -loglevel error -nostdin -n"
      " -f rawvideo -pixel_format rgba -video_size " + std::to_string(width) + "x" + std::to_string(height) +
      " -framerate " + std::to_string(fps) + " -i pipe:0 -an -c:v ffv1 -level 3"
      " -threads 2 -pix_fmt bgr0 " + Quote(filename);
    std::vector<char> args(command.begin(), command.end()); args.push_back(0);
    const BOOL ok = CreateProcessA(NULL, args.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW,
                                  NULL, NULL, &startup, &info);
    CloseHandle(inputRead); CloseHandle(log);
    if (!ok) { CloseHandle(inputWrite); inputWrite = NULL; Fail("start FFmpeg (check cosmos_ffmpeg_path)"); }
    process = info.hProcess; CloseHandle(info.hThread);
#else
    throw std::runtime_error("Direct video capture currently requires Windows");
#endif
  }
  ~CaptureVideoWriter() {
#ifdef WIN32
    if (inputWrite) CloseHandle(inputWrite);
    if (process) { WaitForSingleObject(process, INFINITE); CloseHandle(process); }
#endif
  }
  void Write(const unsigned char *data, int w, int h) {
    if (w != width || h != height) Fail("resolution changed during capture");
#ifdef WIN32
    std::size_t remaining = static_cast<std::size_t>(w) * h * 4;
    while (remaining) {
      DWORD written = 0;
      const DWORD chunk = static_cast<DWORD>((remaining > 65536) ? 65536 : remaining);
      if (!WriteFile(inputWrite, data, chunk, &written, NULL) || !written) Fail("write video frame; see .ffmpeg.log");
      data += written; remaining -= written;
    }
#endif
  }
  void Finish() {
#ifdef WIN32
    if (inputWrite) { CloseHandle(inputWrite); inputWrite = NULL; }
    if (process) {
      WaitForSingleObject(process, INFINITE);
      DWORD code = 1; GetExitCodeProcess(process, &code);
      CloseHandle(process); process = NULL;
      if (code != 0) Fail("FFmpeg failed to finalize video; see .ffmpeg.log");
    }
#endif
  }
private:
  int width, height;
#ifdef WIN32
  HANDLE inputWrite = NULL, process = NULL;
#endif
  static std::string Quote(const std::string &value) {
    if (value.find('"') != std::string::npos) Fail("quote in encoder path");
    return "\"" + value + "\"";
  }
  static void Fail(const std::string &message) { throw std::runtime_error("Capture: " + message); }
  CaptureVideoWriter(const CaptureVideoWriter &) = delete;
  CaptureVideoWriter &operator=(const CaptureVideoWriter &) = delete;
};
#endif
