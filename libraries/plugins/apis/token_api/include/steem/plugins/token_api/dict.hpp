#include<string>
#include<list>
#include<unordered_map>

#include <steem/chain/database.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace steem { namespace plugins { namespace token_api {

using boost::multi_index_container;
using namespace boost::multi_index;

namespace types
{
   using key_type = uint32_t;
   using val_type = std::string;

   using key_collection = std::list< key_type >;
   using val_collection = std::list< val_type >;
}

struct content
{
   types::val_collection vals;
   types::key_collection keys;

   using pcontent = std::shared_ptr< content >;
   virtual void set_file_name( const types::val_type& val )
   {
      //Nothing to do.
   }
   virtual const types::val_type get_file_name() const
   {
      return "";
   };
};

struct file_content: public content
{
   types::val_type file_name;
   void set_file_name( const types::val_type& val ) override
   {
      file_name = val;
   }
   const types::val_type get_file_name() const override
   {
      return file_name;
   };
};

struct item
{
   types::val_type val;
   types::key_type id;

   item( types::val_type& _val, types::key_type _id ): val( _val ), id( _id )
   {

   }
};

struct by_item_id;
struct by_item_val;
using items = multi_index_container
<
   item,
   indexed_by
   <
      ordered_unique< tag< by_item_id >, member< item, types::key_type, &item::id > >,
      ordered_unique< tag< by_item_val >, member< item, types::val_type, &item::val > >
   >
>;

class catcher
{
   private:

      using pcontents = std::list< content::pcontent >;

   protected:

      types::key_type key_counter;
      types::key_type val_counter;
      types::key_type total_size;

      items dict;

      pcontents posts;

      void fill_dict( const std::string& line, content::pcontent& post );

   public:

      catcher();

      virtual void get_content() = 0;
      virtual void save_content(){ /*Nothing to do.*/ };

      virtual void scan();

      void summary();
};

class file_catcher: public catcher
{
   private:

      const std::string summary_file = "summary.txt";
      const std::string directory;

      void read_files();
      void save_files();

   protected:
   public:

      file_catcher( const std::string _directory );
      void get_content() override;
      void save_content() override;
};

class db_catcher: public catcher
{
   private:

      chain::database* db;

      void read_db();

   protected:
   public:

      db_catcher( chain::database* _db );
      void get_content() override;
      void scan() override;
};

class tokenizer
{
   private:

      catcher& c;

   protected:
   public:
   
      tokenizer( catcher& _c );
      void preprocess();
      void save_content();
      void summary();
};

} } } // steem::plugins::token_api

