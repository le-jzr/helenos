#pragma once

// Root service methods.

#include <protocol/core.h>
#include <abi/ipc/ipc.h>

typedef enum ipc_root_retval {
    ipc_root_success,
    ipc_root_failure,
} ipc_root_retval_t;

typedef ipc_object_t *(*ipc_root_handler_t)(const ipc_data_t *args);

ipc_root_retval_t ipc_root_register(const char *name, ipc_root_handler_t handler);
void ipc_root_send(const char *name, const ipc_data_t *args);
void ipc_root_wait_for(const char *name);

typedef struct {
    ipc_root_retval_t (*obj_register)(const char *name, ipc_object_t *obj);
    ipc_object_t *(*obj_get)(const char *name);
    ipc_root_retval_t (*waiter_register)(const char *name, ipc_object_t *obj);
} ipc_root_server_ops_t;

void ipc_root_serve(const ipc_root_server_ops_t *ops);
