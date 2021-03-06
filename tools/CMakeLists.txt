cmake_minimum_required (VERSION 3.1)
cmake_policy(VERSION 3.1)
project (LiteCoreTools)

if(ANDROID)
    return()
endif()

set(TOP ${PROJECT_SOURCE_DIR}/../)

include_directories(${PROJECT_SOURCE_DIR}
                    ${PROJECT_SOURCE_DIR}/litecp
                    ${TOP}C/include/
                    ${TOP}LiteCore/Support/
                    ${TOP}Replicator/
                    ${TOP}vendor/fleece/Fleece/
                    ${TOP}vendor/linenoise-ng/include/)

### CBLITE

aux_source_directory("cblite" CBLITE_SRC)
aux_source_directory("litecp" LITECP_SRC)
aux_source_directory("../vendor/linenoise-ng/src" LINENOISE_SRC)

add_executable(cblite
               ${CBLITE_SRC}
               ${LITECP_SRC}
               ${LINENOISE_SRC}
               Tool.cc
               ArgumentTokenizer.cc)

target_link_libraries(cblite ${LITECORE_LIBRARIES_PRIVATE} LiteCoreREST_Static)

if (APPLE)
    target_link_libraries(cblite
      z
      "-framework CoreFoundation"
      "-framework Security")
elseif(MSVC)
    target_link_libraries(cblite zlibstatic)
else()
    target_link_libraries(cblite pthread z dl
                          ${ICU4C_COMMON} ${ICU4C_I18N} ${LIBCXX_LIB} ${LIBCXXABI_LIB})
endif()
