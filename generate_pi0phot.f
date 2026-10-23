	subroutine generate_pi0phot(main,vertex,orig,success)

! Generate hard-bremsstrahlung gamma p -> pi0 p followed by pi0 -> gamma gamma.
! The pion is stored in the E-arm event record and the recoil proton in the
! P-arm record.  The two photons are stored in NTUP for calorimeter transport.

	USE structureModule
	implicit none
	include 'simulate.inc'

	type(event_main)::main
	type(event)::vertex,orig
	logical success,okkin
	real*8 grnd,egamma,costh,phicm,pion(4),proton(4)
	real*8 ppi,pp,roots,sgev,kgcm,ppicm,dsdt,dsdo,flux
	real*8 pi0phot_dsdt,pi0phot_brem_flux

	success=.false.
	main%gen_weight=1.0
	main%jacobian=1.0
	main%SF_weight=1.0

	egamma=pi0phot_egamma_min+grnd()*(pi0phot_egamma_max-pi0phot_egamma_min)
	if (egamma.ge.vertex%Ein) return
	costh=pi0phot_costh_min+grnd()*(pi0phot_costh_max-pi0phot_costh_min)
	phicm=pi0phot_phi_min+grnd()*(pi0phot_phi_max-pi0phot_phi_min)
	ntup%pi0phot_egamma=egamma
	ntup%pi0phot_costh=costh
	ntup%pi0phot_phicm=phicm

	call pi0phot_twobody(egamma,costh,phicm,pion,proton,okkin)
	if (.not.okkin) return

	ppi=sqrt(pion(2)**2+pion(3)**2+pion(4)**2)
	pp=sqrt(proton(2)**2+proton(3)**2+proton(4)**2)
	if (ppi.le.0.0 .or. pp.le.0.0) return

! Store the pion on the calorimeter (E) side.
	vertex%e%E=pion(1)
	vertex%e%P=ppi
	vertex%e%theta=acos(max(-1.0,min(1.0,pion(4)/ppi)))
	vertex%e%phi=atan2(pion(3),pion(2))
	if (vertex%e%phi.lt.0.0) vertex%e%phi=vertex%e%phi+2.0*pi
	vertex%ue%x=pion(2)/ppi
	vertex%ue%y=pion(3)/ppi
	vertex%ue%z=pion(4)/ppi
	call pi0phot_lab_to_spec(vertex%ue,spec%e%theta,spec%e%phi,
     >     vertex%e%xptar,vertex%e%yptar,okkin)
	if (.not.okkin) return
	vertex%e%delta=100.0*(ppi-spec%e%P)/spec%e%P

! Store the recoil proton on the spectrometer (P) side.
	vertex%p%E=proton(1)
	vertex%p%P=pp
	vertex%p%theta=acos(max(-1.0,min(1.0,proton(4)/pp)))
	vertex%p%phi=atan2(proton(3),proton(2))
	if (vertex%p%phi.lt.0.0) vertex%p%phi=vertex%p%phi+2.0*pi
	vertex%up%x=proton(2)/pp
	vertex%up%y=proton(3)/pp
	vertex%up%z=proton(4)/pp
	call pi0phot_lab_to_spec(vertex%up,spec%p%theta,spec%p%phi,
     >     vertex%p%xptar,vertex%p%yptar,okkin)
	if (.not.okkin) return
	vertex%p%delta=100.0*(pp-spec%p%P)/spec%p%P

! Real-photon invariants and convenient event fields.
	vertex%nu=egamma
	vertex%q=egamma
	vertex%Q2=0.0
	vertex%W=sqrt(Mp*Mp+2.0*Mp*egamma)
	vertex%epsilon=0.0
	vertex%uq%x=0.0
	vertex%uq%y=0.0
	vertex%uq%z=1.0
	vertex%Em=0.0
	vertex%Pm=0.0
	vertex%Emiss=0.0
	vertex%Pmiss=0.0
	vertex%Trec=proton(1)-Mp
	main%w=vertex%W
	main%wcm=vertex%W
	main%thetacm=acos(costh)
	main%phicm=phicm

! Cross section times photon flux.  pi0phot_dsdt is nb/GeV^2;
! dOmega_cm/dt = pi/(k_cm*p_pi_cm).  Convert nb to microbarn.
	dsdt=pi0phot_dsdt(egamma,costh,pi0phot_scale_exponent)
	sgev=Mp*Mp/1.0d6+2.0d0*(Mp/1000.0d0)*(egamma/1000.0d0)
	roots=sqrt(sgev)
	kgcm=(sgev-(Mp/1000.0d0)**2)/(2.0d0*roots)
	ppicm=sqrt(max(0.0d0,((sgev+(Mpi0/1000.0d0)**2-
     > (Mp/1000.0d0)**2)/(2.0d0*roots))**2-(Mpi0/1000.0d0)**2))
	dsdo=dsdt*kgcm*ppicm/pi/1000.0d0
	flux=pi0phot_brem_flux(vertex%Ein,egamma,pi0phot_radlen)
	main%sigcc=flux*dsdo
	main%sigcc_recon=main%sigcc
	ntup%sigcm=dsdo
	if (main%sigcc.le.0.0) return

	call pi0phot_decay(pion,ntup%gamma1,ntup%gamma2,okkin)
	if (.not.okkin) return

! The special generator returns before complete_ev, so initialize the
! outgoing-proton target path here for energy loss and multiple scattering.
	main%target%Eloss(2)=0.0
	main%target%teff(2)=0.0
	call trip_thru_target(3,main%target%z-targ%zoffset,vertex%p%E,
     >   vertex%p%theta,main%target%Eloss(3),main%target%teff(3),Mh,1)
	if (.not.using_Eloss) main%target%Eloss(3)=0.0
	orig=vertex
	success=.true.
	return
	end


	subroutine calculate_pi0phot_central(central,vertex0)

	USE structureModule
	implicit none
	include 'simulate.inc'
	type(event_central)::central
	type(event)::vertex0
	real*8 egamma,costh,phi,pion(4),proton(4),ppi,pp
	real*8 dsdt,dsdo,flux,sgev,roots,kgcm,ppicm
	real*8 pi0phot_dsdt,pi0phot_brem_flux
	logical ok

	egamma=0.5*(pi0phot_egamma_min+pi0phot_egamma_max)
	costh=0.5*(pi0phot_costh_min+pi0phot_costh_max)
	phi=0.5*(pi0phot_phi_min+pi0phot_phi_max)
	call pi0phot_twobody(egamma,costh,phi,pion,proton,ok)
	if (.not.ok) stop 'Failed to calculate central pi0 photoproduction event'
	ppi=sqrt(pion(2)**2+pion(3)**2+pion(4)**2)
	pp=sqrt(proton(2)**2+proton(3)**2+proton(4)**2)
	central%Q2=0.0
	central%q=egamma
	central%nu=egamma
	central%Em=0.0
	central%Pm=0.0
	central%W=sqrt(Mp*Mp+2.0*Mp*egamma)
	central%MM=0.0
	central%e%delta=100.0*(ppi-spec%e%P)/spec%e%P
	central%e%xptar=0.0
	central%e%yptar=0.0
	central%p%delta=100.0*(pp-spec%p%P)/spec%p%P
	central%p%xptar=0.0
	central%p%yptar=0.0
	vertex0%Ein=Ebeam
	vertex0%nu=egamma
	vertex0%q=egamma
	vertex0%Q2=0.0
	vertex0%W=central%W

	dsdt=pi0phot_dsdt(egamma,costh,pi0phot_scale_exponent)
	sgev=(Mp/1000.0d0)**2+2.0d0*(Mp/1000.0d0)*(egamma/1000.0d0)
	roots=sqrt(sgev)
	kgcm=(sgev-(Mp/1000.0d0)**2)/(2.0d0*roots)
	ppicm=sqrt(max(0.0d0,((sgev+(Mpi0/1000.0d0)**2-
     > (Mp/1000.0d0)**2)/(2.0d0*roots))**2-(Mpi0/1000.0d0)**2))
	dsdo=dsdt*kgcm*ppicm/pi/1000.0d0
	flux=pi0phot_brem_flux(Ebeam,egamma,pi0phot_radlen)
	central%sigcc=flux*dsdo
	return
	end


	subroutine transport_pi0phot_calo(main,recon,success)

! Transport both pi0 decay photons through the E-side calorimeter after the
! recoil proton has passed through the ordinary P-arm Monte Carlo.

	USE structureModule
	implicit none
	include 'simulate.inc'
	type(event_main)::main
	type(event)::recon
	logical success,ok1,ok2
	real*8 xb,yb,zb,x,y,z,dx,dy,xfp,yfp,dxfp,dyfp
	real*8 ex,ey,ez,pathlen,tmpfact,delta
	integer i

	success=.false.
	if (.not.using_E_arm_montecarlo) then
	  main%SP%e%delta=recon%e%delta
	  main%SP%e%xptar=recon%e%xptar
	  main%SP%e%yptar=recon%e%yptar
	  main%RECON%e=main%SP%e
	  success=.true.
	  return
	endif

	xb=-main%target%y-spec%e%offset%x
	yb=-main%target%x*spec%e%cos_th-
     >   main%target%z*spec%e%sin_th*sin(spec%e%phi)-spec%e%offset%y
	zb= main%target%z*spec%e%cos_th+
     >   main%target%x*spec%e%sin_th*sin(spec%e%phi)-spec%e%offset%z

! Drift the vertex to the target-center plane used by mc_calo.
	main%SP%e%delta=recon%e%delta
	main%SP%e%xptar=recon%e%xptar
	main%SP%e%yptar=recon%e%yptar
	main%SP%e%z=yb-zb*recon%e%yptar

	ok1=.false.
	ok2=.false.
	ntup%xcal_gamma1=-1.0d10
	ntup%ycal_gamma1=-1.0d10
	ntup%xcal_gamma2=-1.0d10
	ntup%ycal_gamma2=-1.0d10
	ntup%pi0phot_xpred=-1.0d10
	ntup%pi0phot_ypred=-1.0d10
	ntup%pi0phot_pelastic=-1.0d10
	ntup%pi0phot_dpelastic=-1.0d10
	ntup%pi0phot_dx1=-1.0d10
	ntup%pi0phot_dy1=-1.0d10
	ntup%pi0phot_dx2=-1.0d10
	ntup%pi0phot_dy2=-1.0d10
	call pi0phot_elastic_prediction(main,recon)
	pathlen=0.0
	tmpfact=0.0

	do i=1,2
	  if (i.eq.1) then
	    ex=ntup%gamma1(2)
	    if (electron_arm.eq.8) then
	      ey=ntup%gamma1(3)*spec%e%cos_th-ntup%gamma1(4)*spec%e%sin_th
	      ez=ntup%gamma1(3)*spec%e%sin_th+ntup%gamma1(4)*spec%e%cos_th
	    else
	      ey=ntup%gamma1(3)*spec%e%cos_th+ntup%gamma1(4)*spec%e%sin_th
	      ez=-ntup%gamma1(3)*spec%e%sin_th+ntup%gamma1(4)*spec%e%cos_th
	    endif
	    delta=100.0*(ntup%gamma1(1)-spec%e%P)/spec%e%P
	  else
	    ex=ntup%gamma2(2)
	    if (electron_arm.eq.8) then
	      ey=ntup%gamma2(3)*spec%e%cos_th-ntup%gamma2(4)*spec%e%sin_th
	      ez=ntup%gamma2(3)*spec%e%sin_th+ntup%gamma2(4)*spec%e%cos_th
	    else
	      ey=ntup%gamma2(3)*spec%e%cos_th+ntup%gamma2(4)*spec%e%sin_th
	      ez=-ntup%gamma2(3)*spec%e%sin_th+ntup%gamma2(4)*spec%e%cos_th
	    endif
	    delta=100.0*(ntup%gamma2(1)-spec%e%P)/spec%e%P
	  endif
	  if (ez.le.0.0) cycle
	  dx=ex/ez-spec%e%offset%xptar
	  dy=ey/ez-spec%e%offset%yptar
	  x=xb-zb*dx
	  y=yb-zb*dy
	  z=0.0
	  if (i.eq.1) then
	    call mc_calo(spec%e%P,spec%e%theta,delta,x,y,z,dx,dy,
     >       xfp,dxfp,yfp,dyfp,0.0d0,.false.,.false.,.false.,
     >       tmpfact,-main%target%rasterx,-main%target%rastery,ok1,
     >       pathlen,.false.,0.0d0,electron_arm,drift_to_cal,
     >       calo_half_width,calo_half_height)
	    if (ok1) then
	      ntup%xcal_gamma1=xfp
	      ntup%ycal_gamma1=yfp
	      ntup%pi0phot_dx1=xfp-ntup%pi0phot_xpred
	      ntup%pi0phot_dy1=yfp-ntup%pi0phot_ypred
	    endif
	  else
	    call mc_calo(spec%e%P,spec%e%theta,delta,x,y,z,dx,dy,
     >       xfp,dxfp,yfp,dyfp,0.0d0,.false.,.false.,.false.,
     >       tmpfact,-main%target%rasterx,-main%target%rastery,ok2,
     >       pathlen,.false.,0.0d0,electron_arm,drift_to_cal,
     >       calo_half_width,calo_half_height)
	    if (ok2) then
	      ntup%xcal_gamma2=xfp
	      ntup%ycal_gamma2=yfp
	      ntup%pi0phot_dx2=xfp-ntup%pi0phot_xpred
	      ntup%pi0phot_dy2=yfp-ntup%pi0phot_ypred
	    endif
	  endif
	enddo

	if (pizero_ngamma.eq.1) success=ok1.or.ok2
	if (pizero_ngamma.eq.2) success=ok1.and.ok2
	if (.not.success) return
	main%RECON%e=main%SP%e
	return
	end


	subroutine pi0phot_elastic_prediction(main,recon)

! Predict the calorimeter intercept from the reconstructed proton direction
! under elastic ep kinematics, and form the HMS angle-momentum residual.

	USE structureModule
	implicit none
	include 'simulate.inc'
	type(event_main)::main
	type(event)::recon
	real*8 den,pel,px,py,pz,ee,ex,ey,ez,xb,yb,zb,dx,dy

	den=(Ebeam+Mp)**2-Ebeam**2*cos(recon%p%theta)**2
	if (den.le.0.0) return
	pel=2.0*Mp*Ebeam*(Ebeam+Mp)*cos(recon%p%theta)/den
	if (pel.le.0.0) return
	ntup%pi0phot_pelastic=pel
	ntup%pi0phot_dpelastic=100.0*(recon%p%P-pel)/spec%p%P

	px=pel*sin(recon%p%theta)*cos(recon%p%phi)
	py=pel*sin(recon%p%theta)*sin(recon%p%phi)
	pz=pel*cos(recon%p%theta)
	ex=-px
	ey=-py
	ez=Ebeam-pz
	ee=sqrt(ex*ex+ey*ey+ez*ez)
	if (ee.le.0.0) return

	if (electron_arm.eq.8) then
	  py=ey*spec%e%cos_th-ez*spec%e%sin_th
	  pz=ey*spec%e%sin_th+ez*spec%e%cos_th
	else
	  py=ey*spec%e%cos_th+ez*spec%e%sin_th
	  pz=-ey*spec%e%sin_th+ez*spec%e%cos_th
	endif
	if (pz.le.0.0) return
	dx=ex/pz-spec%e%offset%xptar
	dy=py/pz-spec%e%offset%yptar
	xb=-main%target%y-spec%e%offset%x
	yb=-main%target%x*spec%e%cos_th-
     >   main%target%z*spec%e%sin_th*sin(spec%e%phi)-spec%e%offset%y
	zb= main%target%z*spec%e%cos_th+
     >   main%target%x*spec%e%sin_th*sin(spec%e%phi)-spec%e%offset%z
	ntup%pi0phot_xpred=xb+(drift_to_cal-zb)*dx
	ntup%pi0phot_ypred=yb+(drift_to_cal-zb)*dy
	return
	end


	subroutine pi0phot_lab_to_spec(u,theta0,phi0,xptar,yptar,success)

	USE structureModule
	implicit none
	type(Cartesian)::u
	real*8 theta0,phi0,xptar,yptar,uzspec
	logical success

	uzspec=u%y*sin(theta0)*sin(phi0)+u%z*cos(theta0)
	success=uzspec.gt.0.0
	if (.not.success) return
	xptar=u%x/uzspec
	yptar=(u%y*cos(theta0)-u%z*sin(theta0)*sin(phi0))/uzspec
	return
	end


	subroutine pi0phot_decay(pion,gamma1,gamma2,success)

! Isotropic two-photon decay with a general three-dimensional boost.

	implicit none
	include 'constants.inc'
	real*8 pion(4),gamma1(4),gamma2(4)
	real*8 grnd,costh,sinth,phi,e0,q1(3),q2(3),beta(3)
	real*8 beta2,gam,bq,factor
	integer i
	logical success

	success=.false.
	if (pion(1).le.Mpi0) return
	e0=Mpi0/2.0
	costh=2.0*grnd()-1.0
	sinth=sqrt(max(0.0,1.0-costh*costh))
	phi=2.0*pi*grnd()
	q1(1)=e0*sinth*cos(phi)
	q1(2)=e0*sinth*sin(phi)
	q1(3)=e0*costh
	do i=1,3
	  q2(i)=-q1(i)
	  beta(i)=pion(i+1)/pion(1)
	enddo
	beta2=beta(1)**2+beta(2)**2+beta(3)**2
	if (beta2.le.0.0 .or. beta2.ge.1.0) return
	gam=1.0/sqrt(1.0-beta2)

	bq=beta(1)*q1(1)+beta(2)*q1(2)+beta(3)*q1(3)
	gamma1(1)=gam*(e0+bq)
	factor=((gam-1.0)*bq/beta2+gam*e0)
	do i=1,3
	  gamma1(i+1)=q1(i)+factor*beta(i)
	enddo

	bq=beta(1)*q2(1)+beta(2)*q2(2)+beta(3)*q2(3)
	gamma2(1)=gam*(e0+bq)
	factor=((gam-1.0)*bq/beta2+gam*e0)
	do i=1,3
	  gamma2(i+1)=q2(i)+factor*beta(i)
	enddo
	success=.true.
	return
	end
