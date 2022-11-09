#pragma once

#include <type_traits>

namespace yapp
{
   template <typename T, typename U=void>
   struct enable_if_type { typedef U type; };

   template <typename T, typename U=void>
   struct has_typedef : std::false_type {};
   
