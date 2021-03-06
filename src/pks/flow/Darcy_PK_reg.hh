/*
  Flow PK
 
  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Darcy PK registration.
*/

#include "Darcy_PK.hh"

namespace Amanzi {
namespace Flow {

RegisteredPKFactory<Darcy_PK> Darcy_PK::reg_("darcy");

}  // namespace Amanzi
}  // namespace Flow

