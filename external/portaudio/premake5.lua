project "portaudio"
    language "C"
    cdialect "C17"
    kind "StaticLib"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/common/pa_allocation.c",
        "src/common/pa_allocation.h",
        "src/common/pa_converters.c",
        "src/common/pa_converters.h",
        "src/common/pa_cpuload.c",
        "src/common/pa_cpuload.h",
        "src/common/pa_debugprint.c",
        "src/common/pa_debugprint.h",
        "src/common/pa_dither.c",
        "src/common/pa_dither.h",
        "src/common/pa_endianness.h",
        "src/common/pa_front.c",
        "src/common/pa_gitrevision.h",
        "src/common/pa_hostapi.h",
        "src/common/pa_memorybarrier.h",
        "src/common/pa_process.c",
        "src/common/pa_process.h",
        "src/common/pa_ringbuffer.c",
        "src/common/pa_ringbuffer.h",
        "src/common/pa_stream.c",
        "src/common/pa_stream.h",
        "src/common/pa_trace.c",
        "src/common/pa_trace.h",
        "src/common/pa_types.h",
        "src/common/pa_util.h"
    }

    includedirs
    {
        "src/common",
        "include"
    }

    filter "system:windows"
        systemversion "latest"

        files
        {
            "src/hostapi/dsound/pa_win_ds.c",
            "src/hostapi/dsound/pa_win_ds_dynlink.c",
            "src/hostapi/dsound/pa_win_ds_dynlink.h",
            "src/hostapi/wasapi/pa_win_wasapi.c",
            "src/os/win/pa_win_coinitialize.c",
            "src/os/win/pa_win_coinitialize.h",
            "src/os/win/pa_win_hostapis.c",
            "src/os/win/pa_win_util.c",
            "src/os/win/pa_win_util.h",
            "src/os/win/pa_win_version.c",
            "src/os/win/pa_win_version.h",
            "src/os/win/pa_win_waveformat.c",
            "src/os/win/pa_win_wdmks_utils.c",
            "src/os/win/pa_win_wdmks_utils.h",
            "src/os/win/pa_x86_plain_converters.c",
            "src/os/win/pa_x86_plain_converters.h"
        }

        includedirs
        {
            "src/os/win"
        }

        defines
        {
            "PA_ENABLE_DEBUG_OUTPUT",
            "_CRT_SECURE_NO_WARNINGS",
            "PAWIN_USE_WDMKS_DEVICE_INFO",
            "PA_USE_ASIO=0",
            "PA_USE_DS=1",
            "PA_USE_WMME=0",
            "PA_USE_WASAPI=1",
            "PA_USE_WDMKS=0"
        }

    filter "configurations:debug"
        defines "_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:release"
        defines "NDEBUG"
        runtime "Release"
        optimize "on"

    filter "configurations:dist"
        defines "NDEBUG"
        runtime "Release"
        symbols "off"
        optimize "on"
