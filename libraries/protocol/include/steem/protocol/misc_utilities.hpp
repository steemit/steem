#pragma once

namespace steem { namespace protocol {

enum curve_id
{
   quadratic,
   bounded_curation,
   linear,
   square_root,
   convergent_linear,
   convergent_square_root
};

#ifdef STEEM_ENABLE_SMT
enum class smt_event: uint8_t
{
   none,
   setup_completion,
   ico_launch,
   ico_evaluation,
   token_launch
};
#endif

} } // steem::utilities


FC_REFLECT_ENUM(
   steem::protocol::curve_id,
   (quadratic)
   (bounded_curation)
   (linear)
   (square_root)
   (convergent_linear)
   (convergent_square_root)
)

#ifdef STEEM_ENABLE_SMT
FC_REFLECT_ENUM( steem::protocol::smt_event,
   (none)
   (setup_completion)
   (ico_launch)
   (ico_evaluation)
   (token_launch)
)
#endif
