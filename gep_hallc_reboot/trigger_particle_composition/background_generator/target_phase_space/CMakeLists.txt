cmake_minimum_required(VERSION 3.16)
project(gep_target_phase_space LANGUAGES CXX)

find_package(Geant4 REQUIRED)
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree)

add_executable(gep_target_phase_space target_phase_space.cc)
target_compile_features(gep_target_phase_space PRIVATE cxx_std_17)
target_include_directories(gep_target_phase_space PRIVATE
  ${Geant4_INCLUDE_DIRS}
  ${ROOT_INCLUDE_DIRS})
target_link_libraries(gep_target_phase_space PRIVATE
  ${Geant4_LIBRARIES}
  ROOT::Core ROOT::RIO ROOT::Tree)

