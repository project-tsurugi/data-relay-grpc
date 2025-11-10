comp: client

proto=blob_relay/blob_reference.grpc.pb.cc \
blob_relay/blob_relay_streaming.grpc.pb.cc \
blob_relay/blob_reference.pb.cc \
blob_relay/blob_relay_streaming.pb.cc \
tateyama/proto/auth/request.pb.cc \
tateyama/proto/auth/common.pb.cc \
tateyama/proto/auth/response.pb.cc \
tateyama/proto/request/request.pb.cc \
tateyama/proto/request/diagnostics.pb.cc \
tateyama/proto/request/response.pb.cc \
tateyama/proto/core/request.pb.cc \
tateyama/proto/core/response.pb.cc \
tateyama/proto/diagnostics.pb.cc \
tateyama/proto/endpoint/request.pb.cc \
tateyama/proto/endpoint/response.pb.cc \
tateyama/proto/datastore/request.pb.cc \
tateyama/proto/datastore/common.pb.cc \
tateyama/proto/datastore/response.pb.cc \
tateyama/proto/session/request.pb.cc \
tateyama/proto/session/response.pb.cc \
tateyama/proto/session/diagnostic.pb.cc \
tateyama/proto/metrics/request.pb.cc \
tateyama/proto/metrics/response.pb.cc \
tateyama/proto/framework/request.pb.cc \
tateyama/proto/framework/common.pb.cc \
tateyama/proto/framework/response.pb.cc \
tateyama/proto/debug/request.grpc.pb.cc \
tateyama/proto/debug/request.pb.cc \
tateyama/proto/debug/response.grpc.pb.cc \
tateyama/proto/debug/response.pb.cc

client: main.cpp client.h session.h
	c++ -o client main.cpp tateyama/utils/protobuf_utils.cpp ${proto} -I .  -l gflags -l grpc++ -l gpr -l protobuf -l absl_synchronization

clean:
	rm client
