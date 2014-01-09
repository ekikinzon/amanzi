#include <RSAMRdata.H>
#include <POROUSMEDIA_F.H> // For FORT_RICHARD_ALPHA

RSAMRdata::RSAMRdata(int slev, int nlevs, Layout& _layout, PMAmr* amrp, NLScontrol& nlsc)
  : RSdata(slev,nlevs,_layout,nlsc), pm_amr(amrp), source_eval_time(-1)
{
  nLevs = layout.NumLevels();
  pm.resize(pm_amr->finestLevel()+1,PArrayNoManage);
  for (int lev = 0; lev < pm.size(); lev++)  {
    pm.set(lev,dynamic_cast<PorousMedia*>(&pm_amr->getLevel(lev)));
  }

  // Get boundary conditions for pressure
  SetPressureBC(pm[0].get_desc_lst()[Press_Type].getBC(0));
}

void
RSAMRdata::SetDensity()
{
  density = PorousMedia::Density();
}

void
RSAMRdata::SetGravity()
{
  gravity.resize(BL_SPACEDIM,0);
  int gravity_dir = PorousMedia::getGravityDir();
  if (gravity_dir < BL_SPACEDIM) {
    gravity[gravity_dir] = PorousMedia::getGravity();
  }
}

void
RSAMRdata::SetViscosity()
{
  viscosity = PorousMedia::Viscosity();
}

const MFTower*
RSAMRdata::PropertyManager::UpdateProperty(Real t)
{
  UpdateDependents(t);

  if (t!=eval_time) {
    eval_time = t;

    MatFiller* matFiller = property_ctx.matFiller;
    bool ret = matFiller != 0;
    if (!ret) BoxLib::Abort("MatFiller not properly constructed");
    if (! (matFiller->Initialized()) ) BoxLib::Abort("MatFiller not properly constructed");
    int nLevs = property_dataPtr->NumLevels();
    for (int lev=0; lev<nLevs && ret; ++lev) {
      (*property_dataPtr)[lev].setVal(0);
      int dComp = 0;
      int nGrow = property_dataPtr->NComp();
      ret = matFiller->SetProperty(eval_time,lev,(*property_dataPtr)[lev],property_ctx.property_name,dComp,nGrow);
    }
    if (!ret) BoxLib::Abort("Failed to build property");
  }

  return property_dataPtr;
}

RSAMRdata::~RSAMRdata()
{
  delete Pold;
  delete Pnew;
  delete RhoSatNew;
  delete RhoSatOld;

  DarcyVelocity.clear();
  RichardCoefs.clear();
  KappaEC.clear();

  delete Alpha;
  delete Rhs;
  delete Porosity;
  delete SpecificStorage;
  delete PCapParams;
  delete Lambda;
  delete KappaCCavg;
  delete KappaCCdir;
  delete CoeffCC;
  delete Source;

  for (int d=0; d<BL_SPACEDIM; ++d) {
    ctmp[d].clear();
  }

  for (std::map<RSdata_Property,PropertyManager*>::iterator it=managed_properties.begin(),
         End=managed_properties.end(); it != End; ++it) {
    delete it->second;
  }
}

void
RSAMRdata::SetIsSaturated()
{
  int pm_model = PorousMedia::Model();
  is_saturated = (pm_model == PorousMedia::PM_STEADY_SATURATED)
    || (pm_model == PorousMedia::PM_SATURATED);
}

void
RSAMRdata::SetUpMemory(NLScontrol& nlsc)
{
  RSdata::SetUpMemory(nlsc);

  // These will be set prior to each solve call in order to support the case that
  // the unlying multifabs get changed between repeated calls to this solver (AMR
  // typically advances the state data by swapping the underlying pointers).
  RhoSatOld = 0;
  RhoSatNew = 0;
  Pnew = 0;
  Pold = 0;
  CoeffCC = 0;

  PArray<MultiFab> lambda(nLevs,PArrayNoManage);
  PArray<MultiFab> kappaccavg(nLevs,PArrayNoManage);
  PArray<MultiFab> kappaccdir(nLevs,PArrayNoManage);
  PArray<MultiFab> porosity(nLevs,PArrayNoManage);
  PArray<MultiFab> specific_storage(nLevs,PArrayNoManage);
  PArray<MultiFab> source(nLevs,PArrayNoManage);
  PArray<MultiFab> pcap_params(nLevs,PArrayNoManage);

  if (is_saturated) {
    upwind_krel = false;
  }

  for (int lev=0; lev<nLevs; ++lev) {
    if (!is_saturated) {
      lambda.set(lev,pm[lev].LambdaCC_Curr());
      pcap_params.set(lev,pm[lev].PCapParams());
    }
    porosity.set(lev,pm[lev].Porosity());

    if (is_saturated) {
      specific_storage.set(lev,pm[lev].SpecificStorage());
    }

    if (!nlsc.use_fd_jac || semi_analytic_J || variable_switch_saturation_threshold) {
        kappaccavg.set(lev,pm[lev].KappaCCavg());
    }

    source.set(lev,pm[lev].Source());
  }

  if (!nlsc.use_fd_jac || semi_analytic_J || variable_switch_saturation_threshold) {
    KappaCCavg = new MFTower(layout,kappaccavg,nLevs);
  }
  else {
    KappaCCavg = 0;
  }
  if (!is_saturated) {
    Lambda = new MFTower(layout,lambda,nLevs);
    PCapParams = new MFTower(layout,pcap_params,nLevs);
  } else {
    Lambda = 0;
    PCapParams = 0;
  }
  Porosity = new MFTower(layout,porosity,nLevs);

  if (is_saturated) {
    SpecificStorage = new MFTower(layout,specific_storage,nLevs);
  }
  else {
    SpecificStorage = 0;
  }
  Source = new MFTower(layout,source,nLevs);

  ctmp.resize(BL_SPACEDIM);
  Rhs = new MFTower(layout,IndexType(IntVect::TheZeroVector()),1,1,nLevs);
  Alpha = new MFTower(layout,IndexType(IntVect::TheZeroVector()),1,1,nLevs);
  
  RichardCoefs.resize(BL_SPACEDIM,PArrayManage);
  DarcyVelocity.resize(BL_SPACEDIM,PArrayManage);
  for (int d=0; d<BL_SPACEDIM; ++d) {
    PArray<MultiFab> utmp(nLevs,PArrayNoManage);
    ctmp[d].resize(nLevs,PArrayManage);
    for (int lev=0; lev<nLevs; ++lev) {
      BoxArray ba = BoxArray(kappaccavg[lev].boxArray()).surroundingNodes(d);
      ctmp[d].set(lev, new MultiFab(ba,1,0));
      utmp.set(lev,&(pm[lev].UMac_Curr()[d]));
    }
    DarcyVelocity.set(d, new MFTower(layout,utmp,nLevs));
    RichardCoefs.set(d, new MFTower(layout,ctmp[d],nLevs));
    utmp.clear();
  }

  KappaCCdir = new MFTower(layout,IndexType(IntVect::TheZeroVector()),BL_SPACEDIM,1,nLevs);

  // Setup property managers
  PropertyManagerCtx kappaCCdir_ctx;
  kappaCCdir_ctx.matFiller = pm_amr->GetMatFiller();
  kappaCCdir_ctx.property_name = "permeability";
  std::set<PropertyManager*> kappaCCdir_dep; // Empty
  managed_properties[RSdata_KappaCCdir] = new PropertyManager(KappaCCdir,kappaCCdir_dep,kappaCCdir_ctx);

  if (upwind_krel) {
    KappaCCdir = 0;
    CoeffCC = 0;

    kappaEC.resize(BL_SPACEDIM);
    for (int d=0; d<BL_SPACEDIM; ++d) {
      kappaEC[d].resize(nLevs,PArrayNoManage);
    }
    for (int lev=0; lev<nLevs; ++lev) {
      for (int d=0; d<BL_SPACEDIM; ++d) {
        kappaEC[d].set(lev,&(pm[lev].KappaEC()[d]));
      }
    }
    KappaEC.resize(BL_SPACEDIM, PArrayManage);
    for (int d=0; d<BL_SPACEDIM; ++d) {
      if (KappaEC.defined(d)) {
	KappaEC.clear(d);
      }
      KappaEC.set(d, new MFTower(layout,kappaEC[d],nLevs));
    }
  }
  else {
    CoeffCC    = new MFTower(layout,IndexType(IntVect::TheZeroVector()),BL_SPACEDIM,1,nLevs);
  }
}

void
RSAMRdata::ResetRhoSat()
{
  PArray<MultiFab> S_new(nLevs,PArrayNoManage);
  PArray<MultiFab> S_old(nLevs,PArrayNoManage);
  PArray<MultiFab> P_new(nLevs,PArrayNoManage);
  PArray<MultiFab> P_old(nLevs,PArrayNoManage);
  
  for (int lev=0; lev<nLevs; ++lev) {
    S_new.set(lev,&(pm[lev].get_new_data(State_Type)));
    S_old.set(lev,&(pm[lev].get_old_data(State_Type)));
    P_new.set(lev,&(pm[lev].get_new_data(Press_Type)));
    P_old.set(lev,&(pm[lev].get_old_data(Press_Type)));
  }

  delete RhoSatOld; RhoSatOld = new MFTower(layout,S_old,nLevs);
  delete RhoSatNew; RhoSatNew = new MFTower(layout,S_new,nLevs);
  delete Pnew; Pnew = new MFTower(layout,P_new,nLevs);
  delete Pold; Pold = new MFTower(layout,P_old,nLevs);
}

void 
RSAMRdata::SetInflowVelocity(PArray<MFTower>& velocity,
			     Real             t)
{
  for (int d=0; d<BL_SPACEDIM; ++d) {            
    BL_ASSERT(layout.IsCompatible(velocity[d]));
  }

  const Array<Geometry>& geomArray = layout.GeomArray();

  FArrayBox inflow, mask;
  for (OrientationIter oitr; oitr; ++oitr) {
    Orientation face = oitr();
    int dir = face.coordDir();
    for (int lev=0; lev<nLevs; ++lev) {
      MultiFab& uld = velocity[dir][lev];
      if (pm[lev].get_inflow_velocity(face,inflow,mask,t)) {
	int shift = ( face.isHigh() ? -1 : +1 );
	inflow.shiftHalf(dir,shift);
	mask.shiftHalf(dir,shift);
	for (MFIter mfi(uld); mfi.isValid(); ++mfi) {
	  FArrayBox& u = uld[mfi];
	  Box ovlp = inflow.box() & u.box();
          if (ovlp.ok()) {
            for (IntVect iv=ovlp.smallEnd(), End=ovlp.bigEnd(); iv<=End; ovlp.next(iv)) {
              if (mask(iv,0) != 0) {
                u(iv,0) = inflow(iv,0);
              }
            }
          }
	}
      }
    }
  }
}


void
RSAMRdata::FillStateBndry (MFTower& press,
                           Real time)
{
  if (press.NGrow() == 0)
    return;

  int state_indx = Press_Type;
  int src_comp = 0;
  int num_comp = 1;
  for (int lev=0; lev<nLevs; ++lev) {
    MultiFab& mf(press[lev]);
    const BoxArray& grids = mf.boxArray();
    for (FillPatchIterator fpi(pm[lev],mf,mf.nGrow(),time,state_indx,src_comp,num_comp);
       fpi.isValid();
       ++fpi) {
      BoxList boxes = BoxLib::boxDiff(fpi().box(),grids[fpi.index()]);
      for (BoxList::iterator bli = boxes.begin(); bli != boxes.end(); ++bli) {
        mf[fpi.index()].copy(fpi(),*bli,0,*bli,src_comp,num_comp);
      }
    }
  }
}

void
RSAMRdata::calcInvPressure (MFTower&       N,
			    const MFTower& P) const
{
  for (int lev=0; lev<nLevs; ++lev) {
    pm[lev].calcInvPressure(N[lev],P[lev]);
  }
}

void 
RSAMRdata::calcLambda (MFTower&       lbd,
		       const MFTower& N)
{
  for (int lev=0; lev<nLevs; ++lev) {
    pm[lev].calcLambda(&(lbd)[lev],N[lev]);
  }
}

void
RSAMRdata::calcRichardAlpha(MFTower&       alpha,
                            const MFTower& rhoSat,
                            Real           t)
{
  const MFTower* kappaCCdir = GetKappaCCdir(t);

  for (int lev=0; lev<nLevs; ++lev) {
    const MultiFab& rs(rhoSat[lev]);
    for (MFIter mfi(rs); mfi.isValid(); ++mfi) {
      const Box& vbox = mfi.validbox();

      const FArrayBox& pofab = (*Porosity)[lev][mfi];
      //FArrayBox& kcfab = (*KappaCCavg()[lev][mfi];  // FIXME: Original version uses avg, probably not correct
      const FArrayBox& kcfab = (*kappaCCdir)[lev][mfi];
      const FArrayBox& cpfab = (*PCapParams)[lev][mfi];
      const int n_cp_coef = cpfab.nComp();

      const FArrayBox& nfab = rs[mfi];
      FArrayBox& afab = alpha[lev][mfi];
      
      FORT_RICHARD_ALPHA(afab.dataPtr(), ARLIM(afab.loVect()), ARLIM(afab.hiVect()),
			 nfab.dataPtr(), ARLIM(nfab.loVect()),ARLIM(nfab.hiVect()),
			 pofab.dataPtr(), ARLIM(pofab.loVect()),ARLIM(pofab.hiVect()),
			 kcfab.dataPtr(), ARLIM(kcfab.loVect()), ARLIM(kcfab.hiVect()),
			 cpfab.dataPtr(), ARLIM(cpfab.loVect()), ARLIM(cpfab.hiVect()), &n_cp_coef,
			 vbox.loVect(), vbox.hiVect());
      
    }
  }
}

// These next two are icky, but simply forwarded from the original implementation that uses the J in PM
// These aren't actually supported completely anymore anyway, and should probably be removed soon.
Array<int>&
RSAMRdata::rinflowBCLo()
{
  return pm[0].rinflowBCLo();
}

Array<int>&
RSAMRdata::rinflowBCHi()
{
  return pm[0].rinflowBCHi();
}

const MFTower*
RSAMRdata::GetKappaCCdir(Real t)
{
  PropertyManager* pmgr = managed_properties[RSdata_KappaCCdir];
  if (pmgr==0) {
    BoxLib::Abort("Managed property not properly registered");
  }
  return pmgr->UpdateProperty(t);
}

const PArray<MFTower>&
RSAMRdata::GetKappaEC(Real t)
{
  // FIXME: Punt on time-dependent for now...need to bring in averaging of CC values
  return KappaEC;
}

const MFTower*
RSAMRdata::GetSource(Real t)
{
  if (t != source_eval_time) {
    source_eval_time = t;
    int nGrow = 0;
    int strt_comp = 0;
    int num_comp = 1;
    bool do_rho_scale = true;
    for (int lev=0; lev<nLevs; ++lev) {
      pm[lev].getForce((*Source)[lev],nGrow,strt_comp,num_comp,time,do_rho_scale);
    }
  }
  return Source;
}

