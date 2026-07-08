# Error Codes And Warnings

Release: `v1.17.1-alpha`

Stable JSON `status_code` values:

| Enum | String |
| --- | --- |
| `OK` | `ADASDF_OK` |
| `NO_COLLISION` | `ADASDF_NO_COLLISION` |
| `NO_CANDIDATE` | `ADASDF_NO_CANDIDATE` |
| `OUT_OF_DOMAIN` | `ADASDF_OUT_OF_DOMAIN` |
| `INVALID_MODEL` | `ADASDF_INVALID_MODEL` |
| `INVALID_ARGUMENT` | `ADASDF_INVALID_ARGUMENT` |
| `IO_ERROR` | `ADASDF_IO_ERROR` |
| `TIMEOUT` | `ADASDF_TIMEOUT` |
| `UNSUPPORTED_BACKEND` | `ADASDF_UNSUPPORTED_BACKEND` |
| `CUDA_UNAVAILABLE` | `ADASDF_CUDA_UNAVAILABLE` |
| `SCHEMA_VALIDATION_FAILED` | `ADASDF_SCHEMA_VALIDATION_FAILED` |
| `INTERNAL_ERROR` | `ADASDF_INTERNAL_ERROR` |

No-collision and no-candidate are successful executions when the query itself
completed correctly. They are not crashes and should not be treated as process
failures by automation.

Warnings are arrays of `{code, message}` objects and may be empty.
