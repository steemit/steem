find_package(Qt5Core REQUIRED)

add_library( google_patch_diff_merge 
             diff_match_patch.cpp
             diff_match_patch.h
             )

target_include_directories( google_patch_diff_merge PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" )
target_link_libraries( google_patch_diff_merge Qt5::Core )
