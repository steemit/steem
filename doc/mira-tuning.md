# Preface

After MIRAs initial development efforts we released the [Basic MIRA Configuration Guide](https://github.com/steemit/steem/blob/master/doc/mira.md) to help bootstrap users attempting to use MIRA enabled `steemd`. There is actually much more fine tuning that can be done to improve MIRA's performance and I'd like to take the time now to share it with the community. We will break up this process into three phases:

* Phase 1: Gathering statistics
* Phase 2: Analyzing statistics
* Phase 3: Applying performance recommendations

# Phase 1: Gathering statistics

As you may have noticed, within the `database.cfg` file, there is a global option called `statistics`. By default this is set to `false`. This must be set to `true` before proceeding! Here is an example of a `database.cfg` with statistics enabled:

```
$ cat ~/.steemd/database.cfg 
{
  "global": {
    "shared_cache": {
      "capacity": "5368709120"
    },
    "write_buffer_manager": {
      "write_buffer_size": "1073741824"
    },
    "object_count": 62500,
    "statistics": true
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

Once statistics has been enabled, simply perform the action you'd like to optimize. In my example, I will be syncing up the testnet. Start `steemd` like you otherwise normally would. Please be aware that enabling statistics causes a drastic performance impact - you won't want to run this in production. By default, statistics are dumped every 10 minutes so you will want to run for a while. The more data you gather, the more accurate the performance tuning suggestions will potentially be.

# Phase 2: Analyzing statistics

Luckily, you won't need intimate knowledge of RocksDB in order to analyze the statistics data. The developers working on RocksDB have provided us with a tool that can read the gathered statistics and make performance tuning recommendations. This tool can be found within the `steemd` repository at `programs/util/rocksdb_advisor.sh`. From the `program/util` directory run the tool:

```
$ sh rocksdb_advisor.sh

```

If all goes well, you should get output for each object specified in the `rocksdb_advisor.sh` file. Here is an example of the possible output:

```
Advisor for account_authority...
WARNING(TimeSeriesData) check_and_trigger: float division by zero

Rule: bloom-not-enabled
TimeSeriesCondition: bloom-not-enabled statistics: ['[]rocksdb.bloom.filter.useful.count', '[]rocksdb.bloom.filter.full.positive.count', '[]rocksdb.bloom.filter.full.true.positive.count'] behavior: evaluate_expression expression: keys[0]+keys[1]+keys[2]==0 aggregation_op: avg trigger: {'ENTITY_PLACEHOLDER': [0.0, 0.0, 0.0]}
Suggestion: inc-bloom-bits-per-key option : bloom_bits action : increase suggested_values : ['2']
scope: entities:
{'ENTITY_PLACEHOLDER'}
scope: col_fam:
{'boost\\:\\:mpl\\:\\:v_item<steem\\:\\:chain\\:\\:by_id, boost\\:\\:mpl\\:\\:vector0<mpl_\\:\\:na>, 0>', 'boost\\:\\:mpl\\:\\:v_item<steem\\:\\:chain\\:\\:by_last_owner_update, boost\\:\\:mpl\\:\\:vector0<mpl_\\:\\:na>, 0>', 'boost\\:\\:mpl\\:\\:v_item<steem\\:\\:chain\\:\\:by_account, boost\\:\\:mpl\\:\\:vector0<mpl_\\:\\:na>, 0>', 'default'}
```

In reality you will get significantly more output than above. For the sake of simplicity, we will work with one performance suggestion. We can see here the `rocksdb_advisor.sh` provided a suggestion for the `account_authority_object` database.

> Suggestion: inc-bloom-bits-per-key option : bloom_bits action : increase suggested_values : ['2']

Let's move on to applying the advisor's suggestions.

# Phase 3: Applying performance recommendations

If you want to apply the same options to all databases, you would just change the `base` setting as this is applied to every database within a MIRA enabled `steemd` node.

You may notice that you will get different recommendations for different objects. In MIRA's implementation, each object is its own RocksDB database. How do we implement different options for different databases?

## Configuration overlays

A configuration overlay is a set of options overriding the base configuration to be applied to a specified database. In our default configuration, you may notice that one of the objects is called `base`. These settings are applied to every database unless a *configuration overlay* overrides them. A configuration overlay takes the same options as `base`. As an example, we will override `bits_per_key` for the `account_authority_object`.

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
    "statistics": true
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
  },
  "account_authority_object": {
    "block_based_table_options": {
      "block_size": 8192,
      "cache_index_and_filter_blocks": true,
      "bloom_filter_policy": {
        "bits_per_key": 12,
        "use_block_based_builder": false
      }
    }
  }
}
```

> **Note:** When overriding a configuration value, you must override the complete first level option (such as `block_based_table_options` in the above example).

Even though we did not specify `optimize_level_style_compaction` and `increase_parallelism` to the `account_authority_object` configuration, they are inherited from `base`.

## Available options

Not every RocksDB option is made available to MIRA configurations. It is very possible that the RocksDB tool can recommend changing an option that is unavailable through MIRA. Feel free to add it and create a pull request, especially if it is improving your nodes performance. You can see a complete list of available options in the codebase in [libraries/mira/src/configuration.cpp](https://github.com/steemit/steem/blob/master/libraries/mira/src/configuration.cpp). View the recommended options and check the list; I tried to preserve the naming conventions during implementation to make this process easier.

# Conclusion

You may need to repeat this process to achieve optimal results. There is no guarantee that you will see performance improvements as this is experimental in nature. When you are benchmarking your configuration or you have completed your performance tuning, remember to set `statistics` to `false`.

