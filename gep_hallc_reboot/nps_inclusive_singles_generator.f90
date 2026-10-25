program nps_inclusive_singles_generator
  implicit none

  integer, parameter :: dp = kind(1.0d0)
  real(dp), parameter :: pi = 3.14159265358979323846_dp
  real(dp), parameter :: alpha = 1.0_dp / 137.035999084_dp
  real(dp), parameter :: hbarc_gev_fm = 0.1973269804_dp
  real(dp), parameter :: mp = 0.9382720813_dp
  real(dp), parameter :: avogadro = 6.02214076d23
  real(dp), parameter :: electron_charge = 1.602176634d-19
  real(dp), parameter :: microbarn_to_cm2 = 1.0d-30

  integer :: i, n_events, seed_size, out_unit
  integer, allocatable :: seed(:)
  character(len=256) :: arg, output_path, preset
  real(dp) :: beam_energy, beam_current_ua, target_length_cm, target_density
  real(dp) :: nps_angle_deg, nps_distance_cm, nps_width_cm, nps_height_cm
  real(dp) :: energy_min, energy_max, energy_range, area_cm2
  real(dp) :: luminosity, areal_density, electron_flux, phase_weight
  real(dp) :: r1, r2, r3, r4, vertex_z, face_x, face_y, eprime
  real(dp) :: theta0, nx, nz, ux, uz, hit_x, hit_y, hit_z
  real(dp) :: dx, dy, dz, distance, dir_x, dir_y, dir_z, cos_incidence
  real(dp) :: theta, q2, nu, w2, w, f1, f2, w1, w2_response
  real(dp) :: sin2_half, cos2_half, sigma_gev2, sigma_ub_per_gev_sr
  real(dp) :: domega_per_area, rate_weight_hz, total_rate_hz

  ! Defaults: GEp Q2~11 electron arm, full 30x36 NPS face.
  n_events = 1000000
  output_path = 'gep_hallc_reboot/nps_inclusive_singles_events.csv'
  beam_energy = 10.700_dp
  beam_current_ua = 75.0_dp
  target_length_cm = 30.0_dp
  target_density = 0.0708_dp
  nps_angle_deg = 26.651_dp
  nps_distance_cm = 300.0_dp
  nps_width_cm = 61.5_dp
  nps_height_cm = 73.8_dp
  energy_min = 0.30_dp
  energy_max = 10.65_dp
  preset = 'gep'

  if (command_argument_count() >= 1) then
    call get_command_argument(1, arg)
    read(arg, *) n_events
  end if
  if (command_argument_count() >= 2) call get_command_argument(2, output_path)
  if (command_argument_count() >= 3) then
    call get_command_argument(3, preset)
    if (trim(preset) == 'run1918') then
      beam_energy = 10.538_dp
      beam_current_ua = 28.787_dp
      target_length_cm = 10.0_dp
      nps_angle_deg = 18.72_dp
      nps_distance_cm = 300.0_dp
      energy_max = beam_energy - 0.05_dp
    else if (trim(preset) /= 'gep') then
      error stop 'preset must be gep or run1918'
    end if
  end if
  if (n_events <= 0) error stop 'n_events must be positive'

  call random_seed(size=seed_size)
  allocate(seed(seed_size))
  do i = 1, seed_size
    seed(i) = 11011 + 104729 * i
  end do
  call random_seed(put=seed)

  theta0 = nps_angle_deg * pi / 180.0_dp
  nx = sin(theta0)
  nz = cos(theta0)
  ux = cos(theta0)
  uz = -sin(theta0)
  area_cm2 = nps_width_cm * nps_height_cm
  energy_range = energy_max - energy_min

  electron_flux = beam_current_ua * 1.0d-6 / electron_charge
  areal_density = target_density * target_length_cm * avogadro
  luminosity = electron_flux * areal_density
  phase_weight = area_cm2 * energy_range / real(n_events, dp)

  open(newunit=out_unit, file=trim(output_path), status='replace', action='write')
  write(out_unit, '(a)') 'event,weight_Hz,vertex_x_cm,vertex_y_cm,vertex_z_cm,' // &
       'px_GeV,py_GeV,pz_GeV,electron_energy_GeV,Q2_GeV2,W_GeV,' // &
       'face_x_cm,face_y_cm,dsigma_ub_per_GeV_sr'

  total_rate_hz = 0.0_dp
  do i = 1, n_events
    call random_number(r1)
    call random_number(r2)
    call random_number(r3)
    call random_number(r4)

    vertex_z = target_length_cm * (r1 - 0.5_dp)
    face_x = nps_width_cm * (r2 - 0.5_dp)
    face_y = nps_height_cm * (r3 - 0.5_dp)
    eprime = energy_min + energy_range * r4

    hit_x = nps_distance_cm * nx + face_x * ux
    hit_y = face_y
    hit_z = nps_distance_cm * nz + face_x * uz
    dx = hit_x
    dy = hit_y
    dz = hit_z - vertex_z
    distance = sqrt(dx * dx + dy * dy + dz * dz)
    dir_x = dx / distance
    dir_y = dy / distance
    dir_z = dz / distance
    cos_incidence = max(0.0_dp, dir_x * nx + dir_z * nz)
    domega_per_area = cos_incidence / (distance * distance)

    theta = acos(max(-1.0_dp, min(1.0_dp, dir_z)))
    sin2_half = sin(0.5_dp * theta)**2
    cos2_half = 1.0_dp - sin2_half
    q2 = 4.0_dp * beam_energy * eprime * sin2_half
    nu = beam_energy - eprime
    w2 = mp * mp + 2.0_dp * mp * nu - q2

    rate_weight_hz = 0.0_dp
    sigma_ub_per_gev_sr = 0.0_dp
    f1 = 0.0_dp
    f2 = 0.0_dp
    if (q2 > 0.0_dp .and. nu > 0.0_dp .and. w2 > (mp + 0.1349768_dp)**2) then
      call F1F2IN21(1.0_dp, 1.0_dp, q2, w2, f1, f2)
      if (f1 > 0.0_dp .or. f2 > 0.0_dp) then
        w1 = f1 / mp
        w2_response = f2 / nu
        sigma_gev2 = 4.0_dp * alpha**2 * eprime**2 / q2**2 * &
             (w2_response * cos2_half + 2.0_dp * w1 * sin2_half)
        sigma_ub_per_gev_sr = sigma_gev2 * hbarc_gev_fm**2 * 1.0d4
        rate_weight_hz = luminosity * microbarn_to_cm2 * phase_weight * &
             domega_per_area * sigma_ub_per_gev_sr
      end if
    end if
    w = sign(sqrt(abs(w2)), w2)
    total_rate_hz = total_rate_hz + rate_weight_hz

    if (rate_weight_hz > 0.0_dp) then
      write(out_unit, '(i0,13(",",es18.10))') i, rate_weight_hz, 0.0_dp, 0.0_dp, vertex_z, &
           eprime * dir_x, eprime * dir_y, eprime * dir_z, eprime, q2, w, &
           face_x, face_y, sigma_ub_per_gev_sr
    end if
  end do
  close(out_unit)

  write(*, '(a,i0)') 'Trials: ', n_events
  write(*, '(a,a)') 'Preset: ', trim(preset)
  write(*, '(a,f10.4)') 'Beam energy [GeV]: ', beam_energy
  write(*, '(a,f10.4)') 'Beam current [uA]: ', beam_current_ua
  write(*, '(a,f10.4)') 'Target length [cm]: ', target_length_cm
  write(*, '(a,f10.4)') 'NPS angle [deg]: ', nps_angle_deg
  write(*, '(a,es14.6)') 'Luminosity [cm^-2 s^-1]: ', luminosity
  write(*, '(a,es14.6)') 'Born inclusive-inelastic electron rate on full NPS face [Hz]: ', total_rate_hz
  write(*, '(a,a)') 'Output: ', trim(output_path)
end program nps_inclusive_singles_generator
