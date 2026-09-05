cmake_minimum_required(VERSION 4.3)
project(Pricer)

set(CMAKE_CXX_STANDARD 20)

add_executable(Pricer main.cpp
        MonteCarlo.cpp
        MonteCarlo.h
        Option.cpp
        Option.h
        BlackScholesModel.cpp
        BlackScholesModel.h)
