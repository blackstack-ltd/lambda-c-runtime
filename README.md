# lambda-c-runtime: A Framework that lest you write Lambda function in C

This framework utilises the "provided.al2023" in AWS Lambda to take control over the runtime, and run a precompiled binary directly on the Lambda instead of spinning up a runtime.

In other words, your binary _is_ the runtime.

As such, it can also be used as a starting point to running any other compiled binary on lambda, wether it is Rust, Cobol, or whatever - as long as it can compile to the lambda target.

## Getting started

Start by compiling the framework. This is done using a docker container provided by AWS that represents the lambda target, installing compilation tools and dependencies in it, and compiling the code.

### Build the container

Using the Dockerfile in the `src` folder

```
    cd src && docker build -t lambda .
```

### Compile the code

Build the binary, and install it in the `lambda_runtime` folder, overwriting the `bootstrap` shell script (also a fully functioning runtime), with the built binary.

```
    docker run -ti -v $(pwd)/src:/src -v $(pwd)/lambda_runtime:/lambda_runtime -w /src make install
```

### Deploy to AWS

The terraform folder contains a lamda template that can be used to create a lambda function that uses the custom runtime.

```
    cd terraform
    terraform init
    terraform plan -out terraform.plan
    terraform apply "terraform.plan"
```
