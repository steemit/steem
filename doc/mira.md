# Basic MIRA configuration

MIRA has many options that allow the user to improve performance. For most use cases, the default options will be sufficient. However, one consideration when configuring MIRA is resource limiting.

MIRA uses a multi-tiered architecture for retrieving and storing data from its databases. Each blockchain index is considered its own distinct database. At a very high level, the tiers can be thought of as follows:

| Database Tier       |   Location    |  Reading | Writing |
|---------------------|---------------|:--------:|:-------:|
| Object cache        |   Main memory |  &#9745; | &#9745; |
| Global shared cache |   Main memory |  &#9745; | &#9744; |
| Gobal write buffer  |   Main memory |  &#9745; | &#9745; |
| Tier 0 file based   |   Disk        |  &#9745; | &#9745; |
| Tier 1 file based   |   Disk        |  &#9745; | &#9745; |
| ...                 |   Disk        |  &#9745; | &#9745; |

Below is an example of MIRAs default configuration that is designed for a 16GiB node.

```
{
  "global": {
    "shared_cache": {
      "capacity": "5368709120"
    },
    "write_buffer_manager": {
      "write_buffer_size": "1073741824"
    },
    "object_count": 62500,
    "statistics": false
  },
  "base": {
    "optimize_level_style_compaction": true,
    "increase_parallelism": true,
    "block_based_table_options": {
      "block_size": 8192,
      "cache_index_and_filter_blocks": true,
      "bloom_filter_policy": {
        "bits_per_key": 10,
        "use_block_based_builder": false
      }
    }
  }
}
```

Lets break down the most important options with regards to limiting main memory usage.

---

## Object cache

The object cache determines how many database objects MIRA has direct access to. When the application logic accesses the database for reading or writing, it does so through an object within the object cache. Having objects in the object cache circumvents the need to make database accesses to the underlying tiers; this is the most performant layer in the MIRA architecture. Since it is not possible to set the object cache limitations by memory, this layer must be configured according to its heaviest possible usage. The object cache is shared amongst all blockchain index databases. The largest object can be 64KiB, which correlates to the maximum size of one block in the chain. Calculate the heaviest possible usage with the formula `object_count * 64KiB`. Using the default configuration as an example, `62500 * 64 * 1024 = 4096000000` or simply `62500 * 64KiB = 4GiB`.

```
...
      "write_buffer_size": "1073741824"
    },
    "object_count": 62500, <-- Set the object count to limit the object cache usage
    "statistics": false
  },
  "base": {
...
```

> *__Note:__* *The object count is defined as an unsigned integer in the configuration.*

---

## Global shared cache

The global shared cache is a section of main memory that will contain data blocks used for the retrieval of data. It is recommended that the user set its capacity to about 1/3 of their total memory budget. The global shared cache is shared between all blockchain index databases. Using the default configuration as an example, `5368709120 = 5GiB`.

```
{
  "global": {
    "shared_cache": {
      "capacity": "5368709120" <-- Set the capacity to limit global shared cache usage
    },
...
```

> *__Note:__* *The global shared cache capacity is defined as a string in the configuration.*

---

## Global write buffer

The global write buffer is used for performant writes to the database. It lives in main memory and also contains the latest changes to a particular database object. Not only is it used for writing, but also for reading. It is important to ensure there is enough capacity allocated to keep up with the live chain. The global write buffer is contained *within* the global shared cache. Using the default configuration as an example, `1073741824 = 1GiB`.

```
...
    "write_buffer_manager": {
      "write_buffer_size": "1073741824" <-- Set the write buffer size to limit the write buffer usage
    },
...
```

> *__Note:__* *The global write buffer size is defined as a string in the configuration.*

---

## Application memory

When configuring MIRA it is important to consider the normal memory usage of `steemd`. Regardless of the MIRA configuration, `steemd` will tend to use roughly 5.5GiB of memory.

---

## Examples

### Example 1.

Example 1 is appropriate for a 16GiB node.

Using the default configuration:

1. Object count: `62500`
2. Global shared cache: `5368709120` or 5GiB
3. Global write buffer: `1073741824` or 1GiB
4. Approximate application memory usage: `5905580032` or 5.5GiB

Let us calculate the largest total memory usage of MIRA. `(62500 * 64KiB) + 5GiB + 5.5GiB = ~14.5GiB`

> *__Note:__* *The global write buffer size is not included in the total memory calculation for MIRA because it is contained within the global shared cache.*

### Example 2.

Example 2 is appropriate for a 32GiB node.

1. Object count: `125000`
2. Global shared cache: `10737418240` or 10GiB
3. Global write buffer: `2147483648` or 2GiB
4. Approximate application memory usage: `5905580032` or 5.5GiB

Let us calculate the largest total memory usage of MIRA. `(125000 * 64KiB) + 10GiB + 5.5GiB = ~23.5GiB`

> *__Note:__* *The global write buffer size is not included in the total memory calculation for MIRA because it is contained within the global shared cache.*

---

## Notes

The optimal configuration for any one specific piece of hardware will vary. This guide is not meant to be definitive, but a baseline to configure MIRA and get up and running.