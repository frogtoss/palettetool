-- Palette Tool premake5 script
--
-- This can be ran directly, but commonly, it is only run
-- by package maintainers.
-- See http://www.frogtoss.com/labs/premake-for-package-maintainers.html
--
-- IMPORTANT NOTE: premake5 alpha 9 does not handle this script
-- properly.  Build premake5 from Github master, or, presumably,
-- use alpha 10 in the future.


 
workspace "Palette Tool"
  -- these dir specifications assume the generated files have been moved
  -- into a subdirectory.  ex: <root>/build/makefile
  local root_dir = path.join(path.getdirectory(_SCRIPT),"../../")
  local build_dir = path.join(path.getdirectory(_SCRIPT))
  configurations { "Debug", "Release" }

  
  platforms { "x64" }
  
  
  filter "not system:macosx"
    platforms { "x86" }  
  
  
  objdir(path.join(build_dir, "obj/"))

  -- architecture filters
  
  filter "configurations:x86"
    architecture "x86"
  
  filter "configurations:x64"
    architecture "x86_64"

  -- execution type filters
  --
  -- DEBUG | RELEASE | FINAL_RELEASE are the three canonical build types, but support
  -- other popular macros as well. Consider:
  -- 
  --  * DEBUG builds have cross-comp unit debug behaviors (low perf)
  --  * RELEASE builds are built with optimization but can have debug/logging hooks built in.
  --  * FINAL_RELEASE builds *are also release builds*, but are very hard to debug
  --
  --  No explicit support for profile builds yet. Proper profiling should happen in FINAL_RELEASE.
  filter "configurations:Debug"
    defines {"DEBUG", "_DEBUG", "FINAL_RELEASE=0"}
    symbols "On"
    targetsuffix "_d"
  
  filter "configurations:Release"
    defines {"NDEBUG", "RELEASE", "FINAL_RELEASE=0"}
    optimize "On"
  
   
  project "palettetool"
    kind "ConsoleApp"

    files {root_dir.."src/**.h",
           root_dir.."src/**.c",
           root_dir.."src/config/palconfig.h",
 
           root_dir.."src/3rdparty/*.c",
           root_dir.."src/3rdparty/*.h",
    }

    includedirs {
       root_dir.."src/config/",
 
  root_dir.."/src/3rdparty",
    }




    filter "system:linux or system:macosx"
      buildoptions {"--std=gnu99"}

      fatalwarnings {"shadow", "return-type", "implicit-function-declaration"}
      
    -- features: off by default, turn them on and regenerate
    -- if you need them
    filter{}
    warnings "Extra"
    rtti("off")
    exceptionhandling("off")
    filter "action:vs*"
      defines {"_CRT_SECURE_NO_WARNINGS"}
      fatalwarnings {"4715"} -- not all control paths return a value
      


-- cwd for debug execution is relative to installed DLL
-- directory.

    filter "toolset:msc"
      debugdir(root_dir.."../bin/$(Configuration)/win32_$(PlatformTarget)")
      targetdir(root_dir.."../bin/$(Configuration)/win32_$(PlatformTarget)")



newaction
{
   trigger = "dist",
   description = "Create distributable premake dirs (maintainer only)",
   execute = function()


      -- special denotes the action *and* os_str make up the dir name
      -- needed when an action alone is ambiguous (ex: gmake runs on multiple OSes)
      local premake_do_action = function(action,os_str,special)
         local premake_dir
         if special then
            premake_dir = "./"..action.."_"..os_str
         else
            premake_dir = "./"..action
         end
         local premake_path = premake_dir.."/premake5.lua"

         os.mkdir(premake_dir)
         os.execute("cp premake5.lua "..premake_dir)
         os.execute("premake5 --os="..os_str.." --file="..premake_path.." "..action)
         os.execute("rm "..premake_path)
      end


      premake_do_action("vs2022", "windows", false)
      --premake_do_action("xcode4", "macosx", false)      
      premake_do_action("gmake", "linux", true)      
      premake_do_action("gmake", "macosx", true)
      --premake_do_action("gmake", "windows", true) 
   end
}

-- currently there is no premake5 clean action, so this will have to suffice
-- this deletes the premake5 --action=dist generated subdirectories
newaction
{
    trigger     = "clean",
    description = "Clean all build files and output",
    execute = function ()

        files_to_delete = 
        {
            "Makefile",
            "*.make",
            "*.txt",
            "*.7z",
            "*.zip",
            "*.tar.gz",
            "*.db",
            "*.opendb",
            "*.vcproj",
            "*.vcxproj",
            "*.vcxproj.user",
            "*.vcxproj.filters",
            "*.sln",
            "*~*"
        }

        directories_to_delete = 
        {
            "obj",
            "ipch",
            "bin",
            ".vs",
            "Debug",
            "Release",
            "release",
            "lib",
            "test",
            "makefiles",
            "gmake",
            "vs2010",
            "xcode4",
            "gmake_linux",
            "gmake_macosx",
            "gmake_windows"
        }

        for i,v in ipairs( directories_to_delete ) do
          os.rmdir( v )
        end

        if os.is "macosx" then
           os.execute("rm -rf *.xcodeproj")
           os.execute("rm -rf *.xcworkspace")
        end

        if not os.is "windows" then
            os.execute "find . -name .DS_Store -delete"
            for i,v in ipairs( files_to_delete ) do
              os.execute( "rm -f " .. v )
            end
        else
            for i,v in ipairs( files_to_delete ) do
              os.execute( "del /F /Q  " .. v )
            end
        end

    end
}
