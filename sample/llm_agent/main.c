#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qs_api.h"
#include "qs_llama_module.h"
#include "agent_core.h"
#include "handlers/agent_handlers.h"

QS_MEMORY_CONTEXT g_temporary_memory;

/* Prompt template loaded from prompt.conf at startup.
 * Placeholders: {query}, {accumulated_context}
 * Accessible from handlers via extern. */
char* g_agent_prompt_template = NULL;

static int load_prompt_template(void)
{
    const char* path = getenv("QS_AGENT_PROMPT_FILE");
    if (!path || !path[0]) path = "./prompt.conf";

    FILE* fp = fopen(path, "rb");
    if (!fp) {
        /* Not fatal — handler falls back to built-in template */
        printf("[Main] prompt.conf not found at %s, using built-in template.\n", path);
        return 0;
    }

    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return 0; }
    long sz = ftell(fp);
    if (sz <= 0) { fclose(fp); return 0; }
    if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return 0; }

    g_agent_prompt_template = (char*)malloc((size_t)sz + 1);
    if (!g_agent_prompt_template) { fclose(fp); return -1; }

    size_t read_sz = fread(g_agent_prompt_template, 1, (size_t)sz, fp);
    fclose(fp);
    g_agent_prompt_template[read_sz] = '\0';

    printf("[Main] Loaded prompt template from %s (%zu bytes)\n", path, read_sz);
    return 0;
}

static int on_http_event(QS_EVENT_PARAMETER params)
{
    const char* path = api_qs_get_http_path(params);
    if (path == NULL) return 404;

    if (strcmp(path, "/api/agent/init")    == 0) return handler_init(params);
    if (strcmp(path, "/api/agent/think")   == 0) return handler_think(params);
    if (strcmp(path, "/api/agent/execute") == 0) return handler_execute(params);
    if (strcmp(path, "/api/agent/loop")    == 0) return handler_loop(params);
    if (strcmp(path, "/api/agent/run")        == 0) return handler_run(params);
    if (strcmp(path, "/api/agent/run/stream") == 0) return handler_run_stream(params);

    return 404;
}

int main(int argc, char* argv[], char* envp[])
{
    (void)argc; (void)argv; (void)envp;

    if (-1 == api_qs_memory_alloc(&g_temporary_memory, 1024 * 1024 * 8)) {
        printf("[Main] api_qs_memory_alloc failed\n");
        return -1;
    }

    int     server_port    = 4445;
    int     scheduler_mode = QS_SCHEDULER_MODE_LOW;
    int32_t max_connection = 10;
    const char* workspace_root = ".";

    QS_SERVER_SCRIPT_CONTEXT script;
    if (-1 == api_qs_script_read_file(&g_temporary_memory, &script, "./settings.conf")) {
        return -1;
    }
    if (-1 == api_qs_script_run(&script)) { return -1; }

    if (0 != api_qs_script_get_parameter(&script, "server_port")) {
        server_port = atoi(api_qs_script_get_parameter(&script, "server_port"));
    }
    if (0 != api_qs_script_get_parameter(&script, "scheduler_mode")) {
        const char* sm = api_qs_script_get_parameter(&script, "scheduler_mode");
        if      (!strcmp(sm, "high"))   scheduler_mode = QS_SCHEDULER_MODE_HIGH;
        else if (!strcmp(sm, "middle")) scheduler_mode = QS_SCHEDULER_MODE_MIDDLE;
        else                            scheduler_mode = QS_SCHEDULER_MODE_LOW;
    }
    if (0 != api_qs_script_get_parameter(&script, "max_connection")) {
        int v = atoi(api_qs_script_get_parameter(&script, "max_connection"));
        if (v < 10)   v = 10;
        if (v > 1000) v = 1000;
        max_connection = (int32_t)v;
    }
    if (0 != api_qs_script_get_parameter(&script, "agent_workspace_root")) {
        workspace_root = api_qs_script_get_parameter(&script, "agent_workspace_root");
    }

    if (-1 == agent_set_workspace_root(workspace_root)) {
        printf("[Main] invalid agent_workspace_root: %s\n", workspace_root);
        return -1;
    }

    if (-1 == qs_llama_module_prepare()) { return -1; }

    if (-1 == load_prompt_template()) {
        qs_llama_module_shutdown();
        return -1;
    }

    printf("[Main] Workspace root: %s\n", agent_get_workspace_root());
    printf("[Main] Starting agent server on port %d...\n", server_port);

    QS_SERVER_CONTEXT* context = NULL;
    int init_error = 1;
    do {
        if (0 > api_qs_server_init(&context, server_port, max_connection,
                                    QS_SERVER_TYPE_HTTP)) { break; }
        if (-1 == api_qs_set_scheduler(context, scheduler_mode)) { break; }
        if (-1 == api_qs_server_create_router(context))           { break; }
        init_error = 0;
    } while (0);

    if (init_error) {
        printf("[Main] Server initialization failed\n");
        qs_llama_module_shutdown();
        return -1;
    }

    api_qs_set_on_http_event(context, on_http_event);
    api_qs_memory_clean(&g_temporary_memory);

    for (;;) {
        api_qs_update(context);
        api_qs_sleep(context);
    }

    api_qs_free(context);
    qs_llama_module_shutdown();
    if (g_agent_prompt_template) {
        free(g_agent_prompt_template);
        g_agent_prompt_template = NULL;
    }
    api_qs_memory_free(&g_temporary_memory);
    return 0;
}
