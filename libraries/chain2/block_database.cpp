#include <steemit/chain2/block_database.hpp>
#include <fstream>
#include <fc/io/raw.hpp>


namespace steemit {
    namespace chain2 {

        namespace detail {
            class block_database_impl {
            public:
                signed_block head;
                std::fstream out_blocks;
                std::fstream in_blocks;
            };
        }

        block_database::block_database()
                : my(new detail::block_database_impl()) {
        }

        block_database::~block_database() {
        }

        void block_database::open(const fc::path &file) {
            my->out_blocks.close();
            my->in_blocks.close();
            my->out_blocks.open(file.generic_string().c_str(),
                    std::ios::app | std::ios::binary);
            my->in_blocks.open(file.generic_string().c_str(),
                    std::ios::in | std::ios::binary);
            if (fc::file_size(file) > 8) {
                my->head = head();
            }
        }

        uint64_t block_database::append(const signed_block &b) {
            uint64_t pos = my->out_blocks.tellp();
            fc::raw::pack(my->out_blocks, b);
            my->out_blocks.write((char *)&pos, sizeof(pos));
            my->out_blocks.flush();
            my->head = b;
            return pos;
        }

        signed_block block_database::read_block(uint64_t pos) const {
            my->in_blocks.seekg(pos);
            signed_block result;
            fc::raw::unpack(my->in_blocks, result);
            return result;
        }

        signed_block block_database::head() const {
            uint64_t pos;
            my->in_blocks.seekg(-sizeof(pos), std::ios::end);
            my->in_blocks.read((char *)&pos, sizeof(pos));
            return read_block(pos);
        }

    }
}
