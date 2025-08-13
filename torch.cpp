#include "torch.h"
#include <iostream>

#ifdef __ANDROID__
#include <unistd.h>     // for pipe, read, dup2, STDOUT_FILENO, STDERR_FILENO
#include <thread>       // for std::thread
#include <android/log.h>

#define LOG_TAG "torch"
#define BUFFER_SIZE 1024*32

static int stdoutToLogcat(const char *buf, int size) {
    __android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf);
    return size;
}

static int stderrToLogcat(const char *buf, int size) {
    __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, buf);
    return size;
}

void redirectStdoutThread(int pipe_stdout[2]) {
    char bufferStdout[BUFFER_SIZE];
    while (true) {
        int read_size = read(pipe_stdout[0], bufferStdout, sizeof(bufferStdout) - 1);
        if (read_size > 0) {
            bufferStdout[read_size] = '\0';
            stdoutToLogcat(bufferStdout, read_size);
        }
    }
}

void redirectStderrThread(int pipe_stderr[2]) {
    char bufferStderr[BUFFER_SIZE];
    while (true) {
        int read_size = read(pipe_stderr[0], bufferStderr, sizeof(bufferStderr) - 1);
        if (read_size > 0) {
            bufferStderr[read_size] = '\0';
            stderrToLogcat(bufferStderr, read_size);
        }
    }
}

void setupAndroidLogging() {
    static int pfdStdout[2];
    static int pfdStderr[2];

    pipe(pfdStdout);
    pipe(pfdStderr);

    dup2(pfdStdout[1], STDOUT_FILENO);
    dup2(pfdStderr[1], STDERR_FILENO);

    std::thread stdoutThread(redirectStdoutThread, pfdStdout);
    std::thread stderrThread(redirectStderrThread, pfdStderr);

    stdoutThread.detach();
    stderrThread.detach();
}

#endif // __ANDROID__


__attribute__((constructor))
void library_init() {
#ifdef __ANDROID__
    setupAndroidLogging();
#endif
}

#ifdef __cplusplus
extern "C"
{
#endif
#ifdef __MINGW32__
#define ADDAPI __declspec(dllexport)
#else
#define ADDAPI __attribute__((__visibility__("default")))
#endif
#include <tor/feature/api/tor_api.h>

extern ADDAPI int TOR_start(int argc, char *argv[]) {
    tor_main_configuration_t* cfg = tor_main_configuration_new();
    if (!cfg) {
        std::cerr << "libtor.so: Failed to create Tor configuration\n";
        return -1;
    }

    int set_result = tor_main_configuration_set_command_line(cfg, argc, argv);
    if (set_result != 0) {
        std::cerr << "libtor.so: Failed to set command-line args for Tor\n";
        tor_main_configuration_free(cfg);
        return set_result;
    }

    // tor_control_socket_t ctrl = tor_main_configuration_setup_control_socket(cfg);

    int run_result = tor_run_main(cfg);

    // Free configuration
    tor_main_configuration_free(cfg);

    return run_result;
}

const char* TOR_version() {
    return tor_api_get_provider_version();
}

#ifdef __cplusplus
}
#endif