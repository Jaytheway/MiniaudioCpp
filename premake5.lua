-- premake5.lua
workspace "MiniaudioCpp"
	architecture "x64"
	configurations { "Debug", "Release", "Dist", "Test" }
	startproject "MiniaudioCppTest"
		

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "MiniaudioCpp"
include "MiniaudioCppTests"
