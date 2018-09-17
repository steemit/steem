#include <appbase/application.hpp>

#include <steem/plugins/transaction_polling_api/transaction_polling_api.hpp>
#include <steem/plugins/transaction_polling_api/transaction_polling_api_plugin.hpp>
#include <steem/chain/transaction_object.hpp>
#include <steem/chain/transaction_status_object.hpp>

namespace steem { namespace plugins { namespace transaction_polling_api {

    using namespace steem::chain;
class transaction_polling_api_impl
{
   public:
      transaction_polling_api_impl();
      ~transaction_polling_api_impl();

      DECLARE_API_IMPL
      (
         (find_transaction)
      )

      chain::database& _db;
};

transaction_polling_api::transaction_polling_api()
   : my( new transaction_polling_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_TRANSACTION_POLLING_API_PLUGIN_NAME );
}

transaction_polling_api::~transaction_polling_api() {}

transaction_polling_api_impl::transaction_polling_api_impl()
   : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}

transaction_polling_api_impl::~transaction_polling_api_impl() {}

DEFINE_API_IMPL( transaction_polling_api_impl, find_transaction )
{
    FC_ASSERT ( !args.trx_id.is_null() || !args.trx_id.get_string().empty(), "Expect transaction id, experation time is optional" );
    
    find_transaction_return result;
    
    chain::transaction_id_type id = args.trx_id.as<transaction_id_type>();
    auto& index = _db.get_index<transaction_index>().indices().get<by_trx_id>();
    bool trx_in_index = index.find(id) != index.end() ? true : false;
    
    fc::variant expiration = args.expiration;
    if (expiration.is_null() || expiration.get_string().empty())
    {
        // Expiration time is not provided, then, return whether the transaction is aware or not
        if (trx_in_index)
            result.trx_status_code = transaction_status_codes::aware_of_trx;
        else
            result.trx_status_code = transaction_status_codes::not_aware_of_trx;
    }
    else
    {
        fc::time_point_sec exp_time = args.expiration.as< fc::time_point_sec >();
        fc::time_point_sec hb_time = _db.head_block_time();
        
        // Get Last irreversible block and head block time
        const auto libb = _db.fetch_block_by_number(_db.get_dynamic_global_properties().last_irreversible_block_num);
        fc::time_point_sec lib_time = libb->timestamp;
        
        const auto itr_pending_tx = std::find_if(_db._pending_tx.begin(), _db._pending_tx.end(),
                                                 [&](const signed_transaction& t) {
                                                     return t.id() == id;
                                                 });
        bool trx_in_mempool = itr_pending_tx != _db._pending_tx.end() ? true : false;
        
        auto& sindex = _db.get_index<transaction_status_index>().indices().get<by_trx_id>();
        auto sitr = sindex.find(id);
        bool trx_in_status_index = sitr != sindex.end() ? true : false;
        
        fc::time_point_sec tx_block_time;
        if (trx_in_status_index) {
            auto trx_in_status_idx_time = _db.fetch_block_by_number(sitr->block_num);
            tx_block_time = trx_in_status_idx_time->timestamp;
        }
        
        //
        // {b[n]...[n+10].....b[n+lib]b[n+lib+1].........b[n+lib+21]..........................}
        // {<------------------------ TaPoS ----------------------->}
        // {<----------------- trx_in_status_index ---------------->}{<--- trx_in_mempool --->}
        // {.....................---------  trx_in_index ------------------------------------>}
        //
        // Transaction status matrix (all the way to first block time in TaPoS window)
        // exp_time > hb_time | trx_in_status_index | trx_in_index | trx_in_mempool | Valid State | Status Code |
        //  Y (Not Expired)   |            N        |       N      |       N        |   Valid     |      2      |
        //  Y (Not Expired)   |            Y        |       Y      |       Y        |   Valid     |      3      |
        //  Y (Not Expired)   |            Y        |       Y      |       N        |   Valid     |    4/5      |
        //     N (Expired)    |            Y        |       N      |       N        |   Valid     |    4/6      |
        //     N (Expired)    |            N        |       N      |       N        |   Valid     |    6/7      |
        //                                                                                                      |
        //  All other conditions are invalid                                                                    |
        //  Y/N (Expired/Not) |            X        |       X      |       X        |  Invalid    |      X      |
        //
        // Status codes:
        // 0 -> not_aware_of_trx, "Not aware of transaction"
        // 1 -> aware_of_trx, "Aware of transaction"
        // 2 -> exp_time_future_trx_not_in_block_or_mempool, "Expiration time in future, transaction not included in block or mempool"
        // 3 -> trx_in_mempool, "Transaction in mempool"
        // 4 -> trx_inclided_in_block_and_block_not_irreversible, "Transaction has been included in block, block not irreversible"
        // 5 -> trx_included_in_block_and_block_irreversible, "Transaction has been included in block, block is irreversible"
        // 6 -> trx_expired_trx_is_not_irreversible_could_be_in_fork, "Transaction has expired, transaction is not irreversible (transaction could be in a fork)"
        // 7 -> trx_expired_trx_is_irreversible_could_be_in_fork, "Transaction has expired, transaction is irreversible (transaction cannot be in a fork)"
        // 8 -> trx_is_too_old, "Transaction is too old, I don't know about it"

        if ((exp_time > hb_time) && !trx_in_status_index && !trx_in_index && !trx_in_mempool)
        {
            result.trx_status_code = transaction_status_codes::exp_time_future_trx_not_in_block_or_mempool;
            return result;
        }
        
        if ((exp_time > hb_time) && trx_in_status_index && trx_in_index && trx_in_mempool)
        {
            result.trx_status_code = transaction_status_codes::trx_in_mempool;
            return result;
        }
        
        if ((exp_time > hb_time) && trx_in_status_index && trx_in_index && !trx_in_mempool)
        {
            if (tx_block_time > lib_time)
                result.trx_status_code = transaction_status_codes::trx_inclided_in_block_and_block_not_irreversible;
            else
                result.trx_status_code = transaction_status_codes::trx_included_in_block_and_block_irreversible;
            
            return result;
        }
        
        if ((exp_time < hb_time) && trx_in_status_index && !trx_in_index && !trx_in_mempool)
        {
            if (tx_block_time > lib_time)
                result.trx_status_code = transaction_status_codes::trx_inclided_in_block_and_block_not_irreversible;
            else
                result.trx_status_code = transaction_status_codes::trx_included_in_block_and_block_irreversible;
            
            return result;
        }
        
        if ((exp_time < hb_time) && !trx_in_status_index && !trx_in_index && !trx_in_mempool)
        {
            if (exp_time > lib_time)
                result.trx_status_code = transaction_status_codes::trx_expired_trx_is_not_irreversible_could_be_in_fork;
            else
            {
                auto& bindex = _db.get_index<transaction_status_index>().indices().get<by_block_num>();
                
                auto block_itr = bindex.begin();
                if ( block_itr != bindex.end() )
                {
                    auto first_block_num = block_itr->block_num;
                    fc::time_point_sec block_time;
                    auto trx_in_status_idx_time = _db.fetch_block_by_number(first_block_num);
                    block_time = trx_in_status_idx_time->timestamp;
                
                    if (exp_time > block_time)
                        result.trx_status_code = transaction_status_codes::trx_expired_trx_is_irreversible_could_be_in_fork;
                    else
                        result.trx_status_code = transaction_status_codes::trx_is_too_old;
                }
                
                result.trx_status_code = transaction_status_codes::trx_expired_trx_is_irreversible_could_be_in_fork;
                return result;
            }
        }
    }
    
    return result;
}

DEFINE_READ_APIS( transaction_polling_api,
   (find_transaction)
)

} } } // steem::plugins::transaction_polling_api
