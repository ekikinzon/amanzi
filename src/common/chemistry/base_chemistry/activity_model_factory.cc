/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */
#include "activity_model_factory.hh"

#include <sstream>
#include <string>

#include "activity_model.hh"
#include "activity_model_debye_huckel.hh"
// Pitzer equations were implemented (Sergio A Bea)
#include "activity_model_pitzer_hwm.hh"
#include "activity_model_unit.hh"
#include "chemistry_exception.hh"
#include "species.hh"
#include "exceptions.hh"

namespace Amanzi {
namespace AmanziChemistry {

const std::string ActivityModelFactory::debye_huckel = "debye-huckel";
const std::string ActivityModelFactory::pitzer_hwm = "pitzer-hwm";
const std::string ActivityModelFactory::unit = "unit";

ActivityModelFactory::ActivityModelFactory() {
}  // end ActivityModelFactory constructor

ActivityModelFactory::~ActivityModelFactory() {
}  // end ActivityModelFactory destructor

ActivityModel* ActivityModelFactory::Create(
    const std::string& model,
    const ActivityModel::ActivityModelParameters& parameters,
    const std::vector<Species>& primary_species,
    const std::vector<AqueousEquilibriumComplex>& secondary_species) {

  ActivityModel* activity_model = NULL;

  if (model == debye_huckel) {
    activity_model = new ActivityModelDebyeHuckel();
  } else if (model == pitzer_hwm) {
    activity_model = new ActivityModelPitzerHWM();
  } else if (model == unit) {
    activity_model = new ActivityModelUnit();
  } else {
    // default type, error...?
    std::ostringstream error_stream;
    error_stream << "ActivityModelFactory::Create(): \n"
                 << "Unknown activity model name: " << model << "\n"
                 << "       valid names: " << unit << "\n"
                 << "                    " << debye_huckel << "\n"
                 << "                    " << pitzer_hwm << "\n" ;
    Exceptions::amanzi_throw(ChemistryInvalidInput(error_stream.str()));
  }

  if (activity_model == NULL) {
    // something went wrong, should throw an exception and exit gracefully....
    std::ostringstream error_stream;
    error_stream << "ActivityModelFactory::Create(): \n"
                 << "Activity model was not created for some reason....\n";
    Exceptions::amanzi_throw(ChemistryException(error_stream.str()));
  } else {
    // finish any additional setup

    // TODO(bandre): set the name in the object constructor so we can
    // verify that the correct object was created.
    activity_model->name(model);
    activity_model->Setup(parameters, primary_species, secondary_species);
  }
  return activity_model;
}  // end Create()

}  // namespace AmanziChemistry
}  // namespace Amanzi
