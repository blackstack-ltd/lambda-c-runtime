#ifndef _CLAMBDA_H_
#define _CLAMBDA_H_

#include "cJSON.h"

#define CLAMBDA_BUFFER_SIZE 256
#define CLAMBDA_RESP_SIZE 1024

//clambda_err_t (*clambda_user_function)(clambda_t *ctx, void *user_ctx);

typedef struct {
  char *runtime_api;
  char *handler;
  char request_id[CLAMBDA_BUFFER_SIZE];

  cJSON *event_data;

  char err_str[CLAMBDA_BUFFER_SIZE];
} clambda_t;

typedef enum {
  CLAMBDA_ERR_OK = 0,
  CLAMBDA_ERR_ENV,
  CLAMBDA_ERR_URL,
  CLAMBDA_ERR_HTTP,
  CLAMBDA_ERR_JSON,
  CLAMBDA_ERR_USER
} clambda_err_t;

#ifdef __cplusplus
extern "C" {
#endif

  char *clambda_errstr(clambda_err_t err);
  clambda_err_t clambda_init(clambda_t *ctx);
  clambda_err_t clambda_invocation_loop(clambda_t *ctx, clambda_err_t (*clambda_user_function)(clambda_t *ctx, void *user_ctx), void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif // _CLAMBDA_H_
