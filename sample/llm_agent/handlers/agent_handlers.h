#ifndef AGENT_HANDLERS_H
#define AGENT_HANDLERS_H

#include "qs_api.h"

int handler_init(QS_EVENT_PARAMETER params);
int handler_think(QS_EVENT_PARAMETER params);
int handler_execute(QS_EVENT_PARAMETER params);
int handler_loop(QS_EVENT_PARAMETER params);
int handler_run(QS_EVENT_PARAMETER params);

#endif /* AGENT_HANDLERS_H */
