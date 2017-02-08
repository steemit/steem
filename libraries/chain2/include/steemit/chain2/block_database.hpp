#pragma once

#include <fc/filesystem.hpp>
#include <steemit/protocol/block.hpp>

namespace steemit {
    namespace chain2 {

        using namespace steemit::protocol;

        namespace detail { class block_database_impl; }

        class block_database {
        public:
            block_database();

            ~block_database();

            void open(const fc::path &file);

            uint64_t append(const signed_block &b);

            signed_block read_block(uint64_t file_pos) const;

            signed_block head() const;

        private:
            std::unique_ptr<detail::block_database_impl> my;
        };

    }
}
