# lambda-c-runtime: library API documentation

The lambda-c-runtime handles all initialization and error management behind the scenes, providing a clean, concise API that lets you focus on writing the core business logic of your Lambda function without dealing with the Lambda runtime API integration.

## `clambda_t`

The first step is to create a clambda context. This can be created on the stack, heap, or as a static global variable. In most cases, the heap is unnecessary, as allocation time would increase the cold start time.

```c
    clambda_t clambda_ctx;
```

## `clambda_init`

Next, initialize the empty context. This step retrieves the Lambda runtime API base URL and the handler as configured in your Lambda function. The handler attribute enables you to use the same code with different behaviors across Lambda functions.

```c
    clambda_err_t clambda_err = clambda_init(&clambda_ctx);
```

## `clambda_err_t`

The `clambda_err_t` type is an enum that is returned by all API functions. If the return value is anything other than `CLAMBDA_ERR_OK`, the `err_str` attribute in the context will contain a detailed error description.

**Note:** You do not need to print or post this error back to the Lambda runtime API; the library handles this automatically.

```c
    if (clambda_err != CLAMBDA_ERR_OK) {
        /* clambda_ctx.err_str contains detailed error message *
         * this gets printed to stderr automatically           */
        return EXIT_FAILURE;
    }
```

## `clambda_user_function` and `user_ctx`

Your own business logic gets implemented in a function with the following signature:

```c
    clambda_err_t (*clambda_user_function)(clambda_t *ctx, void *user_ctx)
```

Within this function, implement any business logic needed in the Lambda function. The event payload is accessible as `ctx->event_data`, which is a `cJSON *` pointer.
_Refer to the [cJSON library documentation](https://github.com/DaveGamble/cJSON) for details on handling this JSON payload_.

The `user_ctx` pointer is available for maintaining state across invocations. You can cast it to any desired type. If not needed, it can be set to `NULL`.

**Note:** Only the `stderr` stream is sent to CloudWatch. To log to CloudWatch, use:

```c
    fprintf(stderr, "your log here\n");
```

## `clambda_invocation_loop`

The `clambda_invocation_loop` runs in a loop within your main function. The returned error can be ignored unless you need to handle it in a specific way. Again, all necessary error logging and reporting is managed by the library.

```c
    for (;;) {
        clambda_invocation_loop(&clambda_ctx, clambda_user_function, (void *) &user_ctx);
    }
```

And that’s it! For an example implementation, refer to `bootstrap.c`.
