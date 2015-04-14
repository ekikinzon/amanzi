/*
  This is the operators component of the Amanzi code. 

  Copyright 2010-2012 held jointly by LANS/LANL, LBNL, and PNNL. 
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
#include "mfd3d_diffusion.hh"
#include "tensor.hh"

// Operators
#include "Analytic04.hh"
#include "HeatConduction.hh"

#include "OperatorDefs.hh"
#include "OperatorDiffusionTPFA.hh"
#include "OperatorSource.hh"
#include "UpwindSecondOrder.hh"
#include "UpwindStandard.hh"


/* *****************************************************************
* This tests diffusion solvers with zero coefficients
* **************************************************************** */
TEST(OPERATOR_DIFFUSION_TPFA_ZEROCOEF) {
  using namespace Teuchos;
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::AmanziGeometry;
  using namespace Amanzi::Operators;

  Epetra_MpiComm comm(MPI_COMM_WORLD);
  int MyPID = comm.MyPID();
  if (MyPID == 0) std::cout << "\nTest: 2D elliptic solver, nodal discretization" << std::endl;

  // read parameter list
  std::string xmlFileName = "test/operator_diffusion_strip.xml";
  ParameterXMLFileReader xmlreader(xmlFileName);
  ParameterList plist = xmlreader.getParameters();

  // create a mesh
  ParameterList region_list = plist.get<Teuchos::ParameterList>("Regions");
  GeometricModelPtr gm = new GeometricModel(2, region_list, &comm);

  FrameworkPreference pref;
  pref.clear();
  pref.push_back(MSTK);

  MeshFactory meshfactory(&comm);
  meshfactory.preference(pref);
  RCP<const Mesh> mesh = meshfactory(-4.0, 0.0, 4.0, 1.0, 30, 1, gm);

  // model
  Analytic04 ana(mesh);

  // vector spaces
  int ncells = mesh->num_entities(AmanziMesh::CELL, AmanziMesh::OWNED);
  int ncells_wghost = mesh->num_entities(AmanziMesh::CELL, AmanziMesh::USED);
  int nfaces = mesh->num_entities(AmanziMesh::FACE, AmanziMesh::OWNED);
  int nfaces_wghost = mesh->num_entities(AmanziMesh::FACE, AmanziMesh::USED);

  Teuchos::RCP<CompositeVectorSpace> cell_space = Teuchos::rcp(new CompositeVectorSpace());
  cell_space->SetMesh(mesh)->SetComponent("cell",CELL,1)->SetGhosted();

  Teuchos::RCP<CompositeVectorSpace> face_space = Teuchos::rcp(new CompositeVectorSpace());
  face_space->SetMesh(mesh)->SetComponent("face",FACE,1)->SetGhosted();

  
  // create source
  CompositeVector source(*cell_space);
  Epetra_MultiVector& src = *source.ViewComponent("cell", false);
  src.PutScalar(0.0);

  for (int c = 0; c < ncells; c++) {
    const Point& xc = mesh->cell_centroid(c);
    double volume = mesh->cell_volume(c);
    src[0][c] = ana.source_exact(xc, 0.0) * volume;
  }

  Teuchos::RCP<OperatorSource> op1 = Teuchos::rcp(new OperatorSource(cell_space,0));
  op1->UpdateMatrices(source);

  // modify diffusion coefficient
  // -- since rho=mu=1.0, we do not need to scale the tensor coefficient.
  std::vector<WhetStone::Tensor> K;

  for (int c = 0; c != ncells; ++c) {
    const Point& xc = mesh->cell_centroid(c);
    const WhetStone::Tensor& Kc = ana.Tensor(xc, 0.0);
    K.push_back(Kc);
  }

  // -- scalar part
  Teuchos::RCP<CompositeVector> coef = Teuchos::rcp(new CompositeVector(*face_space));
  {
    Epetra_MultiVector& coef_faces = *coef->ViewComponent("face",false);

    for (int f = 0; f != nfaces; ++f) {
      const Point& xf = mesh->face_centroid(f);
      coef_faces[0][f] = ana.ScalarCoefficient(xf, 0.0);
    }
  }
  coef->ScatterMasterToGhosted("face");
  double rho(1.0), mu(1.0);

  // create boundary data
  Point xv(2);
  std::vector<int> bc_model(nfaces_wghost, Operators::OPERATOR_BC_NONE);
  std::vector<double> bc_value(nfaces_wghost);
  std::vector<double> bc_mixed;

  AmanziMesh::Entity_ID_List left;
  mesh->get_set_entities("Left side", AmanziMesh::FACE, AmanziMesh::USED, &left);
  for (int f=0; f!=left.size(); ++f) {
    bc_model[f] = Operators::OPERATOR_BC_DIRICHLET;
    mesh->face_centroid(f, &xv);
    bc_value[f] = ana.pressure_exact(xv, 0.0);
  }

  AmanziMesh::Entity_ID_List right;
  mesh->get_set_entities("Right side", AmanziMesh::FACE, AmanziMesh::USED, &right);
  for (int f=0; f!=right.size(); ++f) {
    bc_model[f] = Operators::OPERATOR_BC_DIRICHLET;
    mesh->face_centroid(f, &xv);
    bc_value[f] = ana.pressure_exact(xv, 0.0);
  }
  
  Teuchos::RCP<BCs> bc = Teuchos::rcp(new BCs(OPERATOR_BC_TYPE_FACE, bc_model, bc_value, bc_mixed));

  // create diffusion operator 
  ParameterList op_list = plist.get<Teuchos::ParameterList>("PK operator").sublist("diffusion operator");
  Point g(2);
  g[0] = 0.0;
  g[1] = 0.0;

  Teuchos::RCP<OperatorDiffusionTPFA> op2 = Teuchos::rcp(new OperatorDiffusionTPFA(*op1, op_list, bc));
  op2->SetUpwind(0);
  op2->SetGravity(g);
  
  const CompositeVectorSpace& cvs = op2->DomainMap();
  
  // populate the diffusion operator
  op2->Setup(K, coef, Teuchos::null);
  op2->UpdateMatrices(Teuchos::null, Teuchos::null);
  op2->ApplyBCs();
  op2->SymbolicAssembleMatrix(op2->schema_prec_dofs());
  op2->AssembleMatrix(op2->schema_prec_dofs());

  // create solution vector
  CompositeVector solution(*cell_space);
  {
    Epetra_MultiVector& p_cell = *solution.ViewComponent("cell");
    for (int c = 0; c < ncells; c++) {
      const Point& xc = mesh->cell_centroid(c);
      p_cell[0][c] = ana.pressure_exact(xc, 0.0);
    }
  }

  // check residual
  CompositeVector residual(*cell_space);
  //  int ierr = op1->ComputeNegativeResidual(solution, residual);
  solution.Print(std::cout);

  op2->rhs()->Print(std::cout);
  int ierr = op2->Apply(solution, residual);

  residual.Print(std::cout);
  residual.Update(1.0, *op2->rhs(), -1.0);
  residual.Print(std::cout);
  CHECK(!ierr);

  double res_norm(0.0);
  ierr |= residual.Norm2(&res_norm);
  CHECK(!ierr);
  CHECK_CLOSE(0.0, res_norm, 1.0e-8);
}


