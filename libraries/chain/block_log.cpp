#include <algorithm>
#include <fstream>
#include <golos/chain/block_log.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/shared_mutex.hpp>

namespace golos { namespace chain {
    namespace detail {
        using read_write_mutex = boost::shared_mutex;
        using read_lock = boost::shared_lock<read_write_mutex>;
        using write_lock = boost::unique_lock<read_write_mutex>;
        static constexpr boost::iostreams::stream_offset min_valid_file_size = sizeof(uint64_t);

        class block_log_impl {
        public:
            optional<signed_block> head;
            block_id_type head_id;

            std::string block_path;
            std::string index_path;
            boost::iostreams::mapped_file block_mapped_file;
            boost::iostreams::mapped_file index_mapped_file;
            read_write_mutex mutex;

            bool has_block_records() const {
                auto size = block_mapped_file.size();
                return (size > min_valid_file_size);
            }

            bool has_index_records() const {
                auto size = index_mapped_file.size();
                return (size >= min_valid_file_size);
            }

            std::size_t get_mapped_size(const boost::iostreams::mapped_file& mapped_file) const {
                auto size = mapped_file.size();
                if (size < min_valid_file_size) {
                    return 0;
                }
                return size;
            }

            uint64_t get_uint64(const boost::iostreams::mapped_file& mapped_file, std::size_t pos) const {
                uint64_t value;
                FC_ASSERT(get_mapped_size(mapped_file) >= pos + sizeof(value));

                auto* ptr = mapped_file.data() + pos;
                value = *reinterpret_cast<uint64_t*>(ptr);
                return value;
            }

            uint64_t get_last_uint64(const boost::iostreams::mapped_file& mapped_file) const {
                uint64_t value;
                auto size = get_mapped_size(mapped_file);
                FC_ASSERT(size >= sizeof(value));

                auto* ptr = mapped_file.data() + size - sizeof(value);
                value = *reinterpret_cast<uint64_t*>(ptr);
                return value;
            }

            uint64_t get_block_pos(uint32_t block_num) const {
                if (head.valid() &&
                    block_num <= protocol::block_header::num_from_id(head_id) &&
                    block_num > 0
                ) {
                    return get_uint64(index_mapped_file, sizeof(uint64_t) * (block_num - 1));
                }
                return block_log::npos;
            }

            uint64_t read_block(uint64_t pos, signed_block& block) const {
                const auto file_size = get_mapped_size(block_mapped_file);
                FC_ASSERT(file_size > pos);

                const auto* ptr = block_mapped_file.data() + pos;
                const auto available_size = file_size - pos;
                const auto max_block_size = std::min<std::size_t>(available_size, STEEMIT_MAX_BLOCK_SIZE);

                fc::datastream<const char*> ds(ptr, max_block_size);
                fc::raw::unpack(ds, block);

                const auto end_pos = pos + ds.tellp();
                FC_ASSERT(get_uint64(block_mapped_file, end_pos) == pos);

                return end_pos + sizeof(uint64_t);
            }

            signed_block read_head() const {
                auto pos = get_last_uint64(block_mapped_file);
                signed_block block;
                read_block(pos, block);
                return block;
            }

            void create_nonexist_file(const std::string& path) const {
                if (!boost::filesystem::is_regular_file(path) || boost::filesystem::file_size(path) == 0) {
                    std::ofstream stream(path, std::ios::out|std::ios::binary);
                    stream << '\0';
                    stream.close();
                }
            }

            void open_block_mapped_file() {
                create_nonexist_file(block_path);
                block_mapped_file.open(block_path, boost::iostreams::mapped_file::readwrite);
            }

            void open_index_mapped_file() {
                create_nonexist_file(index_path);
                index_mapped_file.open(index_path, boost::iostreams::mapped_file::readwrite);
            }

            void construct_index() {
                ilog("Reconstructing Block Log Index...");
                index_mapped_file.close();
                boost::filesystem::remove_all(index_path);
                open_index_mapped_file();
                index_mapped_file.resize(head->block_num() * sizeof(uint64_t));

                uint64_t pos = 0;
                uint64_t end_pos = get_last_uint64(block_mapped_file);
                auto* idx_ptr = index_mapped_file.data();
                signed_block tmp_block;

                while (pos <= end_pos) {
                    *reinterpret_cast<uint64_t*>(idx_ptr) = pos;
                    pos = read_block(pos, tmp_block);
                    idx_ptr += sizeof(pos);
                }
            }

            void open(const fc::path& file) { try {
                block_mapped_file.close();
                index_mapped_file.close();

                block_path = file.string();
                index_path = boost::filesystem::path(file.string() + ".index").string();

                open_block_mapped_file();
                open_index_mapped_file();

                /* On startup of the block log, there are several states the log file and the index file can be
                 * in relation to each other.
                 *
                 *                          Block Log
                 *                     Exists       Is New
                 *                 +------------+------------+
                 *          Exists |    Check   |   Delete   |
                 *   Index         |    Head    |    Index   |
                 *    File         +------------+------------+
                 *          Is New |   Replay   |     Do     |
                 *                 |    Log     |   Nothing  |
                 *                 +------------+------------+
                 *
                 * Checking the heads of the files has several conditions as well.
                 *  - If they are the same, do nothing.
                 *  - If the index file head is not in the log file, delete the index and replay.
                 *  - If the index file head is in the log, but not up to date, replay from index head.
                 */

                if (has_block_records()) {
                    ilog("Log is nonempty");
                    head = read_head();
                    head_id = head->id();

                    if (has_index_records()) {
                        ilog("Index is nonempty");

                        auto block_pos = get_last_uint64(block_mapped_file);
                        auto index_pos = get_last_uint64(index_mapped_file);

                        if (block_pos != index_pos) {
                            ilog("block_pos != index_pos, close and reopen index_stream");
                            construct_index();
                        }
                    } else {
                        ilog("Index is empty");
                        construct_index();
                    }
                } else if (has_index_records()) {
                    ilog("Index is nonempty, remove and recreate it");
                    index_mapped_file.close();
                    block_mapped_file.close();

                    boost::filesystem::remove_all(block_path);
                    boost::filesystem::remove_all(index_path);

                    open_block_mapped_file();
                    open_index_mapped_file();
                }
            } FC_LOG_AND_RETHROW() }

            uint64_t append(const signed_block& b, const std::vector<char>& data) { try {
                const auto index_pos = get_mapped_size(index_mapped_file);

                FC_ASSERT(
                    index_pos == sizeof(uint64_t) * (b.block_num() - 1),
                    "Append to index file occuring at wrong position.",
                    ("position", index_pos)
                    ("expected", (b.block_num() - 1) * sizeof(uint64_t)));

                uint64_t block_pos = get_mapped_size(block_mapped_file);

                block_mapped_file.resize(block_pos + data.size() + sizeof(block_pos));
                auto* ptr = block_mapped_file.data() + block_pos;
                std::memcpy(ptr, data.data(), data.size());
                ptr += data.size();
                *reinterpret_cast<uint64_t*>(ptr) = block_pos;

                index_mapped_file.resize(index_pos + sizeof(index_pos));
                ptr = index_mapped_file.data() + index_pos;
                *reinterpret_cast<uint64_t*>(ptr) = block_pos;

                head = b;
                head_id = b.id();
                return block_pos;
            } FC_LOG_AND_RETHROW() }

            void close() {
                block_mapped_file.close();
                index_mapped_file.close();
                head.reset();
                head_id = block_id_type();
            }
        };
    }

    block_log::block_log()
            : my(std::make_unique<detail::block_log_impl>()) {
    }

    block_log::~block_log() {
        flush();
    }

    void block_log::open(const fc::path& file) {
        detail::write_lock lock(my->mutex);
        my->open(file);
    }

    void block_log::close() {
        detail::write_lock lock(my->mutex);
        my->close();
    }

    bool block_log::is_open() const {
        detail::read_lock lock(my->mutex);
        return my->block_mapped_file.is_open();
    }

    uint64_t block_log::append(const signed_block& block) { try {
        auto data = fc::raw::pack(block);
        detail::write_lock lock(my->mutex);
        return my->append(block, data);
    } FC_LOG_AND_RETHROW() }

    void block_log::flush() {
        // it isn't needed, because all data is already in page cache
    }

    std::pair<signed_block, uint64_t> block_log::read_block(uint64_t pos) const {
        detail::read_lock lock(my->mutex);
        std::pair<signed_block, uint64_t> result;
        result.second = my->read_block(pos, result.first);
        return result;
    }

    optional<signed_block> block_log::read_block_by_num(uint32_t block_num) const { try {
        detail::read_lock lock(my->mutex);
        optional<signed_block> result;
        uint64_t pos = my->get_block_pos(block_num);
        if (pos != npos) {
            signed_block block;
            my->read_block(pos, block);
            FC_ASSERT(
                block.block_num() == block_num,
                "Wrong block was read from block log (${returned} != ${expected}).",
                ("returned", block.block_num())
                ("expected", block_num));
            result = std::move(block);
        }
        return result;
    } FC_LOG_AND_RETHROW() }

    uint64_t block_log::get_block_pos(uint32_t block_num) const {
        detail::read_lock lock(my->mutex);
        return my->get_block_pos(block_num);
    }

    signed_block block_log::read_head() const {
        detail::read_lock lock(my->mutex);
        return my->read_head();
    }

    const optional<signed_block>& block_log::head() const {
        detail::read_lock lock(my->mutex);
        return my->head;
    }
} } // golos::chain
