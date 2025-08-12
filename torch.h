#ifndef TORCH_LIBRARY_H
#define TORCH_LIBRARY_H
#ifdef __cplusplus
extern "C"
{
#endif

int TOR_start(int argc, char *argv[]);
const char* TOR_version();

#ifdef __cplusplus
}
#endif

#endif // TORCH_LIBRARY_H
