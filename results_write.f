	subroutine results_ntu_write(main,vertex,orig,recon,success)

	USE structureModule
	implicit none
	include 'radc.inc'
	include 'hbook.inc'
	include 'simulate.inc'

	integer i,m
	real*8	ntu(96)
	type(event_main):: main
	type(event):: vertex, orig, recon

!local (e,e'p) calculations:
	real*8 poftheta		!p as calculated from theta, assuming elastic.
	real*8 corrsing		!'corrected singles' for elastic
	real*8 Pm_Heepx,Pm_Heepy,Pm_Heepz	!Pm components for Heepcheck.

!local (e,e'pi/K) calculations:
!	real*8 t		!t
	real*8 dummy

	logical success

	if (debug(2)) write(6,*)'r_ntu_write: entering'

!If event did not make it thru spectrometers, return.  Later, we will want to
! add option to write event even if it failed when doing_phsp.

	if (.not.success) return
	
	if (doing_hyd_elast .or. doing_deuterium .or. doing_heavy) then
	  poftheta = Mp*Ebeam / (2*ebeam*sin(recon%e%theta/2.)**2 + Mp)
	  corrsing = recon%e%P - poftheta
	  Pm_Heepz = -(recon%Pmy*recon%uq%y+recon%Pmz*recon%uq%z)
     >		/ sqrt(recon%uq%y**2+recon%uq%z**2)
	  Pm_Heepy =  (recon%Pmz*recon%uq%y-recon%Pmy*recon%uq%z)
     >		/ sqrt(recon%uq%y**2+recon%uq%z**2)
	  Pm_Heepx =  -recon%Pmx
	endif

	m=0
	if(electron_arm.eq.1 .or. electron_arm.eq.3.or. electron_arm.eq.7)then !electron = right side.
	  m=m+1 
	  ntu(m) = recon%e%delta ! 1
	  m=m+1
	  ntu(m) = recon%e%yptar ! 2  mr
	  m=m+1
	  ntu(m) = recon%e%xptar ! 3  mr
	  m=m+1
	  ntu(m) = recon%e%z	! 4
	  m=m+1
	  ntu(m) = main%FP%e%x ! 5
	  m=m+1
	  ntu(m) = main%FP%e%dx	! 6 mr
	  m=m+1
	  ntu(m) = main%FP%e%y ! 7 
	  m=m+1
	  ntu(m) = main%FP%e%dy	!8 mr
	  m=m+1
	  ntu(m) = orig%e%delta ! 9 
	  m=m+1
	  ntu(m) = orig%e%yptar	! 10 mr
	  m=m+1
	  ntu(m) = orig%e%xptar	! 11 mr
	  m=m+1
	  ntu(m) = main%target%z*spec%e%sin_th ! 12
	  m=m+1
	  ntu(m) = recon%p%delta ! 13
	  m=m+1
	  ntu(m) = recon%p%yptar ! 14 mr
	  m=m+1
	  ntu(m) = recon%p%xptar  ! 15 mr
	  m=m+1
 	  ntu(m) = recon%p%z	! 16
	  m=m+1
 	  ntu(m) = main%FP%p%x	! 17
	  m=m+1
	  ntu(m) = main%FP%p%dx	! 18 mr
	  m=m+1
 	  ntu(m) = main%FP%p%y	! 19
	  m=m+1
 	  ntu(m) = main%FP%p%dy	! 20 mr
	  m=m+1
	  ntu(m) = orig%p%delta ! 21
	  m=m+1
	  ntu(m) = orig%p%yptar	! 22 mr
	  m=m+1
	  ntu(m) = orig%p%xptar	! 23 mr
	  m=m+1
 	  ntu(m) = -main%target%z*spec%p%sin_th
	else if (electron_arm.eq.2 .or. electron_arm.eq.4 .or.
     >		 electron_arm.eq.5 .or. electron_arm.eq.6.or. electron_arm.eq.8) then  !e- = left.
	  m=m+1
	  ntu(m) = recon%p%delta ! 1
	  m=m+1
	  ntu(m) = recon%p%yptar ! 2 mr
	  m=m+1
	  ntu(m) = recon%p%xptar !3 mr
	  m=m+1
	  ntu(m) = recon%p%z	! 4 
	  m=m+1
	  ntu(m) = main%FP%p%x	! 5
	  m=m+1
	  ntu(m) = main%FP%p%dx	! 6 mr
	  m=m+1
	  ntu(m) = main%FP%p%y ! 7
	  m=m+1
	  ntu(m) = main%FP%p%dy	! 8 mr
	  m=m+1
	  ntu(m) = orig%p%delta ! 9 
	  m=m+1
	  ntu(m) = orig%p%yptar	! 10 mr
	  m=m+1
	  ntu(m) = orig%p%xptar	! 11 mr
	  m=m+1
	  ntu(m) = main%target%z*spec%p%sin_th ! 12
	  m=m+1
	  ntu(m) = recon%e%delta ! 13
	  m=m+1
	  ntu(m) = recon%e%yptar ! 14 mr
	  m=m+1
	  ntu(m) = recon%e%xptar ! 15 mr
	  m=m+1
	  ntu(m) = recon%e%z ! 16
	  m=m+1
 	  ntu(m) = main%FP%e%x ! 17
	  m=m+1
	  ntu(m) = main%FP%e%dx	! 18 mr
	  m=m+1
 	  ntu(m) = main%FP%e%y ! 19
	  m=m+1
 	  ntu(m) = main%FP%e%dy	! 20 mr
	  m=m+1
	  ntu(m) = orig%e%delta ! 21
	  m=m+1
	  ntu(m) = orig%e%yptar	! 22 mr
	  m=m+1
	  ntu(m) = orig%e%xptar	! 23 mr
	  m=m+1
	  ntu(m) = -main%target%z*spec%e%sin_th ! 24
	else
	  write (6,*) 'results_write not yet set up for your spectrometers.'
	  stop
	endif
	m=m+1
	ntu(m) = recon%q/1000.	! 25 q - GeV/c
	m=m+1
	ntu(m) = recon%nu/1000.	! 26 nu - GeV
	m=m+1
	ntu(m) = recon%Q2/1.e6	! 27 Q^2 - (GeV/c)^2
	m=m+1
	ntu(m) = recon%W/1000.	! 28 W - GeV/c
	m=m+1
	ntu(m) = recon%xbj	! 29 xbj
	m=m+1
	ntu(m) = recon%epsilon	! 30 epsilon
	m=m+1
	ntu(m) = recon%Em/1000.	! 31 GeV
	m=m+1
	ntu(m) = recon%Pm/1000.	! 32 GeV/c
	m=m+1
	ntu(m) = vertex%q/1000.	! 33 q - GeV/c
	m=m+1
	ntu(m) = vertex%nu/1000. ! 34 nu - GeV
	m=m+1
	ntu(m) = vertex%Q2/1.e6	! 35 Q^2 - (GeV/c)^2
	m=m+1
	ntu(m) = main%W/1000.	! 36 W - GeV/c
	m=m+1
	ntu(m) = vertex%xbj	! 37 xbj
	m=m+1
	ntu(m) = main%epsilon	! 38 epsilon
	m=m+1
	ntu(m) = vertex%Emiss/1000. ! 39 GeV
	m=m+1
	ntu(m) = vertex%Pmiss/1000. ! 40 GeV/c
	if (doing_pi0phot .or. doing_pion .or. doing_kaon .or. doing_delta .or. doing_semi .or. doing_rho) then
	  m=m+1
	  ntu(m) = ntup%mm/1000. ! 41 missmass (nucleon)
	  m=m+1
	  ntu(m) = ntup%mmA/1000. ! 42 missmass (nucleus)
	  m=m+1
	  ntu(m) = recon%p%P/1000. ! 43 ppi - GeV/c
	  m=m+1
	  ntu(m) = ntup%t/1.e6	! 44 t - GeV^2
	  m=m+1
	  ntu(m) = main%t/1.e6	! 45
	  m=m+1
	  ntu(m) = recon%zhad	! 46 z
	  m=m+1
 	  ntu(m) = vertex%zhad	! 47
	  m=m+1
	  ntu(m) = recon%pt2/1.e06 ! 48 pt**2 -GeV^2
	  m=m+1
	  ntu(m) = vertex%pt2/1.e06 ! 49
	  m=m+1
	  ntu(m) = recon%theta_pq ! 50 theta_pq - radians
	  m=m+1
	  ntu(m) = recon%phi_pq	! 51 phi_pq - radians
	  m=m+1
	  ntu(m) = main%theta_pq ! 52 theta_pq - radians
	  m=m+1
	  ntu(m) = main%phi_pq	! 53 phi_pq - radians
	  m=m+1
	  ntu(m) = recon%PmPar/1000. ! 54
	  m=m+1
	  ntu(m) = recon%PmPer/1000. ! 55
	  m=m+1
	  ntu(m) = recon%PmOop/1000. ! 56
	  m=m+1
	  ntu(m) = -main%target%rastery	!57 fry - cm
	  m=m+1
	  ntu(m) = ntup%radphot/1000. ! 58 radphot - GeV
	  dummy = pferx*vertex%uq%x + pfery*vertex%uq%y + pferz*vertex%uq%z
	  if (dummy.eq.0) dummy=1.e-20
	  m=m+1
	  ntu(m) = pfer/1000.*abs(dummy)/dummy ! 59 p_fermi - GeV/c
	  m=m+1
	  ntu(m) = main%sigcc	! 60 d5sig or d6sig
	  m=m+1
	  ntu(m) = ntup%sigcm	! 61 pion sig_cm, or SIDIS sig_had
 	  if (doing_kaon) then
	     m=m+1	  
	     ntu(m) = ntup%sigcm1 ! 61 sigcm - saghai model
	     m=m+1	  
	     ntu(m) = ntup%sigcm2 ! 61.5 sigcm - factorized.
	  endif
	  m=m+1		  
	  ntu(m) = main%weight ! 62
	  m=m+1
	  ntu(m) = decdist	! 63 decay distance (cm)
	  m=m+1		
	  ntu(m) = sqrt(Mh2_final) ! 64
	  if(doing_rho) then
	     m=m+1
	     ntu(m) = ntup%rhomass ! 65
	     m=m+1
	     ntu(m) = ntup%rhotheta ! 66
	     m=m+1
	     ntu(m) = ntup%mmA/1000 !67
	  endif
	  if(doing_pizero .or. doing_pi0phot) then
	     m=m+1
	     ntu(m) = ntup%xcal_gamma1 ! 68
	     m=m+1
	     ntu(m) = ntup%ycal_gamma1 ! 69
	     m=m+1
	     ntu(m) = ntup%gamma1(1) ! 70
	     m=m+1
	     ntu(m) = ntup%gamma1(2) ! 71
	     m=m+1
	     ntu(m) = ntup%gamma1(3) ! 72
	     m=m+1
	     ntu(m) = ntup%gamma1(4) ! 73
	     m=m+1
	     ntu(m) = ntup%xcal_gamma2 ! 74
	     m=m+1
	     ntu(m) = ntup%ycal_gamma2 ! 75
	     m=m+1
	     ntu(m) = ntup%gamma2(1) ! 76
	     m=m+1
	     ntu(m) = ntup%gamma2(2) ! 77 
	     m=m+1
	     ntu(m) = ntup%gamma2(3) ! 78
	     m=m+1
	     ntu(m) = ntup%gamma2(4) ! 79
	     if (doing_pi0phot) then
	       m=m+1
	       ntu(m) = ntup%pi0phot_egamma/1000.0
	       m=m+1
	       ntu(m) = ntup%pi0phot_costh
	       m=m+1
	       ntu(m) = ntup%pi0phot_phicm
	       m=m+1
	       ntu(m) = ntup%pi0phot_pelastic/1000.0
	       m=m+1
	       ntu(m) = ntup%pi0phot_dpelastic
	       m=m+1
	       ntu(m) = ntup%pi0phot_xpred
	       m=m+1
	       ntu(m) = ntup%pi0phot_ypred
	       m=m+1
	       ntu(m) = ntup%pi0phot_dx1
	       m=m+1
	       ntu(m) = ntup%pi0phot_dy1
	       m=m+1
	       ntu(m) = ntup%pi0phot_dx2
	       m=m+1
	       ntu(m) = ntup%pi0phot_dy2
	     endif
	  endif
	  if(using_tgt_field) then
	     m=m+1
	     ntu(m) = recon%theta_tarq ! 80
	     m=m+1
	     ntu(m) = recon%phi_targ ! 81
	     m=m+1
	     ntu(m) = recon%beta ! 82
	     m=m+1
	     ntu(m) = recon%phi_s ! 83
	     m=m+1
	     ntu(m) = recon%phi_c ! 84
	     m=m+1
	     ntu(m) = main%beta	! 85
	     m=m+1
	     ntu(m) = vertex%phi_s ! 86
	     m=m+1
	     ntu(m) = vertex%phi_c ! 87	     
	  endif
	else if (doing_hyd_elast .or. doing_deuterium .or. doing_heavy) then
	   m=m+1
	   ntu(m) = corrsing/1000. ! 41
	   m=m+1
	   ntu(m) = Pm_Heepx/1000. ! 42
	   m=m+1
	   ntu(m) = Pm_Heepy/1000. ! 43
	   m=m+1
	   ntu(m) = Pm_Heepz/1000. ! 44
	   m=m+1
	   ntu(m) = recon%PmPar/1000. ! 45
	   m=m+1
	   ntu(m) = recon%PmPer/1000. ! 46
	   m=m+1
	   ntu(m) = recon%PmOop/1000. ! 47 
	   m=m+1
	   ntu(m) = -main%target%rastery ! 48 fry - cm
	   m=m+1
	   ntu(m) = ntup%radphot/1000. ! 49 radphot - GeV
	   m=m+1
	   ntu(m) = main%sigcc ! 50
	   m=m+1
	   ntu(m) = main%weight ! 51
	   m=m+1
	   ntu(m) = recon%e%theta ! 52
	   m=m+1
	   ntu(m) = recon%p%theta ! 53
	   if (doing_hyd_elast) then
	     m=m+1
	     ntu(m) = ntup%pi0phot_pelastic/1000.0
	     m=m+1
	     ntu(m) = ntup%pi0phot_dpelastic
	     m=m+1
	     ntu(m) = ntup%pi0phot_xpred
	     m=m+1
	     ntu(m) = ntup%pi0phot_ypred
	     m=m+1
	     ntu(m) = ntup%pi0phot_dx1
	     m=m+1
	     ntu(m) = ntup%pi0phot_dy1
	   endif
	endif

c	call HFN(NtupleID,ntu)
	if(m.ne.NtupleSize) then
	   write(6,*) 'number of output quantities not the same as output array size'
	   stop
	endif
	do i=1,NtupleSize
	   write(NtupleIO) ntu(i)
	enddo
	if (debug(2)) write(6,*)'r_ntu_write: ending'
	return
	end

!--------------------------------------------------------------------------

	subroutine hybrid_export_init(filename)

	USE structureModule
	implicit none
	include 'simulate.inc'
	character*(*) filename
	integer hybrid_io
	parameter (hybrid_io=42)
	integer hybrid_truth_io
	parameter (hybrid_truth_io=43)

	if (.not.(doing_hyd_elast.or.doing_pi0phot)) then
	  write(6,*) 'hybrid_export supports elastic H(e,e''p) and pi0phot only'
	  stop
	endif

	open(unit=hybrid_io,file=filename,status='replace',form='formatted')
	if (doing_pi0phot) then
	  write(hybrid_io,'(a)') '# simc_geant4_pi0_v1'
	  write(hybrid_io,'(a)') '# energies_and_momenta_GeV positions_cm directions_Hall_xyz'
	  write(hybrid_io,'(a)') '# Hall_axes: x_down y_left z_downstream'
	  write(hybrid_io,'(a)') 'event_id,weight,vertex_x_cm,vertex_y_cm,'//
     > 'vertex_z_cm,beam_E_GeV,gamma1_E_GeV,gamma1_px_GeV,'//
     > 'gamma1_py_GeV,gamma1_pz_GeV,gamma2_E_GeV,gamma2_px_GeV,'//
     > 'gamma2_py_GeV,gamma2_pz_GeV,proton_E_GeV,proton_px_GeV,'//
     > 'proton_py_GeV,proton_pz_GeV'
	  return
	endif
	write(hybrid_io,'(a)') '# simc_geant4_hybrid_v1'
	write(hybrid_io,'(a)') '# energies_and_momenta_GeV positions_cm directions_Hall_xyz'
	write(hybrid_io,'(a)') '# Hall_axes: x_down y_left z_downstream'
	write(hybrid_io,'(a)') 'event_id,weight,vertex_x_cm,vertex_y_cm,vertex_z_cm,'//
     > 'vertex_beam_E_GeV,'//
     > 'vertex_e_E_GeV,vertex_e_px_GeV,vertex_e_py_GeV,vertex_e_pz_GeV,'//
     > 'rad_tail,rad1_GeV,rad2_GeV,rad3_GeV,target_eloss_GeV,target_teff_X0,'//
     > 'primary_x_cm,primary_y_cm,primary_z_cm,primary_E_GeV,'//
     > 'primary_px_GeV,primary_py_GeV,primary_pz_GeV'
	open(unit=hybrid_truth_io,file=trim(filename)//'.truth.csv',
     > status='replace',form='formatted')
	write(hybrid_truth_io,'(a)') '# simc_geant4_hybrid_truth_v1'
	write(hybrid_truth_io,'(a)') '# energies_and_momenta_GeV'
	write(hybrid_truth_io,'(a)')
     > 'event_id,vertex_p_E_GeV,vertex_p_px_GeV,'//
     > 'vertex_p_py_GeV,vertex_p_pz_GeV'
	return
	end

!--------------------------------------------------------------------------

	subroutine hybrid_export_write(event_id,main,vertex,orig,success)

	USE structureModule
	implicit none
	include 'simulate.inc'
	include 'radc.inc'
	integer event_id,hybrid_io,hybrid_truth_io,rad_tail
	parameter (hybrid_io=42)
	parameter (hybrid_truth_io=43)
	type(event_main) main
	type(event) vertex,orig
	logical success
	real*8 theta,phi,post_E,post_P
	real*8 post_px,post_py,post_pz
	real*8 rad1,rad2,rad3

	if (.not.success) return
	if (doing_pi0phot) then
	  write(hybrid_io,'(*(g0,:,","))') event_id,main%weight,
     > main%target%x,main%target%y,main%target%z,vertex%Ein/1000.0d0,
     > ntup%gamma1(1)/1000.0d0,ntup%gamma1(2)/1000.0d0,
     > ntup%gamma1(3)/1000.0d0,ntup%gamma1(4)/1000.0d0,
     > ntup%gamma2(1)/1000.0d0,ntup%gamma2(2)/1000.0d0,
     > ntup%gamma2(3)/1000.0d0,ntup%gamma2(4)/1000.0d0,
     > vertex%p%E/1000.0d0,vertex%p%P*vertex%up%x/1000.0d0,
     > vertex%p%P*vertex%up%y/1000.0d0,
     > vertex%p%P*vertex%up%z/1000.0d0
	  return
	endif

	call physics_angles(spec%e%theta,spec%e%phi,main%SP%e%xptar,
     >     main%SP%e%yptar,theta,phi)
	post_E=spec%e%P*(1.0d0+main%SP%e%delta/100.0d0)
	post_P=sqrt(max(0.0d0,post_E**2-Me**2))
	post_px=post_P*sin(theta)*cos(phi)
	post_py=post_P*sin(theta)*sin(phi)
	post_pz=post_P*cos(theta)

	rad1=0.0d0
	rad2=0.0d0
	rad3=0.0d0
	rad_tail=0
	if (using_rad) then
	  rad1=phot1
	  rad2=phot2
	  rad3=phot3
	  rad_tail=nint(ntup%radarm)
	endif

	write(hybrid_io,'(*(g0,:,","))') event_id,main%weight,
     > main%target%x,main%target%y,main%target%z,vertex%Ein/1000.0d0,
     > vertex%e%E/1000.0d0,vertex%e%P*vertex%ue%x/1000.0d0,
     > vertex%e%P*vertex%ue%y/1000.0d0,vertex%e%P*vertex%ue%z/1000.0d0,
     > rad_tail,rad1/1000.0d0,rad2/1000.0d0,rad3/1000.0d0,
     > main%target%Eloss(2)/1000.0d0,main%target%teff(2),
     > main%target%x,main%target%y,main%target%z,post_E/1000.0d0,
     > post_px/1000.0d0,post_py/1000.0d0,post_pz/1000.0d0
	write(hybrid_truth_io,'(*(g0,:,","))') event_id,
     > vertex%p%E/1000.0d0,vertex%p%P*vertex%up%x/1000.0d0,
     > vertex%p%P*vertex%up%y/1000.0d0,
     > vertex%p%P*vertex%up%z/1000.0d0
	return
	end

!--------------------------------------------------------------------------

	subroutine hybrid_export_close

	implicit none
	integer hybrid_io,hybrid_truth_io
	parameter (hybrid_io=42)
	parameter (hybrid_truth_io=43)
	close(hybrid_io)
	close(hybrid_truth_io)
	return
	end


	subroutine results_ntu_write1(vertex,recon,main,success)

	USE structureModule
	implicit none
	include 'hbook.inc'
	include 'simulate.inc'

	integer*4 nentries
	parameter (nentries = 9)

	real*8	ntu(nentries)
	logical success
	type(event_main):: main
	type(event):: vertex, recon

	if (debug(2)) write(6,*)'r_ntu_write: entering'
	ntu(1) = vertex%e%delta
	ntu(2) = vertex%e%yptar
	ntu(3) = -vertex%e%xptar
	ntu(4) = main%SP%e%z
	if(success)then
	  ntu(5) = recon%e%delta
	  ntu(6) = recon%e%yptar
	  ntu(7) = recon%e%xptar
	  ntu(8) = recon%e%z
	else
	  ntu(5) = 30.
	  ntu(6) = 0.1
	  ntu(7) = 0.1
	  ntu(8) = 4.
	endif
	ntu(9) = main%weight
c	call HFN(NtupleID,ntu)
	if (debug(2)) write(6,*)'r_ntu_write: ending'
	return
	end
