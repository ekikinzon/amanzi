/*
  Operators

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// TPLs
#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_ParameterXMLFileReader.hpp"
#include "UnitTest++.h"

// Amanzi
#include "MeshFactory.hh"
#include "GMVMesh.hh"
#include "LinearOperatorFactory.hh"
#include "Tensor.hh"

// Operators
#include "Analytic01.hh"
#include "Analytic02.hh"

#include "OperatorAccumulation.hh"
#include "OperatorDefs.hh"
#include "OperatorDiffusionNLFVwithGravity.hh"

/* *****************************************************************
* Nonlinear finite volume scheme.
***************************************************************** */
template<class Analytic>
void RunTestDiffusionNLFV_DMP(double gravity, bool testing) {
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::AmanziGeometry;
  using namespace Amanzi::Operators;

  Epetra_MpiComm comm(MPI_COMM_WORLD);
  int MyPID = comm.MyPID();
  if (MyPID == 0) std::cout << "\nTest: 2D elliptic solver, NLFV with DMP, g=" << gravity << std::endl;

  // read parameter list
  std::string xmlFileName = "test/operator_diffusion.xml";
  Teuchos::ParameterXMLFileReader xmlreader(xmlFileName);
  Teuchos::ParameterList plist = xmlreader.getParameters();
  Teuchos::ParameterList op_list = plist.get<Teuchos::ParameterList>("PK operator")
                                        .sublist("diffusion operator nlfv");

  // create an MSTK mesh framework
  FrameworkPreference pref;
  pref.clear();
  pref.push_back(MSTK);

  MeshFactory meshfactory(&comm);
  meshfactory.preference(pref);
  // Teuchos::RCP<const Mesh> mesh = meshfactory(0.0, 0.0, 1.0, 1.0, 2, 2, NULL);
  Teuchos::RCP<const Mesh> mesh = meshfactory("test/random10.exo");

  // modify diffusion coefficient
  Teuchos::RCP<std::vector<WhetStone::Tensor> > K = Teuchos::rcp(new std::vector<WhetStone::Tensor>());
  int ncells = mesh->num_entities(AmanziMesh::CELL, AmanziMesh::OWNED);
  int nfaces = mesh->num_entities(AmanziMesh::FACE, AmanziMesh::OWNED);
  int ncells_wghost = mesh->num_entities(AmanziMesh::CELL, AmanziMesh::USED);
  int nfaces_wghost = mesh->num_entities(AmanziMesh::FACE, AmanziMesh::USED);

  Analytic ana(mesh, gravity);

  for (int c = 0; c < ncells; c++) {
    const Point& xc = mesh->cell_centroid(c);
    const WhetStone::Tensor& Kc = ana.Tensor(xc, 0.0);
    K->push_back(Kc);
  }

  // create boundary data
  std::vector<int> bc_model(nfaces_wghost, Operators::OPERATOR_BC_NONE);
  std::vector<double> bc_value(nfaces_wghost, 0.0), bc_mixed(nfaces_wghost, 0.0);

  for (int f = 0; f < nfaces_wghost; f++) {
    const Point& xf = mesh->face_centroid(f);
    if (fabs(xf[0]) < 1e-6 || fabs(xf[0] - 1.0) < 1e-6 ||
        fabs(xf[1]) < 1e-6) {
      bc_model[f] = OPERATOR_BC_DIRICHLET;
      bc_value[f] = ana.pressure_exact(xf, 0.0);
    } else if (fabs(xf[1] - 1.0) < 1e-6) {
      const AmanziGeometry::Point& normal = mesh->face_normal(f);
      double area = mesh->face_area(f);
      bc_model[f] = OPERATOR_BC_NEUMANN;
      bc_value[f] = ana.velocity_exact(xf, 0.0) * normal / area;
      bc_model[f] = OPERATOR_BC_DIRICHLET;
      bc_value[f] = ana.pressure_exact(xf, 0.0);
    }
  }
  Teuchos::RCP<BCs> bc = Teuchos::rcp(new BCs(Operators::OPERATOR_BC_TYPE_FACE, bc_model, bc_value, bc_mixed));

  // create diffusion operator 
  double rho(1.0);
  AmanziGeometry::Point g(0.0, -gravity);
  Teuchos::RCP<OperatorDiffusion> op = Teuchos::rcp(new OperatorDiffusionNLFVwithGravity(op_list, mesh, rho, g));
  Teuchos::RCP<Operator> global_op = op->global_operator();
  op->SetBCs(bc, bc);
  const CompositeVectorSpace& cvs = global_op->DomainMap();

  // create and initialize state variables.
  Teuchos::RCP<CompositeVector> solution = Teuchos::rcp(new CompositeVector(cvs));
  solution->PutScalar(0.0);

  // create source 
  CompositeVector source(cvs);
  Epetra_MultiVector& src = *source.ViewComponent("cell");
  Epetra_MultiVector& sol = *solution->ViewComponent("cell");

  for (int c = 0; c < ncells; c++) {
    const Point& xc = mesh->cell_centroid(c);
    src[0][c] = ana.source_exact(xc, 0.0);
    // sol[0][c] = ana.pressure_exact(xc, 0.0);
  }

  // populate the diffusion operator
  op->Setup(K, Teuchos::null, Teuchos::null);
  for (int loop = 0; loop < 9; ++loop) {
    global_op->Init();
    op->UpdateMatrices(Teuchos::null, solution.ptr());

    // get and assmeble the global operator
    global_op->UpdateRHS(source, false);
    op->ApplyBCs(true, true);
    global_op->SymbolicAssembleMatrix();
    global_op->AssembleMatrix();

    // create preconditoner using the base operator class
    Teuchos::ParameterList slist = plist.get<Teuchos::ParameterList>("preconditioners");
    global_op->InitPreconditioner("Hypre AMG", slist);

    // solve the problem
    Teuchos::ParameterList lop_list = plist.get<Teuchos::ParameterList>("solvers");
    AmanziSolvers::LinearOperatorFactory<Operator, CompositeVector, CompositeVectorSpace> factory;
    Teuchos::RCP<AmanziSolvers::LinearOperator<Operator, CompositeVector, CompositeVectorSpace> >
       solver = factory.Create("Belos GMRES", lop_list, global_op);

    CompositeVector& rhs = *global_op->rhs();
    int ierr = solver->ApplyInverse(rhs, *solution);

    // compute pressure error
    Epetra_MultiVector& p = *solution->ViewComponent("cell", false);
    double pnorm, pl2_err, pinf_err;
    ana.ComputeCellError(p, 0.0, pnorm, pl2_err, pinf_err);

    // compute flux error
    CompositeVectorSpace cvs_tmp;
    cvs_tmp.SetMesh(mesh)->SetGhosted(true)
      ->AddComponent("cell", AmanziMesh::CELL, 1)
      ->AddComponent("face", AmanziMesh::FACE, 1);
    Teuchos::RCP<CompositeVector> flux = Teuchos::rcp(new CompositeVector(cvs_tmp));
    Epetra_MultiVector& flx = *flux->ViewComponent("face", true);

    op->UpdateFlux(*solution, *flux);
    double unorm, ul2_err, uinf_err;

    ana.ComputeFaceError(flx, 0.0, unorm, ul2_err, uinf_err);

    if (MyPID == 0) {
      pl2_err /= pnorm; 
      ul2_err /= unorm;
      printf("L2(p)=%10.4e  Inf(p)=%10.4e  L2(u)=%10.4e  Inf(u)=%10.4e  (itr=%d ||r||=%10.4e code=%d)\n",
          pl2_err, pinf_err, ul2_err, uinf_err,
          solver->num_itrs(), solver->residual(), solver->returned_code());

      if (testing) CHECK(pl2_err < 0.1 / (loop + 1) && ul2_err < 0.4 / (loop + 1));
      CHECK(solver->num_itrs() < 15);
    }
  }
}


TEST(OPERATOR_DIFFUSION_NLFV_DMP_02) {
  RunTestDiffusionNLFV_DMP<Analytic02>(0.0, true);
}

TEST(OPERATOR_DIFFUSION_NLFV_wGravity) {
  RunTestDiffusionNLFV_DMP<Analytic02>(2.7, true);
}

TEST(OPERATOR_DIFFUSION_NLFV_DMP_01) {
  RunTestDiffusionNLFV_DMP<Analytic01>(2.7, false);
}

