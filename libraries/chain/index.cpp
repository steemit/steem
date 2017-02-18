#include <fc/io/raw.hpp>
#include <steemit/chain/index.hpp>
#include <steemit/chain/database.hpp>

namespace steemit {
    namespace chain {
        void base_primary_index::save_undo(const object &obj) {
            _db.save_undo(obj);
        }

        void base_primary_index::on_add(const object &obj) {
            _db.save_undo_add(obj);
            for (auto ob : _observers) {
                ob->on_add(obj);
            }
        }

        void base_primary_index::on_remove(const object &obj) {
            _db.save_undo_remove(obj);
            for (auto ob : _observers) {
                ob->on_remove(obj);
            }
        }

        void base_primary_index::on_modify(const object &obj) {
            for (auto ob : _observers) {
                ob->on_modify(obj);
            }
        }
    }
} // steemit::chain
