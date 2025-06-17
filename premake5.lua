workspace "shader-compiler"
    configurations { "Debug", "Release" }
    platforms { "x64" }
    warnings "Default"

    filter "platforms:x64"
      architecture "x86_64"
    filter {}
	
	filter "system:Windows"
	  defines { "WINDOWS_BUILD" }
	filter "system:Unix"
	  defines { "LINUX_BUILD" }
	filter {}
   
    filter "configurations:Debug"
      defines { "DEBUG" }

    filter "configurations:Release"
      defines { "RELEASE", "NDEBUG" }
      flags {
        "LinkTimeOptimization",
      }
      optimize "On"

    filter {"platforms:x64", "configurations:Release"}
      targetdir "Build/Release"
    filter {"platforms:x64", "configurations:Debug"}
      targetdir "Build/Debug"
    filter {}

    objdir "%{cfg.targetdir}/obj"

    symbols "FastLink"

    filter {"configurations:Release"}
      symbols "Full"
    filter {}

    flags {
      "MultiProcessorCompile"
    }

    exceptionhandling "Off"
    cppdialect "C++20"
    language "C++"
    characterset "ASCII"
    startproject "BaneGame"

    vpaths {
      ["Lua"] = "**.lua"
    }
	


project "shader-compiler"
	kind "ConsoleApp"
	libdirs { "./Extern/libs/" }
	includedirs { "./Extern/includes/" }

	links {
		"dxcompiler"
	}

	files { "shader-compiler/**.h", "shader-compiler/**.cpp" }

project "d3d-shader-loader"
	kind "StaticLib"
	libdirs { "./Extern/libs/" }
	includedirs { "./Extern/includes/" }

	links {
		"d3d12"
	}

	files { "d3d-shader-loader/**.h", "d3d-shader-loader/**.cpp" }