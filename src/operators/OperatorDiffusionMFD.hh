// OperatorDiffusionMFD: elliptic operators using the MFD family of discretizations.
/*
  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL.
  Amanzi is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
          Ethan Coon (ecoon@lanl.gov)
*/

#ifndef AMANZI_OPERATOR_DIFFUSION_MFD_HH_
#define AMANZI_OPERATOR_DIFFUSION_MFD_HH_

#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"

#include "exceptions.hh"
#include "Tensor.hh"
#include "Point.hh"
#include "CompositeVector.hh"
#include "DenseMatrix.hh"

#include "BCs.hh"
#include "Operator.hh"
#include "OperatorDefs.hh"
#include "OperatorDiffusion.hh"

/*!
Additional options available only for the MFD family of discretizations include:
  
* `"nonlinear coefficient`" [string] specifies a method for treating nonlinear
  diffusion coefficient, if any. Available options are `"none`", `"upwind:
  face`", `"divk: cell-face`" (default), `"divk: face`", `"standard: cell`",
  `"divk: cell-face-twin`" and `"divk: cell-grad-face-twin`".  Symmetry
  preserving methods are the divk-family of methods and the classical
  cell-centered method (`"standard: cell`"). The first part of the name
  indicates the base scheme.  The second part (after the semi-column)
  indicates required components of the composite vector that must be provided
  by a physical PK.

* `"discretization secondary`" [string] specifies the most robust
  discretization method that is used when the primary selection fails to
  satisfy all a priori conditions.  This is typically `"mfd: default`".
  **Used only when an MFD `"primary discretization`" is used**

* `"schema`" [Array(string)] defines the operator stencil. It is a collection of 
  geometric objects.  Typically this is set by the implementation and is not provided.

* `"preconditioner schema`" [Array(string)] **{face,cell}** Defines the
  preconditioner stencil.  It is needed only when the default assembling
  procedure is not desirable. If skipped, the `"schema`" is used instead.
  In addition to the default, **{face}** may be used, which forms the Schur
  complement.
   
* `"consistent faces`" [list] may contain a `"preconditioner`" and
  `"linear operator`" list (see sections Preconditioners_ and LinearSolvers_
  respectively).  If these lists are provided, and the `"discretization
  primary`" is of type `"mfd: *`", then the diffusion method
  UpdateConsistentFaces() can be used.  This method, given a set of cell
  values, determines the faces constraints that satisfy the constraint
  equation in MFD by assembling and inverting the face-only system.  This is
  not currently used by any Amanzi PKs.

* `"diffusion tensor`" [string] allows us to solve problems with symmetric and non-symmetric 
  (but positive definite) tensors. Available options are *symmetric* (default) and *nonsymmetric*.

*/

namespace Amanzi {
namespace Operators {

class OperatorDiffusionMFD : public virtual OperatorDiffusion {
 public:
  OperatorDiffusionMFD(Teuchos::ParameterList& plist,
                       const Teuchos::RCP<Operator>& global_op) :
      OperatorDiffusion(global_op),
      plist_(plist),
      factor_(1.0)
  {
    operator_type_ = OPERATOR_DIFFUSION_MFD;
    InitDiffusion_(plist);
  }

  OperatorDiffusionMFD(Teuchos::ParameterList& plist,
                       const Teuchos::RCP<const AmanziMesh::Mesh>& mesh) :
      OperatorDiffusion(mesh),
      plist_(plist),
      factor_(1.0)
  {
    operator_type_ = OPERATOR_DIFFUSION_MFD;
    InitDiffusion_(plist);
  }

  OperatorDiffusionMFD(Teuchos::ParameterList& plist,
                       const Teuchos::RCP<AmanziMesh::Mesh>& mesh) :
      OperatorDiffusion(mesh),
      plist_(plist),
      factor_(1.0)
  {
    operator_type_ = OPERATOR_DIFFUSION_MFD;
    InitDiffusion_(plist);
  }

  // main virtual members for populating an operator
  virtual void SetTensorCoefficient(const Teuchos::RCP<std::vector<WhetStone::Tensor> >& K) override;
  virtual void SetScalarCoefficient(const Teuchos::RCP<const CompositeVector>& k,
                                    const Teuchos::RCP<const CompositeVector>& dkdp) override;

  // -- To calculate elemetal matrices, we can use input parameters flux 
  //    and u from the previous nonlinear iteration. Otherwise, use null-pointers.
  virtual void UpdateMatrices(const Teuchos::Ptr<const CompositeVector>& flux,
                              const Teuchos::Ptr<const CompositeVector>& u) override;

  // -- Approximation of the Jacobian requires non-null flux from the 
  //    previous nonlinear iteration. The second parameter, u, so far is a
  //    placeholder for new approximation methods.
  virtual void UpdateMatricesNewtonCorrection(const Teuchos::Ptr<const CompositeVector>& flux,
                                              const Teuchos::Ptr<const CompositeVector>& u,
                                              double scalar_limiter=1) override;

  // modify the operator
  // -- by incorporating boundary conditions. Variable 'primary' indicates
  //    that we put 1 on the matrix diagonal. Variable 'eliminate' says
  //    that we eliminate essential BCs for the trial function, i.e. zeros
  //    go in the corresponding matrix columns.
  virtual void ApplyBCs(bool primary, bool eliminate) override;

  // -- by breaking p-lambda coupling.
  virtual void ModifyMatrices(const CompositeVector& u) override;

  // -- by rescaling mass matrices.
  virtual void ScaleMassMatrices(double s) override;

  // main virtual members after solving the problem
  // -- calculate the flux variable.
  virtual void UpdateFlux(const CompositeVector& u, CompositeVector& flux) override;

  // Developments
  // -- working with consistent faces
  virtual int UpdateConsistentFaces(CompositeVector& u) override;
  Teuchos::RCP<const Operator> consistent_face_operator() const { return consistent_face_op_; }
  Teuchos::RCP<Operator> consistent_face_operator() { return consistent_face_op_; }
  
  // -- interface to solvers for treating nonlinear BCs.
  virtual double ComputeTransmissibility(int f) const override;
  virtual double ComputeGravityFlux(int f) const override
    { return 0.0; }
 
  // developer checks
  int nfailed_primary() { return nfailed_primary_; }
  void set_factor(double factor) { factor_ = factor; }

 protected:
  void InitDiffusion_(Teuchos::ParameterList& plist);
  void CreateMassMatrices_();

  void UpdateMatricesNodal_();
  void UpdateMatricesEdge_();
  void UpdateMatricesTPFA_();
  void UpdateMatricesMixed_(const Teuchos::Ptr<const CompositeVector>& flux);
  void UpdateMatricesMixedWithGrad_(const Teuchos::Ptr<const CompositeVector>& flux);

  void AddNewtonCorrectionCell_(const Teuchos::Ptr<const CompositeVector>& flux,
                                const Teuchos::Ptr<const CompositeVector>& u,
                                double scalar_limiter);

  void ApplyBCs_Mixed_(BCs& bc_trial, BCs& bc_test, bool primary, bool eliminate);
  void ApplyBCs_Cell_(BCs& bc_trial, BCs& bc_test, bool primary, bool eliminate);
  void ApplyBCs_Edge_(BCs& bc_trial, BCs& bc_test, bool primary, bool eliminate);
  void ApplyBCs_Nodal_(const Teuchos::Ptr<BCs>& bc_f, const Teuchos::Ptr<BCs>& bc_n,
                       bool primary, bool eliminate);

 protected:
  Teuchos::ParameterList plist_;
  std::vector<WhetStone::DenseMatrix> Wff_cells_;
  bool mass_matrices_initialized_;

  int newton_correction_;
  double factor_;
  bool exclude_primary_terms_;

  // modifiers for flux continuity equations
  bool scaled_constraint_;
  double scaled_constraint_cutoff_, scaled_constraint_fuzzy_;

  int mfd_primary_, mfd_secondary_, mfd_pc_primary_, mfd_pc_secondary_;
  int nfailed_primary_;

  Teuchos::RCP<Operator> consistent_face_op_;
};

}  // namespace Operators
}  // namespace Amanzi


#endif

