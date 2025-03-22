#include <Volk/volk.h>

// Nasty workaround
// volk.lib seems to be broken in the last SDK version. So we just have to compile it ourselves.
// https://github.com/zeux/volk/issues/230
#include <Volk/volk.c>