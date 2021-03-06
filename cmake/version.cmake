SET(VERSION_REGEX "[0-9]+\\.[0-9]+\\.[0-9]+")

MACRO(VERSION_TO_VARS version major minor patch)
  IF(${version} MATCHES ${VERSION_REGEX})
    STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" ${major} "${version}")
    STRING(REGEX REPLACE "^[0-9]+\\.([0-9])+\\.[0-9]+" "\\1" ${minor} "${version}")
    STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" ${patch} "${version}")
  ELSE(${version} MATCHES ${VERSION_REGEX})
    MESSAGE("MACRO(VERSION_TO_VARS ${version} ${major} ${minor} ${patch}")
    MESSAGE(FATAL_ERROR "Problem parsing version string, I can't parse it properly.")
  ENDIF(${version} MATCHES ${VERSION_REGEX})
ENDMACRO(VERSION_TO_VARS)
