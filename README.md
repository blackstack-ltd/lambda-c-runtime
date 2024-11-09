# lambda-c-runtime: A Framework for writing AWS Lambda functions in C

This framework leverages the provided.al2023 runtime in AWS Lambda to directly control the runtime environment, allowing you to run a precompiled binary on Lambda instead of spinning up a standard runtime.

In other words, your binary _is_ the runtime.

This setup can also serve as a starting point for running any other compiled binary on Lambda, whether written in C, Rust, COBOL, or any language that can compile to the Lambda target architecture.

## Getting started

To get started, begin by compiling the framework. This process involves using an AWS-provided Docker container to emulate the Lambda target environment, setting up necessary compilation tools and dependencies, and compiling the code.

### Build the container

Navigate to the `src` folder and build the container using the provided Dockerfile.

```
    cd src
    docker build -t lambda .
```

### Compile the code

Build the binary and install it in the `lambda_runtime` folder. This replaces the `bootstrap` shell script (a functioning runtime in itself) with the compiled binary.

```
    docker run -ti -v $(pwd)/src:/src -v $(pwd)/lambda_runtime:/lambda_runtime -w /src make install
```

### Deploy to AWS

The terraform folder contains a lamda template that can be used to create a lambda function that uses the custom runtime.

Choose a unique name for your Terraform state bucket and create it:

```bash
    aws s3api create-bucket --bucket YOUR_UNIQUE_BUCKET_NAME
```

Replace the placeholder bucket name in `terraform/main.tf` with your unique bucket name:

```yaml
    terraform {
      backend "s3" {
        bucket = "YOUR_UNIQUE_BUCKET_NAME_HERE"
        key    = "lambda_c_runtime"
        region = "us-east-1"
      }
    }
```

If you’re building on an `x86_64` architecture, modify the Lambda architecture in `terraform/lambda.tf`:

```yaml
    resource "aws_lambda_function" "test_lambda" {
      filename         = "lambda_function_payload.zip"
      function_name    = local.namespace
      role             = aws_iam_role.lambda_role.arn
      handler          = "bootstrap.main"
      source_code_hash = data.archive_file.lambda_zip.output_base64sha256
      runtime          = "provided.al2023"
      architectures    = ["x86_64"] # Change arm64 to x86_64
    }
```

Deploy the Lambda stack using Terraform:

```bash
    cd terraform
    terraform init
    terraform plan -out terraform.plan
    terraform apply "terraform.plan"
```

Finally, invoke the Lambda function with a sample payload:

```bash
    aws lambda invoke --function lambda_c_runtime --payload '{"hello":"world"}' --cli-binary-format raw-in-base64-out output.txt
```

## Implement your own logic

See the [clambda api documentation](https://github.com/blackstack-ltd/lambda-c-runtime/tree/main/src)
