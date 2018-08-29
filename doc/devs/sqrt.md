
# Introduction

In this document we derive the approximate integer square root function used by Steem for the curation curve
[here](https://github.com/steemit/steem/issues/1052).

# MSB function

Let function `msb(x)` be defined as follows for `x >= 1`:

- (Definition 1a) `msb(x)` is the index of the most-significant 1 bit in the binary representation of `x`

The following definitions are equivalent to Definition (1a):

- (Definition 1b) `msb(x)` is the length of the binary representation of `x` as a string of bits, minus one
- (Definition 1c) `msb(x)` is the greatest integer such that `2 ^ msb(x) <= x`
- (Definition 1d) `msb(x) = floor(log_2(x))`

Many CPU's (including Intel CPU's since the Intel 386) can compute the `msb()` function very quickly on
machine-word-size integers with a special instruction directly implemented in hardware.  In C++, the
Boost library provides reasonably compiler-independent, hardware-independent access to this
functionality with `boost::multiprecision::detail::find_msb(x)`.

# Approximate logarithms

According to Definition 1d, `msb(x)` is already a (somewhat crude) approximate base-2 logarithm.  The
bits below the most-significant bit provide the fractional part of the linear interpolation.  The
fractional part is called the *mantissa* and the integer part is called the *exponent*; effectively we
have re-invented the floating-point representation.

Here are some Python functions to convert to/from these approximate logarithms:

```
def to_log(x, wordsize=32, ebits=5):
    if x <= 1:
        return x
    # mantissa_bits, mantissa_mask are independent of x
    mantissa_bits = wordsize - ebits
    mantissa_mask = (1 << mantissa_bits)-1

    msb = x.bit_length() - 1
    mantissa_shift = mantissa_bits - msb
    y = (msb << mantissa_bits) | ((x << mantissa_shift) & mantissa_mask)
    return y

def from_log(y, wordsize=32, ebits=5):
    if y <= 1:
        return y
    # mantissa_bits, leading_1, mantissa_mask are independent of x
    mantissa_bits = wordsize - ebits
    leading_1 = 1 << mantissa_bits
    mantissa_mask = leading_1 - 1

    msb = y >> mantissa_bits
    mantissa_shift = mantissa_bits - msb
    y = (leading_1 | (y & mantissa_mask)) >> mantissa_shift
    return y
```

# Approximate square roots

To construct an approximate square root algorithm, start from the identity `log(sqrt(x)) = log(x) / 2`.
We can easily obtain `sqrt(x) ~ from_log(to_log(x) >> 1)`.  We can proceed by manual inlining the inner
function call:

```
def approx_sqrt_v0(x, wordsize=32, ebits=5):
    if x <= 1:
        return x
    # mantissa_bits, leading_1, mantissa_mask are independent of x
    mantissa_bits = wordsize - ebits
    leading_1 = 1 << mantissa_bits
    mantissa_mask = leading_1 - 1

    msb_x = x.bit_length() - 1
    mantissa_shift_x = mantissa_bits - msb_x
    to_log_x = (msb_x << mantissa_bits) | ((x << mantissa_shift_x) & mantissa_mask)

    z = to_log_x >> 1

    msb_z = z >> mantissa_bits
    mantissa_shift_z = mantissa_bits - msb_z
    result = (leading_1 | (z & mantissa_mask)) >> mantissa_shift_z
    return result
```

# Optimized approximate square roots

First, consider the following simplifications:

- The exponent part of `z`, denoted here `msb_z`, is simply `msb_x >> 1`
- The MSB of the mantissa part of `z` is the low bit of `msb_x`
- The lower bits of the mantissa part of `z` are simply the bits of the mantissa part of `x` shifted once

The above simplifications enable a more fundamental improvement:  We can compute
the mantissa and exponent of `z` directly from `x` and `msb_x`.  Therefore, packing
the intermediate result into `to_log_x` and then immediately unpacking it, effectively
becomes a no-op and can be omitted.  This makes the `wordsize` and `ebits` variables fall
out.  Making choices for these parameters and allocating extra space at the top of the word
for exponent bits becomes completely unnecessary!

One subtlety is that the two shift operators result in a net shift of mantissa bits.  The
are shifted left by `mantissa_bits - msb_x` and then shifted right by `mantissa_bits - msb_z`.  The
net shift is therefore a right-shift of `msb_x - msb_z`.

The final code looks like this:

```
def approx_sqrt_v1(x):
    if x <= 1:
        return x
    # mantissa_bits, leading_1, mantissa_mask are independent of x
    msb_x = x.bit_length() - 1
    msb_z = msb_x >> 1
    msb_x_bit = 1 << msb_x
    msb_z_bit = 1 << msb_z
    mantissa_mask = msb_x_bit-1

    mantissa_x = x & mantissa_mask
    if (msb_x & 1) != 0:
        mantissa_z_hi = msb_z_bit
    else:
        mantissa_z_hi = 0
    mantissa_z_lo = mantissa_x >> (msb_x - msb_z)
    mantissa_z = (mantissa_z_hi | mantissa_z_lo) >> 1
    result = msb_z_bit | mantissa_z
    return result
```
