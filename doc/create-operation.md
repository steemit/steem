
This is developer documentation for creating brand-new operations on the STEEM blockchain.

- (1) Define `smt_elevate_account_operation` structure in `smt_operations.hpp`
- (2) Create `FC_REFLECT` definition for the operation struct.
- (3) Implement `validate()` for the operation struct.
- (4) Add operation to `steem::protocol::operation`
- (5) Define evaluator for the operation.
- (6) Define required authorities for the operation.
- (7) Define unit tests for the operation.
- (8) Modify existing RPC objects for the operation.
- (9) Create new database object(s) / index(es) for the operation
- (10) Use new database object(s) / index(es)
- TODO (11) Define new RPC's / objects / indexes for the operation in the relevant plugin
- TODO (12) Use RPC's to implement `cli_wallet` functionality for the operation
- TODO Example of handling autonomous processing

## Step 1

- (1a) Newly defined operations should include a member `extensions_type extensions;`.
This definition adds a reserved zero byte to the binary serialization, allowing
"opcode space" to add fields in the future.

- (1b) Operations should directly subclass `base_operation` or `virtual_operation`.
Operations should not subclass any other operation class.  `base_operation` is
for operations that are intended to be issued by a user.  `virtual_operation` is
a non-consensus entity and can never be issued by a user; virtual operations
record automatic actions for inclusion in the user's account history (for example
receiving post rewards or filling market orders).

- (1c) Operation type must have a default constructor to be compatible with FC
serialization.  Most operations only implement default constructor.

## Step 2

- (2a) The `FC_REFLECT` statement includes the fully qualified name of the class, and
a bubble list of fields.  `FC_REFLECT` is used for various preprocessor-based
type introspection tasks, including autogeneration of serialization/deserialization
code for binary and JSON formats.

- (2b) If you forget to put a field in `FC_REFLECT` definition, it will compile, but
will have a bug (the field will not be serialized / deserialized).  In production,
this kind of bug causes potentially disastrous state corruption, and is not always
noticeable at first.  Our CI server runs an automated script `check_reflect.py` to
scan the code for forgotten fields, and fails the build if any are found.

## Step 3

- (3a) The `validate()` method uses `FC_ASSERT` macro, which will throw
`fc::exception` if not true.  `validate()` is responsible to check correctness
conditions using the operation struct alone, it has no access to the
chain state database.  Checks which require chain state belong in `do_apply()`,
not `validate()`.

- (3b) Examples of checks that should go in `validate()`:  Strings do not
contain illegal characters; integer values are of correct sign and within
compile-time lower/upper bounds; variable-length elements are within
compile-time minimum/maximum size limits; ordering relationships between
fields (e.g. `field_1 >= field_2`); an asset name is in a compile-time
set of allowed asset names.

- (3c) Examples of checks that cannot go in `validate()` are:  An account
must (or must not) exist; an account must have sufficient balance; a
particular operation or event must (or must not) have occurred in the past;
a particular date / block number / hardfork number has (or has not) occurred;
any reference to the contents of any operation other than `*this`.  All such
checks must go in the evaluator.

## Step 4

- (4a) The file `operations.hpp` defines the `steem::protocol::operation`
type, which is an `fc::static_variant` with a lengthy parameter list.  (The
`fc::static_variant` implements a
[tagged union type](https://en.wikipedia.org/wiki/Tagged_union) which uses
C++11 [template parameter pack](http://en.cppreference.com/w/cpp/language/parameter_pack)
to specify the list of potential element types).  The order in which
operations are specified is important, adding a new type will cause the
following types to have different integer ID's, which changes binary
serialization and breaks backward compatibility.

- (4b) New non-virtual operations are usually added just before the beginning
of virtual operations.

- (4c) New virtual operations are usually added at the end.

## Step 5

- (5a) You must add `STEEM_DEFINE_EVALUATOR` macro in `evaluator.hpp` to
generate some boilerplate code.  The macro is defined `evaluator.hpp`,
most of the generated code is support code required by the framework and
does not affect the operation itself.

- (5b) The actual code to execute the operation is written in a method called
`opname_evaluator::do_apply( const opname_operation& o )`.  This code has
access to the database in the `_db` class member.

- (5c) All new operations, and all new operation functionality, must be gated
by checking `_db.has_hardfork( ... )` for feature constants defined in
`libraries/chain/hardfork.d`.

- (5d) For this example, we will add a boolean field to the `account_object`,
later we will see how to adapt this field to different objects.

- (5e) Methods which get objects from the database return `const` references.
The most general way to retrieve chain state is with
`get< type, index >( index_value )`, for example see the
[implementation](https://github.com/steemit/steem/blob/a0a69b10189d93a9be4da7e7fd9d5358af956e34/libraries/chain/database.cpp#L364)
of `get_account(name)`.

- (5f) To modify an object on the database, you must issue `db.modify()`,
which takes as arguments the `const` reference, and a callback
(usually specified as a [lambda](http://en.cppreference.com/w/cpp/language/lambda) )
which actually modifies the fields.  (The callback mechanism is used because the
`chainbase` blockchain database framework needs to do bookkeeping before and after
any mutation of state.)

- (5g) The evaluator must be registered in `database::initialize_evaluators()`.

## Step 6

- (6a) Required authorities are implemented by
`get_required_active_authorities()`,
`get_required_posting_authorities()`,
`get_required_owner_authorities()` which modify a caller-provided
`flat_set`.  These methods are usually inline.

- (6b) The operation's fields alone must provide enough information
to compute the required authorities.  As a result of this requirement,
when designing new operations, sometimes it becomes necessary to add
a field which explicitly replicates information already available
in the database.

## Step 7

- (7a) Unit tests for SMT functionality should go in `smt_tests.cpp`.

- (7b) Operations should have separate tests:  `validate` test,
`authorities` test and `apply` test.

- (7c) If unit tests pass, but a bug or a spec change is discovered,
in addition to fixing the bug, code to demonstrate the bug should
be added to the unit test.  The unit test should now fail without the
bugfix code, and pass with the bugfix code.

## Step 8

- (8a) Objects in libraries/chain are part of blockchain core, what's used for consensus
- (8b) When adding fields to blockchain core object, should add the same field to RPC objects
to make it available to JSON clients
- (8c) Add field definition and reflection
- (8d) Initialize RPC object field from DB object field in RPC object constructor

## Step 9

- (9a) Add `smt_token_object_type` to `enum object_type` in `steem_objects.hpp` and add
to `FC_REFLECT_ENUM` bubble list at the bottom of that file
- (9b) Declare (but do not define) `class smt_token_object;` in `steem_objects.hpp`
- (9c) Define `typedef oid< smt_token_object > smt_token_id_type` in `steem_objects.hpp`
- (9d) Create object header file (one header file per object) in `smt_objects` directory.
Include the new header from `smt_objects.hpp`.
- (9e) All SMT objects are consensus, and therefore should exist in `steem::chain` namespace
- (9f) The object class should subclass `object< smt_token_object_type, smt_token_object >`
- (9g) The constructor should be defined the same as other object classes, with
`Constructor` and `Allocator` template parameters.
- (9h) Any variable-length fields require special attention.  Such fields must be
an interprocess compatible type and must have an `allocator` forwarded from the constructor.
In practice, this means strings should be `shared_string`, and collections (vector, deque)
should be one of the `boost::interprocess` types.  See examples in fields
`transaction_object::packed_trx`, `comment_object::permlink`, and
`feed_history_object::price_history`.
- (9i) The first field should be `id_type id;`
- (9j) Most fields should be default initialized, or set to zero, empty or compile-time
default values.  Usually, the only field initialization done in the class constructor is
passing the allocator, setting integer fields to zero, and executing the caller-provided
`Constructor` callback.  The "real" initialization is performed by that callback, which
will have access to necessary external information (from `database` and the operation).
- (9k) All fields must be passed to `FC_REFLECT` in a bubble list
- (9l) `struct` definitions for any index other than the default `by_id` should follow the class;
`by_id` should *never* be defined by an object class.
- (9m) The index type is defined.  A `by_id` index must always be defined.  See notes below
and Boost `multi_index_container` docs for more information on the purpose and syntax of this
definition.
- (9n) Macro
`CHAINBASE_SET_INDEX_TYPE( steem::chain::smt_token_object, steem::chain::smt_token_index )`
must be invoked.  It should be invoked at the global scope (outside any namespace).
- (9o) Call `add_core_index< smt_token_index >(*this);` in `database::initialize_indexes()` to
register the object type with the database.

### Step 9 additional explanation

Step 9 requires some explanation.

- (9a) Each object type has an integer ID signifying that object type.  These type ID's are
defined by an `enum` in `steem_object_types.hpp`, any new objects must be added here.  In SQL
terms, if we imagine each *database table* has an integer ID, then `smt_token_object_type` is
the ID value that refers to the `smt_token_object` *table*.

- (9c) While object ID's are all integers, at compile time a "note" is attached to each
variable of ID type, which notes the table the ID refers to.  This is implemented by the
`chainbase::oid` class, which takes the class name as a template parameter.  To cut down
on the number of template invocations needed in typical code (and to ease porting of
code first developed with older versions of `chainbase` or its predecessors), a type
alias `typedef oid< smt_token_object > smt_id_type` is added to `steem_object_types.hpp`.

- (9f) The `smt_token_object` class subclasses
`chainbase::object< smt_token_object_type, smt_token_object >`.  This is the
[curiously recurring template pattern](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern),
it involves routing of type parameters to allow interaction of templates and polymorphism.  It sounds
complicated, but the `object` class definition in `chainbase.hpp` makes clear that it is used to simply
`typedef oid< smt_token_object > id_type;`

As a consequence of the above, `smt_token_id_type`, `oid< smt_token_object >` and
`smt_token_object::id_type` all refer to a type representing an integer object ID, with a
compile-time "note" attached that this object ID is a primary key for the `smt_token_object`
database table.  The table ID of that database table is an integer value given by
`smt_token_object_type` or `smt_token_object::type_id`.

- (9h) We're using a memory-mapped file for the database, when the database allocates memory it
must go into the memory-mapped file (not on the heap).  Any variable-length fields in any
database object must have the allocator passed-in, and must be compatible with multiprocess
access.  In practice this means using `boost::interprocess::vector` and `shared_string`.
Examples may be seen with fields `transaction_object::packed_trx`, `comment_object::permlink`, and
`feed_history_object::price_history`.

- (9i) Each individual object has an integer object ID field called `id` which acts as a
"primary key" for that object.  This is required for all objects, as it is used by the
`chainbase` database framework to implement the undo function.  Each object that is
created will be assigned the next sequentially available object ID.

- (9l) Some functionality in `chainbase` requires the `by_id` field.  Since `chainbase`
is designed as a reusable library, not tightly coupled to Steem, it contains no reference
to any `steem` namespaces.  So the name `by_id` must refer to `chainbase::by_id`.  If you
define a `struct by_id;` in the `steem::chain` namespace, the result will be that every
index defined later in the compilation unit which references `by_id` without qualification
will become an incorrect or ambiguous type reference.  The result likely will not
function correctly, and may not compile.

- (9m) The template definition of `smt_token_index` looks intimidating (and the definitions of
indexes like `control_account` moreso).  The indexes are used by the
`multi_index_container` type provided by the Boost library.  The `multi_index_container` library
roughly implements the functionality of an in-memory SQL database (a collection of objects for
which multiple "keys" may be defined for potential random access / iteration, and also supports
"composite keys" consisting of multiple fields).  Since all type information is known about all
"database tables" is known at compile time, the `multi_index_container` is fast and efficient.
So the `smt_token_index` definition is basically a DDL which describes the available indexes --
it is like an SQL `CREATE INDEX` statement with more complicated syntax.  Most of the time,
the index definition can be treated as "boilerplate" that can be copied from existing, working
definitions.  More information about the syntax is available in the Boost documentation.

- (9m) The `by_id` index is used by the `chainbase` infrastructure to implement the undo function.

- (9m) All indexes used in Steem must be `ordered_unique`.  In theory, hashed or non-unique
indexes may be permissible in some situations, and may offer a performance advantage.
However, past experience has shown that the undefined iteration order of these indexes
is a potential source of state corruption bugs (in practice, iteration order of such an
index depends on the specific point in time at which a node starts, and the specific
order in which blocks and transactions are received).

## Step 10

- (10a) You may use `database::create()`, `database::modify()` and `database::remove()`
in evaluators or during per-block processing (these methods are actually implemented in
`chainbase`, the `database` class is derived).
- (10b) The `create()` and `modify()` functions take a callback which should fill in the
new field values for the object.
- (10c) The `modify()` and `remove()` functions take a reference to an existing object.
- (10d) To get a reference to an object, you can use `db.get()` to lookup the object
by index value.  For example
`db.get< smt_token_object, by_control_account >( "alice" );` which throws an exception
if the object does not exist.  In an evaluator, such an exception will cause the operation
(and any transaction or block that contains it) to fail.
- (10e) To get an object that potentially does not exist, you can use `db.find()`, which
is like `db.get()` but it will return a pointer, or `nullptr` if the object does not exist.
- (10f) To iterate over objects in the order of some index, you can call
`auto& idx = get_index< smt_token_index >().indices().get< by_control_account >()` to get a reference
to the `multi_index_container` index, then use `begin()`, `end()` for a traversal from
the end, or use `lower_bound()`, `upper_bound()` for traversal beginning at an arbitrary
point.  Many examples exist in the code and `multi_index_container` documentation.
Beware of concurrent modification!
- (10g) To pass a composite key to `db.get()`, `db.find()` or any of the above index
iterator functions, use `boost::make_tuple()`.
- (10h) If you have an object and you want to start iterating at its location, use
`idx.iterator_to()`.  This technique is used by RPC methods implementing paginated queries.
- (10i) Object lookup methods will generally return a `const` reference / pointer.  The
iteration tree structures in `multi_index_container` and the undo fucntionality in
`chainbase` implement some "pre-" and "post-" bookkeeping for every modification.  Using
`const` references for lookups allows the compiler to enforce that modification only
occurs through the `db.create()`, `db.modify()` and `db.remove()` methods which do
the proper bookkeeping.  The non-`const` reference which allows modification of the object
only exists inside the body of the callback where the modification occurs.

### Step 10 additional explanation

- (10) The following discussion is specific to the `smt_token_object` example code, and
isn't directly applicable to other objects.

For `smt_token_index` two single-column indexes are defined, `by_id`
on the `id` field, and `by_control_account` on the `control_account` field.

Now that an object is created for each SMT, the `is_smt` field in the database describing
whether an object has an associated SMT is redundant information and can be removed.  We want to
continue to provide the same interface to JSON-RPC clients, so we keep the `is_smt` field in the RPC
object.  But we now fill it in based on whether an `smt_token_object` exists with a matching
`control_account`.

Also, we delete the `is_smt` check in the evaluator.  But the unit test code which tests that
an account cannot be elevated twice still passes.  The reason it passes is that object
creation will throw an exception if the created object has the same key value as an existing
object for any unique index.  So the second account creation will fail due to the exception
from the violated uniqueness constraint on the `control_account` field.
