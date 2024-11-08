#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clambda.h"

clambda_err_t my_function(clambda_t *ctx, void *my_context) {
  char *string = NULL;

  if ((string = cJSON_Print(ctx->event_data)) == NULL) {
    snprintf(ctx->err_str, CLAMBDA_BUFFER_SIZE, "cannot print event data");
    return CLAMBDA_ERR_USER;
  }

  fprintf(stderr, "event_data:\n%s\n", string);

  fprintf(stderr, "%u invocations so far\n", (*((unsigned int *) my_context))++);

  free(string);

  return CLAMBDA_ERR_OK;
}

int main(void) {
  clambda_t clambda_ctx;
  clambda_err_t clambda_err = CLAMBDA_ERR_OK;
  unsigned int my_context = 0;

  if ((clambda_err = clambda_init(&clambda_ctx)) != CLAMBDA_ERR_OK) {
    return EXIT_FAILURE;
  }

  for (;;) {
    if ((clambda_err = clambda_invocation_loop(&clambda_ctx, my_function, (void *) &my_context)) != CLAMBDA_ERR_OK) {
      fprintf(stderr, "clambda_invocation_loop error\n");
    }
  }

  return EXIT_SUCCESS;
}
