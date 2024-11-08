# Prerequisites:
# A terraform state bucket with the name "terraform-state"

terraform {
  backend "s3" {
    bucket = "terraform-state"
    key    = "lambda_c_runtime"
  }
}

locals {
  namespace   = "lambda_c_runtime"
}
