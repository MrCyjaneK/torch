#include "torch.h"
#include <iostream>
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

#ifdef __cplusplus
}
#endif