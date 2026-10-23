	program test_pi0phot
	implicit none
	include 'constants.inc'
	real*8 ds,flux,pion(4),proton(4),de,dx,dy,dz
	real*8 pi0phot_dsdt,pi0phot_brem_flux,pi0phot_log_interp
	real*8 tx(2),ty(2)
	real*8 ebeam,egamma,costh,exponent,radlen,phi
	logical ok

	ebeam=10700.0
	egamma=10700.0
	costh=-0.1436
	exponent=7.0
	radlen=0.0175
	phi=1.5707963267948966
	ds=pi0phot_dsdt(egamma,costh,exponent)
	write(6,*) 'endpoint dsdt =',ds
	tx(1)=-0.203
	tx(2)=-0.055
	ty(1)=6.57
	ty(2)=6.78
	write(6,*) 'test interpolation =',pi0phot_log_interp(costh,tx,ty,2)
	if (abs(ds-0.01159d0).gt.0.00020d0) stop 1
	costh=0.8
	if (pi0phot_dsdt(egamma,costh,exponent).ne.0.0) stop 2
	costh=-0.1436
	exponent=6.0
	if (abs(pi0phot_dsdt(egamma,costh,exponent)/ds-
     > 2.0418d0).gt.0.01d0) stop 3

	egamma=10000.0
	flux=pi0phot_brem_flux(ebeam,egamma,radlen)
	if (flux.le.0.0d0) stop 4
	egamma=10700.0
	if (pi0phot_brem_flux(ebeam,egamma,radlen).ne.0.0) stop 5

	costh=-0.1436
	call pi0phot_twobody(egamma,costh,phi,
     > pion,proton,ok)
	if (.not.ok) stop 6
	de=abs(egamma+Mp-pion(1)-proton(1))
	dx=abs(pion(2)+proton(2))
	dy=abs(pion(3)+proton(3))
	dz=abs(10700.0d0-pion(4)-proton(4))
	write(6,*) 'four-momentum residuals =',de,dx,dy,dz
	if (max(de,dx,dy,dz).gt.1.0d-6) stop 7
	if (abs(sqrt(proton(2)**2+proton(3)**2+proton(4)**2)-
     > 6728.6d0).gt.1.0d0) stop 8
	write(6,*) 'pi0phot physics checks passed',ds,flux
	end
