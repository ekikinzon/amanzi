#ifndef AMANZI_OBSERVABLE_LINE_SEGMENT_AQUEOUS_HH
#define AMANZI_OBSERVABLE_LINE_SEGMENT_AQUEOUS_HH

#include "ObservableAmanzi.hh"
#include "ObservableAqueous.hh"
#include "ObservableLineSegment.hh"


namespace Amanzi{

class ObservableLineSegmentAqueous : public ObservableAqueous,
                                     public ObservableLineSegment {
 public:
   ObservableLineSegmentAqueous(std::string variable,
                                std::string region,
                                std::string functional,
                                Teuchos::ParameterList& plist,
                                Teuchos::ParameterList& units_plist,
                                Teuchos::RCP<AmanziMesh::Mesh> mesh);

  virtual void ComputeObservation(State& S, double* value, double* volume, std::string& unit);
  virtual int ComputeRegionSize();
  void InterpolatedValues(State& S,
                          std::string& var,
                          std::string& interpolation,
                          AmanziMesh::Entity_ID_List& ids,
                          std::vector<AmanziGeometry::Point>& line_pnts,
                          std::vector<double>& values);
};


ObservableLineSegmentAqueous::ObservableLineSegmentAqueous(std::string variable,
                                                           std::string region,
                                                           std::string functional,
                                                           Teuchos::ParameterList& plist,
                                                           Teuchos::ParameterList& units_plist,
                                                           Teuchos::RCP<AmanziMesh::Mesh> mesh) :
    Observable(variable, region, functional, plist, units_plist, mesh),
    ObservableAqueous(variable, region, functional, plist, units_plist, mesh),
    ObservableLineSegment(variable, region, functional, plist, units_plist, mesh) {};


int ObservableLineSegmentAqueous::ComputeRegionSize() {
  return ObservableLineSegment::ComputeRegionSize();
}


void ObservableLineSegmentAqueous::ComputeObservation(
    State& S, double* value, double* volume, std::string& unit) {
  Errors::Message msg;
  int dim = mesh_ -> space_dimension();
  
  std::vector<double> values(region_size_);
  double weight_corr = 1e-15;

  InterpolatedValues(S, variable_, interpolation_, entity_ids_, line_points_, values);

  *value = 0.0;
  *volume = 0.0;
  unit = "m";

  if (weighting_ == "none") {
    for (int i = 0; i < region_size_; i++){
      *value += values[i] * lofs_[i];
      *volume += lofs_[i];
    }
  } else if (weighting_ == "flux norm") {
    if (S.HasField("darcy_velocity")) {
      const Epetra_MultiVector& darcy_vel = *S.GetFieldData("darcy_velocity")->ViewComponent("cell");
      for (int i = 0; i < region_size_; i++) {
        int c = entity_ids_[i];

        double norm = 0.0;
        for (int j = 0; j < dim; j++) norm += darcy_vel[j][c] * darcy_vel[j][c];
        norm = sqrt(norm) + weight_corr;

        *value += values[i] * lofs_[i] * norm;
        *volume += lofs_[i] * norm;
      }
    }
  }
}


void ObservableLineSegmentAqueous::InterpolatedValues(State& S,
                                                      std::string& var,
                                                      std::string& interpolation,
                                                      AmanziMesh::Entity_ID_List& ids,
                                                      std::vector<AmanziGeometry::Point>& line_pnts,
                                                      std::vector<double>& values) {
  // if (!S.HasField(var)) {
  //   Errors::Message msg;
  //   msg <<"InterpolatedValue: field "<<var<<" doesn't exist in state";
  //   Exceptions::amanzi_throw(msg);
  // }

  Teuchos::RCP<const Epetra_MultiVector> vector;
  Teuchos::RCP<const CompositeVector> cv;

  if (var == "hydraulic head") {
    if (!S.HasField("hydraulic_head")) {
      Errors::Message msg;
      msg <<"InterpolatedValue: field hydraulic_head doesn't exist in state";
      Exceptions::amanzi_throw(msg);
    }
    cv = S.GetFieldData("hydraulic_head");
    vector = cv->ViewComponent("cell", true);
  } else {
    if (!S.HasField(var)) {
      Errors::Message msg;
      msg <<"InterpolatedValue: field "<<var<<" doesn't exist in state";
      Exceptions::amanzi_throw(msg);
    }
    cv = S.GetFieldData(var);
    vector = cv->ViewComponent("cell", true);
  }

  if (interpolation == "linear") {
    Teuchos::ParameterList plist;
    Operators::ReconstructionCell lifting(mesh_);
    std::vector<AmanziGeometry::Point> gradient; 
      
    cv->ScatterMasterToGhosted();

    if (limiter_){ // At the moment only Kuzmin limiter is implemented for observ.
      plist.set<std::string>("limiter", "Kuzmin");
    }
    
    lifting.Init(vector, plist);
    lifting.ComputeGradient(ids, gradient);

    if (limiter_) {
      // if (!S.HasField("darcy_velocity")){
      //   Errors::Message msg;
      //   msg <<"Limiter can't be apllied without darcy_velocity";
      //   Exceptions::amanzi_throw(msg);
      // }
      
      // Teuchos::RCP<const Epetra_MultiVector> flux =  S.GetFieldData("darcy_velocity")->ViewComponent("cell");
      // // Apply limiter
      // lifting.InitLimiter(flux);
      lifting.ApplyLimiter(ids, gradient);
    }

    for (int i = 0; i < ids.size(); i++) {
      int c = ids[i];
      values[i] = lifting.getValue( gradient[i], c, line_pnts[i]);
    }
  } else if (interpolation == "constant") {    
    for (int i = 0; i < ids.size(); i++) {
      int c = ids[i];
      values[i] = (*vector)[0][c];
    }
  } else {
    Errors::Message msg;
    msg <<"InterpolatedValue: unknown interpolation method "<<interpolation;
    Exceptions::amanzi_throw(msg);
  }
}

}  // namespace Amanzi

#endif
