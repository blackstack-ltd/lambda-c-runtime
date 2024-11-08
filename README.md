# lambda-c-runtime: A Framework that lest you write Lambda function in C

This framework utilises the "provided.al2023" in AWS Lambda to take control over the runtime, and run a precompiled binary directly on the Lambda instead of spinning up a runtime.

In other words, your binary _is_ the runtime.

As such, it can also be used as a starting point to running any other compiled binary on lambda, wether it is Rust, Cobol, or whatever - as long as it can compile to the lambda target.

## Getting started

Start by compiling the framework. This is done using a docker container provided by AWS that represents the lambda target, installing compilation tools and dependencies in it, and compiling the code.

### Build the container

Using the Dockerfile in the `src` folder

```
    cd src
    docker build -t lambda .
```

### Compile the code

Build the binary, and install it in the `lambda_runtime` folder, overwriting the `bootstrap` shell script (also a fully functioning runtime), with the built binary.

```
    docker run -ti -v $(pwd)/src:/src -v $(pwd)/lambda_runtime:/lambda_runtime -w /src make install
```

### Deploy to AWS

The terraform folder contains a lamda template that can be used to create a lambda function that uses the custom runtime.

First, choose a unique name for your terraform stack bucket and create the bucket.

```bash
    aws s3api create-bucket --bucket YOUR_UNIQUE_BUCKET_NAME
```

Next, replace the bucket name in `terraform/main.tf` with your bucket name

```yaml
    terraform {
      backend "s3" {
        bucket = "lambda-c-runtime-terraform-state" <- YOUR_UNIQUE_BUCKET_NAME_HERE
        key    = "lambda_c_runtime"
        region = "us-east-1"
      }
    }
```

If you're building on an X86 architecture, it is important to change the lambda architecture in `terraform/lambda.tf`

```yaml
    resource "aws_lambda_function" "test_lambda" {
      filename         = "lambda_function_payload.zip"
      function_name    = local.namespace
      role             = aws_iam_role.lambda_role.arn
      handler          = "bootstrap.main"
      source_code_hash = data.archive_file.lambda_zip.output_base64sha256
      runtime          = "provided.al2023"
      architectures    = ["arm64"] <- REPLACE WITH ["x86_64"]
    }
```

Now, deploy the stack

```bash
    cd terraform
    terraform init
    terraform plan -out terraform.plan
    terraform apply "terraform.plan"
```

And invoke the lambda function

```bash
    aws lambda invoke --function lambda_c_runtime --payload '{"hello":"world"}' --cli-binary-format raw-in-base64-out output.txt
```
