# Copyright (C) 2024 Eutro <https://eutro.dev>
#
# This file is part of cppemacs.
#
# cppemacs is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cppemacs is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cppemacs. If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-FileCopyrightText: 2024 Eutro <https://eutro.dev>
#
# SPDX-License-Identifier: GPL-3.0-or-later

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${CPPEMACS_PROJECT_CONFIG_IN_FILE}"
  "${CPPEMACS_PROJECT_CONFIG_FILE}"
  INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/pkgconfig"
)

if (NOT ${CMAKE_VERSION} VERSION_LESS "3.14")
  list(APPEND CPPEMACS_WRITE_PACKAGE_VERSION_ARGS
    ARCH_INDEPENDENT)
endif()
write_basic_package_version_file(
  "${CPPEMACS_PROJECT_VERSION_FILE}"
  COMPATIBILITY AnyNewerVersion
  ${CPPEMACS_WRITE_PACKAGE_VERSION_ARGS}
)

# Install target
install(
  TARGETS ${CPPEMACS_TARGET_NAME}
  EXPORT ${CPPEMACS_TARGETS_EXPORT_NAME}
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/"
)

# Install *Config.cmake and *Version.cmake
install(
  FILES "${CPPEMACS_PROJECT_CONFIG_FILE}" "${CPPEMACS_PROJECT_VERSION_FILE}"
  DESTINATION "${CPPEMACS_CONFIG_INSTALL_DIR}"
)
# Install FindEmacs
install(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindEmacs.cmake"
  DESTINATION "${CPPEMACS_CONFIG_INSTALL_DIR}/modules"
)

# Install license file and headers
install(
  DIRECTORY "${CPPEMACS_INCLUDE_BUILD_DIR}/"
  DESTINATION "${CPPEMACS_INCLUDE_INSTALL_DIR}"
  FILES_MATCHING PATTERN "*.hpp"
)  
install(
  FILES "${CPPEMACS_LICENSE_FILE}"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/"
)

# Install documentation
if (TARGET ${PROJECT_NAME}_docs)
  install(
    DIRECTORY "${CPPEMACS_DOCS_BUILD_DIR}/html"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
  )
endif()

# Install export file
install(
  EXPORT ${CPPEMACS_TARGETS_EXPORT_NAME}
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION "${CPPEMACS_CONFIG_INSTALL_DIR}"
)

# CPack
set(CPACK_PACKAGE_VENDOR "Eutro")
set(CPACK_RESOURCE_FILE_LICENSE "${CPPEMACS_LICENSE_FILE}")
include(CPack)
