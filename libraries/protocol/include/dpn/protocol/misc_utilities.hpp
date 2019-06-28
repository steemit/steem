#pragma once

namespace dpn { namespace protocol {

enum curve_id
{
   quadratic,
   bounded_curation,
   linear,
   square_root,
   convergent_linear,
   convergent_square_root
};

} } // dpn::utilities


FC_REFLECT_ENUM(
   dpn::protocol::curve_id,
   (quadratic)
   (bounded_curation)
   (linear)
   (square_root)
   (convergent_linear)
   (convergent_square_root)
)
