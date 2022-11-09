#include <yapp.hpp>

using namespace yapp;

InvalidOffsetException::InvalidOffsetException
(Offset offset)
   : offset(*offset),
     Exception()
{
   std::stringstream stream;

   stream << "Offset " << std::hex << std::showbase << this->offset
          << " is not a valid offset.";

   this->error = stream.str();
}

InvalidRVAException::InvalidRVAException
(RVA rva)
   : rva(*rva),
     Exception()
{
   std::stringstream stream;

   stream << "RVA " << std::hex << std::showbase << this->rva
          << " is not a valid RVA.";

   this->error = stream.str();
}

InvalidVAException::InvalidVAException
(VA va)
   : va(*va),
     Exception()
{
   std::stringstream stream;

   stream << "VA " << std::hex << std::showbase << this->va
          << " is not a valid VA.";

   this->error = stream.str();
}
