	real*8 function pi0phot_brem_flux(ebeam,egamma,radlen)

! Thin-radiator Bethe-Heitler photon flux per incident electron and MeV.
! EBEAM and EGAMMA are in MeV; RADLEN is the effective upstream
! thickness in radiation lengths.

	implicit none
	real*8 ebeam,egamma,radlen,y

	pi0phot_brem_flux=0.0
	if (ebeam.le.0.0 .or. egamma.le.0.0 .or. egamma.ge.ebeam) return
	if (radlen.le.0.0) return
	y=egamma/ebeam
	pi0phot_brem_flux=radlen/egamma*(4.0/3.0-4.0*y/3.0+y*y)
	return
	end


	real*8 function pi0phot_dsdt(egamma,costh,scale_exponent)

! Anderson et al. wide-angle gamma p -> pi0 p model.
! Returns d sigma/dt in nb/GeV^2.  EGAMMA is in MeV and COSTH is the
! pion center-of-mass angle cosine.  The n=7 baseline is the geometric
! mean of independent 4 and 5 GeV extrapolations.  Other exponents are
! pivoted at the 5 GeV value, as documented in
! gep_hallc_reboot/pi0_cross_section_model.md.

	implicit none
	integer i
	real*8 egamma,costh,scale_exponent
	real*8 mpgev,s,s4,s5,x4,x5,base,ratio,pi0phot_log_interp
	real*8 c4(15),v4(15),c5(14),v5(14)
	parameter(mpgev=0.9382720813d0)
	data c4 /-0.785d0,-0.654d0,-0.508d0,-0.356d0,-0.203d0,
     > -0.055d0,0.025d0,0.084d0,0.140d0,0.211d0,0.255d0,
     > 0.329d0,0.435d0,0.529d0,0.644d0/
	data v4 /24.1d0,22.5d0,8.06d0,4.25d0,6.57d0,6.78d0,
     > 5.97d0,3.87d0,4.62d0,1.83d0,4.93d0,15.9d0,40.1d0,
     > 102.9d0,184.2d0/
	data c5 /-0.751d0,-0.626d0,-0.490d0,-0.304d0,-0.167d0,
     > -0.035d0,0.048d0,0.166d0,0.238d0,0.386d0,0.431d0,
     > 0.562d0,0.671d0,0.743d0/
	data v5 /8.4d0,4.46d0,1.78d0,2.45d0,1.87d0,1.61d0,
     > 1.42d0,1.91d0,1.71d0,2.78d0,3.56d0,27.5d0,132.6d0,
     > 179.6d0/

	pi0phot_dsdt=0.0
	if (egamma.le.0.0) return
! Require overlap of both measured angular distributions.  Do not silently
! extrapolate the angular function.
	if (costh.lt.-0.751d0 .or. costh.gt.0.644d0) return
	x4=pi0phot_log_interp(costh,c4,v4,15)
	x5=pi0phot_log_interp(costh,c5,v5,14)
	if (x4.le.0.0 .or. x5.le.0.0) return
	s=mpgev*mpgev+2.0d0*mpgev*egamma/1000.0d0
	s4=mpgev*mpgev+2.0d0*mpgev*4.0d0
	s5=mpgev*mpgev+2.0d0*mpgev*5.0d0
	base=sqrt(x4*(s/s4)**(-7.0d0)*x5*(s/s5)**(-7.0d0))
	ratio=s/s5
	pi0phot_dsdt=base*ratio**(-(scale_exponent-7.0d0))
	return
	end


	real*8 function pi0phot_log_interp(x,xv,yv,n)

	implicit none
	integer n,i
	real*8 x,xv(n),yv(n),f

	pi0phot_log_interp=0.0
	if (x.lt.xv(1) .or. x.gt.xv(n)) return
	do i=1,n-1
	  if (x.ge.xv(i) .and. x.le.xv(i+1)) then
	    f=(x-xv(i))/(xv(i+1)-xv(i))
	    pi0phot_log_interp=exp(log(yv(i))+f*(log(yv(i+1))-log(yv(i))))
	    return
	  endif
	enddo
	return
	end


	subroutine pi0phot_twobody(egamma,costh,phi,pion,proton,success)

! Exact gamma p -> pi0 p two-body kinematics.  Input/output energies and
! momenta are in MeV.  PION and PROTON are (E,px,py,pz) in the SIMC lab
! axes: x down, y beam-left, z downstream.

	implicit none
	include 'constants.inc'
	real*8 egamma,costh,phi,pion(4),proton(4)
	real*8 s,roots,kgcm,epicm,ppicm,epcm,ct,st,cp,sp
	real*8 beta,gamma,pzpi,pzpr
	logical success

	success=.false.
	if (egamma.le.0.0 .or. abs(costh).gt.1.0) return
	s=Mp*Mp+2.0*Mp*egamma
	if (s.le.(Mp+Mpi0)**2) return
	roots=sqrt(s)
	kgcm=(s-Mp*Mp)/(2.0*roots)
	epicm=(s+Mpi0*Mpi0-Mp*Mp)/(2.0*roots)
	ppicm=sqrt(max(0.0,epicm*epicm-Mpi0*Mpi0))
	epcm=(s+Mp*Mp-Mpi0*Mpi0)/(2.0*roots)
	ct=costh
	st=sqrt(max(0.0,1.0-ct*ct))
	cp=cos(phi)
	sp=sin(phi)
	beta=egamma/(egamma+Mp)
	gamma=(egamma+Mp)/roots

	pion(2)=ppicm*st*cp
	pion(3)=ppicm*st*sp
	pzpi=ppicm*ct
	pion(1)=gamma*(epicm+beta*pzpi)
	pion(4)=gamma*(pzpi+beta*epicm)

	proton(2)=-pion(2)
	proton(3)=-pion(3)
	pzpr=-pzpi
	proton(1)=gamma*(epcm+beta*pzpr)
	proton(4)=gamma*(pzpr+beta*epcm)
	success=.true.
	return
	end
