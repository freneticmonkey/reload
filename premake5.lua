
workspace "reload"
configurations {
  "Debug",
  "Release"
}
platforms {
  "windows",
  "macosx",
  "macosxARM",
  "linux64",
  "linuxARM64",
  -- etc.
}

-- enable tracing for debug builds
filter "configurations:Debug"
  symbols "On"
  defines {
    "_DEBUG",
    "TRACY_ENABLE"
  }

filter "configurations:Release"
  optimize "On"
  defines { "NDEBUG" }

-- Architecture is x86_64 by default
architecture "x86_64"

filter "system:windows"
  defines {
    "RELOAD_WINDOWS",
    "WIN32",
    "_WIN32"
  }

filter "system:macosx"
  defines {
    "RELOAD_MACOS"
  }

filter "platforms:macosxARM or linuxARM64"
  architecture "ARM"

filter "system:linux"
  buildoptions {
    "-std=c11",
    "-D_DEFAULT_SOURCE",
    "-D_POSIX_C_SOURCE=200112L",
    "-mcmodel=large",
    "-fPIE",
    "-Werror=return-type",
    "-Werror=implicit-function-declaration"
  }
  defines {
    "RELOAD_LINUX"
  }

solution "reload"
  filename "reload"
  location "projects"

  startproject "reload"

project "raylib"
  kind "SharedLib"
  language "C"
  targetdir( "build" )
  buildoptions {
    "-std=gnu99",
    "-D_GNU_SOURCE",
    "-DGL_SILENCE_DEPRECATION=199309L",

    -- "-mcmodel=large",
    -- "-fPIE",
    -- "-Werror=return-type",
    -- "-Werror=implicit-function-declaration"
  }
  defines {
    "PLATFORM_DESKTOP",
    "GRAPHICS_API_OPENGL_33",
    -- "GLFW_INCLUDE_NONE",
    -- "RAYLIB_NO_INCLUDE_EXTERNALS"
  }
  files {
    "src/ext/raylib/raudio.c",
    "src/ext/raylib/rcore.c",
    "src/ext/raylib/rglfw.c",
    "src/ext/raylib/rmodels.c",
    "src/ext/raylib/rshapes.c",
    "src/ext/raylib/rtext.c",
    "src/ext/raylib/rtextures.c",
    "src/ext/raylib/utils.c",
  }
  includedirs {
    "src/ext/raylib/",
    "src/ext/raylib/external/glfw/include",
  }

  if (os.host() == "macosx") then
    links {
      "Foundation.framework",
      "CoreServices.framework",
      "CoreGraphics.framework",
      "AppKit.framework",
      "IOKit.framework",
      "c",
      --  "tracy",
    }
    filter "files:src/ext/raylib/rglfw.c"
       compileas "Objective-C"
  end
  -- TODO: other OS

project "basic"
  kind "SharedLib"
  language "C"
  targetdir( "build" )
  links {
    "raylib"
  }
  files {
    "src/modules/basic/**.h",
    "src/modules/basic/**.c"
  }
  includedirs {
    "src/lib/",
    "src/ext/",
    "src/ext/raylib/",
  }

project "reload"
  kind "ConsoleApp"
  language "C"
  targetdir( "build" )
  defines {
  }
  flags {
    "MultiProcessorCompile"
  }
  debugdir "."

  libdirs {
    "build"
  }

  links {
    "raylib"
  }

  includedirs {
    "src",
    "src/lib",
    "src/ext/raylib/",
  }

  files {
    "src/**.h",
    "src/**.c"
  }

  -- ignore all testing files
  removefiles {
    "src/test/**",
    "src/**_test.*",

    "src/ext/**",
    "src/modules/**"
  }

  -- enable tracing for debug builds
  filter "configurations:Debug"
    links {
      --  "tracy"
    }
    sysincludedirs {
      --  "ext/tracy"
    }

  if (os.host() == "linux") then
    libdirs {
      os.findlib("m"),
      os.findlib("c")
    }
    links {
      "c",
      "dl",
      "m",
      "pthread",
      "dl"
    }
  end

  if (os.host() == "macosx") then
    links {
      -- For raylib
      -- "Foundation.framework",
      -- "CoreServices.framework",
      -- "CoreGraphics.framework",
      -- "AppKit.framework",
      -- "IOKit.framework",

      "CoreServices.framework",
      "c",

      "dl"
      --  "tracy",
    }
  end

  if (os.host() == "windows") then
  
    defines {
      "_CRT_SECURE_NO_WARNINGS"
    }
    disablewarnings {
      "4005"
    }
    editAndContinue "Off"
    links {
      "winstd"
    }
    sysincludedirs {
      "ext/winstd"
    }

  end
   --  filter "files:src/main.c"
   --    compileas "Objective-C"

project "test"
  kind "ConsoleApp"
  language "C"
  targetdir( "build" )
  debugdir ( "." )
  defines {
    "RELOAD_UNIT"
  }

  links {
    "munit"
  }

  libdirs {
    "build"
  }

  sysincludedirs {
    "ext"
  }

  includedirs {
    "src"
  }

  files {
    "src/**.h",
    "src/**.c",

  }

  -- ignore the reload main
  removefiles {
    "src/main.c"
  }

  if (system == macosx) then
    links {
      "c"
    }
  end

  if (system == linux) then
    libdirs {
      os.findlib("m"),
      os.findlib("c")
    }
    links {
      "c",
      "m",
      "pthread",
    }
  end

  if (system == windows) then
    defines {
      "_CRT_SECURE_NO_WARNINGS"
    }
    disablewarnings {
      "4005"
    }
    -- Turn off edit and continue
    editAndContinue "Off"

  filter "platforms:windows"
    -- if on windows include the win32 impl of unix utils
    sysincludedirs {
      "ext/winstd"
    }
  end

-- External Libraries

project "munit"
  kind "StaticLib"
  language "C"
  targetdir( "build" )

  files {
    "ext/munit/**.h",
    "ext/munit/**.c"
  }

if (system == windows) then
project "winstd"
  kind "StaticLib"
  language "C"
  targetdir( "build" )

  files { "ext/winstd/**" }
end