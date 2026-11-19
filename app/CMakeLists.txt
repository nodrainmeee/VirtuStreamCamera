add_executable(VirtuStreamCamera WIN32
  src/main.cpp
)

target_compile_features(VirtuStreamCamera PRIVATE cxx_std_17)
target_compile_definitions(VirtuStreamCamera PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(VirtuStreamCamera PRIVATE
  mfplat mf mfuuid mfreadwrite shlwapi shell32 ole32 comctl32 uuid
)

if(MSVC)
  target_compile_options(VirtuStreamCamera PRIVATE /W4 /permissive- /EHsc /utf-8)
  target_compile_definitions(VirtuStreamCamera PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
