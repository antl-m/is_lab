-- premake5.lua
workspace "ISLabApp"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "ISLabApp"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "WalnutExternal.lua"
include "ISLabApp"