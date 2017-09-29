#pragma once

namespace steem { namespace utilities {

// TODO:  Rename these curves to match naming in manual.md
enum curve_id
{
   quadratic,
   quadratic_curation,
   linear,
   square_root
};

} } // steem::utilities


FC_REFLECT_ENUM(
   steem::utilities::curve_id,
   (quadratic)
   (quadratic_curation)
   (linear)
   (square_root)
)
