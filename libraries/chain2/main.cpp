#include <graphene/db2/database.hpp>
#include <steemit/chain2/chain_database.hpp>
#include <steemit/chain2/block_objects.hpp>
#include <fc/interprocess/container.hpp>

#include <fc/io/json.hpp>

using namespace steemit::chain2;


int main(int argc, char **argv) {

    try {
        steemit::chain2::database db;
        db.open(".");

        idump((db.head_block_num()));

        signed_block genesis;
        genesis.witness = "dantheman";

        if (db.head_block_num()) {
            genesis.previous = db.head_block().block_id;
            genesis.timestamp = db.head_block().timestamp + fc::seconds(3);
        } else {
            apply(db, genesis);
            return 0;
        }

        idump((genesis));
        db.push_block(genesis);
        idump((db.revision()));

        const auto &head = db.head_block();
        auto packed_block = fc::raw::pack(genesis);
        auto packed_db_obj = fc::raw::pack(head);

        idump((packed_db_obj)(packed_db_obj.size()));

        db.modify(head, [&](block_object &b) {
            fc::datastream<const char *> ds(packed_db_obj.data(), packed_db_obj.size());
            fc::raw::unpack(ds, b);
        });
        packed_db_obj = fc::raw::pack(head);
        idump((packed_db_obj));
        idump((head));
        auto js = fc::json::to_string(head);
        idump((js)(js.size()));
        db.modify(head, [&](block_object &b) {
            auto var = fc::json::from_string(js);
            var.as(b);
        });
        js = fc::json::to_string(head);
        idump((js)(js.size()));

        db.export_to_directory("export_db");

        fc::temp_directory temp_dir(".");
        steemit::chain2::database import_db;
        import_db.open(temp_dir.path());
        import_db.import_from_directory("export_db");
        idump((import_db.head_block()));


    } catch (const fc::exception &e) {
        edump((e.to_detail_string()));
    }

    return 0;
}
