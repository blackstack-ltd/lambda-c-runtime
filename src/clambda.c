#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http.c"
#include "clambda.h"

static cJSON *parse_json(clambda_t *ctx, char *string) {
  cJSON *rc = NULL;

  if ((rc = cJSON_Parse(string)) == NULL) {
    const char *error_ptr = NULL;
    if ((error_ptr = cJSON_GetErrorPtr()) != NULL) {
      snprintf(ctx->err_str, 256, "json parse error: error occurred before: %s", error_ptr);
    } else {
      snprintf(ctx->err_str, 256, "json parse error");
    }
  }

  return rc;
}

static char *post_error(clambda_t *ctx, clambda_err_t error_type) {
  char *response = NULL,
       error_response[CLAMBDA_RESP_SIZE],
       post_url[CLAMBDA_BUFFER_SIZE];
  
  fprintf(stderr, "%s\n", ctx->err_str);

  if (ctx->runtime_api == NULL) {
    return response;
  }

  if (snprintf(error_response, CLAMBDA_RESP_SIZE, "{\"errorMessage\":\"%s\",\"errorType\":\"%s\"}", ctx->err_str, clambda_errstr(error_type)) > (CLAMBDA_RESP_SIZE - 1)) {
    fprintf(stderr, "error response too long for internal response buffer: increase CLAMBDA_RESP_SIZE\n");
    return response;
  }

  if (strlen(ctx->request_id) > 0) {
    if (snprintf(post_url, CLAMBDA_BUFFER_SIZE, "http://%s/2018-06-01/runtime/invocation/%s/error", ctx->runtime_api, ctx->request_id) > (CLAMBDA_BUFFER_SIZE - 1)) {
      fprintf(stderr, "invocation error url too long for internal buffer: increase CLAMBDA_BUFFER_SIZE\n");
      return response;
    }
  } else {
    if (snprintf(post_url, CLAMBDA_BUFFER_SIZE, "http://%s/2018-06-01/runtime/init/error", ctx->runtime_api) > (CLAMBDA_BUFFER_SIZE - 1)) {
      fprintf(stderr, "init error url too long for internal buffer: increase CLAMBDA_BUFFER_SIZE\n");
      return response;
    }
  }

  if ((response = http_post(post_url, error_response)) == NULL) {
    fprintf(stderr, "error post request failed: %s\n", http_strerror());
  }

  return response;
}

static clambda_err_t get_invocation(clambda_t *ctx) {
  clambda_err_t rc = CLAMBDA_ERR_OK;

  char url[CLAMBDA_BUFFER_SIZE],
       env[CLAMBDA_RESP_SIZE],
       *response = NULL;
  hdr_t headers[2] = {
    { .name = "Lambda-Runtime-Aws-Request-Id", .value = "" },
    { .name = "Lambda-Runtime-Trace-Id", .value = "" }
  };

  if (ctx->event_data) {
    cJSON_Delete(ctx->event_data);
    ctx->event_data = NULL;
  }

  if (snprintf(url, CLAMBDA_BUFFER_SIZE, "http://%s/2018-06-01/runtime/invocation/next", ctx->runtime_api) > (CLAMBDA_BUFFER_SIZE - 1)) {
    snprintf(ctx->err_str, CLAMBDA_BUFFER_SIZE, "invocation url too long for internale buffer: increase CLAMBDA_BUFFER_SIZE");
    rc = CLAMBDA_ERR_URL;
    goto clean;
  }

  if ((response = http_get(url, headers, 2)) == NULL) {
    snprintf(ctx->err_str, CLAMBDA_BUFFER_SIZE, "invocation request fails: %s", http_strerror());
    rc = CLAMBDA_ERR_HTTP;
    goto clean;
  }

  if (snprintf(ctx->request_id, CLAMBDA_BUFFER_SIZE, "%s", headers[0].value) > (CLAMBDA_BUFFER_SIZE - 1)) {
    snprintf(ctx->err_str, CLAMBDA_ERR_ENV, "request_id too long for internal buffer: increase CLAMBDA_BUFFER_SIZE");
    rc = CLAMBDA_ERR_ENV;
    goto clean;
  }

  if (snprintf(env, CLAMBDA_RESP_SIZE, "_X_AMZN_TRACE_ID=%s", headers[1].value) > (CLAMBDA_RESP_SIZE - 1)) {
    snprintf(ctx->err_str, CLAMBDA_ERR_ENV, "trace id too long for internal buffer: increase CLAMBDA_RESP_SIZE");
    rc = CLAMBDA_ERR_ENV;
    goto clean;
  }

  if (putenv(env) != 0) {
    snprintf(ctx->err_str, CLAMBDA_BUFFER_SIZE, "cannot set env var '%s'", env);
    rc = CLAMBDA_ERR_ENV;
    goto clean;
  }

  if ((ctx->event_data = parse_json(ctx, response)) == NULL) {
    rc = CLAMBDA_ERR_JSON;
    goto clean;
  }

clean:
  if (response) {
    free(response);
  }

  return rc;
}

static clambda_err_t post_success(clambda_t *ctx) {
  clambda_err_t rc = CLAMBDA_ERR_OK;

  char url[CLAMBDA_BUFFER_SIZE],
       *response = NULL;
  cJSON *json = NULL,
        *status = NULL;

  if (snprintf(url, CLAMBDA_BUFFER_SIZE, "http://%s/2018-06-01/runtime/invocation/%s/response", ctx->runtime_api, ctx->request_id) > (CLAMBDA_BUFFER_SIZE - 1)) {
    snprintf(ctx->err_str, CLAMBDA_BUFFER_SIZE, "response url too long for internal buffer: increase CLAMBDA_BUFFER_SIZE");
    rc = CLAMBDA_ERR_URL;
    goto clean;
  }

  if ((response = http_post(url, "SUCCESS")) == NULL) {
    snprintf(ctx->err_str, CLAMBDA_BUFFER_SIZE, "success post request failed: %s", http_strerror());
    rc = CLAMBDA_ERR_HTTP;
    goto clean;
  }

  if ((json = parse_json(ctx, response)) == NULL) {
    rc = CLAMBDA_ERR_JSON;
    goto clean;
  }

  if ((status = cJSON_GetObjectItemCaseSensitive(json, "status")) == NULL) {
    snprintf(ctx->err_str, CLAMBDA_BUFFER_SIZE, "json parse error: cannot get 'status'");
    rc = CLAMBDA_ERR_JSON;
    goto clean;
  }

  if (!cJSON_IsString(status)) {
    snprintf(ctx->err_str, CLAMBDA_BUFFER_SIZE, "'status' is not a string");
    rc = CLAMBDA_ERR_HTTP;
    goto clean;
  }

  if (strncmp(status->valuestring, "OK", 2) != 0) {
    snprintf(ctx->err_str, CLAMBDA_BUFFER_SIZE, "'status' is not 'OK', but '%s'", status->valuestring);
    rc = CLAMBDA_ERR_HTTP;
    goto clean;
  }

clean:
  if (response) {
    free(response);
  }

  if (json) {
    cJSON_Delete(json);
  }

  return rc;
}

char *clambda_errstr(clambda_err_t err) {
  switch (err) {
    case CLAMBDA_ERR_OK:
      return "Runtime.Success";
    case CLAMBDA_ERR_ENV:
      return "Runtime.Env";
    case CLAMBDA_ERR_URL:
      return "Runtime.Url";
    case CLAMBDA_ERR_HTTP:
      return "Runtime.Http";
    case CLAMBDA_ERR_JSON:
      return "Runtime.Json";
    case CLAMBDA_ERR_USER:
      return "Runtime.User";
    default:
      return "Runtime.Unknown";
  }
  return "Runtime.Unknown";
}

clambda_err_t clambda_init(clambda_t *ctx) {
  clambda_err_t rc = CLAMBDA_ERR_OK;

  memset(ctx, 0, sizeof(clambda_t));

  if ((ctx->runtime_api = getenv("AWS_LAMBDA_RUNTIME_API")) == NULL) {
    rc = CLAMBDA_ERR_ENV;
    snprintf(ctx->err_str, CLAMBDA_BUFFER_SIZE, "getenv: cannot get env var AWS_LAMBDA_RUNTIME_API");
    goto clean;
  }

  if ((ctx->handler = getenv("_HANDLER")) == NULL) {
    rc = CLAMBDA_ERR_ENV;
    snprintf(ctx->err_str, CLAMBDA_BUFFER_SIZE, "getenv: cannot get env var _HANDLER");
    goto clean;
  }

clean:
  if (rc != CLAMBDA_ERR_OK) {
    char *post_response = post_error(ctx, rc);
    if (post_response) {
      free(post_response);
    }
  }

  return rc;
}

clambda_err_t clambda_invocation_loop(clambda_t *ctx, clambda_err_t (*user_function)(clambda_t *ctx, void *user_ctx), void *user_ctx) {
  clambda_err_t rc = CLAMBDA_ERR_OK;
  ctx->request_id[0] = '\0';

  if ((rc = get_invocation(ctx)) != CLAMBDA_ERR_OK) {
    goto clean;
  }

  if ((rc = user_function(ctx, user_ctx)) != CLAMBDA_ERR_OK) {
    goto clean;
  }

  if ((rc = post_success(ctx)) != CLAMBDA_ERR_OK) {
    goto clean;
  }

clean:
  if (rc != CLAMBDA_ERR_OK) {
    char *response = post_error(ctx, rc);
    if (response) {
      free(response);
    }
  }

  return rc;
}
