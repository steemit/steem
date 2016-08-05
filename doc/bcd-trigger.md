
### Broadcast disable

This file documents the `--bcd-trigger` command line parameter.

The `--bcd-trigger` command line parameter controls the triggering of the internal BCD (Broadcast Disable)
state of steemd.  This option takes a list of `[nblocks,nseconds]` pairs.

### Examples

- For example, `[[0,10]]` means to set the BCD state whenever 0 blocks have been received in the last 10 seconds.
- For another example, `[[0,10],[108,385]]` means to set the BCD state whenever 0 blocks have been received in the last 10 seconds, *or* when 108 or fewer blocks have been received in the last 385 seconds.  This is the default setting.

### More notes

- When BCD is set, all transactions submitted via the `network_broadcast_api` will immediately fail.
- Transactions submitted by Piston, cli_wallet and web UI clients are affected by BCD.
- Transactions received on the p2p network *are not* affected by BCD and will be relayed as normal.
- The BCD state is not an actual state variable, rather it is a condition that is checked on each call to the `network_broadcast_api`.

### Purpose

The main purpose of BCD is that many UI's and exchanges want to "fail fast" when the backend node
gets out of sync with the network, as an instant failure is much easier to recover from than the alternative
(initial acceptance followed by discarding on fork switching).

### Per-connection policies

An application-wide default BCD trigger policy is set on the command line or in `config.ini`.  New
`network_broadcast_api` connections will use the default trigger, but a client may override the policy
with the `set_bcd_trigger()` API call.  This client-requested override is implemented on the API object,
and therefore only applies to transactions submitted over the same connection.
