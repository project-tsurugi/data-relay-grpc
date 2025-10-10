# data-relay-grpc Implementation

This directory contains the data-relay-grpc implementation for data-relay-grpc datastore.

## Directory Structure

```
data-relay-grpc/server/data-relay-grpc/blob_relay/
├── README.md           # This file (directory overview and conventions)
├── proto/              # Protocol Buffer definitions
├── server/             # gRPC server implementations
```


## Naming Conventions

### C++ Namespaces

- `data-relay-grpc::blob_relay::backend` - blob_relay backend implementations
- `data-relay-grpc::blob_relay::proto`   - Protocol Buffer generated code


## Development Guidelines

1. **Protocol Definitions**: Place all `.proto` files in the `proto/` subdirectory
2. **Server Implementation**: Implement gRPC servers in the `server/` subdirectory
4. **Naming**: Follow snake_case for file names and variables, PascalCase for types
5. **Documentation**: Include English comments for all public interfaces
6. **Testing**: Create corresponding test files for each implementation


## Message Versioning

Some data-relay-grpc request/response messages include a `version` field for future compatibility.
The version value to use for each message is defined as a constant in `grpc/message_versions.h`.
Both client and service must check and set the version accordingly.

The initial version number for each message is 1.


## Build Integration

Protocol buffer files are automatically compiled during the build process.
