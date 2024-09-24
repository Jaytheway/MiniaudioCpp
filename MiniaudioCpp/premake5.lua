	project "MiniaudioCpp"
		kind "StaticLib"
		language "C++"
		cppdialect "C++20"
		staticruntime "off"

		targetdir ("bin/" .. outputdir .. "/%{prj.name}")
		objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

		files {
			"src/**.h",
			"src/**.cpp",
			"include/**.h",
			"vendor/miniaudio/miniaudio.h",
			"vendor/dr_libs/**.h",
			"vendor/c89atomic/**.h",
			"vendor/choc/**.h",
			"vendor/miniaudio/extras/stb_vorbis.c",
			"vendor/c89atomic.h"
		}

		includedirs
		{
			"include/MiniaudioCpp",
			"vendor",
			"vendor/c89atomic",
			"src/MiniAudioInterface"
		}


		filter "system:windows"
			systemversion "latest"
			cppdialect "C++20"

		filter "system:linux"
			pic "On"
			systemversion "latest"
			cppdialect "C++20"

		filter "configurations:Debug"
			runtime "Debug"
			symbols "on"
			
		filter "configurations:Release"
			runtime "Release"
			optimize "on"
			
		filter "configurations:Dist"
			runtime "Release"
			optimize "on"
			symbols "off"
			
		filter "configurations:Test"
			runtime "Debug"
			symbols "on"
			optimize "off"
			defines { "JPL_TEST" }

