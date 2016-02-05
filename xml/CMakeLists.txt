install(FILES model.xsd references.xsd types.xsd
    DESTINATION ${PCILIB_MODEL_DIR}
)

# Copy files from source directory to destination directory, substituting any
# variables.  Create destination directory if it does not exist.

macro(configure_files srcDir destDir)
    message(STATUS "Configuring directory ${destDir}")
    make_directory(${destDir})

    file(GLOB templateFiles RELATIVE ${srcDir} ${srcDir}/*)
    foreach(templateFile ${templateFiles})
        set(srcTemplatePath ${srcDir}/${templateFile})
        if(NOT IS_DIRECTORY ${srcTemplatePath})
            message(STATUS "Configuring file ${templateFile}")
            configure_file(
                    ${srcTemplatePath}
                    ${destDir}/${templateFile}
                    @ONLY)
        endif(NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach(templateFile)
endmacro(configure_files)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/model.xsd ${CMAKE_CURRENT_BINARY_DIR}/model.xsd)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/references.xsd ${CMAKE_CURRENT_BINARY_DIR}/references.xsd)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/types.xsd ${CMAKE_CURRENT_BINARY_DIR}/types.xsd)

configure_files(${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_BINARY_DIR}/test)
configure_files(${CMAKE_CURRENT_SOURCE_DIR}/test_pywrap ${CMAKE_CURRENT_BINARY_DIR}/test_pywrap)
