/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */
/* -------------------------------------------------------------------------
Amanzi Flow

License: see COPYRIGHT
Author: Ethan Coon

Interface layer between Flow and State, this is a harness for
accessing the new state-dev from the old Flow PK.

 ------------------------------------------------------------------------- */


#include "Flow_State.hh"

namespace Amanzi {
namespace AmanziFlow {

Flow_State::Flow_State(Teuchos::RCP<AmanziMesh::Mesh> mesh) :
    PK_State(std::string("state"), mesh) {
  Construct_();
}

Flow_State::Flow_State(Teuchos::RCP<State> S) :
    PK_State(std::string("state"), S) {
  Construct_();
}

Flow_State::Flow_State(State& S) :
    PK_State(std::string("state"), S) {
  Construct_();
}

Flow_State::Flow_State(const Flow_State& other,
        PKStateConstructMode mode) :
    PK_State(other, STATE_CONSTRUCT_MODE_COPY_POINTERS) {
  if (mode == PK_STATE_CONSTRUCT_MODE_VIEW_DATA) {
    // This is pointer-copying, and is the default behavior.
    // pass
  } else if (mode == PK_STATE_CONSTRUCT_MODE_VIEW_DATA_GHOSTED) {
    // ?
    ASSERT(0);
  } else if (mode == PK_STATE_CONSTRUCT_MODE_COPY_DATA) {
    // Not currently needed by Flow?
    ASSERT(0);
  } else if (mode == PK_STATE_CONSTRUCT_MODE_COPY_DATA_GHOSTED) {
    // Copy data, making new vector for Darcy flux with ghosted entries.
    ghosted_ = true;

    CompositeVectorFactory fac;
    fac.SetMesh(mesh_);
    fac.SetComponent("face", AmanziMesh::FACE, 1);
    Teuchos::RCP<CompositeVector> flux = fac.CreateVector(true);
    flux->CreateData();
    flux->PutScalar(0.);
    *flux->ViewComponent("face",false) = *other.darcy_flux();
    flux->ScatterMasterToGhosted();
    S_->SetData("darcy_flux", name_, flux);
  }
}

void Flow_State::Construct_() {
  // for creating fields
  std::vector<AmanziMesh::Entity_kind> locations(2);
  std::vector<std::string> names(2);
  std::vector<int> ndofs(2,1);
  locations[0] = AmanziMesh::CELL; locations[1] = AmanziMesh::FACE;
  names[0] = "cell"; names[1] = "face";

  // Require data, all owned
  S_->RequireScalar("fluid_density", name_);
  S_->RequireScalar("fluid_viscosity", name_);
  S_->RequireConstantVector("gravity", name_, mesh_->space_dimension());

  S_->RequireField("pressure", name_)->SetMesh(mesh_)->SetGhosted(false)
    ->SetComponents(names, locations, ndofs);
  S_->RequireField("permeability", name_)->SetMesh(mesh_)->SetGhosted(false)
    ->SetComponent("cell", AmanziMesh::CELL, 2);
  S_->RequireField("porosity", name_)->SetMesh(mesh_)->SetGhosted(false)
    ->SetComponent("cell", AmanziMesh::CELL, 1);
  S_->RequireField("water_saturation", name_)->SetMesh(mesh_)->SetGhosted(false)
    ->SetComponent("cell", AmanziMesh::CELL, 1);
  S_->RequireField("prev_water_saturation", name_)->SetMesh(mesh_)->SetGhosted(false)
    ->SetComponent("cell", AmanziMesh::CELL, 1);
  S_->RequireField("specific_storage", name_)->SetMesh(mesh_)->SetGhosted(false)
    ->SetComponent("cell", AmanziMesh::CELL, 1);
  S_->RequireField("specific_yield", name_)->SetMesh(mesh_)->SetGhosted(false)
    ->SetComponent("cell", AmanziMesh::CELL, 1);
  S_->RequireField("darcy_flux", name_)->SetMesh(mesh_)->SetGhosted(false)
    ->SetComponent("face", AmanziMesh::FACE, 1);
  S_->RequireField("darcy_velocity", name_)->SetMesh(mesh_)->SetGhosted(false)
    ->SetComponent("cell", AmanziMesh::CELL, mesh_->space_dimension());

};

void Flow_State::Initialize() {
  if (standalone_mode_) {
    S_->Setup();
    S_->GetField("fluid_density",name_)->set_initialized();
    S_->GetField("fluid_viscosity",name_)->set_initialized();
    S_->GetField("gravity",name_)->set_initialized();
    S_->GetField("pressure",name_)->set_initialized();
    S_->GetField("permeability",name_)->set_initialized();
    S_->GetField("porosity",name_)->set_initialized();
    S_->GetField("water_saturation",name_)->set_initialized();
    S_->GetField("prev_water_saturation",name_)->set_initialized();
    S_->GetField("specific_storage",name_)->set_initialized();
    S_->GetField("specific_yield",name_)->set_initialized();
    S_->GetField("darcy_flux",name_)->set_initialized();
    S_->GetField("darcy_velocity",name_)->set_initialized();
    S_->Initialize();
  } else {
    // BEGIN REMOVE ME once flow tests pass --etc
    S_->GetFieldData("pressure",name_)->PutScalar(-1.);
    S_->GetFieldData("water_saturation",name_)->PutScalar(-1.);
    S_->GetFieldData("prev_water_saturation",name_)->PutScalar(-1.);
    S_->GetFieldData("darcy_flux",name_)->PutScalar(-1.);
    S_->GetFieldData("darcy_velocity",name_)->PutScalar(-1.);
    S_->GetFieldData("specific_storage",name_)->PutScalar(-1.);
    S_->GetFieldData("specific_yield",name_)->PutScalar(-1.);
    // END REMOVE ME

    // secondary variables, will be initialized by the PK
    S_->GetField("water_saturation",name_)->set_initialized();
    S_->GetField("prev_water_saturation",name_)->set_initialized();

    S_->GetField("darcy_flux",name_)->set_initialized();
    S_->GetField("darcy_velocity",name_)->set_initialized();

    // no clue how these work or where they are initialized, but it seems not
    // in the state.
    S_->GetField("specific_storage",name_)->set_initialized();
    S_->GetField("specific_yield",name_)->set_initialized();
  }
}

Teuchos::RCP<AmanziGeometry::Point>
Flow_State::gravity() {
  Teuchos::RCP<Epetra_Vector> gvec = S_->GetConstantVectorData("gravity", name_);
  Teuchos::RCP<AmanziGeometry::Point> gpoint =
    Teuchos::rcp(new AmanziGeometry::Point(gvec->MyLength()));
  for (int i=0; i!=gvec->MyLength(); ++i) (*gpoint)[i] = (*gvec)[i];
  return gpoint;
}


// debug routines
void Flow_State::set_fluid_density(double rho) {
  *fluid_density() = rho;
}

void Flow_State::set_fluid_viscosity(double mu) {
  *fluid_viscosity() = mu;
}

void Flow_State::set_porosity(double phi) {
  porosity()->PutScalar(phi);
}

void Flow_State::set_pressure_hydrostatic(double z0, double p0) {
  int dim = mesh_->space_dimension();
  int ncells = mesh_->num_entities(AmanziMesh::CELL, AmanziMesh::USED);

  double rho = *fluid_density();
  double g = (*gravity())[dim - 1];

  Epetra_Vector& pres = ref_pressure();
  for (int c=0; c!=ncells; ++c) {
    const AmanziGeometry::Point& xc = mesh_->cell_centroid(c);
    pres[c] = p0 + rho * g * (xc[dim - 1] - z0);
  }

}

void Flow_State::set_permeability(double Kh, double Kv) {
  horizontal_permeability()->PutScalar(Kh);
  vertical_permeability()->PutScalar(Kv);
}


void Flow_State::set_permeability(double Kh, double Kv, const string region) {
  Epetra_Vector& horizontal_perm = ref_horizontal_permeability();
  Epetra_Vector& vertical_perm = ref_vertical_permeability();

  AmanziMesh::Entity_ID_List block;
  mesh_->get_set_entities(region, AmanziMesh::CELL, AmanziMesh::OWNED, &block);
  int ncells = block.size();

  for (int i=0; i!=ncells; ++i) {
    int c = block[i];
    horizontal_perm[c] = Kh;
    vertical_perm[c] = Kv;
  }
}


void Flow_State::set_gravity(double g) {
  Teuchos::RCP<Epetra_Vector> gvec = S_->GetConstantVectorData("gravity",name_);
  int dim = mesh_->space_dimension();

  gvec->PutScalar(0.);
  (*gvec)[dim-1] = g;
  (*gvec)[2] = g;  // Waiting for Markus ticket (lipnikov@lanl.gov)
}


void Flow_State::set_specific_storage(double ss) {
  specific_storage()->PutScalar(ss);
}

} // namespace
} // namespace
