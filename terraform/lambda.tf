data "aws_iam_policy_document" "assume_role" {
  statement {
    effect = "Allow"
    principals {
      type        = "Service"
      identifiers = ["lambda.amazonaws.com"]
    }
    actions = ["sts:AssumeRole"]
  }
}

data "archive_file" "lambda_zip" {
  type        = "zip"
  source_dir  = "../lambda_runtime"
  output_path = "lambda_function_payload.zip"
}

resource "aws_iam_role" "lambda_role" {
  name               = local.namespace
  assume_role_policy = data.aws_iam_policy_document.assume_role.json
}

resource "aws_iam_role_policy_attachment" "lambda_execution_role" {
  role       = aws_iam_role.lambda_role.name
  policy_arn = "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole"
}

resource "aws_lambda_function" "test_lambda" {
  filename         = "lambda_function_payload.zip"
  function_name    = local.namespace
  role             = aws_iam_role.lambda_role.arn
  handler          = "bootstrap.main"
  source_code_hash = data.archive_file.lambda_zip.output_base64sha256
  runtime          = "provided.al2023"
}
