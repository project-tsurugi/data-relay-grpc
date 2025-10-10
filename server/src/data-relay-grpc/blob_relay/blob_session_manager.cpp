#include <data-relay-grpc/blob_relay/blob_session_manager.h>

namespace data_relay_grpc::blob_relay {

blob_session_manager::blob_session_manager() = default;

blob_session& blob_session_manager::get_session(blob_session::session_id_type session_id) {
    return *blob_sessions_.at(session_id);
}

blob_session::blob_tag_type blob_session_manager::get_tag(blob_session::transaction_id_type, blob_session::blob_id_type) {
    return 0; // FIXME
}

blob_session::blob_path_type blob_session_manager::get_path(blob_session::blob_id_type) {
    return std::filesystem::path("/tmp/");  // FIXME
}

} // namespace
