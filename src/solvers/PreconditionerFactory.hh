/*
  Solvers

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)

  Base factory for preconditioners.
*/

#ifndef AMANZI_PRECONDITIONER_FACTORY_HH_
#define AMANZI_PRECONDITIONER_FACTORY_HH_

#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"

#include "Preconditioner.hh"

namespace Amanzi {
namespace AmanziPreconditioners {

class PreconditionerFactory {
 public:
  PreconditionerFactory() {};
  ~PreconditionerFactory() {};

  Teuchos::RCP<Preconditioner> Create(const std::string& name,
          const Teuchos::ParameterList& prec_list);
  Teuchos::RCP<Preconditioner> Create(Teuchos::ParameterList& prec_list);
};

}  // namespace AmanziPreconditioners
}  // namespace Amanzi

#endif
