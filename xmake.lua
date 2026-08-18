-- include subprojects
includes("lib/commonlibsse-ng")
add_requires("nlohmann_json v3.12.0")
-- set project constants
set_project("skilltree-to-json")
set_version("0.0.0")
set_license("GPL-3.0")
set_languages("c++23")
set_warnings("allextra")

-- add common rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

-- define targets
target("skilltree-to-json")
    add_rules("commonlibsse-ng.plugin", {
        name = "skilltree-to-json",
        author = "Mattdokn",
        description = "SKSE64 plugin template using CommonLibSSE-NG"
    })
    add_packages("nlohmann_json")
    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
