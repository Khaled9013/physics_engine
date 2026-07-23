#ifndef BALLISTICS_CLI_FILE_BYTE_SINK_H
#define BALLISTICS_CLI_FILE_BYTE_SINK_H

#include "ballistics/interfaces/byte_sink.h"
#include <stdio.h>

BallisticsByteSink ballistics_cli_file_byte_sink(FILE *file);

#endif
