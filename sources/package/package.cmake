# CPack configuration: builds installable packages with `make package`.
# Included from the top-level CMakeLists.txt.

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "Tobias Nielsen")
set(CPACK_PACKAGE_CONTACT "Tobias Nielsen")
set(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README.MD)
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

# Plain tarball everywhere; a .deb as well when dpkg is available.
set(CPACK_GENERATOR "TGZ")
find_program(DPKG_PROGRAM dpkg)
if(DPKG_PROGRAM)
    list(APPEND CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)
