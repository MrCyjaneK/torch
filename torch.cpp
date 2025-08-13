#include "torch.h"
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <atomic>
#include <cstring>

#ifdef __ANDROID__
#include <unistd.h>
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

static std::atomic<bool> tor_running{false};
static std::thread tor_thread;

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // Reap all available child processes
    }
}

__attribute__((constructor))
void library_init() {
#ifdef __ANDROID__
    setupAndroidLogging();
#endif
    signal(SIGCHLD, sigchld_handler);
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

void run_tor_in_thread(int argc, char** argv) {
    tor_main_configuration_t* cfg = tor_main_configuration_new();
    if (!cfg) {
        std::cerr << "torch: Failed to create Tor configuration\n";
        tor_running = false;
        return;
    }

    int set_result = tor_main_configuration_set_command_line(cfg, argc, argv);
    if (set_result != 0) {
        std::cerr << "torch: Failed to set command-line args for Tor\n";
        tor_main_configuration_free(cfg);
        tor_running = false;
        return;
    }

    std::cout << "torch: Starting TOR in background thread\n";
    int run_result = tor_run_main(cfg);

    tor_main_configuration_free(cfg);
    tor_running = false;
    
    if (run_result != 0) {
        std::cerr << "torch: Tor exited with code: " << run_result << "\n";
    }
}

extern ADDAPI int TOR_start(int argc, char *argv[]) {
#ifdef __APPLE__
    // iOS doesn't allow fork(), use threads instead
    if (tor_running.load()) {
        std::cout << "torch: TOR is already running\n";
        return 0;
    }
    
    if (tor_thread.joinable()) {
        tor_thread.join();
    }
    
    char** argv_copy = new char*[argc];
    for (int i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]) + 1;
        argv_copy[i] = new char[len];
        strcpy(argv_copy[i], argv[i]);
    }
    
    tor_running = true;
    tor_thread = std::thread([argc, argv_copy]() {
        run_tor_in_thread(argc, argv_copy);
        
        for (int i = 0; i < argc; i++) {
            delete[] argv_copy[i];
        }
        delete[] argv_copy;
    });
    
    tor_thread.detach();
    std::cout << "torch: TOR started in background thread\n";
    return 0;
#else
    pid_t pid = fork();
    
    if (pid == -1) {
        std::cerr << "torch: Failed to fork process\n";
        return -1;
    }
    
    if (pid == 0) {
        tor_main_configuration_t* cfg = tor_main_configuration_new();
        if (!cfg) {
            std::cerr << "torch: Failed to create Tor configuration\n";
            _exit(-1);
        }

        int set_result = tor_main_configuration_set_command_line(cfg, argc, argv);
        if (set_result != 0) {
            std::cerr << "torch: Failed to set command-line args for Tor\n";
            tor_main_configuration_free(cfg);
            _exit(set_result);
        }

        int run_result = tor_run_main(cfg);

        tor_main_configuration_free(cfg);

        _exit(run_result);
    }
    
    std::cout << "torch: TOR started in background process (PID: " << pid << ")\n";
    return 0;
#endif
}

const char* TOR_version() {
    return tor_api_get_provider_version();
}

#ifdef __cplusplus
}
#endif